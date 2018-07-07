#pragma once

#include "data.h"
#include "geometry.h"

struct Position { Point v; };
DeclareDataType(Position);

struct Direction { Dir v; };
DeclareDataType(Direction);

struct Speed { double d; };
DeclareDataType(Speed);

enum ShapeType {
    Circle = 0,
    Box = 1,
};
struct Shape {
    ShapeType type;
    Vec rad;
};
DeclareDataType(Shape);

class Core;
void updatePhysics(Core &core);
