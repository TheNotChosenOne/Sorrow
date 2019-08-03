#include "game.h"

#include "Box2D.h"
#include <memory>

#include "game/swarm.h"
#include "game/controller.h"
#include "physics/physics.h"
#include "visual/visuals.h"
#include "visual/camera.h"
#include "core/core.h"
#include "game/npc.h"

static const size_t WORLD_SIZE = 1000.0;

Game::Game() { }

Game::~Game() { }

SwarmGame::SwarmGame() { }

SwarmGame::~SwarmGame() { }

void SwarmGame::registration(Core &core) {
    core.tracker.addSource(std::move(std::make_unique< ColourData >()));
    core.systems.addSystem(std::move(std::make_unique< ControllerSystem >()));
    core.systems.addSystem(std::move(std::make_unique< PhysicsSystem >()));
    core.systems.addSystem(std::move(std::make_unique< DamageSystem >()));
    core.systems.addSystem(std::move(std::make_unique< SeekerSystem >()));
    core.systems.addSystem(std::move(std::make_unique< TurretSystem >()));
    core.systems.addSystem(std::move(std::make_unique< SwarmSystem >()));
    core.systems.addSystem(std::move(std::make_unique< LifetimeSystem >()));
    core.systems.addSystem(std::move(std::make_unique< CameraSystem >()));
}

void SwarmGame::create(Core &core) {
    static const size_t groupRoot = 2;
    const size_t bugs = core.options["c"].as< size_t >();
    const Point3 colours[] = { { 0xFF, 0xFF, 0xFF }, { 0xFF, 0, 0 }, { 0, 0xAA, 0 }, { 0, 0, 0xFF } };
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
            Turret{ 2.0, rnd_range(0.0, 2.0), 60, 0.4, 3.0 },
            Turret2{ 0.2, rnd_range(0.0, 0.2), 15.0, 0.1, 0.5 },
            MouseFollow{}
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
