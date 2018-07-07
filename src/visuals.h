#pragma once

#include "data.h"
#include "geometry.h"

struct Colour { Point3 colour; };
DeclareDataType(Colour);

class Tracker;
class Renderer;
void draw(Tracker &track, Renderer &renderer);
