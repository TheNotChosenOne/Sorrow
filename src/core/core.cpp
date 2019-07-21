#include "core.h"

#include "input/input.h"
#include "visual/renderer.h"
#include "entities/tracker.h"
#include "entities/systems.h"

#include <algorithm>

void ThreadedWorld::locked(const std::function< void() > &func) {
    std::unique_lock lock(tex);
    func();
}

double Core::scale() const {
    if (-0.0001 <= radius && radius <= 0.0001) { return 1.0; }
    const double dim = std::min(renderer.getWidth(), renderer.getHeight()) / 2.0;
    return dim / radius;
}
