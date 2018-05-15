#include "utility.h"

double diffLength(const Vec a, const Vec b) {
    const Vec diff = a - b;
    return gmtl::length(diff);
}

double diffLength2(const Vec a, const Vec b) {
    const Vec diff = a - b;
    return gmtl::lengthSquared(diff);
}
