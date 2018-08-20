#pragma once

#include "entities/data.h"
#include "entities/tracker.h"
#include "geometry.h"

#include <vector>

struct Position { Point v; };
DeclareDataType(Position);

struct Direction { Dir v; };
DeclareDataType(Direction);

struct Speed { double d; };
DeclareDataType(Speed);

enum ShapeType {
    ShapeCircle = 0,
    ShapeBox = 1,
};
struct Shape {
    ShapeType type;
    Vec rad;
};
DeclareDataType(Shape);

struct HitData {
    std::vector< Entity::EntityID > id;
};
DeclareDataType(HitData);

class Core;
void updatePhysics(Core &core);
