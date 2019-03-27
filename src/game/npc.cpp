#include "npc.h"

#include "core/core.h"
#include "core/geometry.h"
#include "visual/visuals.h"
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

void processSeeker(Core &core) {
    Entity::ExecSimple< PhysBody, const Seeker >::run(core.tracker,
    [&](auto &bodies, const auto &seekers) {
        for (size_t i = 0; i < seekers.size(); ++i) {
            Entity::EntityID tid = seekers[i].target;
            if (!core.tracker.alive(tid)) {
                //std::cout << "No such target: " << tid << '\n';
                continue;
            }
            const auto tbody = core.tracker.optComponent< PhysBody >(tid);
            if (!tbody) {
                //std::cout << "Target has no body: " << tid << '\n';
                continue;
            }
            const b2Body *target = tbody->get().body;
            const auto target_at = VCast(target->GetPosition());
            //const auto velo = VCast(target->GetLinearVelocity());

            const auto m_body = bodies[i].body;
            const auto vec_to = target_at - VCast(m_body->GetPosition());
            //const double dist_targ = std::sqrt(vec_to.squared_length());

            const auto go = 1000.0 * normalized(vec_to);
            m_body->ApplyForceToCenter(VCast(go), true);

            //std::cout << "M: " << VCast(m_body->GetPosition()) << " T: " << target_at << " V: " << go << '\n';

            //const auto target_at = tbody->get().body->GetPosition();
                        //healths[i].hp = std::max(0.0, healths[i].hp - optDmg->get().dmg);
        }
    });
}

void processTurrets(Core &core, double seconds) {
    Entity::Exec<
        Entity::Packs< const PhysBody, const Team >,
        Entity::Packs< const PhysBody, const Team, Turret >
    >::run(core.tracker,
    [&](auto &noturrets, auto &turreters) {
        auto &turrets = turreters.first.template get< Turret >();
        auto &tteams = turreters.first.template get< const Team >();
        auto &tbodies = turreters.first.template get< const PhysBody >();
        auto &nteams = noturrets.first.template get< const Team >();
        auto &nbodies = noturrets.first.template get< const PhysBody >();
        for (size_t i = 0; i < turrets.size(); ++i) {
            if (0.0 != turrets[i].cooldown) {
                turrets[i].cooldown = std::max(0.0, turrets[i].cooldown - seconds);
            }
            if (0.0 != turrets[i].cooldown) { continue; }
            const double range_square = turrets[i].range * turrets[i].range;
            const auto body = tbodies[i];
            const auto source_at = VCast(body.body->GetPosition());
            const auto check_target = [&](const PhysBody &target, const Team target_team, const Entity::EntityID tid) -> bool {
                if (target_team.team == tteams[i].team) { return false; }
                const auto centre = VCast(target.body->GetPosition());
                const Vec vec_to = centre - source_at;
                if (vec_to.squared_length() > range_square) { return false; }
                //std::cout << source_at << ' ' << vec_to << ' ' << VCast(target.body->GetPosition()) << std::endl;
                //std::cout << target_team.team << ' ' << tteams[i].team << std::endl;
                turrets[i].cooldown = turrets[i].cooldown_length;
                const double bul_rad = 0.25;
                const b2Body *bd = body.body;
                const b2Fixture *fixes = bd->GetFixtureList();
                const b2Shape *shape = fixes->GetShape();
                const b2CircleShape * circ = static_cast< const b2CircleShape * >(shape);
                const auto offset = source_at + 1.5 * (bul_rad + circ->m_radius) * normalized(vec_to);
                const auto pointy = Point( offset.x(), offset.y() );
                auto bul_body = makeBall(core, pointy, bul_rad);
                const auto go = 100.0 * VCast(normalized(vec_to));
                bul_body->ApplyLinearImpulse(go, bul_body->GetPosition(), true);
                core.tracker.createWith(core,
                    PhysBody{ bul_body },
                    Colour{ { 0xFF, 0, 0 } },
                    Team({ tteams[i].team }),
                    Damage{ turrets[i].dmg },
                    Lifetime{ turrets[i].lifetime },
                    Seeker({ tid })
                );
                return true;
            };
            bool shot = false;
            for (size_t j = 0; j < tbodies.size(); ++j) {
                if  (i != j && check_target(tbodies[j], tteams[j], noturrets.second[j])) { shot = true; break; }
            }
            if (!shot) {
                for (size_t j = 0; j < nbodies.size(); ++j) {
                    if  (check_target(nbodies[j], nteams[j], turreters.second[j])) { break; }
                }
            }
        }
    });
}
