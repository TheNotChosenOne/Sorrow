#include "logicManager.h"

#include <SDL2/SDL.h>

#include "structmember.h"
#include <algorithm>
#include <utility>
#include <stdio.h>

#include "physicsManager.h"
#include "entityManager.h"
#include "visualManager.h"
#include "logicManager.h"
#include "utility.h"
#include "mirror.h"
#include "renderer.h"

namespace {

static bool PyHas(PyObject *dict, const std::string &key) {
    PyObject *str = PyUnicode_FromString(key.c_str());
    const bool result = 1 == PyDict_Contains(dict, str);
    Py_DECREF(str);
    return result;
}

static PyObject *getGlobalFunc(const std::string &name) {
    PyObject *mods = PyImport_GetModuleDict();
    PyObject *mainString = Py_BuildValue("s", "__main__");
    PyObject *funcString = Py_BuildValue("s", name.c_str());
    PyObject *main = PyDict_GetItem(mods, mainString);
    PyObject *func = PyObject_GetAttr(main, funcString);
    rassert(func, "Script must define function: ", name);
    rassert(PyCallable_Check(func), "Function must be callable: ", name);
    Py_DECREF(mainString);
    Py_DECREF(funcString);
    return func;
}

static PyObject *getControlFunc(const std::string &controllerName) {
    return getGlobalFunc("control_" + controllerName);
}

};

PythonData::PythonData(): dict(nullptr) {
    dict = PyDict_New();
}

PythonData::~PythonData() {
    Py_DECREF(dict);
}

void PythonData::setBool(const std::string &key, bool val) {
    PyObject *v = PyBool_FromLong(val);
    PyDict_SetItemString(dict, key.c_str(), v);
    Py_DECREF(v);
}

void PythonData::setInt(const std::string &key, int64_t val) {
    PyObject *v = PyLong_FromLongLong(val);
    PyDict_SetItemString(dict, key.c_str(), v);
    Py_DECREF(v);
}

void PythonData::setDouble(const std::string &key, double val) {
    PyObject *v = PyFloat_FromDouble(val);
    PyDict_SetItemString(dict, key.c_str(), v);
    Py_DECREF(v);
}

void PythonData::setString(const std::string &key, const std::string &val) {
    PyObject *v = PyUnicode_FromString(val.c_str());
    PyDict_SetItemString(dict, key.c_str(), v);
    Py_DECREF(v);
}

bool PythonData::getBool(const std::string &key) {
    rassert(PyHas(dict, key), key);
    const bool b = PyObject_IsTrue(PyDict_GetItemString(dict, key.c_str()));
    return b;
}

int64_t PythonData::getInt(const std::string &key) {
    rassert(PyHas(dict, key), key);
    const int64_t i = PyLong_AsLongLong(PyDict_GetItemString(dict, key.c_str()));
    return i;
}

double PythonData::getDouble(const std::string &key) {
    rassert(PyHas(dict, key), key);
    const double d = PyFloat_AsDouble(PyDict_GetItemString(dict, key.c_str()));
    return d;
}

std::string PythonData::getString(const std::string &key) {
    rassert(PyHas(dict, key), key);
    Py_ssize_t length;
    const char *data = PyUnicode_AsUTF8AndSize(PyDict_GetItemString(dict, key.c_str()), &length);
    std::string s(data, length);
    return s;
}

bool PythonData::has(const std::string &key) {
    return PyHas(dict, key);
}

template<>
PyObject *toPython< PythonData >(PythonData &p) {
    Py_INCREF(p.dict);
    return p.dict;
}

std::ostream &operator<<(std::ostream &os, const PythonData &pd) {
    os << fromPython< std::string >(PyObject_Str(pd.dict));
    return os;
}

LogicManager::LogicManager() {
    pyCore = nullptr;
    Py_Initialize();
    const char *filename = "src/script.py";
    FILE *fp = fopen(filename, "r");
    rassert(fp, "Failed to open: ", filename);
    PyRun_SimpleFile(fp, filename);

    PyTypesInit();
}

void LogicManager::setup(Core &core) {
    pyCore = toPython(core);
    Py_INCREF(pyCore);
    PyObject *func = getGlobalFunc("setup");
    PyObject *args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, pyCore);
    PyObject *result = PyObject_CallObject(func, args);
    if (!result) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            std::cout << "\nKeyboard interrupt caught\n";
            exit(1);
        }
        PyErr_Print();
    }
    Py_XDECREF(result);
    Py_DECREF(args);
    Py_DECREF(func);
}

LogicManager::~LogicManager() {
    components.clear();
    nursery.clear();
    for (auto &p : controlFuncs) {
        Py_DECREF(p.second);
    }
    Py_XDECREF(pyCore);
    Py_FinalizeEx();
}

void LogicManager::create() {
    nursery.push_back(std::make_unique< PythonData >());
}

void LogicManager::graduate() {
    components.reserve(components.size() + nursery.size());
    for (auto &ptr : nursery) {
        components.push_back(std::move(ptr));
    }
    nursery.clear();
}

void LogicManager::reorder(const std::map< size_t, size_t > &remap) {
    for (const auto &pair : remap) {
        components[pair.second] = std::move(components[pair.first]);
    }
}

void LogicManager::cull(size_t count) {
    components.resize(components.size() - count);
}

PythonData &LogicManager::get(Entity e) {
    if (components.size() <= e) {
        return *nursery[e - components.size()];
    }
    return *components[e];
}

const PythonData &LogicManager::get(Entity e) const {
    if (components.size() <= e) {
        return *nursery[e - components.size()];
    }
    return *components[e];
}

void LogicManager::logicUpdate(Core &core) {
    PyObject *lifeString = toPython("lifetime");
    PyObject *deathString = toPython("onDeath");
    PyObject *args = PyTuple_New(2);
    Py_INCREF(pyCore);
    PyTuple_SetItem(args, 0, pyCore);
    for (size_t i = 0; i < components.size(); ++i) {
        PyObject *dict = components[i]->dict;
        if (components[i]->has("controller")) {
            const std::string &c = components[i]->getString("controller");
            auto loc = controlFuncs.find(c);
            if (controlFuncs.end() == loc) {
                loc = controlFuncs.insert(std::make_pair(c, getControlFunc(c))).first;
            }
            EntityHandle &eh = core.entities.getHandleFromLow(i);
            PyTuple_SetItem(args, 1, toPython(eh));
            PyObject *result = PyObject_CallObject(loc->second, args);
            if (!result) {
                if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
                    std::cout << "\nKeyboard interrupt caught\n";
                    exit(1);
                }
                PyErr_Print();
            }
            Py_XDECREF(result);
        }
        if (PyDict_Contains(dict, lifeString)) {
            double x = fromPython< double >(PyDict_GetItem(dict, lifeString));
            x = std::max(0.0, x - PHYSICS_TIMESTEP);
            PyDict_SetItem(dict, lifeString, toPython(x));
            if (0.0 == x) {
                size_t id = core.entities.getHandleFromLow(i)->id();
                core.entities.kill(id);
            }
        }
    }
    for (size_t i = 0; i < components.size(); ++i) {
        PyObject *dict = components[i]->dict;
        if (PyDict_Contains(dict, deathString)) {
            EntityHandle &handle = core.entities.getHandleFromLow(i);
            if (handle->dying()) {
                PyTuple_SetItem(args, 1, toPython(handle));
                PyObject *result = PyObject_CallObject(PyDict_GetItem(dict, deathString), args);
                if (!result) {
                    if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
                        std::cout << "\nKeyboard interrupt caught\n";
                        exit(1);
                    }
                    PyErr_Print();
                }
                Py_XDECREF(result);
            }
        }
    }
    Py_DECREF(deathString);
    Py_DECREF(lifeString);
    Py_DECREF(args);
    const double interp = 0.00000001;
    const Vec target = core.player->getPhys().pos - (core.visuals.FOV / 2.0);
    core.visuals.cam = interp * core.visuals.cam + (1.0 - interp) * target;
}

namespace {

struct PyLogicManager {
    PyObject_HEAD
    LogicManager *lm;
};

static PyObject *Py_get(PyLogicManager *self, PyObject *args) {
    return toPython(self->lm->get(fromPython< int64_t >(PyTuple_GetItem(args, 0))));
}

static PyMethodDef lmMethods[] = {
    { "get", reinterpret_cast< PyCFunction >(Py_get), READONLY, "Get logic component" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject lmType = [](){
    PyTypeObject obj;
    obj.tp_name = "logicManager";
    obj.tp_basicsize = sizeof(PyLogicManager);
    obj.tp_doc = "Technically correct";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_methods = lmMethods;
    return obj;
}();

}

RUN_STATIC(addPyTypeInitializer([](){ PyType_Ready(&lmType); }))

template<>
PyObject *toPython< LogicManager >(LogicManager &lm) {
    PyLogicManager *plm;
    plm = reinterpret_cast< PyLogicManager * >(lmType.tp_alloc(&lmType, 0));
    if (plm) {
        plm->lm = &lm;
    }
    return reinterpret_cast< PyObject * >(plm);
}
