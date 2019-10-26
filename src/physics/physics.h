#pragma once

#include "entities/data.h"
#include "entities/tracker.h"
#include "entities/systems.h"
#include "physics/geometry.h"

#include <vector>

class b2Body;
struct PhysBody {
    b2Body *body;
};
DeclareDataType(PhysBody);
template<>
void Entity::initComponent< PhysBody >(Core &core, uint64_t id, PhysBody &body);
template<>
void Entity::deleteComponent< PhysBody >(Core &core, uint64_t id, PhysBody &body);

struct HitData {
    std::vector< Entity::EntityID > id;
};
DeclareDataType(HitData);

class PhysicsSystem: public Entity::BaseSystem {
    public:
    PhysicsSystem();
    ~PhysicsSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};
