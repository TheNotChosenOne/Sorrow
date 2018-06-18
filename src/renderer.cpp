#include "renderer.h"

#include <Python.h>
#include <structmember.h>

#include "mirror.h"

Renderer::Renderer(size_t width, size_t height): width(width), height(height) {
}

Renderer::~Renderer() {
}

void Renderer::clear() {
}

void Renderer::update() {
}

size_t Renderer::getWidth() const {
    return width;
}

size_t Renderer::getHeight() const {
    return height;
}

void Renderer::drawPoint(Vec, Vec3, double depth) {
}

void Renderer::drawBox(Vec, Vec, Vec3, double depth) {
}

void Renderer::drawCircle(Vec, Vec, Vec3, double depth) {
}

namespace {

struct PyRenderer {
    PyObject_HEAD
    Renderer *renderer;
};

static PyObject *Py_getWidth(PyRenderer *self, PyObject *) {
    const size_t width = self->renderer->getWidth();
    return toPython(width);
}

static PyObject *Py_getHeight(PyRenderer *self, PyObject *) {
    const size_t height = self->renderer->getHeight();
    return toPython(height);
}

static PyObject *Py_drawPoint(PyRenderer *self, PyObject *args) {
    PyObject *pos, *col;
    if (!PyArg_ParseTuple(args, "OO", &pos, &col)) { return nullptr; }
    self->renderer->drawPoint(fromPython< Vec >(pos), fromPython< Vec3 >(col));
    Py_RETURN_NONE;
}

static PyObject *Py_drawBox(PyRenderer *self, PyObject *args) {
    PyObject *pos, *rad, *col;
    if (!PyArg_ParseTuple(args, "NNN", &pos, &rad, &col)) { return nullptr; }
    self->renderer->drawBox(fromPython< Vec >(pos),
                            fromPython< Vec >(rad),
                            fromPython< Vec3 >(col));
    Py_RETURN_NONE;
}

static PyObject *Py_drawCircle(PyRenderer *self, PyObject *args) {
    PyObject *pos, *rad, *col;
    if (!PyArg_ParseTuple(args, "NNN", &pos, &rad, &col)) { return nullptr; }
    self->renderer->drawCircle(fromPython< Vec >(pos),
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

static PyTypeObject rendererType = [](){
    PyTypeObject obj;
    obj.tp_name = "renderer";
    obj.tp_basicsize = sizeof(PyRenderer);
    obj.tp_doc = "What do you think the artist was trying to say when they chose this colour?";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_methods = inputMethods;
    return obj;
}();

}

RUN_STATIC(addPyTypeInitializer([](){ PyType_Ready(&rendererType); }))

template<>
PyObject *toPython< Renderer >(Renderer &rend) {
    PyRenderer *pin;
    pin = reinterpret_cast< PyRenderer * >(rendererType.tp_alloc(&rendererType, 0));
    if (pin) {
        pin->renderer = &rend;
    }
    return reinterpret_cast< PyObject * >(pin);
}
