#include "npc.h"

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

void SeekerSystem::execute(Core &core, double) {
    Entity::ExecSimple< PhysBody, const Seeker >::run(core.tracker,
    [&](auto &bodies, const auto &seekers) {
        for (size_t i = 0; i < seekers.size(); ++i) {
            Entity::EntityID tid = seekers[i].target;
            if (!core.tracker.alive(tid)) {
                continue;
            }
            const auto tbody = core.tracker.optComponent< PhysBody >(tid);
            if (!tbody) {
                continue;
            }
            const b2Body *target = tbody->get().body;
            const auto target_at = VPC< Vec >(target->GetPosition());

            const auto m_body = bodies[i].body;
            const auto vec_to = target_at - VPC< Vec >(m_body->GetPosition());

            const auto go = 1000.0 * normalized(vec_to);
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

void TurretSystem::execute(Core &core, double seconds) {
    std::random_device rd;
    std::mt19937 gen(rd());

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
            const auto source_at = VPC< Vec >(body.body->GetPosition());
            const auto check_target = [&](const PhysBody &target, const Team target_team, const Entity::EntityID tid) -> bool {
                if (target_team.team == tteams[i].team) { return false; }
                const auto centre = VPC< Vec >(target.body->GetPosition());
                const Vec vec_to = centre - source_at;
                if (vec_to.squared_length() > range_square) { return false; }
                turrets[i].cooldown = turrets[i].cooldown_length;
                const double bul_rad = 0.75;
                const b2Body *bd = body.body;
                const b2Fixture *fixes = bd->GetFixtureList();
                const b2Shape *shape = fixes->GetShape();
                const b2CircleShape * circ = static_cast< const b2CircleShape * >(shape);
                const auto offset = source_at + 1.5 * (bul_rad + circ->m_radius) * normalized(vec_to);
                const auto pointy = Point( offset.x(), offset.y() );
                auto bul_body = makeBall(core, pointy, bul_rad);
                const auto go = 100.0 * VPC< b2Vec2 >(normalized(vec_to));
                bul_body->ApplyLinearImpulse(go, bul_body->GetPosition(), true);
                core.tracker.createWith(core,
                    PhysBody{ bul_body },
                    Colour{ { 0xFF, 0, 0 } },
                    Team({ tteams[i].team }),
                    Damage{ turrets[i].dmg },
                    Lifetime{ turrets[i].lifetime },
                    Seeker({ tid }),
                    fullHealth(0.1),
                    HitData{ }
                );
                return true;
            };
            bool shot = false;
            if (!tbodies.empty()) {
                const size_t start = std::uniform_int_distribution< size_t >(0, tbodies.size() - 1)(gen);
                for (size_t j = start; j < tbodies.size(); ++j) {
                    if  (i != j && check_target(tbodies[j], tteams[j], noturrets.second[j])) { shot = true; break; }
                }
                if (!shot) {
                    for (size_t j = 0; j < start; ++j) {
                        if  (i != j && check_target(tbodies[j], tteams[j], noturrets.second[j])) { shot = true; break; }
                    }
                }
            }
            if (!nbodies.empty()) {
                const size_t start = std::uniform_int_distribution< size_t >(0, nbodies.size() - 1)(gen);
                if (!shot) {
                    for (size_t j = start; j < nbodies.size(); ++j) {
                        if  (check_target(nbodies[j], nteams[j], turreters.second[j])) { break; }
                    }
                }
                if (!shot) {
                    for (size_t j = 0; j < start; ++j) {
                        if  (check_target(nbodies[j], nteams[j], turreters.second[j])) { break; }
                    }
                }
            }
        }
    });

    Entity::Exec<
        Entity::Packs< const PhysBody, const Team >,
        Entity::Packs< const PhysBody, const Team, Turret2 >
    >::run(core.tracker,
    [&](auto &noturrets, auto &turreters) {
        auto &turrets = turreters.first.template get< Turret2 >();
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
            const auto source_at = VPC< Vec >(body.body->GetPosition());
            const auto check_target = [&](const PhysBody &target, const Team target_team, const Entity::EntityID tid) -> bool {
                if (target_team.team == tteams[i].team) { return false; }
                const auto ocol = core.tracker.optComponent< Colour >(tid);
                if (!ocol) { return false; }
                const Point3 col = (*ocol).get().colour;
                if (col != Point3(0xFF, 0, 0)) { return false; }
                const auto centre = VPC< Vec >(target.body->GetPosition());
                const Vec vec_to = centre - source_at;
                if (vec_to.squared_length() > range_square) { return false; }
                //std::cout << source_at << ' ' << vec_to << ' ' << VPC< Vec >(target.body->GetPosition()) << std::endl;
                //std::cout << target_team.team << ' ' << tteams[i].team << std::endl;
                turrets[i].cooldown = turrets[i].cooldown_length;
                const double bul_rad = 0.10;
                const b2Body *bd = body.body;
                const b2Fixture *fixes = bd->GetFixtureList();
                const b2Shape *shape = fixes->GetShape();
                const b2CircleShape * circ = static_cast< const b2CircleShape * >(shape);
                const auto offset = source_at + 1.5 * (bul_rad + circ->m_radius) * normalized(vec_to);
                const auto pointy = Point( offset.x(), offset.y() );
                auto bul_body = makeBall(core, pointy, bul_rad);
                const auto go = 100.0 * VPC< b2Vec2 >(normalized(vec_to));
                bul_body->ApplyLinearImpulse(go, bul_body->GetPosition(), true);
                core.tracker.createWith(core,
                    PhysBody{ bul_body },
                    Colour{ { 0xFF, 0xFF, 102 } },
                    Team({ tteams[i].team }),
                    Damage{ turrets[i].dmg },
                    Lifetime{ turrets[i].lifetime }
                );
                return true;
            };
            for (size_t j = 0; j < nbodies.size(); ++j) {
                if  (check_target(nbodies[j], nteams[j], turreters.second[j])) { break; }
            }
        }
    });
}
