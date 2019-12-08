#include "core/geometry.h"

#include <cmath>

Vec normalized(const Vec v) {
    const auto len = CGAL::sqrt(v.squared_length());
    if (0.0 == len) {
        return v;
    }
    return v / len;
}

b2Vec2 normalized(const b2Vec2 v) {
    b2Vec2 n(v);
    n.Normalize();
    return n;
}
