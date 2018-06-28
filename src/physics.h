#pragma once

#include "data.h"

#include <gmtl/Vec.h>

typedef gmtl::Vec2d Vec;

struct Position { Vec v; };
DeclareDataType(Position);

struct Direction { Vec v; };
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
