#include "physicsManager.h"

#include "mirror.h"
#include "core.h"
#include "entityManager.h"
#include "quadtree.h"
#include "visualManager.h"
#include "renderer.h"

#include <array>
#include <memory>
#include <limits>
#include <vector>
#include <Python.h>
#include <algorithm>
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
    //comp.pos += comp.vel * PHYSICS_TIMESTEP;
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

static inline void collideStaticBoxSingle(const EntityManager &eman, size_t j,
        PhysicsComponent &phys, const PhysicsComponent &stat,
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
    const Vec norm = getBest(phys.pos, topPoint, botPoint, leftPoint, ritePoint);

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

static inline void collideStaticCircleSingle(const EntityManager &eman, size_t j,
        PhysicsComponent &phys, const PhysicsComponent &stat,
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

typedef std::vector< size_t >::const_iterator Siter;
typedef std::pair< size_t, Vec > Adjustment;
static inline void staticCollideSingle(const EntityManager &eman, Comps &comps, PhysicsComponent &dyn,
        const Siter withBegin, const Siter withEnd) {
    size_t contactCount = 0;
    Vec normal;
    double depth = 0.0;
    double elasticity = 0.0;
    for (auto iter = withBegin; iter != withEnd; ++iter) {
        const size_t j = *iter;
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

static inline void dynamicCollideSingle(EntityManager &eman, Comps &comps, size_t i,
        const Siter withBegin, const Siter withEnd, std::vector< Adjustment > &adjustments) {
    auto &a = comps[i];
    for (auto iter = withBegin; iter != withEnd; ++iter) {
        const size_t j = *iter;
        if (j <= i) { continue; }
        auto &b = comps[j];

        const double trad = a.rad[0] + b.rad[0];
        if ((std::abs(a.pos[0] - b.pos[0]) >= trad) ||
            (std::abs(a.pos[1] - b.pos[1]) >= trad)) { continue; }
        Vec norm = a.pos - b.pos;
        const double len2 = gmtl::lengthSquared(norm);
        if (len2 >= trad * trad) { continue; }
        gmtl::normalize(norm);

        const double depth = (trad - std::sqrt(len2)) / 2.0;
        const double closingSpeed = std::abs(gmtl::dot(norm, a.vel)) +
                                    std::abs(gmtl::dot(norm, b.vel));
        if (a.gather) {
            a.contacts.push_back({ eman.getIDFromLow(j),
                                   b.pos + b.rad[0] * norm, norm,
                                   depth, closingSpeed * b.mass });
        }
        if (b.gather) {
            b.contacts.push_back({ eman.getIDFromLow(i),
                                   b.pos + b.rad[0] * norm, -norm,
                                   depth, closingSpeed * a.mass });
        }
        if (a.phased | b.phased) { continue; }

        const double massP = a.mass / (a.mass + b.mass);
        adjustments.push_back({ i, norm * depth * massP });
        adjustments.push_back({ j, norm * depth * (massP - 1.0) });
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
    for (const size_t i : indices) {
        tagged.push_back({ i, &comps[i] });
    }
    return Quadtree(tagged);
}

static void collide(Core &core, Comps &comps) {
    static std::vector< std::vector< size_t > > select;
    populate(select, comps);
    const auto &dynamics = select[0];
    const auto &statics = select[1];

    Quadtree staticQT = getQuadFor(comps, statics);
    std::vector< size_t > suggested;
    //staticQT.draw(core);

    // Static collisions
    for (const size_t i : dynamics) {
        auto &dyn = comps[i];
        suggested.clear();
        staticQT.suggest(dyn.pos, dyn.rad, suggested);
        staticCollideSingle(core.entities, comps, dyn, suggested.begin(), suggested.end());
    }

    std::vector< Adjustment > adjustments;

    Quadtree qt = getQuadFor(comps, dynamics);
    //qt.draw(core);

    // Dynamic collisions
    for (const size_t i : dynamics) {
        auto &a = comps[i];
        suggested.clear();
        qt.suggest(a.pos, a.rad, suggested);
        dynamicCollideSingle(core.entities, comps, i,
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

void PhysicsManager::setCore(Core &core) {
    this->core = &core;
}

#include "/home/swan/code/wykobi/wykobi.hpp"
#include "/home/swan/code/wykobi/wykobi_utilities.hpp"
#include "/home/swan/code/wykobi/wykobi_algorithm.hpp"

struct SweptCircle {
    wykobi::point2d< double > from;
    wykobi::point2d< double > to;
    double radius;
    wykobi::quadix2d sweep;
    size_t index;
};

struct SweptBox {
    wykobi::point2d< double > from;
    wykobi::point2d< double > to;
    double radius[2];
    wykobi::quadix2d sweep;
    size_t index;
};

SweptCircle getSweepC(const PhysicsComponent &phys, size_t index) {
    rassert(0 != phys.vel[0] || 0 != phys.vel[1], "Only moving objects should be swept");
    const Vec a = phys.pos;
    const Vec b = phys.pos + phys.vel * PHYSICS_TIMESTEP;
    Vec cross(phys.vel[1], -phys.vel[0]);
    gmtl::normalize(cross);
    const double rad = phys.rad[0];
    const Vec v1 = a + rad * cross;
    const Vec v2 = a - rad * cross;
    const Vec v3 = b - rad * cross;
    const Vec v4 = b + rad * cross;
    return SweptCircle {
        wykobi::make_point(a[0], a[1]),
        wykobi::make_point(b[0], b[1]),
        rad,
        wykobi::make_quadix(v1[0], v1[1], v2[0], v2[1], v3[0], v3[1], v4[0], v4[1]),
        index
    };
}

SweptBox getSweepB(const PhysicsComponent &phys, size_t index) {
    rassert(0 != phys.vel[0] || 0 != phys.vel[1], "Only moving objects should be swept");
    const Vec a = phys.pos;
    const Vec b = phys.pos + phys.vel;
    const double w = phys.rad[0];
    const double h = phys.rad[1];
    const Vec vel = phys.vel;
    Vec v1, v2, v3, v4;
    if (0.0 == vel[0]) {
        if (vel[1] > 0) { // Up
            v1 = Vec(a[0] - w, a[0] - h);
            v2 = Vec(a[0] + w, a[0] - h);
            v3 = Vec(b[0] + w, b[0] + h);
            v4 = Vec(b[0] - w, b[0] + h);
        } else { // Down
            v1 = Vec(b[0] - w, b[0] - h);
            v2 = Vec(b[0] + w, b[0] - h);
            v3 = Vec(a[0] + w, a[0] + h);
            v4 = Vec(a[0] - w, a[0] + h);
        }
    } else if (0.0 == vel[1]) {
        if (vel[0] > 0) { // Right
            v1 = Vec(b[0] + w, b[1] + h);
            v2 = Vec(b[0] + w, b[1] - h);
            v3 = Vec(a[0] - w, a[1] - h);
            v4 = Vec(a[0] - w, a[1] + h);
        } else { // Left
            v1 = Vec(a[0] + w, a[1] + h);
            v2 = Vec(a[0] + w, a[1] - h);
            v3 = Vec(b[0] - w, b[1] - h);
            v4 = Vec(b[0] - w, b[1] + h);
        }
    } else {
        const Vec access(vel[0] / std::abs(vel[0]), vel[1] / std::abs(vel[1]));
        const Vec cross(access[1], -access[0]);
        v1 = a + Vec(w * cross[0], h * cross[1]);
        v2 = a - Vec(w * cross[0], h * cross[1]);
        v3 = b - Vec(w * cross[0], h * cross[1]);
        v4 = b + Vec(w * cross[0], h * cross[1]);
    }
    return SweptBox {
        wykobi::make_point(a[0], a[1]),
        wykobi::make_point(b[0], b[1]),
        { w, h },
        wykobi::make_quadix(v1[0], v1[1], v2[0], v2[1], v3[0], v3[1], v4[0], v4[1]),
        index
    };
}

inline bool pointInCircle(const Vec p, const Vec c, const double r) {
    return diffLength2(p, c) <= r * r;
}

inline bool pointInBox(const Vec p, const Vec c, const Vec dim) {
    return ((c[0] - dim[0] <= p[0]) && (p[0] <= c[0] + dim[0]) &&
            (c[1] - dim[1] <= p[1]) && (p[1] <= c[1] + dim[1]));

}

inline bool lineCircleCollision(const Vec from, const Vec to,
                                const Vec pos, const double r, double &t) {
    Vec d = to - from;
    gmtl::normalize(d);
    const Vec m = from - pos;
    const double b = gmtl::dot(m, d);
    const double c = gmtl::dot(m, m) - r * r;
    if (c > .0 && b > 0.0) { return false; }
    const double discr = b * b - c;
    if (discr < 0.0) { return false; }

    t = (-b - std::sqrt(discr)) / diffLength(to, from);
    return true;
}

inline bool lineBoxCollision(const Vec from, const Vec to,
                             const Vec pos, const Vec dim, double &t) {
    Vec d = to - from;
    gmtl::normalize(d);
    double tmin = 0.0;
    double tmax = infty< double >();
    for (size_t i = 0; i < 2; ++i) {
        if (0.0 == std::abs(d[i])) {
            if (from[i] < pos[i] - dim[i] || from[i] > pos[i] + dim[i]) { return false; }
        } else {
            const double inv = 1.0 / d[i];
            double t1 = (pos[i] - dim[i] - from[i]) * inv;
            double t2 = (pos[i] + dim[i] - from[i]) * inv;
            if (t1 > t2) { std::swap(t1, t2); }
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) { return false; }
        }
    }
    t = tmin / diffLength(to, from);
    return true;
}

inline bool lineCircleCircleSum(const Vec from, const Vec to,
                                const Vec cc, const double rad1,
                                const double rad2, double &t) {
    const double rad = rad1 + rad2;
    if (pointInCircle(cc, from, rad)) {
        t = 0.0;
        return true;
    }
    if (lineCircleCollision(from, to, cc, rad, t)) {
        return t <= 1.0;
    }
    return false;
}

inline bool lineCircleBoxSum(const Vec from, const Vec to,
                             const Vec box, const Vec dim,
                             const double rad, double &t, Core &core) {
    // Collision against a circle on each corner, and then 2 rectangles
    // covering vertical and horizontal ranges
    for (double xx = -1.0; xx <= 1.0; xx += 2.0) {
        for (double yy = -1.0; yy <= 1.0; yy += 2.0) {
            const Vec corn = box + Vec(dim[0] * xx, dim[1] * yy);
            if (pointInCircle(corn, from, rad)) {
                const auto view = core.visuals.getViewMatrix(core.renderer);
                core.renderer.drawCircle(view * gmtl::Point2d(corn), view * Vec(rad, rad), Vec3(0xFF, 0, 0));
                t = 0.0;
                return true;
            }
        }
    }

    const Vec dimVert(dim[0], dim[1] + rad);
    const Vec dimHori(dim[0] + rad, dim[1]);

    if (pointInBox(from, box, dimVert)) {
        const auto view = core.visuals.getViewMatrix(core.renderer);
        core.renderer.drawBox(view * gmtl::Point2d(box), view * dimVert, Vec3(0xFF, 0, 0));
        t = 0.0;
        return true;
    }
    if (pointInBox(from, box, dimHori)) {
        const auto view = core.visuals.getViewMatrix(core.renderer);
        core.renderer.drawBox(view * gmtl::Point2d(box), view * dimHori, Vec3(0xFF, 0, 0));
        t = 0.0;
        return true;
    }

    double minty = infty< double >();
    for (double xx = -1.0; xx <= 1.0; xx += 2.0) {
        for (double yy = -1.0; yy <= 1.0; yy += 2.0) {
            const Vec corn = box + Vec(dim[0] * xx, dim[1] * yy);
            double tt;
            if (lineCircleCollision(from, to, corn, rad, tt)) {
                minty = std::min(minty, tt);
            }
        }
    }

    double tt;
    if (lineBoxCollision(from, to, box, dimVert, tt)) {
        minty = std::min(minty, tt);
    }
    if (lineBoxCollision(from, to, box, dimHori, tt)) {
        minty = std::min(minty, tt);
    }
    t = minty;
    return t <= 1.0;
}

bool collideBox(Core &core, const SweptCircle &sc, const PhysicsComponent &phys, double &t) {
    // circle, circle, quadix - box
    return lineCircleBoxSum(Vec(sc.from[0], sc.from[1]), Vec(sc.to[0], sc.to[1]),
                            phys.pos, phys.rad, sc.radius, t, core);
}

bool collideCircle(const SweptCircle &sc, const PhysicsComponent &phys, double &t) {
    return lineCircleCircleSum(Vec(sc.from[0], sc.from[1]), Vec(sc.to[0], sc.to[1]),
                                phys.pos, phys.rad[0], sc.radius, t);
}

bool collideSC(const SweptCircle &sc, const SweptCircle &phys, double &t) {
    const Vec p0(sc.from[0], sc.from[1]);
    const Vec p1(phys.from[0], phys.from[1]);
    const Vec v0 = Vec(sc.to[0], sc.to[1]) - Vec(sc.from[0], sc.from[1]);
    const Vec v1 = Vec(phys.to[0], phys.to[1]) - Vec(phys.from[0], phys.from[1]);

    const Vec s = p1 - p0;
    const Vec v = v1 - v0;
    const double r = sc.radius + phys.radius;
    const double c = gmtl::dot(s, s) - r * r;
    if (c < 0.0) {
        t = 0.0;
        return true;
    }

    const double a = gmtl::dot(v, v);
    if (a < 0.0) { return false; }
    const double b = gmtl::dot(v, s);
    if (b >= 0.0) { return false; }

    const double d = b * b - a * c;
    if (d < 0.0) { return false; }

    t = (-b - std::sqrt(d)) / a;
    return true;
}

void PhysicsManager::updatePhysics(Core &core) {
    //move(components);
    //collide(core, components);
    
    std::vector< SweptBox > sweepB;
    std::vector< SweptCircle > sweepC;
    std::vector< size_t > stillB;
    std::vector< size_t > stillC;
    for (size_t i = 0; i < components.size(); ++i) {
        PhysicsComponent &phys = components[i];
        moveSingle(phys);
        const bool still = 0.0 == phys.vel[0] && 0.0 == phys.vel[1];
        if (Shape::Circle == phys.shape) {
            if (still) {
                stillC.push_back(i);
            } else {
                sweepC.push_back(getSweepC(phys, i));
            }
        } else {
            if (still) {
                stillB.push_back(i);
            } else {
                sweepB.push_back(getSweepB(phys, i));
            }
        }
    }
    rassert(sweepB.empty(), "Cannot handle non-static boxes");

    // sweepC:
    //  stillB
    //  stillC
    //  sweepC
    for (const SweptCircle &sc : sweepC) {
        double minty = 1.0;
        double t;
        for (const size_t i : stillB) {
            if (collideBox(core, sc, components[i], t)) { minty = std::min(minty, t); }
        }
        for (const size_t i : stillC) {
            if (collideCircle(sc, components[i], t)) { minty = std::min(minty, t); }
        }
        for (const SweptCircle &other : sweepC) {
            if (&other == &sc) { continue; }
            if (collideSC(sc, other, t)) { minty = std::min(minty, t); }
        }
        const size_t i = sc.index;
        components[i].pos += minty * components[i].vel * PHYSICS_TIMESTEP;
        components[i].vel = Vec(0, 0);
    }
    std::cout << sweepC.size() << '\n';
}

namespace {

static inline bool inside(const PhysicsComponent &phys, const Vec &pos) {
    if (Shape::Box == phys.shape) {
        const bool hColl = phys.pos[0] - phys.rad[0] <= pos[0] && pos[0] <= phys.pos[0] + phys.rad[0];
        const bool vColl = phys.pos[1] - phys.rad[1] <= pos[1] && pos[1] <= phys.pos[1] + phys.rad[1];
        return hColl && vColl;
    } else {
        return diffLength2(phys.pos, pos) <= phys.rad[0] * phys.rad[1];
    }
}

}

RaycastResult PhysicsManager::rayCast(const Vec pos, const Vec dir) const {
    double minty = infty< double >();
    size_t minEnt = static_cast< size_t >(-1);
    for (size_t i = 0; i < components.size(); ++i) {
        const PhysicsComponent &phys = components[i];
        if (inside(phys, pos)) {
            return { core->entities.getHandleFromLow(i), 0.0 };
        }
        if (Shape::Circle == phys.shape) {
            const Vec m = pos - phys.pos;
            const double b = gmtl::dot(m, dir);
            const double c = gmtl::dot(m, m) - phys.rad[0] * phys.rad[1];
            if (c > 0.0 && b > 0.0) { continue; }
            const double discr = b * b - c;
            if (discr < 0.0) { continue; }
            const double t = -b - std::sqrt(discr);
            if (t < minty) {
                minty = t;
                minEnt = i;
            }
        } else {
            double tmin = 0.0;
            double tmax = infty< double >();
            bool miss = false;
            for (size_t i = 0; i < 2; ++i) {
                if (0.0 == dir[i]) {
                    if (pos[i] < phys.pos[i] - phys.rad[i] ||
                        pos[i] > phys.pos[i] + phys.rad[i]) {
                        miss = true;
                        break;
                    }
                } else {
                    const double delta = 1.0 / dir[i];
                    double t1 = (phys.pos[i] - phys.rad[i] - pos[i]) * delta;
                    double t2 = (phys.pos[i] + phys.rad[i] - pos[i]) * delta;
                    if (t1 > t2) { std::swap(t1, t2); }
                    tmin = std::max(tmin, t1);
                    tmax = std::min(tmax, t2);
                    if (tmin > tmax) { miss = true; break; }
                }
            }
            if (!miss && tmin > 0.0 && tmin < minty) {
                minty = tmin;
                minEnt = i;
            }
        }
    }
    if (static_cast< size_t >(-1) == minEnt) {
        return { nullptr, 0 };
    }
    return { core->entities.getHandleFromLow(minEnt), minty };
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

static PyObject *Py_rayCast(PyPhysicsManager *self, PyObject *args) {
    Vec pos, dir;
    fromPython(pos, PyTuple_GetItem(args, 0));
    fromPython(dir, PyTuple_GetItem(args, 1));
    auto res = self->pm->rayCast(pos, dir);
    PyObject *tupperware = PyTuple_New(2);
    PyTuple_SetItem(tupperware, 0, toPython(res.hit));
    PyTuple_SetItem(tupperware, 1, toPython(res.dist));
    return tupperware;
}

static PyMethodDef pmMethods[] = {
    { "get", reinterpret_cast< PyCFunction >(Py_get), READONLY, "Get physics component" },
    { "timestep", reinterpret_cast< PyCFunction >(Py_timestep), READONLY, "Time between physics steps" },
    { "stepsPerSecond", reinterpret_cast< PyCFunction >(Py_stepsPerSecond), READONLY, "Physics steps per second" },
    { "rayCast", reinterpret_cast< PyCFunction >(Py_rayCast), READONLY, "Cast a ray" },
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
