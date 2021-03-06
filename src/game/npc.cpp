#include "game/npc.h"

#include "core/core.h"
#include "core/geometry.h"
#include "visual/visuals.h"
#include "physics/physics.h"
#include "physics/geometry.h"
#include "entities/exec.h"

#include <random>

Health fullHealth(double hp) {
    return Health{ hp, hp };
}

b2Body *randomBall(Core &core, Point centre, double rad) {
    const Point p = Point(rnd(rad / 2.0), rnd(rad / 2.0));
    return makeCircle(core, Point(p.x() + centre.x(), p.y() + centre.y()), 1.0);
}

b2Body *randomBall(Core &core, double rad) {
    return randomBall(core, Point(0.0, 0.0), rad);
}

DamageSystem::DamageSystem()
    : BaseSystem("Damage", Entity::getConstySignature< Health, const HitData, const Team, const Damage >()) {
}

DamageSystem::~DamageSystem() { }

void DamageSystem::init(Core &core) {
    core.tracker.addSource< HealthData >();
    core.tracker.addSource< TeamData >();
    core.tracker.addSource< DamageData >();
}

void DamageSystem::execute(Core &core, double) {
    std::vector< Entity::EntityID > kill;
    Entity::Exec< Entity::Packs< const HitData, Health >, Entity::Packs< const HitData, Health, const Team > >::run(core.tracker,
    [&](auto &noteam, auto &team) {
        {
            const auto &hits = noteam.first.template get< const HitData >();
            auto &healths = noteam.first.template get< Health >();
            for (size_t i = 0; i < hits.size(); ++i) {
                for (const auto hit : hits[i].id) {
                    const auto optDmg = core.tracker.optComponent< const Damage >(hit);
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
            const auto &hits = team.first.template get< const HitData >();
            auto &healths = team.first.template get< Health >();
            const auto &teams = team.first.template get< const Team >();
            for (size_t i = 0; i < hits.size(); ++i) {
                for (const auto hit : hits[i].id) {
                    const auto optTeam = core.tracker.optComponent< const Team >(hit);
                    if (optTeam && optTeam->get().team == teams[i].team) { continue; }
                    const auto optDmg = core.tracker.optComponent< const Damage >(hit);
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

SeekerSystem::SeekerSystem()
    : BaseSystem("Seeker", Entity::getConstySignature< PhysBody, const Seeker, const TargetValue, const Team >()) {
}

SeekerSystem::~SeekerSystem() { }

void SeekerSystem::init(Core &core) {
    core.tracker.addSource< SeekerData >();
    core.tracker.addSource< TargetValueData >();
    core.tracker.addSource< TeamData >();
}

void SeekerSystem::execute(Core &core, double time_delta) {
    const double force = 10000.0;
    const double tick_velo = force / time_delta;

    std::vector< size_t > retargets;

    const auto seek = [&](size_t index, PhysBody &pb, Seeker &seeker, bool teamed) {
        const auto tid = seeker.target;
        if (!core.tracker.alive(tid)) {
            if (teamed && seeker.retargeting) {
                retargets.push_back(index);
            }
            return;
        }

        const auto target_body = core.tracker.optComponent< PhysBody >(tid);
        if (!target_body) {
            return;
        }

        const auto body = pb.body;
        const b2Body *target = target_body->get().body;

        const auto target_at = VPC< Vec >(target->GetPosition());
        const auto me_at = VPC< Vec >(body->GetPosition());
        const double estimate_intercept_t = (target_at - me_at).squared_length() / tick_velo;

        const auto target_velo = VPC< Vec >(target->GetLinearVelocity());
        const auto target_predicted = target_at + target_velo * estimate_intercept_t * 2.0;

        const auto vec_to = target_predicted - me_at;

        const auto go = force * normalized(vec_to);
        body->ApplyForceToCenter(VPC< b2Vec2 >(go), true);
    };

    Entity::Exec<
        Entity::Packs< PhysBody, Seeker >,
        Entity::Packs< PhysBody, Seeker, const Team >
    >::run(core.tracker,
    [&](auto &unteamed, auto &teamed) {
        for (size_t i = 0; i < unteamed.second.size(); ++i) {
            seek(i, unteamed.first.template get< PhysBody >()[i],
                    unteamed.first.template get< Seeker >()[i],
                    false);
        }
        for (size_t i = 0; i < teamed.second.size(); ++i) {
            seek(i, teamed.first.template get< PhysBody >()[i],
                    teamed.first.template get< Seeker >()[i],
                    true);
        }

        if (retargets.empty()) {
            return;
        }
        std::map< uint16_t, std::vector< size_t > > teamy;
        for (size_t i = 0; i < retargets.size(); ++i) {
            const auto team = teamed.first.template get< const Team >()[i];
            teamy[team.team].push_back(i);
        }

        Entity::ExecSimple< const PhysBody, const TargetValue, const Team >::run(core.tracker,
        [&](const auto &ids, const auto &tbodies, const auto &values, const auto &teams) {
            for (const auto &pair : teamy) {
                for (const size_t index : pair.second) {
                    const auto &pb = teamed.first.template get< PhysBody >()[index];
                    const auto &seeker = teamed.first.template get< Seeker >()[index];

                    const auto body = pb.body;
                    const auto seeker_at = VPC< Vec >(body->GetPosition());
                    const auto square_range = seeker.retargetingRange * seeker.retargetingRange;

                    Entity::EntityID new_id = 0;
                    double best_value = 0.0;
                    for (size_t i = 0; i < ids.size(); ++i) {
                        if (teams[i].team == pair.first) {
                            continue;
                        }

                        const auto value = values[i].value;
                        if (value < best_value) {
                            continue;
                        }

                        const auto at = VPC< Vec >(tbodies[i].body->GetPosition());
                        const auto vec_to = seeker_at - at;
                        if (vec_to.squared_length() > square_range) {
                            continue;
                        }

                        best_value = value;
                        new_id = ids[i];
                    }

                    if (best_value > 0.0) {
                        teamed.first.template get< Seeker >()[index].target = new_id;
                    }
                }
            }
        });
    });
}

LifetimeSystem::LifetimeSystem()
    : BaseSystem("Lifetime", Entity::getConstySignature< Lifetime >()) {
}

LifetimeSystem::~LifetimeSystem() { }

void LifetimeSystem::init(Core &core) {
    core.tracker.addSource< LifetimeData >();
}

void LifetimeSystem::execute(Core &core, double seconds) {
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

TurretSystem::TurretSystem()
    : BaseSystem("Turret", Entity::getConstySignature<
            const Colour,
            const PhysBody,
            const Team,
            Turret
        >()) {
}

TurretSystem::~TurretSystem() { }

void TurretSystem::init(Core &core) {
    core.tracker.addSource< TurretData >();
}

bool Turret::trigger() {
    if (0.0 != cooldown) { return false; }
    cooldown = cooldown_length;
    return true;
}

Entity::EntityID standardBullet(Core &core, const BulletInfo &bi, Entity::EntityID sourceID,
                                Point at, Vec to, std::optional< Entity::EntityID > target) {
    auto body = makeCircle(core, at, bi.radius);
    const auto go = 1000.0 * VPC< b2Vec2 >(to);
    body->ApplyLinearImpulse(go, body->GetPosition(), true);
    auto colour = Colour{ { 0, 0, 0 } };
    const auto source_colour = core.tracker.optComponent< const Colour >(sourceID);
    if (source_colour) {
        colour = *source_colour;
    }

    const auto id = core.tracker.createWith(core,
        PhysBody{ body },
        colour,
        Damage{ bi.dmg },
        Lifetime{ bi.lifetime }
    );

    const auto team = core.tracker.optComponent< const Team >(sourceID);
    if (team) {
        core.tracker.addComponent(core, id, Team{ team->get().team });
    }

    if (bi.health > 0.0) {
        core.tracker.addComponent(core, id, fullHealth(bi.health));
        core.tracker.addComponent(core, id, HitData{});
    }

    if (bi.seeking && target) {
        core.tracker.addComponent(core, id, Seeker{
            *target,
            1024.0,
            true,
        });
    } else if (bi.seeking) {
        core.tracker.addComponent(core, id, Seeker{
            0,
            1024.0,
            true,
        });
    }

    return id;
}

BulletCreator getStandardBulletCreator(BulletInfo bi) {
    return [bi](Core &core, Entity::EntityID sourceID, Point at, Vec to, std::optional< Entity::EntityID > target) {
        return standardBullet(core, bi, sourceID, at, to, target);
    };
}

void runGunners(
    Core &core,
    const double seconds,
    std::pair< Entity::Packs< const PhysBody, const Team >, const Entity::IDMap > &unarmed,
    std::pair< Entity::Packs< const PhysBody, const Team, Turret >, const Entity::IDMap > &armed
) {
    std::random_device rd;
    std::mt19937 gen(rd());
    auto &turret_groups = armed.first.template get< Turret >();
    auto &armed_teams = armed.first.template get< const Team >();
    auto &armed_bodies = armed.first.template get< const PhysBody >();
    auto &unarmed_teams = unarmed.first.template get< const Team >();
    auto &unarmed_bodies = unarmed.first.template get< const PhysBody >();
    for (size_t entity_index = 0; entity_index < turret_groups.size(); ++entity_index) {
        for (auto &turret : turret_groups[entity_index]) {
            if (0.0 != turret.cooldown) {
                turret.cooldown = std::max(0.0, turret.cooldown - seconds);
            }
            if (!turret.automatic || 0.0 != turret.cooldown) { continue; }
            const auto team = armed_teams[entity_index].team;
            const double range_square = turret.range * turret.range;
            const auto body = armed_bodies[entity_index];
            const auto source_at = VPC< Vec >(body.body->GetPosition());
            const auto check_target =
            [&](const PhysBody &target, const TeamNumber target_team, const Entity::EntityID tid) -> bool {
                if (target_team == team) { return false; }
                const auto centre = VPC< Vec >(target.body->GetPosition());
                const Vec vec_to = centre - source_at;
                if (vec_to.squared_length() > range_square) { return false; }

                // Firing
                turret.cooldown = turret.cooldown_length;
                const b2Body *bd = body.body;
                const b2Fixture *fixes = bd->GetFixtureList();
                const b2Shape *shape = fixes->GetShape();
                const b2CircleShape * circ = static_cast< const b2CircleShape * >(shape);

                const auto offset = source_at + 1.5 * (1.0 + circ->m_radius) * normalized(vec_to);
                const auto at = Point( offset.x(), offset.y() );

                turret.bullet(core, armed.second[entity_index], at, normalized(vec_to), std::optional(tid));
                return true;
            };

            bool shot = false;
            if (!armed_bodies.empty()) {
                const size_t start = std::uniform_int_distribution< size_t >(0, armed_bodies.size() - 1)(gen);
                for (size_t j = start; j < armed_bodies.size(); ++j) {
                    if (check_target(armed_bodies[j], armed_teams[j].team, armed.second[j])) {
                        shot = true;
                        break;
                    }
                }

                if (!shot) {
                    for (size_t j = 0; j < start; ++j) {
                        if (check_target(armed_bodies[j], armed_teams[j].team, armed.second[j])) {
                            shot = true;
                            break;
                        }
                    }
                }
            }

            if (!shot && !unarmed_bodies.empty()) {
                const size_t start = std::uniform_int_distribution< size_t >(0, unarmed_bodies.size() - 1)(gen);
                for (size_t j = start; j < unarmed_bodies.size(); ++j) {
                    if  (check_target(unarmed_bodies[j], unarmed_teams[j].team, unarmed.second[j])) {
                        shot = true;
                        break;
                    }
                }
                if (!shot) {
                    for (size_t j = 0; j < start; ++j) {
                        if  (check_target(unarmed_bodies[j], unarmed_teams[j].team, unarmed.second[j])) {
                            shot = true;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void TurretSystem::execute(Core &core, double seconds) {
    Entity::Exec<
        Entity::Packs< const PhysBody, const Team >,
        Entity::Packs< const PhysBody, const Team, Turret >
    >::run(core.tracker,
    [&](auto &noturrets, auto &turreters) {
        runGunners(core, seconds, noturrets, turreters);
    });
}
