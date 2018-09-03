#include "geometry.h"

const b2Vec2 VCast(const Vec &v) {
    return b2Vec2(v.x(), v.y());
}

const b2Vec2 VCast(const Point &v) {
    return b2Vec2(v.x(), v.y());
}

const Vec VCast(const b2Vec2 &v) {
    return Vec(v.x, v.y);
}

const b2Vec2 PCast(const Point &v) {
    return b2Vec2(v.x(), v.y());
}

const b2Vec2 PCast(const Vec &v) {
    return b2Vec2(v.x(), v.y());
}

const Point PCast(const b2Vec2 &v) {
    return Point(v.x, v.y);
}
