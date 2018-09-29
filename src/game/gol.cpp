#include "gol.h"

#include "entities/tracker.h"
#include "physics/physics.h"
#include "core/core.h"

#include <random>

namespace {

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> distro(0.0, 1.0);

}

GOL::GOL(Core &core, Grid &&grid, const CreateFunc &cf): grid(grid), create(cf) {
    for (size_t row = 0; row < grid.getHeight(); ++row) {
        for (size_t col = 0; col < grid.getWidth(); ++col) {
            if (distro(gen) < 0.2) {
                Entity::EntityID eid = create(core, grid.getSize());
                auto optPhys = core.tracker.optComponent< PhysBody >(eid);
                rassert(optPhys, eid);
                b2Body *body = optPhys->get().body;
                const auto centre = grid.gridOrigin(row, col) + Vec(grid.getSize(), grid.getSize());
                body->SetTransform(PCast(centre), 0);
            }
        }
    }
}

void GOL::update(Core &) {

}
