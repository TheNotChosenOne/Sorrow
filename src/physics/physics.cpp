#include "physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "core/core.h"

#include <utility>
#include <Box2D.h>
#include <map>

namespace {

typedef Entity::Packs< const Position, const Shape > Statics;
typedef Entity::Packs< const Position, const Shape, HitData > StaticsC;
typedef Entity::Packs<       Position, const Shape,          Direction, Speed > Dynamics;
typedef Entity::Packs<       Position, const Shape, HitData, Direction, Speed > DynamicsC;

static inline bool miss(const Point &bl, const Point &tr, const Point &p2, const Shape &s2) {
    return (bl.x() + s2.rad.x() < p2.x()) ||
           (tr.x() - s2.rad.x() > p2.x()) ||
           (bl.y() + s2.rad.y() < p2.y()) ||
           (tr.y() - s2.rad.y() > p2.y());
}

template< typename ...Args >
static inline bool miss(const Point &p, const Shape &s, const Entity::Packs< Args... > &stat) {
    const Rect thiss { p, s.rad };
    stat.template get< Position >();
    for (size_t i = 0; i < stat.template get< Position >().size(); ++i) {
        if (collide(thiss, Rect{ stat.template at< Position >(i).v, stat.template at< const Shape >(i).rad })) {
            return false;
        }
    }
    return true;
}

}

void updatePhysics(Core &core) {
    Entity::Exec< Statics, DynamicsC, Dynamics, StaticsC >::run(core.tracker,
    [&](auto &statics, auto &d, auto &, auto &) {
        auto &statPos = statics.first.template get< const Position >();
        auto &statShape = statics.first.template get< const Shape >();
        auto &dynPos = d.first.template get< Position >();
        auto &dynShape = d.first.template get< const Shape >();

        static std::vector< b2Body * > statBodies;
        statBodies.reserve(statPos.size());
        static std::vector< b2Body * > dynBodies;
        dynBodies.reserve(dynPos.size());
        if (statBodies.empty()) {
            for (size_t i = 0; i < statPos.size(); ++i) {
                b2BodyDef def;
                def.position.Set(statPos[i].v[0], statPos[i].v[1]);
                b2Body* body = core.b2world->CreateBody(&def);
                b2PolygonShape box;
                rassert(statShape[i].rad[0] * statShape[i].rad[1] > 0.0);
                box.SetAsBox(statShape[i].rad[0], statShape[i].rad[1]);
                body->CreateFixture(&box, 0.0f);
                statBodies.push_back(body);
            }
        }
        if (dynBodies.empty()) {
            for (size_t i = 0; i < dynPos.size(); ++i) {
                b2BodyDef def;
                def.type = b2_dynamicBody;
                def.position.Set(dynPos[i].v[0], dynPos[i].v[1]);
                b2Body* body = core.b2world->CreateBody(&def);
                b2PolygonShape box;
                box.SetAsBox(dynShape[i].rad[0], dynShape[i].rad[1]);
                b2FixtureDef fixture;
                fixture.shape = &box;
                fixture.density = 1.0f;
                fixture.friction = 0.3f;
                body->CreateFixture(&fixture);
                const Vec force = 60.0 * d.first.template at< Speed  >(i).d * d.first.template at< Direction >(i).v.to_vector();
                body->ApplyForce(b2Vec2(force.x(), force.y()), b2Vec2(dynPos[i].v[0], dynPos[i].v[1]), true);
                body->ApplyLinearImpulse(b2Vec2(force.x(), force.y()), b2Vec2(dynPos[i].v[0], dynPos[i].v[1]), true);
                dynBodies.push_back(body);
            }
        } else {
            for (size_t i = 0; i < dynPos.size(); ++i) {
                const Vec force = 25.0 * d.first.template at< Speed  >(i).d * d.first.template at< Direction >(i).v.to_vector();
                dynBodies[i]->ApplyForce(b2Vec2(force.x(), force.y()), b2Vec2(dynPos[i].v[0], dynPos[i].v[1]), true);
                dynBodies[i]->ApplyLinearImpulse(b2Vec2(force.x(), force.y()), b2Vec2(dynPos[i].v[0], dynPos[i].v[1]), true);
            }
        }
        core.b2world->Step(1.0 / 25.0, 10, 8);
        for (size_t i = 0; i < dynBodies.size(); ++i) {
            const auto p = dynBodies[i]->GetPosition();
            dynPos[i].v = Point(p.x, p.y);
            auto v = dynBodies[i]->GetLinearVelocity();
            d.first.template at< Speed >(i).d = v.Length();
            if (0.0 == d.first.template at< Speed >(i).d) {
                v.Normalize();
                d.first.template at< Direction >(i).v = Dir(v.x, v.y);
            } else {
                d.first.template at< Direction >(i).v = Dir(0.0, 0.0);
            }
        }
    });
}
