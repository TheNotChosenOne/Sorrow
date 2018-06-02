#include "entityManager.h"

#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <Python.h>
#include <structmember.h>

#include "physicsManager.h"
#include "visualManager.h"
#include "logicManager.h"

EntityView::EntityView(EntityManager &entMan, size_t id, bool alive)
    : manager(entMan)
    , ent(id)
    , alive(alive) {
}

PhysicsComponent &EntityView::getPhys() {
    return manager.core->physics.get(manager.idToLow.at(ent));
}

const PhysicsComponent &EntityView::getPhys() const {
    return manager.core->physics.get(manager.idToLow.at(ent));
}

VisualComponent &EntityView::getVis() {
    return manager.core->visuals.get(manager.idToLow.at(ent));
}

const VisualComponent &EntityView::getVis() const {
    return manager.core->visuals.get(manager.idToLow.at(ent));
}

LogicComponent &EntityView::getLog() {
    rassert(manager.entities.count(ent) && manager.idToLow.find(ent) != manager.idToLow.end(), ent);
    return manager.core->logic.get(manager.idToLow.at(ent));
}

const LogicComponent &EntityView::getLog() const {
    return manager.core->logic.get(manager.idToLow.at(ent));
}

Entity EntityView::id() const {
    rassert(manager.entities.count(ent), ent)
    return ent;
}

std::ostream &operator<<(std::ostream &os, const EntityView &ev) {
    return (os << "entity " << ev.ent << '(' << ev.alive << ')');
}

namespace {
struct PyEntityHandle {
    PyObject_HEAD
    EntityHandle eh;
};

static void PyEntityHandle_free(PyEntityHandle *peh) {
    peh->eh.reset();
}

static PyObject *Py_getPhys(PyEntityHandle *peh, PyObject *) {
    return toPython(peh->eh->getPhys());
}

static PyObject *Py_getVis(PyEntityHandle *peh, PyObject *) {
    return toPython(peh->eh->getVis());
}

static PyObject *Py_getLog(PyEntityHandle *peh, PyObject *) {
    return toPython(peh->eh->getLog());
}

static PyMethodDef entityHandleMethods[] = {
    { "getPhys", reinterpret_cast< PyCFunction >(Py_getPhys), READONLY, "" },
    { "getVis", reinterpret_cast< PyCFunction >(Py_getVis), READONLY, "" },
    { "getLog", reinterpret_cast< PyCFunction >(Py_getLog), READONLY, "" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject entityHandleType = [](){
    PyTypeObject obj;
    obj.tp_name = "entityHandle";
    obj.tp_basicsize = sizeof(PyEntityHandle);
    obj.tp_doc = "I've got it handled";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_dealloc = reinterpret_cast< void (*)(PyObject *) >(PyEntityHandle_free);
    obj.tp_methods = entityHandleMethods;
    return obj;
}();

}

template<>
PyObject *toPython< EntityHandle >(EntityHandle &eh) {
    RUN_ONCE(PyType_Ready(&entityHandleType));
    PyEntityHandle *peh;
    peh = reinterpret_cast< PyEntityHandle * >(entityHandleType.tp_alloc(&entityHandleType, 0));
    if (peh) {
        peh->eh = eh;
    }
    return reinterpret_cast< PyObject * >(peh);
}

EntityManager::EntityManager() {
}

EntityManager::~EntityManager() {
}

void EntityManager::setCore(Core &core) {
    this->core = &core;
}

void EntityManager::attach(ManagerPtr manager) {
    managers.insert(std::move(manager));
}

Entity EntityManager::create() {
    const size_t id = nextID++;
    const size_t lowID = entities.size() + graveyard.size();
    for (auto &manPtr : managers) {
        manPtr->create();
    }

    entities.insert(id);
    idToLow[id] = lowID;
    lowToID.push_back(id);

    return id;
}

void EntityManager::kill(Entity e) {
    graveyard.insert(e);
    entities.erase(e);
}

EntityHandle &EntityManager::getHandle(Entity e) {
    rassert(entities.count(e), e);
    EntityHandle &h = handles[e];
    if (!h) {
        h = std::make_shared< EntityView >(*this, e, entities.find(e) != entities.end());
    }
    return h;
}

EntityHandle &EntityManager::getHandleFromLow(size_t e) {
    rassert(e < lowToID.size(), e);
    rassert(entities.count(lowToID[e]), e, lowToID.size(), entities.size());
    return getHandle(lowToID[e]);
}

Entity EntityManager::fromHandle(EntityHandle &e) {
    return e->ent;
}

void EntityManager::update() {
    for (auto &manPtr : managers) {
        manPtr->graduate();
    }
    if (graveyard.empty() || entities.empty()) { return; }
    std::map< size_t, size_t > remap;

    auto targetIter = graveyard.begin();
    for (auto move : entities) {
        size_t id = move;
        if (idToLow[id] < entities.size()) { continue; }
        size_t deadID = *targetIter++;

        size_t lowID = idToLow[id];
        size_t deadLow = idToLow[deadID];

        remap[lowID] = deadLow;
        idToLow[id] = deadLow;
        lowToID[deadLow] = id;
    }
    lowToID.resize(entities.size());
    for (auto &manPtr : managers) {
        manPtr->reorder(remap);
        manPtr->cull(graveyard.size());
    }
    for (const auto e : graveyard) {
        const auto iloc = idToLow.find(e);
        rassert(idToLow.end() != iloc, idToLow, e);
        idToLow.erase(iloc);
        auto loc = handles.find(e);
        loc->second->alive = false;
        handles.erase(loc);
    }
    graveyard.clear();

}

const std::set< Entity > &EntityManager::all() const {
    return entities;
}

namespace {

struct PyEntityManager {
    PyObject_HEAD
    EntityManager *entityMan;
};

static PyObject *Py_create(PyEntityManager *pem, PyObject *) {
    EntityManager &em = *pem->entityMan;
    Entity id = em.create();
    EntityHandle &eh = em.getHandle(id);
    return toPython(eh);
}

static PyObject *Py_kill(PyEntityManager *pem, PyObject *args) {
    PyObject *pwhich = PyTuple_GetItem(args, 0);
    rassert(pwhich->ob_type == &entityHandleType, "Wrong type");
    PyEntityHandle *peh = reinterpret_cast< PyEntityHandle * >(pwhich);
    pem->entityMan->kill(peh->eh->id());
    Py_RETURN_NONE;
}

static PyObject *Py_getHandle(PyEntityManager *pem, PyObject *args) {
    return toPython(pem->entityMan->getHandle(fromPython< int64_t >(PyTuple_GetItem(args, 0))));
}

static PyMethodDef entityManagerMethods[] = {
    { "create", reinterpret_cast< PyCFunction >(Py_create), READONLY, "Make a new entity" },
    { "kill", reinterpret_cast< PyCFunction >(Py_kill), READONLY, "Destroy an entity" },
    { "getHandle", reinterpret_cast< PyCFunction >(Py_getHandle), READONLY, "Get entity handle" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject entityManagerType = [](){
    PyTypeObject obj;
    obj.tp_name = "entityManager";
    obj.tp_basicsize = sizeof(PyEntityManager);
    obj.tp_doc = "Power Tripping!";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_methods = entityManagerMethods;
    return obj;
}();

}
template<>
PyObject *toPython< EntityManager >(EntityManager &entMan) {
    RUN_ONCE(PyType_Ready(&entityManagerType));
    PyEntityManager *pin;
    pin = reinterpret_cast< PyEntityManager * >(entityManagerType.tp_alloc(&entityManagerType, 0));
    if (pin) {
        pin->entityMan = &entMan;
    }
    return reinterpret_cast< PyObject * >(pin);
}
