#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Direction_2.h>
#include <Box2D.h>

typedef CGAL::Simple_cartesian< double > Kernel;
typedef Kernel::Vector_2 Vec;
typedef Kernel::Vector_3 Vec3;
typedef Kernel::Point_2 Point;
typedef Kernel::Point_3 Point3;
typedef Kernel::Direction_2 Dir;
typedef Kernel::Segment_2 Seg;
typedef Kernel::Circle_2 Circle;

template< typename T, typename V >
T VPC(const V v) {
    return T(v[0], v[1]);
}

template< typename T >
T VPC(const b2Vec2 v) {
    return T(v.x, v.y);
}
