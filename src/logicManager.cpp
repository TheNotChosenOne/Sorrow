#include "logicManager.h"

#include <SDL2/SDL.h>

#include "structmember.h"
#include <algorithm>
#include <utility>
#include <stdio.h>
#include <chrono>

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

static PyObject *getGlobalFunc(const std::string &name, bool required=true) {
    PyObject *mods = PyImport_GetModuleDict();
    PyObject *mainString = Py_BuildValue("s", "__main__");
    PyObject *funcString = Py_BuildValue("s", name.c_str());
    PyObject *main = PyDict_GetItem(mods, mainString);
    const bool has = PyObject_HasAttr(main, funcString);
    rassert(!required || has, "Script must define function: ", name);
    if (!required && !has) { return nullptr; }
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

static void pyPerfInit() {
    PyRun_SimpleString(""
        "import cProfile, pstats, io\n"
        "def _profileStart():\n"
        "\tpr = cProfile.Profile()\n"
        "\tpr.enable()\n"
        "\treturn pr\n"
        ""
        "def _profileStop(pr, sortby):\n"
        "\tpr.disable()\n"
        "\ts = io.StringIO()\n"
        "\tps = pstats.Stats(pr, stream=s).sort_stats(*sortby)\n"
        "\tps.print_stats('{method.*objects}|src/script|sorrow', 1.)\n"
        "\tprint(s.getvalue())\n");
}

static void pyPerfStart() {
    PyRun_SimpleString("pr = _profileStart()");
}

static void pyPerfStop() {
    PyRun_SimpleString("_profileStop(pr, ('cumulative',))");
}

static void safeCall(PyObject *func, PyObject *args) {
    PyObject *result = PyObject_CallObject(func, args);
    if (!result) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            std::cout << "\nKeyboard interrupt caught\n";
            exit(1);
        }
        PyErr_Print();
    }
    Py_XDECREF(result);
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



LogicManager::LogicManager(const boost::program_options::variables_map &opts)
    : perfTimer(opts["pyperf"].as< double >()) {
    perfActive = false;
    pyCore = nullptr;
    Py_Initialize();
    const char *filename = "src/script.py";
    FILE *fp = fopen(filename, "r");
    rassert(fp, "Failed to open: ", filename);
    PyRun_SimpleFile(fp, filename);

    PyTypesInit();
}

void LogicManager::setup(Core &core) {
    this->core = &core;
    pyCore = toPython(core);
    Py_INCREF(pyCore);
    PyObject *func = getGlobalFunc("setup");
    PyObject *args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, pyCore);
    safeCall(func, args);
    Py_DECREF(args);
    Py_DECREF(func);

    if (!core.options["pyperf"].defaulted()) {
        pyPerfInit();
    }
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
    for (auto &p : groups) {
        p.second.clear();
    }
    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i]->has("group")) {
            const std::string &g = components[i]->getString("group");
            groups[g].insert(i);
        }
    }

    PyObject *lifeString = toPython("lifetime");
    PyObject *deathString = toPython("onDeath");
    PyObject *args = PyTuple_New(2);
    Py_INCREF(pyCore);
    PyTuple_SetItem(args, 0, pyCore);

    if (!perfActive && !core.options["pyperf"].defaulted()) {
        pyPerfStart();
        perfActive = true;
    }
    const auto startTime = std::chrono::high_resolution_clock::now();

    for (const auto &group : groups) {
        const std::string &name = group.first;
        PyObject *func = getGlobalFunc("control_pre_" + name, false);
        if (func) {
            PyObject *listy = PyList_New(group.second.size());
            size_t i = 0;
            for (const auto x : group.second) {
                EntityHandle &eh = core.entities.getHandleFromLow(x);
                PyList_SetItem(listy, i++, toPython(eh));
            }
            PyTuple_SetItem(args, 1, listy);
            safeCall(func, args);
        }
    }

    for (size_t i = 0; i < components.size(); ++i) {
        if (components[i]->has("controller")) {
            const std::string &c = components[i]->getString("controller");
            auto loc = controlFuncs.find(c);
            if (controlFuncs.end() == loc) {
                loc = controlFuncs.insert(std::make_pair(c, getControlFunc(c))).first;
            }
            EntityHandle &eh = core.entities.getHandleFromLow(i);
            PyTuple_SetItem(args, 1, toPython(eh));
            safeCall(loc->second, args);
        }
    }
    for (size_t i = 0; i < components.size(); ++i) {
        PyObject *dict = components[i]->dict;
        if (PyDict_Contains(dict, lifeString)) {
            double x = fromPython< double >(PyDict_GetItem(dict, lifeString));
            x = std::max(0.0, x - PHYSICS_TIMESTEP);
            PyObject *o = toPython(x);
            PyDict_SetItem(dict, lifeString, o);
            Py_DECREF(o);
            if (0.0 == x) {
                size_t id = core.entities.getHandleFromLow(i)->id();
                core.entities.kill(id);
            }
        }
        if (PyDict_Contains(dict, deathString)) {
            EntityHandle &handle = core.entities.getHandleFromLow(i);
            if (handle->dying()) {
                PyTuple_SetItem(args, 1, toPython(handle));
                safeCall(PyDict_GetItem(dict, deathString), args);
            }
        }
    }

    for (const auto &group : groups) {
        const std::string &name = group.first;
        PyObject *func = getGlobalFunc("control_post_" + name, false);
        if (func) {
            PyObject *listy = PyList_New(group.second.size());
            size_t i = 0;
            for (const auto x : group.second) {
                EntityHandle &eh = core.entities.getHandleFromLow(x);
                PyList_SetItem(listy, i++, toPython(eh));
            }
            PyTuple_SetItem(args, 1, listy);
            safeCall(func, args);
        }
    }

    for (const auto &p : groups) {
        if (p.second.empty()) {
            const std::string &name = p.first;
            PyObject *func = getGlobalFunc("control_death_" + name, false);
            if (func) {
                PyTuple_SetItem(args, 1, toPython(name));
                safeCall(func, args);
            }
            groups.erase(p.first);
        }
    }

    Py_DECREF(deathString);
    Py_DECREF(lifeString);
    Py_DECREF(args);

    const auto stopTime = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration< double >(stopTime - startTime);
    if (perfActive && perfTimer.tick(duration)) {
        pyPerfStop();
        perfActive = false;
    }
}

const std::set< size_t > &LogicManager::getGroup(const std::string &name) const {
    return groups.at(name);
}

namespace {

struct PyLogicManager {
    PyObject_HEAD
    LogicManager *lm;
};

static PyObject *Py_get(PyLogicManager *self, PyObject *args) {
    return toPython(self->lm->get(fromPython< int64_t >(PyTuple_GetItem(args, 0))));
}

static PyObject *Py_getGroup(PyLogicManager *self, PyObject *args) {
    std::string name;
    fromPython(name, PyTuple_GetItem(args, 0));
    const auto &in = self->lm->getGroup(name);
    PyObject *listy = PyList_New(in.size());
    size_t i = 0;
    Core &core = *self->lm->core;
    for (const auto &x : in) {
        EntityHandle eh = core.entities.getHandleFromLow(x);
        PyList_SetItem(listy, i++, toPython(eh));
    }
    return listy;
}

static PyMethodDef lmMethods[] = {
    { "get", reinterpret_cast< PyCFunction >(Py_get), READONLY, "Get logic component" },
    { "getGroup", reinterpret_cast< PyCFunction >(Py_getGroup), READONLY, "Get logic group" },
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
