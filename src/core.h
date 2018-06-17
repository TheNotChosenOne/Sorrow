#pragma once

#include <boost/program_options.hpp>
#include <cstddef>
#include <memory>

#include "forwardMirror.h"


class Renderer;
class Input;
class EntityManager;
class PhysicsManager;
class VisualManager;
class LogicManager;
typedef size_t Entity;
class AI;

struct Core {
    Renderer &renderer;
    Input &input;
    EntityManager &entities;
    PhysicsManager &physics;
    VisualManager &visuals;
    LogicManager &logic;
    AI &ai;
    boost::program_options::variables_map options;
};

template<>
PyObject *toPython< Core >(Core &core);
