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

    /*
    static const size_t groupRoot = 2;
    for (size_t i = 0; i < bugs; ++i) {
        b2Body *body = randomBall(core, 500.0);
        const double x = body->GetPosition().x;
        const double y = body->GetPosition().y;

        const double gx = ((x + WORLD_SIZE / 2.0) / WORLD_SIZE) * groupRoot;
        const double gy = ((y + WORLD_SIZE / 2.0) / WORLD_SIZE) * groupRoot;

        const int64_t gridX = clamp(int64_t(0), int64_t(groupRoot - 1), static_cast< int64_t >(gx));
        const int64_t gridY = clamp(int64_t(0), int64_t(groupRoot - 1), static_cast< int64_t >(gy));
        const uint16_t tag = gridX * groupRoot + gridY;

        const auto colour = colours[tag];
        const auto pusher = 10000.0  * (body->GetPosition() - (b2Vec2(WORLD_SIZE / 2.0, WORLD_SIZE / 2.0)));
        body->ApplyForceToCenter(pusher, true);

        core.tracker.createWith(core,
            PhysBody{ body },
            Colour{ colour },
            HitData{},
            SwarmTag{ tag },
            fullHealth(10.0),
            Team{ tag },
            Damage{ 0.2 },
            Turret{  2.0, rnd_range(0.0, 2.0), 60.0, 0.4, 2.0, 0.25, 0.01, true },
            Turret2{ 0.2, rnd_range(0.0, 0.2), 15.0, 0.1, 0.25, 0.05, 0.0, false },
            MouseFollow{}
        );
    }
    */
}

void SwarmGame::unregister(Core &) {
}

void SwarmGame::cleanup(Core &core) {
    core.tracker.killAll(core);
}

bool SwarmGame::update(Core &) {
    return false;
}
