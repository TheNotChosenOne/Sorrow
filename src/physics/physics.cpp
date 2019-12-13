#include "physics/physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "core/core.h"

#include <utility>
#include <Box2D.h>
#include <memory>
#include <map>

namespace {

std::vector< std::pair< Entity::EntityID, Entity::EntityID > > k_collisions;

class PhysListener: public b2ContactListener {
    void PostSolve(b2Contact *contact, const b2ContactImpulse *) {
        const void *a = contact->GetFixtureA()->GetUserData();
        const void *b = contact->GetFixtureB()->GetUserData();
        const Entity::EntityID eidA = reinterpret_cast< Entity::EntityID >(a);
        const Entity::EntityID eidB = reinterpret_cast< Entity::EntityID >(b);
        k_collisions.push_back(std::make_pair(eidA, eidB));
    }
};
std::unique_ptr< PhysListener > physListener;

void update(Core &core, double seconds, std::vector< PhysBody > &, std::vector< PhysBody > &, std::vector< HitData > &hits,
    const Entity::IDMap &, const Entity::IDMap &idmap) {
    k_collisions.clear();
    core.b2world.locked([&](){
        //core.b2world->Step(1.0 / 60.0, 8, 3);
        core.b2world.b2w->Step(seconds, 8, 3);
    });
    for (auto &hit : hits) {
        hit.id.clear();
    }
    std::map< Entity::EntityID, size_t > backmapper;
    for (size_t i = 0; i < idmap.size(); ++i) {
        backmapper[idmap[i]] = i;
    }
    for (const auto &cp : k_collisions) {
        auto loc = backmapper.find(cp.first);
        if (backmapper.end() != loc) {
            hits[loc->second].id.push_back(cp.second);
        }
        loc = backmapper.find(cp.second);
        if (backmapper.end() != loc) {
            hits[loc->second].id.push_back(cp.first);
        }
    }
}
    
}

template<>
void Entity::initComponent< PhysBody >(Core &, const uint64_t id, PhysBody &body) {
    rassert(body.body);
    body.body->GetFixtureList()->SetUserData(reinterpret_cast< void * >(id));
}

template<>
void Entity::deleteComponent< PhysBody >(Core &core, const uint64_t, PhysBody &body) {
    core.b2world.locked([&](){
        core.b2world.b2w->DestroyBody(body.body);
    });
}

PhysicsSystem::PhysicsSystem()
    : BaseSystem("Physics", Entity::getSignature< PhysBody, HitData >()) {
}

PhysicsSystem::~PhysicsSystem() { }

void PhysicsSystem::init(Core &core) {
    core.tracker.addSource< PhysBodyData >();
    core.tracker.addSource< HitDataData >();

    physListener = std::make_unique< PhysListener >();
    core.b2world.locked([&](){
        core.b2world.b2w->SetContactListener(physListener.get());
    });
}

void PhysicsSystem::execute(Core &core, double seconds) {
    Entity::Exec< Entity::Packs< PhysBody >, Entity::Packs< PhysBody, HitData > >::run(core.tracker,
    [&](auto &basics, auto &complexes) {
        update(core, seconds, basics.first.template get< PhysBody >(), complexes.first.template get< PhysBody >(),
                complexes.first.template get< HitData >(), basics.second, complexes.second);
    });
}
