#include "controller.h"

#include "physics/physics.h"
#include "entities/exec.h"
#include "core/core.h"
#include "input/input.h"
#include "game/npc.h"
#include "visual/visuals.h"

#include <SDL2/SDL.h>
#include <memory>

void KeyboardController(Core &core, PhysBody &pb, Entity::EntityID eid) {
    b2Body *body = pb.body;
    const auto centre = body->GetPosition();
    const double mass = body->GetMass();
    const double hSpeed = 100.0 * mass;
    //const double vSpeed = 250.0 * mass;
    if (core.input.isHeld(SDLK_w)) {
        //body->ApplyForce(b2Vec2(0.0, vSpeed), centre, true);
        body->ApplyForce(b2Vec2(0.0, hSpeed), centre, true);
    }
    if (core.input.isHeld(SDLK_s)) {
        body->ApplyForce(b2Vec2(0.0, -hSpeed), centre, true);
        // Nobody cares
    }
    if (core.input.isHeld(SDLK_a)) {
        body->ApplyForce(b2Vec2(-hSpeed, 0.0), centre, true);
    }
    if (core.input.isHeld(SDLK_d)) {
        body->ApplyForce(b2Vec2(hSpeed, 0.0), centre, true);
    }
    if (core.input.mouseHeld(SDL_BUTTON_LEFT)) {
        const auto moused = core.input.mouseToWorld(core);
        const auto dir = normalized(moused - PCast(centre));
        b2Body *body = makeBall(core, PCast(centre) + dir, 0.5);
        body->SetLinearVelocity(VCast(dir * 128.0));

        const auto optTeam = core.tracker.optComponent< Team >(eid);
        if (optTeam) {
            core.tracker.createWith(core, PhysBody{ body }, Colour{ { 0xFF, 165, 0 } }, Damage{ 1.0 }, Lifetime{ 2.0 }, Team{ optTeam->get().team }, fullHealth(0.1), HitData{} );
        } else {
            core.tracker.createWith(core, PhysBody{ body }, Colour{ { 0xFF, 165, 0 } }, Damage{ 1.0 }, Lifetime{ 2.0 }, fullHealth(1.0), HitData{} );
        }
    }
}

ControllerSystem::ControllerSystem(): BaseSystem("Controller", Entity::getSignature< PhysBody, const Controller, Team >()) {
}

ControllerSystem::~ControllerSystem() { }

void ControllerSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< ControllerData >());
}

void ControllerSystem::execute(Core &core, double) {
    Entity::Exec< Entity::Packs< PhysBody, const Controller > >::run(core.tracker,
    [&](auto &data) {
        auto &controllers = data.first.template get< const Controller >();
        auto &pbs = data.first.template get< PhysBody >();
        for (size_t i = 0; i < pbs.size(); ++i) {
            controllers[i].controller(core, pbs[i], data.second[i]);
        }
    });
}
