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

void Renderer::drawPoint(Vec, Vec3) {
}

void Renderer::drawBox(Vec, Vec, Vec3) {
}

void Renderer::drawCircle(Vec, Vec, Vec3) {
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

static PyTypeObject rendererType {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "renderer",
    .tp_basicsize = sizeof(PyRenderer),
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
PyObject *toPython< Renderer >(Renderer &rend) {
    RUN_ONCE(PyType_Ready(&rendererType));
    PyRenderer *pin;
    pin = reinterpret_cast< PyRenderer * >(rendererType.tp_alloc(&rendererType, 0));
    if (pin) {
        pin->renderer = &rend;
    }
    return reinterpret_cast< PyObject * >(pin);
}
