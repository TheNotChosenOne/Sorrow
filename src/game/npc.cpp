#include "npc.h"

#include "core/core.h"
#include "physics/physics.h"
#include "entities/exec.h"

Health fullHealth(double hp) {
    return Health{ hp, hp };
}

b2Body *makeBall(Core &core, Point centre, double rad) {
    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(centre.x(), centre.y());

    b2CircleShape circle;
    circle.m_radius = rad;

    b2FixtureDef fixture;
    fixture.density = 15.0f;
    fixture.friction = 0.7f;
    fixture.shape = &circle;


    b2Body *body = core.b2world->CreateBody(&def);
    body->CreateFixture(&fixture);
    return body;
}

void processDamage(Core &core) {
    std::vector< Entity::EntityID > kill;
    Entity::Exec< Entity::Packs< HitData, Health >, Entity::Packs< HitData, Health, Team > >::run(core.tracker,
    [&](auto &noteam, auto &team) {
        {
            auto &hits = noteam.first.template get< HitData >();
            auto &healths = noteam.first.template get< Health >();
            for (size_t i = 0; i < hits.size(); ++i) {
                for (auto hit : hits[i].id) {
                    const auto optDmg = core.tracker.optComponent< Damage >(hit);
                    if (optDmg) {
                        healths[i].hp = std::max(0.0, healths[i].hp - optDmg->get().dmg);
                        if (0.0 == healths[i].hp) {
                            kill.push_back(noteam.second[i]);
                        }
                    }
                }
            }
        }
        {
            auto &hits = team.first.template get< HitData >();
            auto &healths = team.first.template get< Health >();
            auto &teams = team.first.template get< Team >();
            for (size_t i = 0; i < hits.size(); ++i) {
                for (auto hit : hits[i].id) {
                    const auto optTeam = core.tracker.optComponent< Team >(hit);
                    if (optTeam && optTeam->get().team == teams[i].team) { continue; }
                    const auto optDmg = core.tracker.optComponent< Damage >(hit);
                    if (optDmg) {
                        healths[i].hp = std::max(0.0, healths[i].hp - optDmg->get().dmg);
                        if (0.0 == healths[i].hp) {
                            kill.push_back(team.second[i]);
                        }
                    }
                }
            }
        }
    });
    for (const auto eid : kill) {
        core.tracker.killEntity(core, eid);
    }
}

void processLifetimes(Core &core, double seconds) {
    std::vector< Entity::EntityID > kill;
    Entity::Exec< Entity::Packs< Lifetime > >::run(core.tracker,
    [&](auto &data) {
        auto &lifetimes = data.first.template get< Lifetime >();
        for (size_t i = 0; i < lifetimes.size(); ++i) {
            lifetimes[i].seconds -= seconds;
            if (lifetimes[i].seconds <= 0.0) {
                kill.push_back(data.second[i]);
            }
        }
    });
    for (const auto eid : kill) {
        core.tracker.killEntity(core, eid);
    }
}
