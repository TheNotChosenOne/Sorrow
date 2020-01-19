#pragma once

#include "entities/data.h"
#include "entities/tracker.h"
#include "entities/systems.h"

#include <functional>

struct Core;
struct PhysBody;
struct Turret;

struct Controller {
    std::function< void(Core &, PhysBody &, Entity::EntityID) > controller;
};
DeclareDataType(Controller);

struct TurretController {
    std::function< void(Core &, std::vector< Turret > &, Entity::EntityID) > controller;
};
DeclareDataType(TurretController);

void KeyboardController(Core &core, PhysBody &pb, Entity::EntityID);
void KeyboardTurretController(Core &core, std::vector< Turret > &turrets, Entity::EntityID eid);

class ControllerSystem: public Entity::BaseSystem {
    public:
    ControllerSystem();
    ~ControllerSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};
