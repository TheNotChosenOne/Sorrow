#pragma once

#include "entities/data.h"
#include "entities/tracker.h"
#include "entities/systems.h"

#include <functional>

class Core;
struct PhysBody;

struct Controller {
    std::function< void(Core &, PhysBody &, Entity::EntityID) > controller;
};
DeclareDataType(Controller);

void KeyboardController(Core &core, PhysBody &pb, Entity::EntityID);

class ControllerSystem: public Entity::BaseSystem {
    public:
    ControllerSystem();
    ~ControllerSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};
