#pragma once

#include "entities/data.h"
#include "entities/tracker.h"
#include "geometry.h"

#include <vector>

class b2Body;
struct PhysBody {
    b2Body *body;
};
DeclareDataType(PhysBody);

struct HitData {
    std::vector< Entity::EntityID > id;
};
DeclareDataType(HitData);

class Core;
void updatePhysics(Core &core);
