#include "physics/physics.h"
#include "input/controller.h"
#include "visual/visuals.h"
#include "visual/camera.h"
#include "input/input.h"
#include "core/core.h"
#include "game/npc.h"

#include <iostream>
#include <random>
#include <memory>

#include "game/stress.h"

GenerationSystem::GenerationSystem()
    : BaseSystem("Generation", Entity::getSignature< Generation, PhysBody, Colour, Lifetime >()) { }

GenerationSystem::~GenerationSystem() { }

void GenerationSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< GenerationData >());
}

void GenerationSystem::execute(Core &core, double seconds) {
    std::vector< size_t > growing;
    Entity::ExecSimple< Generation >::run(core.tracker,
    [&](const auto &, auto &generations) {
        for (auto &gen : generations) {
            gen.age += seconds;
            if (gen.age > 1.0 && gen.generation < 10) {
                growing.push_back(gen.generation);
            }
        }
    });
    if (!growing.empty()) {
        std::mt19937_64 rng;
        std::uniform_real_distribution< double > distro(0.5, 1.2);
        for (const auto gen : growing) {
            core.tracker.createWith(core, PhysBody{ randomBall(core, 10.0) }, Colour{ { 0xFF, 0, 0 } }, Lifetime{ distro(rng) }, Generation{ 0.0, gen + 1 });
        }
    }
}

Stresser::Stresser() { }

Stresser::~Stresser() { }

void Stresser::registration(Core &core) {
    core.tracker.addSource(std::make_unique< ColourData >());
    core.tracker.addSource(std::make_unique< GenerationData >());
    core.systems.addSystem(std::make_unique< ControllerSystem >());
    core.systems.addSystem(std::make_unique< PhysicsSystem >());
    core.systems.addSystem(std::make_unique< GenerationSystem >());
    core.systems.addSystem(std::make_unique< DamageSystem >());
    core.systems.addSystem(std::make_unique< LifetimeSystem >());
    core.systems.addSystem(std::make_unique< CameraSystem >());
}

void Stresser::unregister(Core &) { }

void Stresser::cleanup(Core &core) {
    core.tracker.killAll(core);
}

bool Stresser::update(Core &core) {
    return core.tracker.count() == 0;
}

void Stresser::create(Core &core) {
    core.tracker.createWith(core, PhysBody{ randomBall(core, 0.0) }, Colour{ { 0xFF, 0, 0 } }, Lifetime{ 1.2 }, Generation{ 0.0, 1 });
}
