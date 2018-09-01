#include "physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "core/core.h"

#include <utility>
#include <Box2D.h>
#include <map>

namespace {

//void update(Core &core, std::vector< PhysBody > &basics, std::vector< PhysBody > &cmplxs, std::vector< HitData > &hd,
//    const Entity::IDMap &bidm, const Entity::IDMap &cidm) {
void update(Core &core, std::vector< PhysBody > &, std::vector< PhysBody > &, std::vector< HitData > &,
    const Entity::IDMap &, const Entity::IDMap &) {
    core.b2world->Step(1.0 / 60.0, 8, 3);
}

}

void updatePhysics(Core &core) {
    Entity::Exec< Entity::Packs< PhysBody >, Entity::Packs< PhysBody, HitData > >::run(core.tracker,
    [&](auto &basics, auto &complexes) {
        update(core, basics.first.template get< PhysBody >(), complexes.first.template get< PhysBody >(),
                complexes.first.template get< HitData >(), basics.second, complexes.second);
    });
}
