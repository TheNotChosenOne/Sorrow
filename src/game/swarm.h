#pragma once

#include "entities/data.h"
#include "entities/systems.h"

struct SwarmTag {
    uint16_t tag;
};
DeclareDataType(SwarmTag);
struct MouseFollow { };
DeclareDataType(MouseFollow);

class SwarmSystem: public Entity::BaseSystem {
    public:
    SwarmSystem();
    ~SwarmSystem();
    void init(Core &core);
    void execute(Core &core, double seconds);
};
