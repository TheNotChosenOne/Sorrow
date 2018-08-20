#include "physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "core/core.h"

#include <utility>
#include <map>

namespace {

typedef Entity::Packs< Position, HitData, const Shape, const Direction, const Speed > DynamicsC;
typedef Entity::Packs< Position, HitData, const Shape > StaticsC;
typedef Entity::Packs< Position, const Shape, const Direction, const Speed > Dynamics;
typedef Entity::Packs< Position, const Shape > Statics;

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
    [&](auto &, auto &d, auto &, auto &) {
        for (size_t i = 0; i < d.first.template get< Position >().size(); ++i) {
            Vec v = d.first.template at< const Direction >(i).v.vector();
            v /= std::sqrt(v.squared_length());
            d.first.template at< Position >(i).v += v * d.first.template at< const Speed >(i).d;
        }
    });
}
