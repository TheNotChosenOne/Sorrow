#include "controller.h"

#include "physics/physics.h"
#include "entities/exec.h"
#include "core/core.h"
#include "input/input.h"

#include <SDL2/SDL.h>

void KeyboardController(Core &core, PhysBody &pb) {
    b2Body *body = pb.body;
    const auto centre = body->GetPosition();
    const double mass = body->GetMass();
    const double hSpeed = 100.0 * mass;
    const double vSpeed = 250.0 * mass;
    if (core.input.isHeld(SDLK_w)) {
        body->ApplyForce(b2Vec2(0.0, vSpeed), centre, true);
    }
    if (core.input.isPressed(SDLK_s)) {
        // Nobody cares
    }
    if (core.input.isHeld(SDLK_a)) {
        body->ApplyForce(b2Vec2(-hSpeed, 0.0), centre, true);
    }
    if (core.input.isHeld(SDLK_d)) {
        body->ApplyForce(b2Vec2(hSpeed, 0.0), centre, true);
    }
}

void applyControllers(Core &core) {
    Entity::ExecSimple< PhysBody, const Controller >::run(core.tracker,
    [&](auto &pbs, const auto &ctrllrs) {
        for (size_t i = 0; i < pbs.size(); ++i) {
            ctrllrs[i].controller(core, pbs[i]);
        }
    });
}
