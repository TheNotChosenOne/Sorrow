#include "core.h"

#include "input/input.h"
#include "visual/renderer.h"
#include "entities/tracker.h"
#include "entities/systems.h"

void ThreadedWorld::locked(const std::function< void() > &func) {
    std::unique_lock lock(tex);
    func();
}
