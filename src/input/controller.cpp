#include "input/controller.h"

#include "physics/physics.h"
#include "physics/geometry.h"
#include "entities/exec.h"
#include "core/core.h"
#include "input/input.h"
#include "game/npc.h"
#include "visual/visuals.h"

#include <SDL2/SDL.h>
#include <memory>

namespace {

bool keyHeld(Core &core, const Layout &layout, const std::string &key) {
    const auto loc = layout.find(key);
    if (loc == layout.end()) {
        return false;
    }

    return core.input.isHeld(loc->second);
}

/*
bool mouseHeld(Core &core, const Layout &layout, const std::string &key) {
    const auto loc = layout.find(key);
    if (loc == layout.end()) {
        return false;
    }

    return core.input.mouseHeld(loc->second);
}
*/

}

// if (core.input.isHeld(SDLK_a)) {
// SDL_BUTTON_LEFT
void KeyboardController(Core &core, PhysBody &pb, Entity::EntityID, const Layout &layout) {
    b2Body *body = pb.body;
    const auto centre = body->GetPosition();
    const double mass = body->GetMass();
    const double speed = 100.0 * mass;

    if (keyHeld(core, layout, "up")) {
        body->ApplyForce(b2Vec2(0.0, speed), centre, true);
    }
    if (keyHeld(core, layout, "down")) {
        body->ApplyForce(b2Vec2(0.0, -speed), centre, true);
    }
    if (keyHeld(core, layout, "left")) {
        body->ApplyForce(b2Vec2(-speed, 0.0), centre, true);
    }
    if (keyHeld(core, layout, "rite")) {
        body->ApplyForce(b2Vec2(speed, 0.0), centre, true);
    }
}

void KeyboardTurretController(Core &core, std::vector< Turret > &turrets, Entity::EntityID eid, const Layout &layout) {
    const bool fire = keyHeld(core, layout, "fire");
    const bool alt = keyHeld(core, layout, "altFire");

    if (!fire && !alt) {
        return;
    }

    const auto phys = core.tracker.optComponent< PhysBody >(eid);
    if (!phys) { return; }

    const auto centre = phys->get().body->GetPosition();
    const auto direction = (centre.x > 0) ? Vec(-1.0, 0.0) : Vec(1.0, 0.0);
    const auto at = VPC< Point >(centre) + direction;

    if (fire) {
        for (auto &turret : turrets) {
            if (turret.name != "primary") { continue; }
            if (turret.trigger()) {
                turret.bullet(core, eid, at, direction, std::nullopt);
            }
        }
    }

    if (alt) {
        for (auto &turret : turrets) {
            if (turret.name != "secondary") { continue; }
            if (turret.trigger()) {
                turret.bullet(core, eid, at, direction, std::nullopt);
            }
        }
    }
}

ControllerSystem::ControllerSystem(): BaseSystem("Controller", Entity::getConstySignature< PhysBody, Turret, const TurretController, const Controller, const Team >()) {
}

ControllerSystem::~ControllerSystem() { }

void ControllerSystem::init(Core &core) {
    core.tracker.addSource< ControllerData >();
    core.tracker.addSource< TurretControllerData >();
    core.tracker.addSource< TeamData >();
}

void ControllerSystem::execute(Core &core, double) {
    Entity::Exec< Entity::Packs< PhysBody, const Controller > >::run(core.tracker,
    [&](auto &data) {
        auto &controllers = data.first.template get< const Controller >();
        auto &pbs = data.first.template get< PhysBody >();
        for (size_t i = 0; i < pbs.size(); ++i) {
            controllers[i].controller(core, pbs[i], data.second[i], controllers[i].layout);
        }
    });
    Entity::Exec< Entity::Packs< Turret, const TurretController > >::run(core.tracker,
    [&](auto &data) {
        auto &controllers = data.first.template get< const TurretController >();
        auto &pbs = data.first.template get< Turret >();
        for (size_t i = 0; i < pbs.size(); ++i) {
            controllers[i].controller(core, pbs[i], data.second[i], controllers[i].layout);
        }
    });
}
