#include "entities/systems.h"

#include "core/core.h"

#include <functional>
#include <iostream>
#include <tuple>

namespace Entity {
    BaseSystem::BaseSystem(const std::string &name, const Signature &sig): name(name), signature(sig) { }

    BaseSystem::~BaseSystem() { }

    void BaseSystem::init(Core &)  { }

    void SystemManager::threadJob() {
        while (true) {
            std::function< void() > func;
            {
                std::unique_lock< std::mutex > lk(tex);
                cv.wait(lk, [this]{ return terminating || !work_queue.empty(); });
                if (terminating) { return; }
                func = work_queue.front();
                work_queue.pop();
            }
            func();
            {
                std::lock_guard< std::mutex > lock(tex);
                processed += 1;
                // Can we do better than this?
                cv.notify_all();
            }
        }
    }

    SystemManager::SystemManager(boost::program_options::variables_map &options) {
        terminating = false;
        threads.resize(options["j"].as< size_t >());
    }

    SystemManager::~SystemManager() {
        {
            std::unique_lock lock(tex);
            terminating = true;
            cv.notify_all();
        }
        for (auto &t : threads) {
            t.join();
        }
    }

    void SystemManager::addSystem(std::unique_ptr< BaseSystem > system) {
        timers[system.get()] = AccumulateTimer();
        systems.push_back(std::move(system));
    }

    void SystemManager::execute(Core &core, double seconds) {
        for (auto &stage : stages) {
            overhead.add([&](){
                std::lock_guard< std::mutex > lock(tex);
                for (auto *system : stage) {
                    [this, &core, seconds](BaseSystem *system) {
                        work_queue.push([this, &core, seconds, system]() {
                            timers[system].add([&](){
                                system->execute(core, seconds);
                            });
                        });
                    }(system);
                }
                processed = 0;
            });

            std::unique_lock< std::mutex > lock(tex);
            cv.notify_all();
            cv.wait(lock, [&]{ return stage.size() == processed; });
        }

        reaping_time.add([&](){
            core.tracker.finalizeKills(core);
        });
    }

    void SystemManager::init(Core &core) {
        std::vector< BaseSystem* > pile;
        for (auto &s : systems) {
            s->init(core);
            pile.push_back(s.get());
        }

        while (!pile.empty()) {
            std::vector< BaseSystem* > notfit;
            stages.resize(stages.size() + 1);
            std::set< TypeID > types;

            for (size_t i = 0; i < pile.size(); ++i) {
                const auto sig = pile[i]->signature;
                // Does this stage already operate on these types?
                bool conflict = false;
                for (TypeID tid : sig) {
                    if (types.end() != types.find(tid)) {
                        conflict = true;
                        break;
                    }
                }

                if (conflict) { // This needs to be dealt with in some later stage
                    notfit.push_back(pile[i]);
                } else { // Add this to this stage
                    for (TypeID tid : sig) {
                        types.insert(tid);
                    }
                    stages[stages.size() - 1].push_back(pile[i]);
                }
            }

            pile = std::move(notfit);
            std::cout << "Stage " << stages.size() << ":\n";
            for (auto sys : stages[stages.size() - 1]) {
                std::cout << '\t' << sys->name << ": " << signatureString(sys->signature) << '\n';
            }
        }

        if (0 == threads.size()) {
            const size_t max = std::thread::hardware_concurrency();
            size_t width = 0;
            for (auto &stage : stages) {
                width = std::max(width, stage.size());
            }
            const size_t actual = std::min(width, max);
            threads.resize(actual);
            std::cout << "Using " << actual << " threads\n";
        }
        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::thread([this](){ threadJob(); });
        }
    }

    void SystemManager::dumpTimes() {
        typedef std::tuple< double, std::string > Stat;
        std::vector< Stat > stats;
        stats.reserve(systems.size());

        double total = 0.0;
        double stage_durations = 0.0;
        for (const auto &stage : stages) {
            double stage_duration = 0.0;
            for (const auto *system : stage) {
                auto &timer = timers[system];
                std::string text = system->name + ": " + signatureString(system->signature);
                stats.emplace_back( timer.empty(), text );
                const double system_time = std::get< 0 >(stats[stats.size() - 1]);
                total += system_time;
                stage_duration = std::max(stage_duration, system_time);
            }
            stage_durations += stage_duration;
        }
        stats.emplace_back(overhead.empty(), "System Manager");
        stats.emplace_back(reaping_time.empty(), "Reaping");

        std::sort(stats.begin(), stats.end(), [](const Stat &l, const Stat &r) -> bool {
            return std::get< 0 >(l) > std::get< 0 >(r);
        });

        std::cout << '\t' << total << " in " << stages.size() << " stages\n";
        for (const auto &stat : stats) {
            std::cout << '\t' << std::get< 0 >(stat) << " " << std::get< 1 >(stat) << '\n';
        }
    }
}  // namespace Entity
