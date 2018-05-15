#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <map>

#include "componentManager.h"

typedef size_t Entity;
typedef size_t EntityHandle;
class BaseComponentManager;

typedef std::unique_ptr< BaseComponentManager > ManagerPtr;

class EntityManager {
    private:
        std::set< Entity > entities;
        std::set< Entity > graveyard;
        std::set< ManagerPtr > managers;
        std::map< size_t, size_t > handleMap;

    public:
        EntityManager();
        ~EntityManager();

        void attach(ManagerPtr manager);
        Entity create();
        void kill(Entity e);
        EntityHandle getHandle(Entity e);
        Entity fromHandle(EntityHandle e);

        // Used to clear out killed entities
        void update();

        const std::set< Entity > &all() const;
};
