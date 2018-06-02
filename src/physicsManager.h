#pragma once

#include <memory>
#include <vector>
#include <ostream>

#include "componentManager.h"
#include "utility.h"
#include "mirror.h"

#include <boost/hana.hpp>
#include <Python.h>

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

class PhysicsManager: public ComponentManager< PhysicsComponent > {
    private:
        struct Bind {
            Entity which;
            Entity to;
            Vec offset;
        };
        std::vector< Bind > bindings;

        void bind(Components &);
        void physicsUpdate(Core &core, const Components &lasts, Components &nexts);

    protected:
        PhysicsComponent makeDefault() const override;

    public:
        const double timestep = PHYSICS_TIMESTEP;
        const double stepsPerSecond = STEPS_PER_SECOND;

        void addBinding(Entity which, Entity to, Vec offset);
        void updatePhysics(Core &core);
};

template<>
PyObject *toPython< PhysicsManager >(PhysicsManager &pm);
