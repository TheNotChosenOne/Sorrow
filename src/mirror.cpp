#include "mirror.h"

#include <iostream>

namespace {

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

static PyTypeObject mirrorType = [](){
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
}();

static char *PyVec2Arr[][2] = {
    { const_cast< char * >("x"), const_cast< char * >("") },
    { const_cast< char * >("y"), const_cast< char * >("") },
    { nullptr, nullptr } };
static PyStructSequence_Desc PyVec2 {
    .name = const_cast< char * >("Vec2"),
    .doc = const_cast< char * >("A 2D vector!"),
    .fields = reinterpret_cast< PyStructSequence_Field * >(PyVec2Arr),
    .n_in_sequence = 2,
};

static char *PyVec3Arr[][2] = {
    { const_cast< char * >("x"), const_cast< char * >("") },
    { const_cast< char * >("y"), const_cast< char * >("") },
    { const_cast< char * >("z"), const_cast< char * >("") },
    { nullptr, nullptr } };
static PyStructSequence_Desc PyVec3 {
    .name = const_cast< char * >("Vec3"),
    .doc = const_cast< char * >("A 3D vector!"),
    .fields = reinterpret_cast< PyStructSequence_Field * >(PyVec3Arr),
    .n_in_sequence = 3,
};

static PyTypeObject *k_PyVec2Type;
static PyTypeObject *k_PyVec3Type;

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

void PyTypesInit() {
    PyType_Ready(&mirrorType);
    k_PyVec2Type = PyStructSequence_NewType(&PyVec2);
    k_PyVec3Type = PyStructSequence_NewType(&PyVec3);
    k_PyVec2Type->tp_flags |= Py_TPFLAGS_HEAPTYPE;
    k_PyVec3Type->tp_flags |= Py_TPFLAGS_HEAPTYPE;

    PyObject *mods = PyImport_GetModuleDict();
    PyObject *mainString = Py_BuildValue("s", "__main__");
    PyObject *main = PyDict_GetItem(mods, mainString);

    Py_INCREF(k_PyVec2Type);
    Py_INCREF(k_PyVec3Type);
    PyModule_AddObject(main, "Vec2", reinterpret_cast< PyObject * >(k_PyVec2Type));
    PyModule_AddObject(main, "Vec3", reinterpret_cast< PyObject * >(k_PyVec3Type));

    Py_DECREF(mainString);
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

template<>
void fromPython< Vec >(Vec &v, PyObject *obj) {
    for (size_t i = 0; i < 2; ++i) {
        v[i] = PyFloat_AsDouble(PyTuple_GetItem(obj, i));
    }
}

template<>
void fromPython< Vec3 >(Vec3 &v, PyObject *obj) {
    for (size_t i = 0; i < 3; ++i) {
        v[i] = PyFloat_AsDouble(PyTuple_GetItem(obj, i));
    }
}

template<>
PyObject *toPython< const Vec >(const Vec &v) {
    PyObject *obj = PyStructSequence_New(k_PyVec2Type);
    for (size_t i = 0; i < 2; ++i) {
        PyStructSequence_SetItem(obj, i, PyFloat_FromDouble(v[i]));
    }
    return obj;
}

template<>
PyObject *toPython< Vec >(Vec &v) {
    PyObject *obj = PyStructSequence_New(k_PyVec2Type);
    for (size_t i = 0; i < 2; ++i) {
        PyStructSequence_SetItem(obj, i, PyFloat_FromDouble(v[i]));
    }
    return obj;
}

template<>
PyObject *toPython< const Vec3 >(const Vec3 &v) {
    PyObject *obj = PyStructSequence_New(k_PyVec3Type);
    for (size_t i = 0; i < 3; ++i) {
        PyStructSequence_SetItem(obj, i, PyFloat_FromDouble(v[i]));
    }
    return obj;
}

template<>
PyObject *toPython< Vec3 >(Vec3 &v) {
    PyObject *obj = PyStructSequence_New(k_PyVec3Type);
    for (size_t i = 0; i < 3; ++i) {
        PyStructSequence_SetItem(obj, i, PyFloat_FromDouble(v[i]));
    }
    return obj;
}
