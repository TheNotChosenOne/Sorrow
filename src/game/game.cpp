#include "game/game.h"

#include <Box2D.h>
#include <memory>

#include "game/swarm.h"
#include "input/controller.h"
#include "physics/physics.h"
#include "visual/visuals.h"
#include "visual/camera.h"
#include "core/core.h"
#include "core/geometry.h"
#include "game/npc.h"

static const size_t WORLD_SIZE = 1000.0;

Game::Game() { }

Game::~Game() { }

double Game::gravity() const {
    return 0.0;
}

SwarmGame::SwarmGame() { }

SwarmGame::~SwarmGame() { }

void SwarmGame::registration(Core &core) {
    core.tracker.addSource< ColourData >();
    core.systems.addSystem(std::make_unique< ControllerSystem >());
    core.systems.addSystem(std::make_unique< PhysicsSystem >());
    core.systems.addSystem(std::make_unique< DamageSystem >());
    core.systems.addSystem(std::make_unique< SeekerSystem >());
    core.systems.addSystem(std::make_unique< TurretSystem >());
    core.systems.addSystem(std::make_unique< SwarmSystem >());
    core.systems.addSystem(std::make_unique< HiveTrackerSystem >());
    core.systems.addSystem(std::make_unique< HiveSpawnerSystem >());
    core.systems.addSystem(std::make_unique< LifetimeSystem >());
    core.systems.addSystem(std::make_unique< CameraSystem >());
}

void SwarmGame::create(Core &core) {
    const size_t bugs = core.options["c"].as< size_t >();
    const Point3 colours[] = { { 0xFF, 0xFF, 0xFF }, { 0xFF, 0, 0 }, { 0, 0xAA, 0 }, { 0, 0, 0xFF } };

    for (size_t i = 0; i < 4; ++i) {
        core.tracker.createWith(core,
            Hive{
                uint8_t(i),
                colours[i],
                bugs,
                0,
                0.0,
                10
            }
        );
    }
}

void SwarmGame::unregister(Core &) {
}

void SwarmGame::cleanup(Core &core) {
    core.tracker.killAll(core);
}

bool SwarmGame::update(Core &) {
    return false;
}
