#include "ai.h"

#include <random>
#include <Python.h>
#include <structmember.h>

#include "utility.h"
#include "mirror.h"
#include "core.h"

#include "physicsManager.h"
#include "logicManager.h"
#include "entityManager.h"

void AI::setCore(Core &core) {
    this->core = &core;
}

std::vector< EntityHandle > AI::findIn(Vec pos, double radius, const std::string &group, bool match) {
    const double rad2 = radius * radius;
    std::vector< EntityHandle > handles;
    if (group.empty()) {
        for (auto &e : core->entities.all()) {
            auto &eh = core->entities.getHandle(e);
            const auto phys = eh->getPhys();
            if (diffLength2(pos, phys.pos) < rad2) {
                handles.push_back(eh);
            }
        }
    } else if (match) {
        for (auto &e : core->entities.all()) {
            auto &eh = core->entities.getHandle(e);
            const auto &phys = eh->getPhys();
            if (diffLength2(pos, phys.pos) < rad2) {
                auto &log = eh->getLog();
                if (log.has("group") && group == log.getString("group")) {
                    handles.push_back(eh);
                }
            }
        }
    } else {
        for (auto &e : core->entities.all()) {
            auto &eh = core->entities.getHandle(e);
            const auto &phys = eh->getPhys();
            if (diffLength2(pos, phys.pos) < rad2) {
                auto &log = eh->getLog();
                if (log.has("group") && group != log.getString("group")) {
                    handles.push_back(eh);
                }
            }
        }
    }
    return handles;
}

namespace {

struct PyAI {
    PyObject_HEAD
    AI *ai;
};

static PyObject *Py_findIn(PyAI *pai, PyObject *args) {
    Vec pos;
    double rad;
    std::string group;
    bool match = true;
    fromPython(pos, PyTuple_GetItem(args, 0));
    fromPython(rad, PyTuple_GetItem(args, 1));
    if (PyTuple_Size(args) > 2) {
        fromPython(group, PyTuple_GetItem(args, 2));
        if (PyTuple_Size(args) > 3) {
            fromPython(match, PyTuple_GetItem(args, 3));
        }
    }
    auto listy = pai->ai->findIn(pos, rad, group, match);
    if (!listy.empty()) {
        static std::mt19937_64 mt;
        size_t index = std::uniform_int_distribution< size_t >(0, listy.size() - 1)(mt);
        std::swap(listy[0], listy[index]);
    }
    return toPython(listy);
}

static PyMethodDef aiMethods[] = {
    { "findIn", reinterpret_cast< PyCFunction >(Py_findIn), READONLY, "Find an entity within" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject aiType = [](){
    PyTypeObject obj;
    obj.tp_name = "ai";
    obj.tp_basicsize = sizeof(PyAI);
    obj.tp_doc = "Shallow Mind";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_methods = aiMethods;
    return obj;
}();

RUN_STATIC(addPyTypeInitializer([](){ PyType_Ready(&aiType); }))

}

template<>
PyObject *toPython< AI >(AI &ai) {
    PyAI *pai;
    pai = reinterpret_cast< PyAI * >(aiType.tp_alloc(&aiType, 0));
    if (pai) {
        pai->ai = &ai;
    }
    return reinterpret_cast< PyObject * >(pai);
}
