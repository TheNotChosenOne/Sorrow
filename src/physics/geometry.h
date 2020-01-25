#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Direction_2.h>
#include <Box2D.h>

#include "core/geometry.h"

class Core;

struct PhysProperties {
    bool dynamic = true;
    bool rotates = true;
    bool sensor = false;
    uint16_t category = 0x0001; // These are the defaults
    uint16_t mask = 0xFFFF;
};

b2Body *makeCircle(Core &core, Point centre, double radius, PhysProperties properties = PhysProperties{});

b2Body *makeRect(Core &core, Point centre, double width, double height, PhysProperties properties = PhysProperties{});
