#include "game/hall.h"

#include "game/npc.h"
#include "core/core.h"
#include "visual/camera.h"
#include "visual/visuals.h"
#include "physics/physics.h"
#include "input/controller.h"
#include "entities/tracker.h"
#include "physics/geometry.h"

HallGame::HallGame() { }
HallGame::~HallGame() { }

void HallGame::registration(Core &core) {
    core.tracker.addSource< ColourData >();
    core.systems.addSystem(std::make_unique< ControllerSystem >());
    core.systems.addSystem(std::make_unique< PhysicsSystem >());
    core.systems.addSystem(std::make_unique< DamageSystem >());
    core.systems.addSystem(std::make_unique< SeekerSystem >());
    core.systems.addSystem(std::make_unique< TurretSystem >());
    core.systems.addSystem(std::make_unique< LifetimeSystem >());
    core.systems.addSystem(std::make_unique< CameraSystem >());
}

double HallGame::gravity() const {
    return -98.0;
}

void HallGame::unregister(Core &) {
}

void HallGame::create(Core &core) {
    // Make the floor
    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0.0, -5.0), 128.0, 5.0, false) },
        Colour{ { 0, 0xFF, 0 } }
    );

    // Make the player
    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0.0, 5.0), 3.0, 8.0, true, false) },
        Colour{ { 0xAA, 0xAA, 0xAA } },
        HitData{},
        Damage{ 0.2 },
        Controller{ KeyboardController },
        Team{ 0 },
        fullHealth( std::numeric_limits< double >::infinity() )
    );

    // Make a turret
    /*
    core.tracker.createWith(core,
        PhysBody{ makeCircle(core, Point(10.0, 10.0), 2.0, false) },
        Colour{ { 0xFF, 0, 0 } },
        HitData{},
        fullHealth(10.0),
        Team{ 1 },
        Turret{ 2.0, 0.0, 60.0, 0.4, 2.0, 0.25, 0.01, true },
        Turret{ 0.2, 0.0, 60.0, 0.4, 2.0, 0.25, 0.01, false }
    );
    */
}

void HallGame::cleanup(Core &) {
}

bool HallGame::update(Core &) {
    return false;
}
