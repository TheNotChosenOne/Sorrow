#include "logicManager.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <utility>
#include <stdio.h>

#include "physicsManager.h"
#include "entityManager.h"
#include "visualManager.h"
#include "logicManager.h"
#include "utility.h"

namespace {

bool PyHas(PyObject *dict, const std::string &key) {
    PyObject *str = PyUnicode_FromString(key.c_str());
    const bool result = 1 == PyDict_Contains(dict, str);
    Py_DECREF(str);
    return result;
}

};

PythonData::PythonData(): dict(nullptr) {
    dict = PyDict_New();
}

PythonData::~PythonData() {
    Py_DECREF(dict);
}

void PythonData::setBool(const std::string &key, bool val) {
    PyObject *str = PyUnicode_FromString(key.c_str());
    PyObject *v = PyBool_FromLong(val);
    PyDict_SetItem(dict, str, v);
    Py_DECREF(str);
    Py_DECREF(v);
}

void PythonData::setInt(const std::string &key, int64_t val) {
    PyObject *str = PyUnicode_FromString(key.c_str());
    PyObject *v = PyLong_FromLongLong(val);
    PyDict_SetItem(dict, str, v);
    Py_DECREF(str);
    Py_DECREF(v);
}

void PythonData::setDouble(const std::string &key, double val) {
    PyObject *str = PyUnicode_FromString(key.c_str());
    PyObject *v = PyFloat_FromDouble(val);
    PyDict_SetItem(dict, str, v);
    Py_DECREF(str);
    Py_DECREF(v);
}

void PythonData::setString(const std::string &key, const std::string &val) {
    PyObject *str = PyUnicode_FromString(key.c_str());
    PyObject *v = PyUnicode_FromString(val.c_str());
    PyDict_SetItem(dict, str, v);
    Py_DECREF(str);
    Py_DECREF(v);
}

bool PythonData::getBool(const std::string &key) {
    rassert(PyHas(dict, key), key);
    PyObject *str = PyUnicode_FromString(key.c_str());
    const bool b = PyObject_IsTrue(PyDict_GetItem(dict, str));
    Py_DECREF(str);
    return b;
}

int64_t PythonData::getInt(const std::string &key) {
    rassert(PyHas(dict, key), key);
    PyObject *str = PyUnicode_FromString(key.c_str());
    const int64_t i = PyLong_AsLongLong(PyDict_GetItem(dict, str));
    rassert(!PyErr_Occurred())
    Py_DECREF(str);
    return i;
}

double PythonData::getDouble(const std::string &key) {
    rassert(PyHas(dict, key), key);
    PyObject *str = PyUnicode_FromString(key.c_str());
    const double d = PyFloat_AsDouble(PyDict_GetItem(dict, str));
    rassert(!PyErr_Occurred())
    Py_DECREF(str);
    return d;
}

std::string PythonData::getString(const std::string &key) {
    rassert(PyHas(dict, key), key);
    PyObject *str = PyUnicode_FromString(key.c_str());
    Py_ssize_t length;
    const char *data = PyUnicode_AsUTF8AndSize(PyDict_GetItem(dict, str), &length);
    std::string s(data, length - 1);
    rassert(!PyErr_Occurred())
    Py_DECREF(str);
    return s;
}

LogicManager::LogicManager() {
    Py_Initialize();
    const std::string filename = "src/script.py";
    FILE *fp = fopen(filename.c_str(), "r");
    rassert(fp, "Failed to open: ", filename);
    PyRun_SimpleFile(fp, filename.c_str());
    PyObject *mods = PyImport_GetModuleDict();
    PyObject *mainString = Py_BuildValue("s", "__main__");
    PyObject *funcString = Py_BuildValue("s", "control");
    PyObject *main = PyDict_GetItem(mods, mainString);
    controlFunc = PyObject_GetAttr(main, funcString);
    rassert(controlFunc, "Script must define control(obj)");
    rassert(PyCallable_Check(controlFunc), "Script must define control function");
    Py_DECREF(mainString);
    Py_DECREF(funcString);
}

LogicManager::~LogicManager() {
    components.clear();
    nursery.clear();
    Py_DECREF(controlFunc);
    Py_FinalizeEx();
}

void LogicManager::create() {
    nursery.push_back(std::make_unique< PythonData >());
    nursery.back()->setInt("id", components.size() + nursery.size() - 1);
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
        components[pair.second]->setInt("id", pair.second);
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
    const auto &targetPhys = core.physics.get(core.player);
    PyObject *args = PyTuple_New(1);
    for (size_t i = 0; i < components.size(); ++i) {
        PyObject *dict = components[i]->dict;
        if (1 == PyDict_Size(dict)) { continue; }
        //auto &phys = core.physics.get(i);
        Py_INCREF(dict);
        PyTuple_SetItem(args, 0, dict);
        PyObject *result = PyObject_CallObject(controlFunc, args);
        Py_DECREF(result);

        /*
        if (log.count("drone")) {
            Vec diff = targetPhys.pos - phys.pos;
            gmtl::normalize(diff);
            phys.impulse += diff * log["speed"];
            for (const auto &k : phys.contacts) {
                const auto &blog = lasts[k.which];
                double dmg = k.force;
                if (blog.count("dmg")) {
                    dmg += blog.at("dmg");
                }
                log["hp"] = std::max(0.0, log["hp"] - dmg);
                if (0.0 == log["hp"]) {
                    std::cout << "RIP: " << i << '\n';
                    core.visuals.get(i).colour = Vec3(0x77, 0x77, 0x77);
                    log.erase("drone");
                    log["debris"] = 1;
                    log["lifetime"] = std::numeric_limits< double >::infinity();
                    phys.gather = false;
                    break;
                }
            }
        } else if (log.count("bullet")) {
            if (!phys.contacts.empty()) {
                log.erase("bullet");
                log["debris"] = 1;
                phys.gather = false;
            }
        } else if (log.count("debris")) {
            log["lifetime"] = std::max(0.0, log["lifetime"] - PHYSICS_TIMESTEP);
            if (0.0 == log["lifetime"]) {
                core.entities.kill(i);
            }
        } else if (log.count("player")) {
            const double speed = log["speed"];
            if (core.input.isHeld(SDLK_a)) {
                if (phys.vel[0] > -speed) {
                    phys.acc[0] += std::max(-speed, phys.acc[0] - speed);
                }
            }
            if (core.input.isHeld(SDLK_d)) {
                if (phys.vel[0] < speed) {
                    phys.acc[0] += std::min(speed, phys.acc[0] + speed);
                }
            }
            if (core.input.isHeld(SDLK_w)) {
                if (gmtl::dot(Vec(0, 1), phys.surface) > 0.75) {
                    phys.acc[1] += 0.011 * (1 / PHYSICS_TIMESTEP) * speed;
                }
            }
            log["reload"] = std::max(0.0, log["reload"] - PHYSICS_TIMESTEP);
            if (core.input.mouseHeld(SDL_BUTTON_LEFT)) {
                if (0.0 == log["reload"]) {
                    log["reload"] = log["reloadTime"];
                    static const double brad = 0.5;
                    Vec dir = core.visuals.screenToWorld(core.input.mousePos()) - phys.pos;
                    gmtl::normalize(dir);
                    Entity b = core.entities.create();
                    auto &bphys = core.physics.get(b);
                    bphys.pos = phys.pos + dir * (1.0001 * (phys.rad[0] + brad));
                    bphys.rad = { brad, brad };
                    bphys.impulse = dir * log["bulletForce"] * (1.0 / PHYSICS_TIMESTEP);
                    phys.impulse -= bphys.impulse;
                    bphys.area = 2 * brad;
                    bphys.mass = pi< double > * brad * brad / 1.0;
                    bphys.shape = Shape::Circle;
                    bphys.isStatic = false;
                    bphys.elasticity = 0.95;
                    bphys.phased = false;
                    bphys.gather = true;

                    auto &vis = core.visuals.get(b);
                    vis.draw = true;
                    vis.colour = Vec3(0xFF, 0, 0);

                    auto &blog  = core.logic.get(b);
                    blog["dmg"] = 1;
                    blog["bullet"] = 1;
                    blog["lifetime"] = log["blifetime"];
                }
            }
        }
    */
    }
    Py_DECREF(args);
    const double interp = 0.00000001;
    const Vec target = targetPhys.pos - (core.visuals.FOV / 2.0);
    core.visuals.cam = interp * core.visuals.cam + (1.0 - interp) * target;

    graduate();
}
