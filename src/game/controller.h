#pragma once

#include "entities/data.h"

#include <functional>

class Core;
struct PhysBody;

struct Controller {
    std::function< void(Core &, PhysBody &) > controller;
};
DeclareDataType(Controller);

void KeyboardController(Core &core, PhysBody &pb);

void applyControllers(Core &core);
