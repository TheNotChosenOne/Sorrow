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

typedef PhysicsManager::Components Comps;

struct Dimensions {
    Vec min;
    Vec max;
    double rad;
};

static void move(const Comps &lasts, Comps &nexts) {
    for (size_t i = 0; i < nexts.size(); ++i) {
        const auto &last = lasts[i];
        auto &next = nexts[i];

        next = last;
        next.contacts.clear();

        const double s = static_cast< double >(!last.isStatic);
        const double mass = std::abs((1.0 - s) - last.mass);
        next.vel = s * (last.vel + (last.acc * last.mass + last.impulse / mass) * PHYSICS_TIMESTEP);
        next.pos += next.vel * DAMPING * PHYSICS_TIMESTEP;
        next.acc = Vec();
        next.surface = Vec();
        next.impulse = Vec();
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

static void collide(Core &core, Comps &comps, const Comps &) {
    static std::vector< std::vector< size_t > > select;
    populate(select, comps);
    const auto &dynamics = select[0];
    const auto &statics = select[1];

    // Static collisions
    for (size_t i = 0; i < dynamics.size(); ++i) {
        auto &a = comps[dynamics[i]];
        size_t contactCount = 0;
        Vec normal;
        double depth = 0.0;
        double elasticity = 0.0;
        for (const size_t j : statics) {
            auto &b = comps[j];
            if (Shape::Box == b.shape) {
                const double width = b.rad[0];
                const double hight = b.rad[1];
                const double left = b.pos[0] - width;
                const double rite = b.pos[0] + width;
                const double bot = b.pos[1] - hight;
                const double top = b.pos[1] + hight;
                const double bestX = std::max(left, std::min(rite, a.pos[0]));
                const double bestY = std::max(bot, std::min(top, a.pos[1]));
                const double rad = a.rad[0];
                const double trad = rad * rad;
                Vec diff = a.pos - Vec(bestX, bestY);
                const double depth2 = gmtl::lengthSquared(diff);
                if ((a.pos[1] <= bot  - rad) | (a.pos[1] >= top  + rad) |
                    (a.pos[0] <= left - rad) | (a.pos[0] >= rite + rad) |
                    (depth2 >= trad)) { continue; }

                const Vec topPoint(clamp(left, rite, a.pos[0]), top);
                const Vec botPoint(clamp(left, rite, a.pos[0]), bot);
                const Vec leftPoint(left, clamp(bot, top, a.pos[1]));
                const Vec ritePoint(rite, clamp(bot, top, a.pos[1]));
                std::vector< std::pair< double, Vec > > distNorms = {
                    { diffLength(a.pos, topPoint), { 0, 1 } },
                    { diffLength(a.pos, botPoint), { 0, -1 } },
                    { diffLength(a.pos, leftPoint), { -1, 0 } },
                    { diffLength(a.pos, ritePoint), { 1, 0 } },
                };
                std::sort(distNorms.begin(), distNorms.end(), [](const auto &a, const auto &b) {
                        return a.first < b.first;
                });
                const Vec norm = distNorms.front().second;

                const double dep = a.rad[0] - std::sqrt(depth2);
                normal += norm;
                depth += dep;
                elasticity += b.elasticity;
                ++contactCount;
                if (a.gather) {
                    a.contacts.push_back({ core.entities.getHandleFromLow(j)->id(),
                                           Vec(bestX, bestY), norm,
                                           dep, gmtl::length(a.vel) * a.mass });
                }
            } else {
                const double trad = a.rad[0] + b.rad[0];
                if ((std::abs(a.pos[0] - b.pos[0]) >= trad) |
                    (std::abs(a.pos[1] - b.pos[1]) >= trad)) { continue; }
                Vec diff = a.pos - b.pos;
                const double len2 = gmtl::lengthSquared(diff);
                if (len2 >= trad * trad) { continue; }
                gmtl::normalize(diff);
                const double dep = (trad - std::sqrt(len2)) / 2.0;
                normal += diff;
                depth += dep;
                elasticity += b.elasticity;
                ++contactCount;
                if (a.gather) {
                    a.contacts.push_back({ core.entities.getHandleFromLow(j)->id(),
                                           b.pos + b.rad[0] * diff, diff,
                                           dep, gmtl::length(a.vel) * a.mass });
                }
            }
        }
        if (0 != contactCount) {
            gmtl::normalize(normal);
            elasticity /= contactCount;
            depth /= contactCount;

            const Vec add = normal * depth;
            a.impulse = add;
            a.pos += add;
            a.surface = normal;

            Vec velNorm = a.vel;
            const Vec crossNorm = Vec(normal[1], -normal[0]);
            const double speed = gmtl::length(velNorm);
            gmtl::normalize(velNorm);
            a.vel = (FRICTION * gmtl::dot(velNorm, crossNorm) * crossNorm -
                    a.elasticity * elasticity * gmtl::dot(velNorm, normal) * normal) * speed;
        }
    }

    typedef std::pair< size_t, Vec > Adjustment;
    std::vector< Adjustment > adjustments;
    const size_t dynCount = dynamics.size();

    std::vector< TaggedPhys > tagged;
    tagged.reserve(dynamics.size());
    for (size_t i = 0; i < dynCount; ++i) {
        tagged.push_back({ i, &comps[dynamics[i]] });
    }
    Quadtree qt(tagged);
    //std::cout << "Dynamic: " << dynamics.size() << qt << '\n';
    //qt.draw(core);

    // Dynamic collisions
    std::vector< size_t > suggested;
    for (size_t i = 0; i < dynCount; ++i) {
        auto &a = comps[dynamics[i]];
        suggested.clear();
        qt.suggest(a.pos, a.rad, suggested);
        for (size_t j : suggested) {
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
                a.contacts.push_back({ core.entities.getHandleFromLow(dynamics[j])->id(),
                                       b.pos + b.rad[0] * norm, norm,
                                       depth, closingSpeed * b.mass });
            }
            if (b.gather) {
                b.contacts.push_back({ core.entities.getHandleFromLow(dynamics[i])->id(),
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
    for (const auto adjust : adjustments) {
        comps[adjust.first].pos += adjust.second;
    }
};

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

PhysicsComponent PhysicsManager::makeDefault() const {
    return {
        Vec(0, 0), Vec(0, 0), Vec(0, 0), Vec(0, 0), Vec(0, 0), Vec(0, 0),
        0, 0, 0,
        Shape::Circle,
        true, true, false,
        std::vector< Contact >()
    };
}

void PhysicsManager::addBinding(Entity which, Entity to, Vec offset) {
    bindings.push_back({ which, to, offset });
}

void PhysicsManager::bind(Comps &next) {
    for (const auto &bind : bindings) {
        const auto &owner = next[bind.to];
        auto &pet = next[bind.which];
        pet.pos = owner.pos + bind.offset;
        pet.vel = owner.vel;
        pet.acc = owner.acc;
    }
}


void PhysicsManager::physicsUpdate(Core &core, const Comps &lasts, Comps &nexts) {
    if (nexts.empty()) { return; }
    move(lasts, nexts);
    bind(nexts);
    collide(core, nexts, lasts);
}

void PhysicsManager::updatePhysics(Core &core) {
    ComponentManager::update([this, &core](const Comps &l, Comps &n) { physicsUpdate(core, l, n); });
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

template<>
PyObject *toPython< PhysicsManager >(PhysicsManager &pm) {
    RUN_ONCE(PyType_Ready(&pmType));
    PyPhysicsManager *ppm;
    ppm = reinterpret_cast< PyPhysicsManager * >(pmType.tp_alloc(&pmType, 0));
    if (ppm) {
        ppm->pm = &pm;
    }
    return reinterpret_cast< PyObject * >(ppm);
}
