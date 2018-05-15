#include "entityManager.h"

#include <map>
#include <set>
#include <vector>
#include <algorithm>

EntityManager::EntityManager() {
}

EntityManager::~EntityManager() {
}

void EntityManager::attach(ManagerPtr manager) {
    managers.insert(std::move(manager));
}

Entity EntityManager::create() {
    const size_t size = entities.size();
    for (auto &manPtr : managers) {
        manPtr->create();
    }
    entities.insert(size);
    return size;
}

void EntityManager::kill(Entity e) {
    graveyard.insert(e);
    entities.erase(e);
}

EntityHandle EntityManager::getHandle(Entity e) {
    handleMap[e] = e;
    return e;
}

Entity EntityManager::fromHandle(EntityHandle e) {
    return handleMap[e];
}

void EntityManager::update() {
    for (auto &manPtr : managers) {
        manPtr->graduate();
    }
    if (graveyard.empty() || entities.empty()) { return; }
    std::map< size_t, size_t > remap;
    const size_t alive = entities.size();

    auto moveIter = std::lower_bound(entities.begin(), entities.end(), alive);
    auto targetIter = graveyard.begin();
    for (; moveIter != entities.end(); ++moveIter, ++targetIter) {
        remap[*moveIter] = *targetIter;
    }
    for (auto &manPtr : managers) {
        manPtr->reorder(remap);
        manPtr->cull(graveyard.size());
    }
    for (auto it = handleMap.begin(); it != handleMap.end(); ++it) {
        if (graveyard.count(it->second)) {
            handleMap.erase(it);
        } else if (remap.count(it->second)) {
            it->second = remap[it->second];
        }
    }
    graveyard.clear();
    entities.clear();
    for (size_t i = 0; i < alive; ++i) {
        entities.insert(i);
    }
}

const std::set< Entity > &EntityManager::all() const {
    return entities;
}
