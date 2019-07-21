#pragma once

#include "entities/systems.h"
#include "entities/tracker.h"
#include "entities/data.h"
#include "core/geometry.h"

struct Camera {
    double radius;
};
DeclareDataType(Camera);

class CameraSystem: public Entity::BaseSystem {
    public:
    CameraSystem();
    ~CameraSystem();
    void init(Core &core);
    void execute(Core &core, double seconds);
};
