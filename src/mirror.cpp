#include "mirror.h"

#include <iostream>

namespace {

static PyTypeObject mirrorTypeMaker();
static PyTypeObject mirrorType = mirrorTypeMaker();

static PyObject *PyMirror_new(PyTypeObject *type, PyObject *, PyObject *) {
    PyMirror *self;
    self = reinterpret_cast< PyMirror * >(type->tp_alloc(type, 0));
    if (self) {
        self->mirror = nullptr;
    }
    return reinterpret_cast< PyObject * >(self);
}

static void PyMirror_free(PyObject *pMirrorObj) {
    PyMirror *pMirror = reinterpret_cast< PyMirror * >(pMirrorObj);
    delete pMirror->mirror;
    mirrorType.tp_free(pMirrorObj);
}

static PyObject *PyMirror_get(PyObject *pMirrorObj, PyObject *attrName) {
    Py_ssize_t length;
    const char *data = PyUnicode_AsUTF8AndSize(attrName, &length);
    const std::string attr(data, length);

    PyMirror *pMirror = reinterpret_cast< PyMirror * >(pMirrorObj);
    Mirror &mirror = *pMirror->mirror;

    PyObject *ret = mirror.get(attr);
    if (ret) { return ret; }
    Py_RETURN_NONE;
}

static int PyMirror_set(PyObject *pMirrorObj, PyObject *attrName, PyObject *value) {
    Py_ssize_t length;
    const char *data = PyUnicode_AsUTF8AndSize(attrName, &length);
    const std::string attr(data, length);

    PyMirror *pMirror = reinterpret_cast< PyMirror * >(pMirrorObj);
    Mirror &mirror = *pMirror->mirror;

    mirror.set(attr, value);

    return 0;
}

static PyTypeObject mirrorTypeMaker() {
    PyTypeObject obj;
    obj.tp_name = "mirror";
    obj.tp_dealloc = &PyMirror_free;
    obj.tp_basicsize = sizeof(PyMirror);
    obj.tp_getattro = &PyMirror_get;
    obj.tp_setattro = &PyMirror_set;
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_doc = "You see yourself. It's terrifying";
    obj.tp_new = &PyMirror_new;
    return obj;
}

}

Mirror::~Mirror() {
}

PyObject *Mirror::get(const std::string &name) {
    std::cout << "Get: " << name << '\n';
    return nullptr;
}

void Mirror::set(const std::string &name, PyObject *obj) {
    std::cout << "Set: " << name << " = " << obj << '\n';
}

PyObject *PyMirrorMake(Mirror *mirror) {
    PyMirror *self;
    self = reinterpret_cast< PyMirror * >(mirrorType.tp_alloc(&mirrorType, 0));
    if (self) {
        self->mirror = mirror;
    }
    return reinterpret_cast< PyObject * >(self);
}

std::vector< std::function< void() > > *PyTypes_InitList;
void addPyTypeInitializer(const std::function< void() > &func) {
    static std::vector< std::function< void() > > listy;
    PyTypes_InitList = &listy;
    listy.push_back(func);
}

RUN_STATIC(addPyTypeInitializer([](){ PyType_Ready(&mirrorType); }))
void PyTypesInit() {
    for (const auto &f : *PyTypes_InitList) { f(); }
    PyTypes_InitList->clear();
}

template<>
PyObject *toPython< const bool >(const bool &b) {
    if (b) { Py_RETURN_TRUE; }
    else   { Py_RETURN_FALSE; }
}

template<>
PyObject *toPython< bool >(bool &b) {
    if (b) { Py_RETURN_TRUE; }
    else   { Py_RETURN_FALSE; }
}

template<>
void fromPython< bool >(bool &v, PyObject *obj) {
    v = PyObject_IsTrue(obj);
}

template<>
PyObject *toPython< const int64_t >(const int64_t &v) {
    return PyLong_FromLongLong(v);
}

template<>
PyObject *toPython< int64_t >(int64_t &v) {
    return PyLong_FromLongLong(v);
}

template<>
void fromPython< int64_t >(int64_t &v, PyObject *obj) {
    v = PyLong_AsLongLong(obj);
}

template<>
PyObject *toPython< const size_t >(const size_t &v) {
    return PyLong_FromLongLong(v);
}

template<>
PyObject *toPython< size_t >(size_t &v) {
    return PyLong_FromLongLong(v);
}

template<>
void fromPython< size_t >(size_t &v, PyObject *obj) {
    v = PyLong_AsLongLong(obj);
}

template<>
PyObject *toPython< const double >(const double &v) {
    return PyFloat_FromDouble(v);
}

template<>
PyObject *toPython< double >(double &v) {
    return PyFloat_FromDouble(v);
}

template<>
void fromPython< double >(double &v, PyObject *obj) {
    v = PyFloat_AsDouble(obj);
}

PyObject *toPython(const char *v) {
    return PyUnicode_FromString(v);
}

template<>
PyObject *toPython< const std::string >(const std::string &v) {
    return PyUnicode_FromString(v.c_str());
}

template<>
PyObject *toPython< std::string >(std::string &v) {
    return PyUnicode_FromString(v.c_str());
}

template<>
void fromPython< std::string >(std::string &v, PyObject *obj) {
    Py_ssize_t length;
    const char *data = PyUnicode_AsUTF8AndSize(obj, &length);
    v = std::string(data, length);
}
