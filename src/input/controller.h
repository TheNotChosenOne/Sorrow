#pragma once

#include "entities/data.h"
#include "entities/tracker.h"
#include "entities/systems.h"

#include <SDL2/SDL.h>
#include <functional>

struct Core;
struct PhysBody;
struct Turret;

typedef std::map< std::string, SDL_Keycode > Layout;

struct Controller {
    std::function< void(Core &, PhysBody &, Entity::EntityID, const Layout &layout) > controller;
    Layout layout;
};
DeclareDataType(Controller);

struct TurretController {
    std::function< void(Core &, std::vector< Turret > &, Entity::EntityID, const Layout &layout) > controller;
    Layout layout;
};
DeclareDataType(TurretController);

void KeyboardController(Core &core, PhysBody &pb, Entity::EntityID, const Layout &layout);
void KeyboardTurretController(Core &core, std::vector< Turret > &turrets, Entity::EntityID eid, const Layout &layout);

class ControllerSystem: public Entity::BaseSystem {
    public:
    ControllerSystem();
    ~ControllerSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};
