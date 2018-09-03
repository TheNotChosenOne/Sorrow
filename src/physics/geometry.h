#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Direction_2.h>

#include "core/geometry.h"

struct Rect {
    Point cen;
    Vec rad;
};

bool collide(const Circle &, const Circle &);
bool collide(const Circle &, const Rect &);
bool collide(const Rect &, const Circle &);
bool collide(const Rect &, const Rect &);

template< typename T >
Vec normalized(const T &t) {
    Vec v { t.x(), t.y() };
    if (0.0 == v.x() && 0.0 == v.y()) { return v; }
    return v / std::sqrt(v.squared_length());
}
