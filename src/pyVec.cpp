#include "pyVec.h"

#include <sstream>

#include "mirror.h"
#include "structmember.h"

namespace {

struct PyVec {
    PyObject_HEAD
    Vec v;
};

struct PyVec3 {
    PyObject_HEAD
    Vec3 v;
};

static PyMemberDef PyVecMembers[] = {
    { const_cast< char * >("x"), T_DOUBLE, offsetof(PyVec, v) + 0 * sizeof(double),
      0, const_cast< char * >("x") },
    { const_cast< char * >("y"), T_DOUBLE, offsetof(PyVec, v) + 1 * sizeof(double),
      0, const_cast< char * >("y") },
    { nullptr, 0, 0, 0, nullptr }
};

static PyMemberDef PyVec3Members[] = {
    { const_cast< char * >("x"), T_DOUBLE, offsetof(PyVec3, v) + 0 * sizeof(double),
      0, const_cast< char * >("x") },
    { const_cast< char * >("y"), T_DOUBLE, offsetof(PyVec3, v) + 1 * sizeof(double),
      0, const_cast< char * >("y") },
    { const_cast< char * >("z"), T_DOUBLE, offsetof(PyVec3, v) + 2 * sizeof(double),
      0, const_cast< char * >("z") },
    { nullptr, 0, 0, 0, nullptr }
};

template< typename T >
static PyObject *Py_normalize(T *pv, PyObject *) {
    gmtl::normalize(pv->v);
    Py_RETURN_NONE;
}

template< typename T >
static PyObject *Py_repr(T *pv) {
    std::stringstream ss;
    ss << pv->v;
    return toPython(ss.str());
}

template< typename T >
static Py_hash_t Py_hash(T *pv) {
    size_t seed = 0;
    std::cout << decltype(pv->v)::Size << '\n';
    for (size_t i = 0; i < decltype(pv->v)::Size; ++i) {
        seed ^= std::hash< double >{}(pv->v[i]) + 0x87387337 + (seed << 7) + (seed >> 3);
    }
    return seed;
}

template< typename T >
static int Py_init(T *self, PyObject *args, PyObject *) {
    if (PyTuple_Size(args) < 2) {
        double val = 0.0;
        if (1 == PyTuple_Size(args)) {
            val = fromPython< double >(PyTuple_GetItem(args, 0));
        }
        for (size_t i = 0; i < decltype(self->v)::Size; ++i) {
            self->v[i] = val;
        }
    } else {
        for (size_t i = 0; i < decltype(self->v)::Size; ++i) {
            fromPython(self->v[i], PyTuple_GetItem(args, i));
        }
    }
    return 0;
}

template< typename T >
static PyObject *Py_new(PyTypeObject *type, PyObject *, PyObject *) {
    T *self = reinterpret_cast< T * >(type->tp_alloc(type, 0));
    self->v = decltype(self->v)();
    return reinterpret_cast< PyObject * >(self);
}

static PyMethodDef PyVecMethods[] = {
    { "normalize", reinterpret_cast< PyCFunction >(Py_normalize< PyVec >), READONLY, "Normalize" },
    { nullptr, nullptr, 0, nullptr }
};

static PyMethodDef PyVec3Methods[] = {
    { "normalize", reinterpret_cast< PyCFunction >(Py_normalize< PyVec3 >), READONLY, "Normalize" },
    { nullptr, nullptr, 0, nullptr }
};

template< typename T >
static Py_ssize_t PyVec_mp_length(PyObject *) {
    return T::Size;
}

template< typename T >
static PyObject *PyVec_subscript(T *pv, PyObject *key) {
    return toPython(pv->v[fromPython< size_t >(key)]);
}

template< typename T >
static int PyVec_ass_subscript(T *pv, PyObject *key, PyObject *v) {
    pv->v[fromPython< size_t >(key)] = fromPython< double >(v);
    return 0;
}

static PyMappingMethods PyVecMapping = {
    .mp_length = PyVec_mp_length< Vec >,
    .mp_subscript = reinterpret_cast< binaryfunc >(PyVec_subscript< PyVec >),
    .mp_ass_subscript = reinterpret_cast< objobjargproc >(PyVec_ass_subscript< PyVec >),
};

static PyMappingMethods PyVec3Mapping = {
    .mp_length = PyVec_mp_length< Vec3 >,
    .mp_subscript = reinterpret_cast< binaryfunc >(PyVec_subscript< PyVec3 >),
    .mp_ass_subscript = reinterpret_cast< objobjargproc >(PyVec_ass_subscript< PyVec3 >),
};

static PyTypeObject PyVecType = [](){
    PyTypeObject obj;
    obj.tp_name = "Vec2";
    obj.tp_basicsize = sizeof(PyVec);
    obj.tp_doc = "2D Vector";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_as_mapping = &PyVecMapping;
    obj.tp_repr = reinterpret_cast< reprfunc >(Py_repr< PyVec >);
    obj.tp_hash = reinterpret_cast< hashfunc >(Py_hash< PyVec >);
    obj.tp_methods = PyVecMethods;
    obj.tp_members = PyVecMembers;
    obj.tp_init = reinterpret_cast< initproc >(Py_init< PyVec >);
    obj.tp_new = reinterpret_cast< newfunc >(Py_new< PyVec >);
    return obj;
}();

static PyTypeObject PyVec3Type = [](){
    PyTypeObject obj;
    obj.tp_name = "Vec3";
    obj.tp_basicsize = sizeof(PyVec3);
    obj.tp_doc = "3D Vector";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_as_mapping = &PyVec3Mapping;
    obj.tp_repr = reinterpret_cast< reprfunc >(Py_repr< PyVec3 >);
    obj.tp_hash = reinterpret_cast< hashfunc >(Py_hash< PyVec3 >);
    obj.tp_methods = PyVec3Methods;
    obj.tp_members = PyVec3Members;
    obj.tp_init = reinterpret_cast< initproc >(Py_init< PyVec3 >);
    obj.tp_new = reinterpret_cast< newfunc >(Py_new< PyVec3 >);
    return obj;
}();

RUN_STATIC(addPyTypeInitializer([](){
    PyType_Ready(&PyVecType);
    PyType_Ready(&PyVec3Type);

    PyObject *mods = PyImport_GetModuleDict();
    PyObject *mainString = toPython("__main__");
    PyObject *main = PyDict_GetItem(mods, mainString);

    Py_INCREF(&PyVecType);
    Py_INCREF(&PyVec3Type);
    PyModule_AddObject(main, "Vec2", reinterpret_cast< PyObject * >(&PyVecType));
    PyModule_AddObject(main, "Vec3", reinterpret_cast< PyObject * >(&PyVec3Type));

    Py_DECREF(mainString);
}));

}

template<>
PyObject *toPython< Vec >(Vec &v) {
    PyObject *pv = PyVecType.tp_alloc(&PyVecType, 0);
    reinterpret_cast< PyVec * >(pv)->v = v;
    return pv;
}

template<>
PyObject *toPython< const Vec >(const Vec &v) {
    PyObject *pv = PyVecType.tp_alloc(&PyVecType, 0);
    reinterpret_cast< PyVec * >(pv)->v = v;
    return pv;
}

template<>
PyObject *toPython< Vec3 >(Vec3 &v) {
    PyObject *pv = PyVec3Type.tp_alloc(&PyVec3Type, 0);
    reinterpret_cast< PyVec3 * >(pv)->v = v;
    return pv;
}

template<>
PyObject *toPython< const Vec3 >(const Vec3 &v) {
    PyObject *pv = PyVec3Type.tp_alloc(&PyVec3Type, 0);
    reinterpret_cast< PyVec3 * >(pv)->v = v;
    return pv;
}

template<>
void fromPython< Vec >(Vec &v, PyObject *obj) {
    v = reinterpret_cast< PyVec * >(obj)->v;
}

template<>
void fromPython< Vec3 >(Vec3 &v, PyObject *obj) {
    v = reinterpret_cast< PyVec3 * >(obj)->v;
}
