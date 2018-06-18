#pragma once

#include <memory>
#include <vector>
#include <ostream>

#include "componentManager.h"
#include "utility.h"
#include "mirror.h"

#include <boost/hana.hpp>
#include <Python.h>

const double SPEED_LIMIT = 500.0;
const double STEPS_PER_SECOND = 250.0;
const double PHYSICS_TIMESTEP = 1.0 / STEPS_PER_SECOND;

enum Shape {
    Box    = 0,
    Circle = 1
};

struct Contact {
    BOOST_HANA_DEFINE_STRUCT(Contact,
        (Entity, which),
        (Vec, where),
        (Vec, norm),
        (double, depth),
        (double, force));
};
std::ostream &operator<<(std::ostream &os, const Contact &k);
bool operator==(const Contact &a, const Contact &b);

struct PhysicsComponent {
    BOOST_HANA_DEFINE_STRUCT(PhysicsComponent,
        (Vec, pos),
        (Vec, vel),
        (Vec, acc),
        (Vec, impulse),
        (Vec, rad),           // width / height
        (Vec, surface),       // Unit vector norm if colliding
        (double, mass),
        (double, area),       // Cross section area for air resistance
        (double, elasticity),
        (Shape, shape),
        (bool, isStatic),     // Doesn't move
        (bool, phased),       // Doesn't have any effect on collisions
        (bool, gather),       // Whether to gather contacts
        (std::vector< Contact >, contacts));
};

template<>
PyObject *toPython< Shape >(Shape &s);

template<>
void fromPython< Shape >(Shape &s, PyObject *obj);

class Core;

class EntityView;
typedef std::shared_ptr< EntityView > EntityHandle;
struct RaycastResult {
    EntityHandle hit;
    double dist;
};

struct Binding {
    Entity to;
    Entity which;
    Vec offset;
};

class PhysicsManager: public BaseComponentManager {
    public:
        typedef PhysicsComponent Component;
        typedef std::vector< Component > Components;

    private:
        Components components;
        Components nursery;
        Core *core;
        std::vector< Binding > bindings;

        void create() override;
        void graduate() override;
        void reorder(const std::map< size_t, size_t > &remap) override;
        void cull(size_t count) override;

    public:
        const double timestep = PHYSICS_TIMESTEP;
        const double stepsPerSecond = STEPS_PER_SECOND;

        void setCore(Core &core);

        void updatePhysics(Core &core);

        Component &get(Entity e);
        const Component &get(Entity e) const;

        void bind(Entity to, Entity which, const Vec offset);
        void unbind(Entity to, Entity which);
        RaycastResult rayCast(const Vec pos, const Vec dir) const;
};

template<>
PyObject *toPython< PhysicsManager >(PhysicsManager &pm);
