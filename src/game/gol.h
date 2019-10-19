#pragma once

#include "grid.h"
#include "entities/data.h"

#include <functional>

struct Core;

struct PlantTag { };
DeclareDataType(PlantTag);

class GOL {
public:
    typedef std::function< uint64_t(Core &, double) > CreateFunc;

private:
    Grid grid;
    CreateFunc create;

public:
    GOL(Core &core, Grid &&grid, const CreateFunc &cf);
    void update(Core &core);
};
