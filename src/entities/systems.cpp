#include "entities/systems.h"

#include "core/core.h"

#include <functional>
#include <iostream>
#include <tuple>
#include <coz.h>

namespace Entity {
    BaseSystem::BaseSystem(const std::string &name, const ConstySignature &sig): name(name), signature(sig) { }

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
            COZ_BEGIN("SYSTEM");
            func();
            COZ_END("SYSTEM");
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
        system_timers[system.get()] = AccumulateTimer();
        systems.push_back(std::move(system));
    }

    void SystemManager::execute(Core &core, double seconds) {
        overhead.add([&](){
            for (size_t i = 0; i < stages.size(); ++i) {
                auto &stage = stages[i];
                {
                    std::lock_guard< std::mutex > lock(tex);
                    for (auto *system : stage) {
                        [this, &core, seconds](BaseSystem *system) {
                            work_queue.push([this, &core, seconds, system]() {
                                system_timers[system].add([&](){
                                    system->execute(core, seconds);
                                });
                            });
                        }(system);
                    }
                    processed = 0;
                }

                std::unique_lock< std::mutex > lock(tex);
                stage_timers[i].add([&](){
                    cv.notify_all();
                    cv.wait(lock, [&]{ return stage.size() == processed; });
                });
            }

            reaping_time.add([&](){
                core.tracker.finalizeKills(core);
            });
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
            stage_timers.resize(stages.size());
            std::set< ConstyTypeID > types;

            for (size_t i = 0; i < pile.size(); ++i) {
                const auto sig = pile[i]->signature;
                // Does this stage already operate on these types?
                bool conflict = false;
                for (ConstyTypeID tid : sig) {
                    for (const auto &other : types) {
                        if (other.second != tid.second) { continue; }
                        if (other.first && tid.first) { continue; }
                        conflict = true;
                        break;
                    }
                }

                if (conflict) { // This needs to be dealt with in some later stage
                    notfit.push_back(pile[i]);
                } else { // Add this to this stage
                    for (ConstyTypeID tid : sig) {
                        types.insert(tid);
                    }
                    stages[stages.size() - 1].push_back(pile[i]);
                }
            }

            pile = std::move(notfit);
            if (core.options.count("verbose")) {
                std::cout << "Stage " << stages.size() << ":\n";
                for (auto sys : stages[stages.size() - 1]) {
                    std::cout << '\t' << sys->name << ": " << signatureString(sys->signature) << '\n';
                }
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
        typedef std::vector< Stat > Stats;
        typedef std::tuple< double, std::string, Stats > StatGroup;
        std::vector< StatGroup > statgroups;
        statgroups.reserve(systems.size() + 1);

        double stage_times = 0.0;
        for (size_t i = 0; i < stages.size(); ++i) {
            Stats stats;
            const auto &stage = stages[i];
            for (const auto *system : stage) {
                auto &timer = system_timers[system];
                std::string text = system->name + ": " + signatureString(system->signature);
                stats.emplace_back( timer.empty(), text );
            }
            const double stage_time = stage_timers[i].empty();
            stage_times += stage_time;

            std::sort(stats.begin(), stats.end(), [](const Stat &l, const Stat &r) -> bool {
                return std::get< 0 >(l) > std::get< 0 >(r);
            });

            statgroups.emplace_back(stage_time, "Stage " + std::to_string(i + 1), stats);
        }

        {
            Stats stats;
            const double reaping = reaping_time.empty();
            stats.emplace_back(reaping, "Reaping");

            double overhead_time = overhead.empty();
            overhead_time -= reaping;
            overhead_time -= stage_times;
            stats.emplace_back(overhead_time, "System Manager");

            double admin = 0.0;
            for (const auto &stat : stats) {
                admin += std::get< 0 >(stat);
            }

            std::sort(stats.begin(), stats.end(), [](const Stat &l, const Stat &r) -> bool {
                return std::get< 0 >(l) > std::get< 0 >(r);
            });
            statgroups.emplace_back(admin, "Admininstration", stats);
        }

        std::sort(statgroups.begin(), statgroups.end(), [](const StatGroup &l, const StatGroup &r) -> bool {
            return std::get< 0 >(l) > std::get< 0 >(r);
        });

        for (const auto &group : statgroups) {
            std::cout << "\t" << std::get< 1 >(group) << ": " << std::get< 0 >(group) << '\n';
            for (const auto &stat : std::get< 2 >(group)) {
                std::cout << "\t\t" << std::get< 0 >(stat) << " " << std::get< 1 >(stat) << '\n';
            }
        }

        std::cout.flush();
    }
}  // namespace Entity
