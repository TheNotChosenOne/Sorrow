#include "entityManager.h"

#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <Python.h>
#include <structmember.h>

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

/*
namespace {

struct PyEntityManager {
    PyObject_HEAD
    EntityManager *entityMan;
};

static PyObject *Py_getWidth(PyEntityManager *self, PyObject *) {
    const size_t width = self->entityMan->getWidth();
    return toPython(width);
}

static PyObject *Py_getHeight(PyEntityManager *self, PyObject *) {
    const size_t height = self->entityMan->getHeight();
    return toPython(height);
}

static PyObject *Py_drawPoint(PyEntityManager *self, PyObject *args) {
    PyObject *pos, *col;
    if (!PyArg_ParseTuple(args, "OO", &pos, &col)) { return nullptr; }
    self->entityMan->drawPoint(fromPython< Vec >(pos), fromPython< Vec3 >(col));
    Py_RETURN_NONE;
}

static PyObject *Py_drawBox(PyEntityManager *self, PyObject *args) {
    PyObject *pos, *rad, *col;
    if (!PyArg_ParseTuple(args, "NNN", &pos, &rad, &col)) { return nullptr; }
    self->entityMan->drawBox(fromPython< Vec >(pos),
                            fromPython< Vec >(rad),
                            fromPython< Vec3 >(col));
    Py_RETURN_NONE;
}

static PyObject *Py_drawCircle(PyEntityManager *self, PyObject *args) {
    PyObject *pos, *rad, *col;
    if (!PyArg_ParseTuple(args, "NNN", &pos, &rad, &col)) { return nullptr; }
    self->entityMan->drawCircle(fromPython< Vec >(pos),
                               fromPython< Vec >(rad),
                               fromPython< Vec3 >(col));
    Py_RETURN_NONE;
}

static PyMethodDef inputMethods[] = {
    { "getWidth", reinterpret_cast< PyCFunction >(Py_getWidth), READONLY, "Screen width" },
    { "getHeight", reinterpret_cast< PyCFunction >(Py_getHeight), READONLY, "Screen height" },
    { "drawPoint", reinterpret_cast< PyCFunction >(Py_drawPoint), READONLY, "Draw a point" },
    { "drawBox", reinterpret_cast< PyCFunction >(Py_drawBox), READONLY, "Draw a box" },
    { "drawCircle", reinterpret_cast< PyCFunction >(Py_drawCircle), READONLY, "Draw a circle" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject entityManagerType {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "entity_manager",
    .tp_basicsize = sizeof(PyEntityManager),
    .tp_itemsize = 0,
    .tp_dealloc = nullptr,
    .tp_print = nullptr,
    .tp_getattr = nullptr,
    .tp_setattr = nullptr,
    .tp_as_async = nullptr,
    .tp_repr = nullptr,
    .tp_as_number = nullptr,
    .tp_as_sequence = nullptr,
    .tp_as_mapping = nullptr,
    .tp_hash = nullptr,
    .tp_call = nullptr,
    .tp_str = nullptr,
    .tp_getattro = nullptr,
    .tp_setattro = nullptr,
    .tp_as_buffer = nullptr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "What do you think the artist was trying to say when they chose this colour?",
    .tp_traverse = nullptr,
    .tp_clear = nullptr,
    .tp_richcompare = nullptr,
    .tp_weaklistoffset = 0,
    .tp_iter = 0,
    .tp_iternext = 0,
    .tp_methods = inputMethods,
    .tp_members = 0,
    .tp_getset = 0,
    .tp_base = 0,
    .tp_dict = 0,
    .tp_descr_get = 0,
    .tp_descr_set = 0,
    .tp_dictoffset = 0,
    .tp_init = 0,
    .tp_alloc = 0,
    .tp_new = nullptr,
    .tp_free = nullptr,
    .tp_is_gc = nullptr,
    .tp_bases = nullptr,
    .tp_mro = nullptr,
    .tp_cache = nullptr,
    .tp_subclasses = nullptr,
    .tp_weaklist = nullptr,
    .tp_del = nullptr,
    .tp_version_tag = 0,
    .tp_finalize = nullptr,
};

}
template<>
PyObject *toPython< EntityManager >(EntityManager &rend) {
    RUN_ONCE(PyType_Ready(&entityManagerType));
    PyEntityManager *pin;
    pin = reinterpret_cast< PyEntityManager * >(entityManagerType.tp_alloc(&entityManagerType, 0));
    if (pin) {
        pin->entityMan = &rend;
    }
    return reinterpret_cast< PyObject * >(pin);
}
*/
