#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Direction_2.h>

typedef CGAL::Simple_cartesian< double > Kernel;
typedef Kernel::Vector_2 Vec;
typedef Kernel::Vector_3 Vec3;
typedef Kernel::Point_2 Point;
typedef Kernel::Point_3 Point3;
typedef Kernel::Direction_2 Dir;
typedef Kernel::Segment_2 Seg;
typedef Kernel::Circle_2 Circle;
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
