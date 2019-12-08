#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Direction_2.h>
#include <Box2D.h>

#include "core/geometry.h"

class Core;

b2Body *makeCircle(Core &core, Point centre, double radius, bool dynamic = true, bool rotates = true);

b2Body *makeRect(Core &core, Point centre, double width, double height, bool dynamic = true, bool rotates = true);
