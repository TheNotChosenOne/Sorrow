#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <map>

#include "componentManager.h"
#include "forwardMirror.h"
#include "core.h"

// An id unique during the entire execution of the engine
typedef size_t Entity;
class BaseComponentManager;

struct PhysicsComponent;
struct VisualComponent;
class PythonData;
typedef PythonData LogicComponent;
class EntityView {
    private:
        EntityManager &manager;
        size_t ent;
        bool alive;

    public:
        EntityView(EntityManager &entMan, size_t id, bool alive);
        PhysicsComponent &getPhys();
        const PhysicsComponent &getPhys() const;
        VisualComponent &getVis();
        const VisualComponent &getVis() const;
        LogicComponent &getLog();
        const LogicComponent &getLog() const;
        Entity id() const;
        bool isAlive() const;
        bool dying() const;

    friend class EntityManager;
    friend std::ostream &operator<<(std::ostream &os, const EntityView &ev);
};
typedef std::shared_ptr< EntityView > EntityHandle;

template<>
PyObject *toPython< EntityHandle >(EntityHandle &eh);

std::ostream &operator<<(std::ostream &os, const EntityView &ev);

typedef std::unique_ptr< BaseComponentManager > ManagerPtr;

class EntityManager {
    private:
        std::map< size_t, EntityHandle > handles;
        std::set< Entity > entities;
        std::set< Entity > graveyard;
        std::set< ManagerPtr > managers;
        std::vector< size_t > lowToID;
        std::map< size_t, size_t > idToLow;
        Core *core;
        size_t nextID = 0;

    public:
        EntityManager();
        ~EntityManager();

        void setCore(Core &core);
        void attach(ManagerPtr manager);
        EntityHandle &getHandleFromLow(size_t low);

        Entity create();
        void kill(Entity e);
        size_t getLowFromID(Entity e) const;
        Entity getIDFromLow(size_t i) const;
        EntityHandle &getHandle(Entity id);
        Entity fromHandle(EntityHandle &e);

        // Used to clear out killed entities
        void update();

        const std::set< Entity > &all() const;

    friend class EntityView;
};

template<>
PyObject *toPython< EntityManager >(EntityManager &ent);
