#include "physics.h"
#include "tracker.h"
#include "core.h"

namespace {

typedef ComponentCollection< Position, const Shape, const Direction, const Speed > Dynamics;
typedef ComponentCollection< Position, const Shape > Statics;

static inline bool miss(const Point &bl, const Point &tr, const Point &p2, const Shape &s2) {
    return (bl.x() + s2.rad.x() < p2.x()) ||
           (tr.x() - s2.rad.x() > p2.x()) ||
           (bl.y() + s2.rad.y() < p2.y()) ||
           (tr.y() - s2.rad.y() > p2.y());
}

static inline bool miss(const Point &p, const Shape &s, const Statics &stat) {
    const Rect thiss { p, s.rad };
    for (size_t i = 0; i < stat.get< Position >().size(); ++i) {
        if (collide(thiss, Rect{ stat.at< Position >(i).v, stat.at< const Shape >(i).rad })) {
            return false;
        }
    }
    return true;
}

}

void updatePhysics(Core &core) {
    core.tracker.partitionExec< Statics, Dynamics >([&](Statics &s, Dynamics &d) {
        for (size_t i = 0; i < d.get< Position >().size(); ++i) {
            Vec v = d.at< const Direction >(i).v.vector();
            v /= std::sqrt(v.squared_length());
            const Point maybe = d.at< Position >(i).v + v * d.at< const Speed >(i).d;
            if (miss(maybe, d.at< const Shape >(i), s)) {
                d.at< Position >(i).v = maybe;
            }
        }
    });
}
