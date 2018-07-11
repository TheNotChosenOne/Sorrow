#include "geometry.h"

bool collide(const Rect &a, const Rect &b) {
    for (size_t i = 0;  i < 2; ++i) {
        if ((a.cen[i] - a.rad[i] > b.cen[i] + b.rad[i]) ||
            (a.cen[i] + a.rad[i] < b.cen[i] - b.rad[i])) {
            return false;
        }
    }
    return true;
}
