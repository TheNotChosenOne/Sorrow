#include "physicsManager.h"
#include "visualManager.h"
#include "entityManager.h"
#include "renderer.h"

#include <gmtl/MatrixOps.h>
#include <gmtl/CoordOps.h>
#include <gmtl/Generate.h>
#include <gmtl/Matrix.h>
#include <gmtl/Xforms.h>
#include <gmtl/Coord.h>

#include <Python.h>
#include <structmember.h>

#include <algorithm>
#include <vector>

namespace {

struct MinVis {
    Vec3 col;
    Vec pos;
    Vec rad;
    double alpha;
    double depth;
};

typedef std::vector< std::vector< MinVis > > DrawLists;

static void gather(DrawLists &dl, const VisualManager::Components &comps,
                   const Core &core, const gmtl::Matrix33d view) {
    dl.clear();
    dl.resize(3);
    for (auto &l : dl) { l.reserve(comps.size()); }

    for (size_t i = 0; i < comps.size(); ++i) {
        const auto &phys = core.physics.get(i);
        const auto &vis = comps[i];
        const size_t which = static_cast< size_t >(phys.shape);
        dl[vis.draw * (which + 1)].push_back({
            vis.colour, view * gmtl::Point2d(phys.pos), view * phys.rad, 1.0 - vis.transparency, vis.depth
        });
    }
}

};

gmtl::Matrix33d VisualManager::getViewMatrix(Renderer &rend) const {
    Vec scaler = Vec(rend.getWidth(), rend.getHeight());
    scaler[0] /= FOV[0];
    scaler[1] /= FOV[1];

    gmtl::Matrix33d viewMat;
    gmtl::identity(viewMat);
    gmtl::setScale(viewMat, scaler);
    gmtl::setTrans(viewMat, -(viewMat * cam));
    return viewMat;
}

void VisualManager::updater(Core &core, const Components &lasts, Components &next) {
    next = lasts;
    static std::vector< std::vector< MinVis > > shaped;
    const auto viewMat = getViewMatrix(core.renderer);
    gather(shaped, next, core, viewMat);
    for (const MinVis &mv : shaped[1]) { // Boxes
        core.renderer.drawBox(mv.pos, mv.rad, mv.col, mv.alpha, mv.depth);
    }
    for (const MinVis &mv : shaped[2]) { // Circles
        core.renderer.drawCircle(mv.pos, mv.rad, mv.col, mv.alpha, mv.depth);
    }
    core.renderer.update();
    core.renderer.clear();
}

VisualManager::VisualManager()
    : cam(0, 0)
    , FOV(1024, 1024) {
}

void VisualManager::visualUpdate(Core &core) {
    ComponentManager::update([this, &core](const Components &l, Components &r) {
        updater(core, l, r);
    });
}

Vec VisualManager::screenToWorld(const Vec v) const {
    const Vec scaled(FOV[0] * v[0], FOV[1] * v[1]);
    return cam + scaled;
}

namespace {

struct PyVisualManager {
    PyObject_HEAD
    VisualManager *visMan;
};

static PyObject *Py_screenToWorld(PyVisualManager *self, PyObject *args) {
    const Vec v = fromPython< Vec >(PyTuple_GetItem(args, 0));
    return toPython(self->visMan->screenToWorld(v));
}

static PyObject *Py_getCam(PyVisualManager *self, PyObject *) {
    return toPython(self->visMan->cam);
}

static PyObject *Py_setCam(PyVisualManager *self, PyObject *args) {
    fromPython(self->visMan->cam, PyTuple_GetItem(args, 0));
    Py_RETURN_NONE;
}

static PyObject *Py_getFOV(PyVisualManager *self, PyObject *) {
    return toPython(self->visMan->FOV);
}

static PyObject *Py_setFOV(PyVisualManager *self, PyObject *args) {
    fromPython(self->visMan->FOV, PyTuple_GetItem(args, 0));
    Py_RETURN_NONE;
}

static PyMethodDef visManMethods[] = {
    { "screenToWorld", reinterpret_cast< PyCFunction >(Py_screenToWorld), READONLY, "Get world pos" },
    { "getCam", reinterpret_cast< PyCFunction >(Py_getCam), READONLY, "Get camera" },
    { "setCam", reinterpret_cast< PyCFunction >(Py_setCam), READONLY, "Set camera" },
    { "getFOV", reinterpret_cast< PyCFunction >(Py_getFOV), READONLY, "Get FOV" },
    { "setFOV", reinterpret_cast< PyCFunction >(Py_setFOV), READONLY, "Set FOV" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject visManType = [](){
    PyTypeObject obj;
    obj.tp_name = "visualManager";
    obj.tp_basicsize = sizeof(PyVisualManager);
    obj.tp_doc = "BEHOLD!";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_methods = visManMethods;
    return obj;
}();

}

RUN_STATIC(addPyTypeInitializer([](){ PyType_Ready(&visManType); }))

template<>
PyObject *toPython< VisualManager >(VisualManager &visMan) {
    PyVisualManager *pvm;
    pvm = reinterpret_cast< PyVisualManager * >(visManType.tp_alloc(&visManType, 0));
    if (pvm) {
        pvm->visMan = &visMan;
    }
    return reinterpret_cast< PyObject * >(pvm);
}
