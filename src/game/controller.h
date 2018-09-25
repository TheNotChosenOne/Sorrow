#pragma once

#include "entities/data.h"
#include "entities/tracker.h"

#include <functional>

class Core;
struct PhysBody;

struct Controller {
    std::function< void(Core &, PhysBody &, Entity::EntityID) > controller;
};
DeclareDataType(Controller);

void KeyboardController(Core &core, PhysBody &pb, Entity::EntityID);

void applyControllers(Core &core);
