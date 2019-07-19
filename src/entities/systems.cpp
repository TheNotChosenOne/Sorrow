#include "systems.h"

#include "core/core.h"

#include <functional>
#include <iostream>
#include <tuple>

namespace Entity {
    BaseSystem::BaseSystem(const std::string &name, const Signature sig): name(name), signature(sig) { }

    BaseSystem::~BaseSystem() { }

    void BaseSystem::init(Core &)  { }

    void SystemManager::addSystem(std::unique_ptr< BaseSystem > system) {
        systems.push_back(std::move(system));
        timers.emplace_back();
    }

    void SystemManager::execute(Core &core, double seconds) {
        for (size_t i = 0; i < systems.size(); ++i) {
            timers[i].add([&](){
                systems[i]->execute(core, seconds);
            });
        }
    }

    void SystemManager::init(Core &core) {
        for (auto &s : systems) {
            s->init(core);
        }
    }

    void SystemManager::dumpTimes() {
        typedef std::tuple< double, std::string > Stat;
        std::vector< Stat > stats;
        stats.reserve(systems.size());

        for (size_t i = 0; i < systems.size(); ++i) {
            std::string text = systems[i]->name + ": " + signatureString(systems[i]->signature);
            stats.emplace_back( timers[i].empty(), text );
        }
        std::sort(stats.begin(), stats.end(), [](const Stat &l, const Stat &r) -> bool {
            return std::get< 0 >(l) > std::get< 0 >(r);
        });

        for (const auto &stat : stats) {
            std::cout << '\t' << std::get< 0 >(stat) << " " << std::get< 1 >(stat) << '\n';
        }
    }
}  // namespace Entity
