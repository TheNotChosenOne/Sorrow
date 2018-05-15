#include "logicManager.h"

#include <SDL2/SDL.h>

#include <algorithm>

#include "physicsManager.h"
#include "entityManager.h"
#include "visualManager.h"
#include "logicManager.h"

void LogicManager::updater(Core &core, const Components &lasts, Components &nexts) {
    const auto &targetPhys = core.physics.get(core.player);
    for (size_t i = 0; i < nexts.size(); ++i) {
        if (lasts[i].empty()) { continue; }
        nexts[i] = lasts[i];
        auto &log = nexts[i];
        auto &phys = core.physics.get(i);
        if (log.count("drone")) {
            Vec diff = targetPhys.pos - phys.pos;
            gmtl::normalize(diff);
            phys.impulse += diff * log["speed"];
            for (const auto &k : phys.contacts) {
                const auto &blog = lasts[k.which];
                double dmg = k.force;
                if (blog.count("dmg")) {
                    dmg += blog.at("dmg");
                }
                log["hp"] = std::max(0.0, log["hp"] - dmg);
                if (0.0 == log["hp"]) {
                    std::cout << "RIP: " << i << '\n';
                    core.visuals.get(i).colour = Vec3(0x77, 0x77, 0x77);
                    log.erase("drone");
                    log["debris"] = 1;
                    log["lifetime"] = std::numeric_limits< double >::infinity();
                    phys.gather = false;
                    break;
                }
            }
        } else if (log.count("bullet")) {
            if (!phys.contacts.empty()) {
                log.erase("bullet");
                log["debris"] = 1;
                phys.gather = false;
            }
        } else if (log.count("debris")) {
            log["lifetime"] = std::max(0.0, log["lifetime"] - PHYSICS_TIMESTEP);
            if (0.0 == log["lifetime"]) {
                core.entities.kill(i);
            }
        } else if (log.count("player")) {
            const double speed = log["speed"];
            if (core.input.isHeld(SDLK_a)) {
                if (phys.vel[0] > -speed) {
                    phys.acc[0] += std::max(-speed, phys.acc[0] - speed);
                }
            }
            if (core.input.isHeld(SDLK_d)) {
                if (phys.vel[0] < speed) {
                    phys.acc[0] += std::min(speed, phys.acc[0] + speed);
                }
            }
            if (core.input.isHeld(SDLK_w)) {
                if (gmtl::dot(Vec(0, 1), phys.surface) > 0.75) {
                    phys.acc[1] += 0.011 * (1 / PHYSICS_TIMESTEP) * speed;
                }
            }
            log["reload"] = std::max(0.0, log["reload"] - PHYSICS_TIMESTEP);
            if (core.input.mouseHeld(SDL_BUTTON_LEFT)) {
                if (0.0 == log["reload"]) {
                    log["reload"] = log["reloadTime"];
                    static const double brad = 0.5;
                    Vec dir = core.visuals.screenToWorld(core.input.mousePos()) - phys.pos;
                    gmtl::normalize(dir);
                    Entity b = core.entities.create();
                    auto &bphys = core.physics.get(b);
                    bphys.pos = phys.pos + dir * (1.0001 * (phys.rad[0] + brad));
                    bphys.rad = { brad, brad };
                    bphys.impulse = dir * log["bulletForce"] * (1.0 / PHYSICS_TIMESTEP);
                    phys.impulse -= bphys.impulse;
                    bphys.area = 2 * brad;
                    bphys.mass = pi< double > * brad * brad / 1.0;
                    bphys.shape = Shape::Circle;
                    bphys.isStatic = false;
                    bphys.elasticity = 0.95;
                    bphys.phased = false;
                    bphys.gather = true;

                    auto &vis = core.visuals.get(b);
                    vis.draw = true;
                    vis.colour = Vec3(0xFF, 0, 0);

                    auto &blog  = core.logic.get(b);
                    blog["dmg"] = 1;
                    blog["bullet"] = 1;
                    blog["lifetime"] = log["blifetime"];
                }
            }
        }
    }
    const double interp = 0.00000001;
    const Vec target = targetPhys.pos - (core.visuals.FOV / 2.0);
    core.visuals.cam = interp * core.visuals.cam + (1.0 - interp) * target;
}

void LogicManager::logicUpdate(Core &core) {
    ComponentManager::update([this, &core](const Components &l, Components &n) {
        updater(core, l, n);
    });
}
