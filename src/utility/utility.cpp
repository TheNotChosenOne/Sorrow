#include "utility/utility.h"

#include <random>

std::mt19937_64 rng(0x88888888);
std::uniform_real_distribution< double > distro(0.0, 1.0);

double rnd(const double x) {
    return x * 2.0 * (distro(rng) - 0.5);
}

double rnd_range(const double l, const double h) {
    return distro(rng) * (h - l) + l;
}
