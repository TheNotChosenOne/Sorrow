#pragma once

#include "entities/data.h"

struct SwarmTag {
    uint16_t tag;
};
DeclareDataType(SwarmTag);
struct MouseFollow { };
DeclareDataType(MouseFollow);

class Core;
void updateSwarms(Core &core);
