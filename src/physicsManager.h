#pragma once

#include <memory>
#include <vector>
#include <ostream>

#include "componentManager.h"
#include "utility.h"
#include "mirror.h"

#include <boost/hana.hpp>
#include <Python.h>

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

class PhysicsManager: public ComponentManager< PhysicsComponent > {
    private:
        struct Bind {
            Entity which;
            Entity to;
            Vec offset;
        };
        std::vector< Bind > bindings;

        void bind(Components &);
        void physicsUpdate(const Components &lasts, Components &nexts);

    protected:
        PhysicsComponent makeDefault() const override;

    public:
        void addBinding(Entity which, Entity to, Vec offset);
        void updatePhysics();
};

const double PHYSICS_TIMESTEP = 1.0 / 250.0;
