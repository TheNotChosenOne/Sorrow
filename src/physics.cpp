#include "physics.h"
#include "tracker.h"
#include "core.h"

namespace {

typedef ComponentCollection< Position, Shape, Direction, Speed > Dynamics;
typedef ComponentCollection< Position, Shape > Statics;

static inline bool miss(const Point &p1, const Shape &s1, const Point &p2, const Shape &s2) {
    const Point bl1 = p1 - s1.rad;
    const Point tr1 = p1 + s1.rad;
    const Point bl2 = p2 - s2.rad;
    const Point tr2 = p2 + s2.rad;
    bool missed = false;
    for (size_t i = 0; i < 2; ++i) {
        missed = missed | (tr2[i] < bl1[i]) | (tr1[i] < bl2[i]);
    }
    return missed;
}

static inline bool miss(const Point &p, const Shape &s, const Statics &stat) {
    for (size_t i = 0; i < stat.get< Position >().size(); ++i) {
        if (!miss(p, s, stat.at< Position >(i).v, stat.at< Shape >(i))) { return false; }
    }
    return true;
}

}

void updatePhysics(Core &core) {
    core.tracker.partitionExec< Statics, Dynamics >([&](Statics &s, Dynamics &d) {
        for (size_t i = 0; i < d.get< Position >().size(); ++i) {
            const Point maybe = d.at< Position >(i).v + d.at< Direction >(i).v.vector() * d.at< Speed >(i).d;
            if (miss(maybe, d.at< Shape >(i), s)) {
                d.at< Position >(i).v = maybe;
            }
        }
    });
}
