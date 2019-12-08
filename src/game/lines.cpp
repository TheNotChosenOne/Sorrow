#include "physics/physics.h"
#include "input/controller.h"
#include "visual/visuals.h"
#include "visual/camera.h"
#include "input/input.h"
#include "game/swarm.h"
#include "core/core.h"
#include "game/npc.h"

#include <iostream>

#include "game/lines.h"

Liner::Liner() { }
Liner::~Liner() { }

void Liner::registration(Core &core) {
    core.tracker.addSource(std::make_unique< ColourData >());
    core.tracker.addSource(std::make_unique< MouseFollowData >());
    core.tracker.addSource(std::make_unique< SeekerData >());
    core.systems.addSystem(std::make_unique< ControllerSystem >());
    core.systems.addSystem(std::make_unique< PhysicsSystem >());
    core.systems.addSystem(std::make_unique< CameraSystem >());
}

void Liner::unregister(Core &) { }

void Liner::cleanup(Core &core) {
    core.tracker.killAll(core);
}

bool Liner::update(Core &) {
    return false;
}

void Liner::create(Core &core) {
    const auto tid = core.tracker.createWith(core,
        PhysBody{ randomBall(core, 1.0) },
        Colour{ { 0x00, 0x00, 0x00 } }
    );
    core.tracker.createWith(core,
        PhysBody{ randomBall(core, 1.0) },
        Colour{ { 0xFF, 0xFF, 0xFF } },
        Controller{ KeyboardController },
        Seeker{ tid }
    );
}
