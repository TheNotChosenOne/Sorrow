#pragma once

#include "entities/data.h"
#include "physics/geometry.h"

struct Colour { Point3 colour; };
DeclareDataType(Colour);

namespace Entity { class Tracker; }
class Renderer;
void draw(Entity::Tracker &track, Renderer &renderer, const Point position, const double scale);
