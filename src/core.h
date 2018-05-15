#pragma once

#include <cstddef>

class Renderer;
class Input;
class EntityManager;
class PhysicsManager;
class VisualManager;
class LogicManager;
typedef size_t Entity;
typedef size_t EntityHandle;

struct Core {
    Renderer &renderer;
    Input &input;
    EntityManager &entities;
    PhysicsManager &physics;
    VisualManager &visuals;
    LogicManager &logic;
    EntityHandle player;
};
