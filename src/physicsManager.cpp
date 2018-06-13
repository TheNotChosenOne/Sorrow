#include "physicsManager.h"

#include "mirror.h"
#include "core.h"
#include "entityManager.h"
#include "quadtree.h"

#include <array>
#include <memory>
#include <limits>
#include <algorithm>
#include <vector>
#include <Python.h>
#include <structmember.h>

namespace {

static const double FRICTION = (1.0 - 0.03);
static const double DAMPING = 0.99;

struct Dimensions {
    Vec min;
    Vec max;
    double rad;
};

typedef PhysicsManager::Components Comps;

static inline void moveSingle(PhysicsComponent &comp) {
    const double s = static_cast< double >(!comp.isStatic);
    const double mass = std::abs((1.0 - s) - comp.mass);
    comp.vel = s *
               (comp.vel + (comp.acc * comp.mass + comp.impulse / mass) * PHYSICS_TIMESTEP) *
               std::min(1.0, std::pow(0.999, comp.area)) *
               DAMPING;
    comp.pos += comp.vel * PHYSICS_TIMESTEP;
    comp.acc = Vec();
    comp.surface = Vec();
    comp.impulse = Vec();
}

static void move(Comps &nexts) {
    for (size_t i = 0; i < nexts.size(); ++i) {
        nexts[i].contacts.clear();
        moveSingle(nexts[i]);
    }
}

static inline void populate(std::vector< std::vector< size_t > > &select, Comps &comps) {
    select.resize(2);
    for (size_t i = 0; i < select.size(); ++i) {
        select[i].clear();
        select[i].reserve(comps.size());
    }
    for (size_t i = 0; i < comps.size(); ++i) {
        select[comps[i].isStatic].push_back(i);
    }
}

static inline Vec getBest(const Vec &p, const Vec &v0, const Vec &v1, const Vec &v2, const Vec &v3) {
    static const Vec norms[] = { { -1, 0 }, { 1, 0 }, { 0, 1 }, { 0, -1 } };
    const double d0 = diffLength(p, v2);
    const double d1 = diffLength(p, v3);
    const double d2 = diffLength(p, v0);
    const double d3 = diffLength(p, v1);
    if (d0 < d1) {
        if (d0 < d2) {
            if (d0 < d3) {
                return norms[0];
            } else {
                return norms[3];
            }
        } else {
            if (d2 < d3) {
                return norms[2];
            } else {
                return norms[3];
            }
        }
    } else {
        if (d1 < d2) {
            if (d1 < d3) {
                return norms[1];
            } else {
                return norms[3];
            }
        } else {
            if (d2 < d3) {
                return norms[2];
            } else {
                return norms[3];
            }
        }
    }
}

static inline void collideStaticBoxSingle(
        const EntityManager &eman, size_t j, PhysicsComponent &phys, const PhysicsComponent &stat,
        Vec &normal, double &depth, double &elasticity, size_t &contactCount) {
    const double width = stat.rad[0];
    const double hight = stat.rad[1];
    const double left = stat.pos[0] - width;
    const double rite = stat.pos[0] + width;
    const double bot = stat.pos[1] - hight;
    const double top = stat.pos[1] + hight;
    const double bestX = std::max(left, std::min(rite, phys.pos[0]));
    const double bestY = std::max(bot, std::min(top, phys.pos[1]));
    const double rad = phys.rad[0];
    const double trad = rad * rad;
    Vec diff = phys.pos - Vec(bestX, bestY);
    const double depth2 = gmtl::lengthSquared(diff);
    if ((phys.pos[1] <= bot  - rad) | (phys.pos[1] >= top  + rad) |
        (phys.pos[0] <= left - rad) | (phys.pos[0] >= rite + rad) |
        (depth2 >= trad)) { return; }

    const Vec topPoint(clamp(left, rite, phys.pos[0]), top);
    const Vec botPoint(clamp(left, rite, phys.pos[0]), bot);
    const Vec leftPoint(left, clamp(bot, top, phys.pos[1]));
    const Vec ritePoint(rite, clamp(bot, top, phys.pos[1]));
    std::vector< std::pair< double, Vec > > distNorms = {
        { diffLength(phys.pos, topPoint), { 0,  1  } },
        { diffLength(phys.pos, botPoint), { 0, -1 } },
        { diffLength(phys.pos, leftPoint), { -1, 0 } },
        { diffLength(phys.pos, ritePoint), {  1, 0 } },
    };
    std::sort(distNorms.begin(), distNorms.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });
    const Vec nnorm = getBest(phys.pos, topPoint, botPoint, leftPoint, ritePoint);
    const Vec norm = distNorms.front().second;
    rassert(norm == nnorm, norm, nnorm);

    const double dep = phys.rad[0] - std::sqrt(depth2);
    normal += norm;
    depth += dep;
    elasticity += stat.elasticity;
    ++contactCount;
    if (phys.gather) {
        phys.contacts.push_back({ eman.getIDFromLow(j),
                               Vec(bestX, bestY), norm,
                               dep, gmtl::length(phys.vel) * phys.mass });
    }
}

static inline void collideStaticCircleSingle(
        const EntityManager &eman, size_t j, PhysicsComponent &phys, const PhysicsComponent &stat,
        Vec &normal, double &depth, double &elasticity, size_t &contactCount) {
    const double trad = phys.rad[0] + stat.rad[0];
    if ((std::abs(phys.pos[0] - stat.pos[0]) >= trad) |
        (std::abs(phys.pos[1] - stat.pos[1]) >= trad)) { return; }
    Vec diff = phys.pos - stat.pos;
    const double len2 = gmtl::lengthSquared(diff);
    if (len2 >= trad * trad) { return; }
    gmtl::normalize(diff);
    const double dep = (trad - std::sqrt(len2)) / 2.0;
    normal += diff;
    depth += dep;
    elasticity += stat.elasticity;
    ++contactCount;
    if (phys.gather) {
        phys.contacts.push_back({ eman.getIDFromLow(j),
                               stat.pos + stat.rad[0] * diff, diff,
                               dep, gmtl::length(phys.vel) * phys.mass });
    }
}

static inline void collideStaticUpdate(PhysicsComponent &dyn,
        Vec normal, double depth, double elasticity, size_t contactCount) {
    gmtl::normalize(normal);
    elasticity /= contactCount;
    depth /= contactCount;

    const Vec add = normal * depth;
    dyn.impulse = add;
    dyn.pos += add;
    dyn.surface = normal;

    Vec velNorm = dyn.vel;
    const Vec crossNorm = Vec(normal[1], -normal[0]);
    const double speed = gmtl::length(velNorm);
    gmtl::normalize(velNorm);
    dyn.vel = (FRICTION * gmtl::dot(velNorm, crossNorm) * crossNorm -
            dyn.elasticity * elasticity * gmtl::dot(velNorm, normal) * normal) * speed;
}

static inline void staticCollideSingle(const EntityManager &eman, Comps &comps,
                                       const std::vector< size_t > &statics, size_t i) {
    auto &dyn = comps[i];
    size_t contactCount = 0;
    Vec normal;
    double depth = 0.0;
    double elasticity = 0.0;
    for (const size_t j : statics) {
        const auto &stat = comps[j];
        if (Shape::Box == stat.shape) {
            collideStaticBoxSingle(eman, j, dyn, stat, normal, depth, elasticity, contactCount);
        } else {
            collideStaticCircleSingle(eman, j, dyn, stat, normal, depth, elasticity, contactCount);
        }
    }
    if (0 != contactCount && !dyn.phased) {
        collideStaticUpdate(dyn, normal, depth, elasticity, contactCount);
    }
}

typedef std::vector< size_t >::const_iterator Siter;
typedef std::pair< size_t, Vec > Adjustment;
static inline void dynamicCollideSingle(EntityManager &eman, Comps &comps,
        const std::vector< size_t > &dynamics, size_t i,
        const Siter withBegin, const Siter withEnd, std::vector< Adjustment > &adjustments) {
    auto &a = comps[dynamics[i]];
    for (auto iter = withBegin; iter != withEnd; ++iter) {
        const size_t j = *iter;
        if (j <= i) { continue; }
        auto &b = comps[dynamics[j]];

        const double trad = a.rad[0] + b.rad[0];
        if (dynamics[i] == j ||
            (std::abs(a.pos[0] - b.pos[0]) >= trad) ||
            (std::abs(a.pos[1] - b.pos[1]) >= trad)) { continue; }
        Vec norm = a.pos - b.pos;
        const double len2 = gmtl::lengthSquared(norm);
        if (len2 >= trad * trad) { continue; }
        gmtl::normalize(norm);

        const double depth = (trad - std::sqrt(len2)) / 2.0;
        const double closingSpeed = std::abs(gmtl::dot(norm, a.vel)) +
                                    std::abs(gmtl::dot(norm, b.vel));
        if (a.gather) {
            a.contacts.push_back({ eman.getIDFromLow(dynamics[j]),
                                   b.pos + b.rad[0] * norm, norm,
                                   depth, closingSpeed * b.mass });
        }
        if (b.gather) {
            b.contacts.push_back({ eman.getIDFromLow(dynamics[i]),
                                   b.pos + b.rad[0] * norm, -norm,
                                   depth, closingSpeed * a.mass });
        }
        if (a.phased | b.phased) { continue; }

        const double massP = a.mass / (a.mass + b.mass);
        adjustments.push_back({ dynamics[i], norm * depth * massP });
        adjustments.push_back({ dynamics[j], norm * depth * (massP - 1.0) });
        //a.pos += norm * depth * massP;
        //b.pos -= norm * depth * (1.0 - massP);

        const Vec aVel = a.vel;
        const Vec bVel = b.vel;

        const Vec crossNorm(-norm[1], norm[0]);
        const double aLength = gmtl::length(a.vel);
        const double bLength = gmtl::length(b.vel);
        const Vec aWith = gmtl::dot(norm, a.vel) * norm;
        const Vec bWith = gmtl::dot(b.vel, norm) * norm;

        Vec aNormed = aVel;
        gmtl::normalize(aNormed);
        Vec bNormed = bVel;
        gmtl::normalize(bNormed);

        const double aFactor = gmtl::dot(crossNorm, aNormed);
        const double bFactor = gmtl::dot(bNormed, crossNorm);
        const Vec aAgainst = aLength * aFactor * crossNorm;
        const Vec bAgainst = bLength * bFactor * crossNorm;

        const double e = a.elasticity * b.elasticity;
        const double mass = a.mass + b.mass;
        a.vel = e * (aWith * (a.mass - b.mass) + (2.0 * b.mass * bWith)) / mass;
        b.vel = e * (bWith * (b.mass - a.mass) + (2.0 * a.mass * aWith)) / mass;

        a.vel += aAgainst;
        b.vel += bAgainst;

        a.surface += 0.1 * norm;
        b.surface -= 0.1 * norm;
        gmtl::normalize(a.surface);
        gmtl::normalize(b.surface);
    }
}

static inline Quadtree getQuadFor(const Comps &comps, const std::vector< size_t > &indices) {
    std::vector< TaggedPhys > tagged;
    tagged.reserve(indices.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        tagged.push_back({ i, &comps[indices[i]] });
    }
    return Quadtree(tagged);
}

static void collide(Core &core, Comps &comps) {
    static std::vector< std::vector< size_t > > select;
    populate(select, comps);
    const auto &dynamics = select[0];
    const auto &statics = select[1];

    // Static collisions
    for (size_t i = 0; i < dynamics.size(); ++i) {
        staticCollideSingle(core.entities, comps, statics, dynamics[i]);
    }

    std::vector< Adjustment > adjustments;
    const size_t dynCount = dynamics.size();

    Quadtree qt = getQuadFor(comps, dynamics);
    //qt.draw(core);

    // Dynamic collisions
    std::vector< size_t > suggested;
    for (size_t i = 0; i < dynCount; ++i) {
        auto &a = comps[dynamics[i]];
        suggested.clear();
        qt.suggest(a.pos, a.rad, suggested);
        dynamicCollideSingle(core.entities, comps, dynamics, i,
                             suggested.begin(), suggested.end(), adjustments);
    }
    for (const auto adjust : adjustments) {
        comps[adjust.first].pos += adjust.second;
    }
};

}

void PhysicsManager::create() {
    nursery.emplace_back();
}

void PhysicsManager::graduate() {
    std::copy(nursery.begin(), nursery.end(), std::back_inserter(components));
    nursery.clear();
}

void PhysicsManager::reorder(const std::map< size_t, size_t > &remap) {
    for (const auto &pair : remap) {
        components[pair.second] = std::move(components[pair.first]);
    }
}

void PhysicsManager::cull(size_t count) {
    components.resize(components.size() - count);
}

PhysicsManager::Component &PhysicsManager::get(Entity e) {
    if (components.size() <= e) {
        return nursery[e - components.size()];
    }
    return components[e];
}

const PhysicsManager::Component &PhysicsManager::get(Entity e) const {
    if (components.size() <= e) {
        return nursery[e - components.size()];
    }
    return components[e];
}

template<>
PyObject *toPython< Shape >(Shape &v) {
    return Py_BuildValue("s", Shape::Circle == v ? "circle" : "box");
}

template<>
void fromPython< Shape >(Shape &s, PyObject *obj) {
    s = fromPython< std::string >(obj) == "box" ? Shape::Box : Shape::Circle;
}

std::ostream &operator<<(std::ostream &os, const Contact &k) {
    os << k.which << " hit at " << k.where << ' ' << k.norm << " (";
    os << k.depth << ", " << k.force << ")";
    return os;
}

bool operator==(const Contact &a, const Contact &b) {
    return a.which == b.which && a.where == b.where && a.norm == b.norm &&
           a.depth == b.depth && a.force == b.force;
}

void PhysicsManager::updatePhysics(Core &core) {
    move(components);
    collide(core, components);
}

namespace {

struct PyPhysicsManager {
    PyObject_HEAD
    PhysicsManager *pm;
};

static PyObject *Py_get(PyPhysicsManager *self, PyObject *args) {
    const size_t i = fromPython< int64_t >(PyTuple_GetItem(args, 0));
    return toPython(self->pm->get(i));
}

static PyObject *Py_timestep(PyPhysicsManager *self, PyObject *) {
    return toPython(self->pm->timestep);
}

static PyObject *Py_stepsPerSecond(PyPhysicsManager *self, PyObject *) {
    return toPython(self->pm->stepsPerSecond);
}

static PyMethodDef pmMethods[] = {
    { "get", reinterpret_cast< PyCFunction >(Py_get), READONLY, "Get physics component" },
    { "timestep", reinterpret_cast< PyCFunction >(Py_timestep), READONLY, "Time between physics steps" },
    { "stepsPerSecond", reinterpret_cast< PyCFunction >(Py_stepsPerSecond), READONLY, "Physics steps per second" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject pmType = [](){
    PyTypeObject obj;
    obj.tp_name = "physicsManager";
    obj.tp_basicsize = sizeof(PyPhysicsManager);
    obj.tp_doc = "I AM THE LAW!";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_methods = pmMethods;
    return obj;
}();

}

RUN_STATIC(addPyTypeInitializer([](){ PyType_Ready(&pmType); }))

template<>
PyObject *toPython< PhysicsManager >(PhysicsManager &pm) {
    PyPhysicsManager *ppm;
    ppm = reinterpret_cast< PyPhysicsManager * >(pmType.tp_alloc(&pmType, 0));
    if (ppm) {
        ppm->pm = &pm;
    }
    return reinterpret_cast< PyObject * >(ppm);
}
