#include "input.h"

#include <Python.h>
#include <functional>
#include <structmember.h>

#include "mirror.h"

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

bool Input::mouseHeld(size_t) const {
    return false;
}

bool Input::mousePressed(size_t) const {
    return false;
}

bool Input::mouseReleased(size_t) const {
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

#define LinkPythonInput(func) \
    static PyObject *Py_ ## func (PyInput *self, PyObject *args) {\
        size_t key; \
        if (PyArg_ParseTuple(args, "K", &key)) { \
            Py_RETURN_BOOL(self->input->func (key)); \
        } \
        return nullptr; \
    }

LinkPythonInput(isHeld);
LinkPythonInput(isPressed);
LinkPythonInput(isReleased);
LinkPythonInput(mouseHeld);
LinkPythonInput(mousePressed);
LinkPythonInput(mouseReleased);

static PyObject *Py_mousePos(PyInput *self, PyObject *) {
    return toPython< Vec >(self->input->mousePos());
}

static PyMethodDef inputMethods[] = {
    { "isHeld", reinterpret_cast< PyCFunction >(Py_isHeld), READONLY, "Is the key held" },
    { "isPressed", reinterpret_cast< PyCFunction >(Py_isPressed), READONLY, "Was the key pressed" },
    { "isReleased", reinterpret_cast< PyCFunction >(Py_isReleased), READONLY, "Was the key released" },
    { "mouseHeld", reinterpret_cast< PyCFunction >(Py_mouseHeld), READONLY, "Is the button held" },
    { "mousePressed", reinterpret_cast< PyCFunction >(Py_mousePressed), READONLY, "Was the button pressed" },
    { "mouseReleased", reinterpret_cast< PyCFunction >(Py_mouseReleased), READONLY, "Was the button released" },
    { "mousePos", reinterpret_cast< PyCFunction >(Py_mousePos), READONLY, "Mouse position" },
    { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject inputType = [](){
    PyTypeObject obj;
    obj.tp_name = "input";
    obj.tp_basicsize = sizeof(PyInput);
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_doc = "How do you know your senses aren't lying to you?";
    obj.tp_methods = inputMethods;
    return obj;
}();

}
template<>
PyObject *toPython< Input >(Input &in) {
    RUN_ONCE(PyType_Ready(&inputType));
    PyInput *pin;
    pin = reinterpret_cast< PyInput * >(inputType.tp_alloc(&inputType, 0));
    if (pin) {
        pin->input = &in;
    }
    return reinterpret_cast< PyObject * >(pin);
}
