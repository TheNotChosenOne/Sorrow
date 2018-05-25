#include "input.h"

#include <Python.h>
#include <structmember.h>

Input::Input() {
}

Input::~Input() {
}

void Input::update() {
}

bool Input::shouldQuit() const {
    return false;
}

bool Input::isPressed(size_t) const {
    return false;
}

bool Input::isHeld(size_t) const {
    return false;
}

bool Input::isReleased(size_t) const {
    return false;
}

bool Input::mouseHeld(uint8_t) const {
    return false;
}

bool Input::mousePressed(uint8_t) const {
    return false;
}

bool Input::mouseReleased(uint8_t) const {
    return false;
}

Vec Input::mousePos() const {
    return Vec();
}

namespace {

struct PyInput {
    PyObject_HEAD
    Input *input;
};

static PyObject *pyIsHeld(PyInput *self, PyObject *args) {
    size_t key;

    self->input->
}
static PyObject *pyIsPressed(PyInput *self, PyObject *args) {
}
static PyObject *pyIsReleased(PyInput *self, PyObject *args) {
}
static PyObject *pyMouseHeld(PyInput *self, PyObject *args) {
}
static PyObject *pyMousePressed(PyInput *self, PyObject *args) {
}
static PyObject *pyMouseReleased(PyInput *self, PyObject *args) {
}

static PyMethodDef inputMethods[] = {
    { "isHeld", reinterpret_cast< PyCFunction >(pyIsHeld), READONLY, "Is the key held" },
    { "isHeld", reinterpret_cast< PyCFunction >(pyIsPressed), READONLY, "Is the key held" },
    { "isHeld", reinterpret_cast< PyCFunction >(pyIsReleased), READONLY, "Is the key held" },
    { "isHeld", reinterpret_cast< PyCFunction >(pyMouseHeld), READONLY, "Is the key held" },
    { "isHeld", reinterpret_cast< PyCFunction >(pyMousePressed), READONLY, "Is the key held" },
    { "isHeld", reinterpret_cast< PyCFunction >(pyMouseReleased), READONLY, "Is the key held" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject inputType {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "input",
    .tp_basicsize = sizeof(PyInput),
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
    .tp_doc = "How do you know your senses aren't lying to you?",
    .tp_traverse = nullptr,
    .tp_clear = nullptr,
    .tp_richcompare = nullptr,
    .tp_weaklistoffset = 0,
    .tp_iter = 0,
    .tp_iternext = 0,
    .tp_methods = 0,
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
PyObject *toPython< Input >(Input &t) {
    

}
