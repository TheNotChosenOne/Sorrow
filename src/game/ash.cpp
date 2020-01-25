#include "game/ash.h"

#include "game/npc.h"
#include "core/core.h"
#include "visual/camera.h"
#include "visual/visuals.h"
#include "physics/physics.h"
#include "input/controller.h"
#include "entities/tracker.h"
#include "physics/geometry.h"

ASHGame::ASHGame() { }
ASHGame::~ASHGame() { }

void ASHGame::registration(Core &core) {
    core.tracker.addSource< ColourData >();
    core.systems.addSystem(std::make_unique< ControllerSystem >());
    core.systems.addSystem(std::make_unique< PhysicsSystem >());
    core.systems.addSystem(std::make_unique< DamageSystem >());
    core.systems.addSystem(std::make_unique< SeekerSystem >());
    core.systems.addSystem(std::make_unique< TurretSystem >());
    core.systems.addSystem(std::make_unique< LifetimeSystem >());
}

void ASHGame::unregister(Core &) {
}

void ASHGame::create(Core &core) {

    const auto bul = getStandardBulletCreator(BulletInfo{
        2.0, 0.50, 0.25, 1.0, false
    });

    const auto mis = getStandardBulletCreator(BulletInfo{
        10.0, 1.0, 0.5, 2.0, true
    });

    core.tracker.createWith(core,
        PhysBody{ makeCircle(core, Point(-64.0, 0.0), 3.0, true, false) },
        Colour{ { 0xAA, 0xAA, 0xAA } },
        HitData{},
        Controller{ KeyboardController },
        TurretController{ KeyboardTurretController },
        Team{ 1 },
        Damage{ std::numeric_limits< double >::infinity() },
        TargetValue{ 1.0 },
        fullHealth(3.0),
        Turret{ "Missile", mis, 2.0, 0.0, 0.0, false },
        Turret{ "Turret", bul, 0.01, 0.0, 0.0, false }
    );

    core.tracker.createWith(core,
        PhysBody{ makeCircle(core, Point(64.0, 0.0), 3.0, true, false) },
        Colour{ { 0xAA, 0xAA, 0xAA } },
        HitData{},
        Controller{ KeyboardController },
        TurretController{ KeyboardTurretController },
        Team{ 2 },
        Damage{ std::numeric_limits< double >::infinity() },
        TargetValue{ 1.0 },
        fullHealth(3.0),
        Turret{ "Missile", mis, 2.0, 0.0, 0.0, false },
        Turret{ "Turret", bul, 0.01, 0.0, 0.0, false }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(-128, 0), 16.0, 256 + 16, false, false) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(128, 0), 16.0, 256 + 16, false, false) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0, 128.0), 256 + 16.0, 16, false, false) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0, -128.0), 256 + 16.0, 16, false, false) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

}

void ASHGame::cleanup(Core &) {
}

bool ASHGame::update(Core &) {
    return false;
}
