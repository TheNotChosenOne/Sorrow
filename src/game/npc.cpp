#include "game/npc.h"

#include "core/core.h"
#include "core/geometry.h"
#include "visual/visuals.h"
#include "physics/physics.h"
#include "entities/exec.h"

#include <random>

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

    b2Body *body;
    core.b2world.locked([&](){
        body = core.b2world.b2w->CreateBody(&def);
        body->CreateFixture(&fixture);
    });
    return body;
}

b2Body *randomBall(Core &core, Point centre, double rad) {
    const Point p = Point(rnd(rad / 2.0), rnd(rad / 2.0));
    return makeBall(core, Point(p.x() + centre.x(), p.y() + centre.y()), 1.0);
}

b2Body *randomBall(Core &core, double rad) {
    return randomBall(core, Point(0.0, 0.0), rad);
}

DamageSystem::DamageSystem()
    : BaseSystem("Damage", Entity::getSignature< HitData, Health, Team, Damage >()) {
}

DamageSystem::~DamageSystem() { }

void DamageSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< HealthData >());
    core.tracker.addSource(std::make_unique< TeamData >());
    core.tracker.addSource(std::make_unique< DamageData >());
}

void DamageSystem::execute(Core &core, double) {
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

SeekerSystem::SeekerSystem()
    : BaseSystem("Seeker", Entity::getSignature< PhysBody, const Seeker >()) {
}

SeekerSystem::~SeekerSystem() { }

void SeekerSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< SeekerData >());
}

void SeekerSystem::execute(Core &core, double time_delta) {
    const double force = 10000.0;
    const double tick_velo = force / time_delta;

    Entity::ExecSimple< PhysBody, const Seeker >::run(core.tracker,
    [&](const auto &ids, auto &bodies, const auto &seekers) {
        for (size_t i = 0; i < seekers.size(); ++i) {
            Entity::EntityID tid = seekers[i].target;
            if (!core.tracker.alive(tid)) {
                core.tracker.killEntity(core, ids[i]);
                continue;
            }
            const auto tbody = core.tracker.optComponent< PhysBody >(tid);
            if (!tbody) {
                continue;
            }
            const auto m_body = bodies[i].body;
            const b2Body *target = tbody->get().body;

            const auto target_at = VPC< Vec >(target->GetPosition());
            const auto me_at = VPC< Vec >(m_body->GetPosition());
            const double estimate_intercept_t = (target_at - me_at).squared_length() / tick_velo;

            const auto target_velo = VPC< Vec >(target->GetLinearVelocity());
            const auto target_predicted = target_at + target_velo * estimate_intercept_t * 2.0;

            const auto vec_to = target_predicted - me_at;

            const auto go = force * normalized(vec_to);
            m_body->ApplyForceToCenter(VPC< b2Vec2 >(go), true);
        }
    });
}

LifetimeSystem::LifetimeSystem()
    : BaseSystem("Lifetime", Entity::getSignature< Lifetime >()) {
}

LifetimeSystem::~LifetimeSystem() { }

void LifetimeSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< LifetimeData >());
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
    : BaseSystem("Turret", Entity::getSignature<
            Colour,
            PhysBody,
            Team,
            Turret,
            Turret2,
            Damage,
            Lifetime,
            Seeker,
            Health,
            HitData
        >()) {
}

TurretSystem::~TurretSystem() { }

void TurretSystem::init(Core &core) {
    core.tracker.addSource(std::make_unique< TurretData >());
    core.tracker.addSource(std::make_unique< Turret2Data >());
}

template< typename TTurret >
void runGunners(
    Core &core,
    const double seconds,
    std::pair< Entity::Packs< const PhysBody, const Team >, const Entity::IDMap > &unarmed,
    std::pair< Entity::Packs< const PhysBody, const Team, TTurret >, const Entity::IDMap > &armed
) {
    std::random_device rd;
    std::mt19937 gen(rd());

    auto &turrets = armed.first.template get< TTurret >();
    auto &armed_teams = armed.first.template get< const Team >();
    auto &armed_bodies = armed.first.template get< const PhysBody >();

    auto &unarmed_teams = unarmed.first.template get< const Team >();
    auto &unarmed_bodies = unarmed.first.template get< const PhysBody >();

    for (size_t turret_index = 0; turret_index < turrets.size(); ++turret_index) {
        auto &turret = turrets[turret_index];
        if (0.0 != turret.cooldown) {
            turret.cooldown = std::max(0.0, turret.cooldown - seconds);
        }

        if (0.0 != turret.cooldown) { continue; }

        const auto team = armed_teams[turret_index].team;
        const double range_square = turret.range * turret.range;
        const auto body = armed_bodies[turret_index];
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

            const auto offset = source_at + 1.5 * (turret.bullet_radius + circ->m_radius) * normalized(vec_to);
            const auto pointy = Point( offset.x(), offset.y() );
            auto bul_body = makeBall(core, pointy, turret.bullet_radius);
            const auto go = 100.0 * VPC< b2Vec2 >(normalized(vec_to));
            bul_body->ApplyLinearImpulse(go, bul_body->GetPosition(), true);

            auto bullet_colour = Colour{ { 0xFF, 0xFF, 0x99 } };
            const auto turret_colour = core.tracker.optComponent< Colour >(armed.second[turret_index]);
            if (turret_colour) {
                bullet_colour = *turret_colour;
            }

            const auto id = core.tracker.createWith(core,
                PhysBody{ bul_body },
                bullet_colour,
                Team({ team }),
                Damage{ turret.dmg },
                Lifetime{ turret.lifetime }
            );

            if (turret.bullet_health > 0.0) {
                core.tracker.addComponent(core, id, fullHealth(turret.bullet_health));
                core.tracker.addComponent(core, id, HitData{});
            }

            if (turret.bullet_seeking) {
                core.tracker.addComponent(core, id, Seeker{ tid });
            }
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

void TurretSystem::execute(Core &core, double seconds) {
    Entity::Exec<
        Entity::Packs< const PhysBody, const Team >,
        Entity::Packs< const PhysBody, const Team, Turret >
    >::run(core.tracker,
    [&](auto &noturrets, auto &turreters) {
        runGunners< Turret >(core, seconds, noturrets, turreters);
    });

    Entity::Exec<
        Entity::Packs< const PhysBody, const Team >,
        Entity::Packs< const PhysBody, const Team, Turret2 >
    >::run(core.tracker,
    [&](auto &noturrets, auto &turreters) {
        runGunners< Turret2 >(core, seconds, noturrets, turreters);
    });
}
