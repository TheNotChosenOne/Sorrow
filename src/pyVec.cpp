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

static PyTypeObject PyVecTypeMaker();
static PyTypeObject PyVecType = PyVecTypeMaker();

static PyTypeObject PyVec3TypeMaker();
static PyTypeObject PyVec3Type = PyVec3TypeMaker();

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
    for (size_t i = 0; i < decltype(pv->v)::Size; ++i) {
        seed ^= std::hash< double >{}(pv->v[i]) + 0x87387337 + (seed << 7) + (seed >> 3);
    }
    return seed;
}

template< typename T >
PyObject *Py_richcompare(T *a, T *b, int op) {
    switch (op) {
    case Py_EQ:
        Py_RETURN_BOOL(a->v == b->v);
    case Py_NE:
        Py_RETURN_BOOL(a->v != b->v);
    default:
        PyErr_SetString(PyExc_TypeError, "Vectors only supports == or != comparisons");
        return nullptr;
    }
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
static PyObject *PyVec_add(T *a, T *b) {
    return toPython< decltype(a->v) >(a->v + b->v);
}

template< typename T >
static PyObject *PyVec_sub(T *a, T *b) {
    return toPython< decltype(a->v) >(a->v - b->v);
}

static PyObject *PyVec_mul(PyObject *a, PyObject *b) {
    if (&PyVecType == a->ob_type && &PyVecType == b->ob_type) {
        Vec va = reinterpret_cast< PyVec * >(a)->v;
        Vec vb = reinterpret_cast< PyVec * >(b)->v;
        return toPython(Vec(va[0] * vb[0], va[1] * vb[1]));
    } else if (&PyVecType == a->ob_type) {
        return toPython(Vec(reinterpret_cast< PyVec * >(a)->v * fromPython< double >(b)));
    } else if (&PyVecType == b->ob_type) {
        return toPython(Vec(reinterpret_cast< PyVec * >(b)->v * fromPython< double >(a)));
    } else {
        return nullptr;
    }
}

static PyObject *PyVec3_mul(PyObject *a, PyObject *b) {
    if (&PyVec3Type == a->ob_type && &PyVec3Type == b->ob_type) {
        Vec3 va = reinterpret_cast< PyVec3 * >(a)->v;
        Vec3 vb = reinterpret_cast< PyVec3 * >(b)->v;
        return toPython(Vec3(va[0] * vb[0], va[1] * vb[1], va[2] * vb[2]));
    } else if (&PyVec3Type == a->ob_type) {
        return toPython(Vec3(reinterpret_cast< PyVec3 * >(a)->v * fromPython< double >(b)));
    } else if (&PyVec3Type == b->ob_type) {
        return toPython(Vec3(reinterpret_cast< PyVec3 * >(b)->v * fromPython< double >(a)));
    } else {
        return nullptr;
    }
}

static PyObject *PyVec_inplace_mul(PyObject *a, PyObject *b) {
    if (&PyVecType == a->ob_type && &PyVecType == b->ob_type) {
        Vec &va = reinterpret_cast< PyVec * >(a)->v;
        Vec vb = reinterpret_cast< PyVec * >(b)->v;
        for (size_t i = 0; i < Vec::Size; ++i) {
            va[i] *= vb[i];
        }
    } else if (&PyVecType == a->ob_type) {
        Vec &va = reinterpret_cast< PyVec * >(a)->v;
        va *= fromPython< double >(b);
    } else {
        return nullptr;
    }
    Py_INCREF(a);
    return a;
}

static PyObject *PyVec3_inplace_mul(PyObject *a, PyObject *b) {
    if (&PyVec3Type == a->ob_type && &PyVec3Type == b->ob_type) {
        Vec3 &va = reinterpret_cast< PyVec3 * >(a)->v;
        Vec3 vb = reinterpret_cast< PyVec3 * >(b)->v;
        for (size_t i = 0; i < Vec3::Size; ++i) {
            va[i] *= vb[i];
        }
    } else if (&PyVec3Type == a->ob_type) {
        Vec3 &va = reinterpret_cast< PyVec3 * >(a)->v;
        va *= fromPython< double >(b);
    } else {
        return nullptr;
    }
    Py_INCREF(a);
    return a;
}

static PyObject *PyVec_div(PyObject *a, PyObject *b) {
    if (&PyVecType == a->ob_type && &PyVecType == b->ob_type) {
        Vec va = reinterpret_cast< PyVec * >(a)->v;
        Vec vb = reinterpret_cast< PyVec * >(b)->v;
        return toPython(Vec(va[0] / vb[0], va[1] / vb[1]));
    } else if (&PyVecType == a->ob_type) {
        return toPython(Vec(reinterpret_cast< PyVec * >(a)->v / fromPython< double >(b)));
    } else if (&PyVecType == b->ob_type) {
        return toPython(Vec(reinterpret_cast< PyVec * >(b)->v / fromPython< double >(a)));
    } else {
        return nullptr;
    }
}

static PyObject *PyVec3_div(PyObject *a, PyObject *b) {
    if (&PyVec3Type == a->ob_type && &PyVec3Type == b->ob_type) {
        Vec3 va = reinterpret_cast< PyVec3 * >(a)->v;
        Vec3 vb = reinterpret_cast< PyVec3 * >(b)->v;
        return toPython(Vec3(va[0] / vb[0], va[1] / vb[1], va[2] / vb[2]));
    } else if (&PyVec3Type == a->ob_type) {
        return toPython(Vec3(reinterpret_cast< PyVec3 * >(a)->v / fromPython< double >(b)));
    } else if (&PyVec3Type == b->ob_type) {
        return toPython(Vec3(reinterpret_cast< PyVec3 * >(b)->v / fromPython< double >(a)));
    } else {
        return nullptr;
    }
}

static PyObject *PyVec_inplace_div(PyObject *a, PyObject *b) {
    if (&PyVecType == a->ob_type && &PyVecType == b->ob_type) {
        Vec &va = reinterpret_cast< PyVec * >(a)->v;
        Vec vb = reinterpret_cast< PyVec * >(b)->v;
        for (size_t i = 0; i < Vec::Size; ++i) {
            va[i] /= vb[i];
        }
    } else if (&PyVecType == a->ob_type) {
        Vec &va = reinterpret_cast< PyVec * >(a)->v;
        va /= fromPython< double >(b);
    } else {
        return nullptr;
    }
    Py_INCREF(a);
    return a;
}

static PyObject *PyVec3_inplace_div(PyObject *a, PyObject *b) {
    if (&PyVec3Type == a->ob_type && &PyVec3Type == b->ob_type) {
        Vec3 &va = reinterpret_cast< PyVec3 * >(a)->v;
        Vec3 vb = reinterpret_cast< PyVec3 * >(b)->v;
        for (size_t i = 0; i < Vec3::Size; ++i) {
            va[i] /= vb[i];
        }
    } else if (&PyVec3Type == a->ob_type) {
        Vec3 &va = reinterpret_cast< PyVec3 * >(a)->v;
        va /= fromPython< double >(b);
    } else {
        return nullptr;
    }
    Py_INCREF(a);
    return a;
}

template< typename T >
static PyObject *PyVec_neg(T *a) {
    return toPython< decltype(a->v) >(-a->v);
}

template< typename T >
static PyObject *PyVec_pos(T *a) {
    Py_INCREF(a);
    return reinterpret_cast< PyObject * >(a);
}

template< typename T >
static PyObject *PyVec_abs(T *a) {
    decltype(a->v) v;
    for (size_t i = 0; i < decltype(a->v)::Size; ++i) {
        v[i] = std::abs(a->v[i]);
    }
    return toPython< decltype(a->v) >(v);
}

template< typename T >
static int PyVec_bool(T *a) {
    return decltype(a->v)() != a->v;
}

template< typename T >
static PyObject *PyVec_inplace_add(T *a, T *b) {
    a->v += b->v;
    Py_INCREF(a);
    return reinterpret_cast< PyObject * >(a);
}

template< typename T >
static PyObject *PyVec_inplace_sub(T *a, T *b) {
    a->v -= b->v;
    Py_INCREF(a);
    return reinterpret_cast< PyObject * >(a);
}

static PyNumberMethods PyVecNumber = [](){
    PyNumberMethods m;
    m.nb_add = reinterpret_cast< binaryfunc >(PyVec_add< PyVec >);
    m.nb_subtract = reinterpret_cast< binaryfunc >(PyVec_sub< PyVec >);
    m.nb_multiply = reinterpret_cast< binaryfunc >(PyVec_mul);
    m.nb_true_divide = reinterpret_cast< binaryfunc >(PyVec_div);
    m.nb_negative = reinterpret_cast< unaryfunc >(PyVec_neg< PyVec >);
    m.nb_positive = reinterpret_cast< unaryfunc >(PyVec_pos< PyVec >);
    m.nb_absolute = reinterpret_cast< unaryfunc >(PyVec_abs< PyVec >);
    m.nb_bool = reinterpret_cast< inquiry >(PyVec_bool< PyVec >);
    m.nb_inplace_add = reinterpret_cast< binaryfunc >(PyVec_inplace_add< PyVec >);
    m.nb_inplace_subtract = reinterpret_cast< binaryfunc >(PyVec_inplace_sub< PyVec >);
    m.nb_inplace_multiply = reinterpret_cast< binaryfunc >(PyVec_inplace_mul);
    m.nb_inplace_true_divide = reinterpret_cast< binaryfunc >(PyVec_inplace_div);
    return m;
}();

static PyNumberMethods PyVec3Number = [](){
    PyNumberMethods m;
    m.nb_add = reinterpret_cast< binaryfunc >(PyVec_add< PyVec3 >);
    m.nb_subtract = reinterpret_cast< binaryfunc >(PyVec_sub< PyVec3 >);
    m.nb_multiply = reinterpret_cast< binaryfunc >(PyVec3_mul);
    m.nb_true_divide = reinterpret_cast< binaryfunc >(PyVec3_div);
    m.nb_negative = reinterpret_cast< unaryfunc >(PyVec_neg< PyVec3 >);
    m.nb_positive = reinterpret_cast< unaryfunc >(PyVec_pos< PyVec3 >);
    m.nb_absolute = reinterpret_cast< unaryfunc >(PyVec_abs< PyVec3 >);
    m.nb_bool = reinterpret_cast< inquiry >(PyVec_bool< PyVec3 >);
    m.nb_inplace_add = reinterpret_cast< binaryfunc >(PyVec_inplace_add< PyVec3 >);
    m.nb_inplace_subtract = reinterpret_cast< binaryfunc >(PyVec_inplace_sub< PyVec3 >);
    m.nb_inplace_multiply = reinterpret_cast< binaryfunc >(PyVec3_inplace_mul);
    m.nb_inplace_true_divide = reinterpret_cast< binaryfunc >(PyVec3_inplace_div);
    return m;
}();

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

static PyTypeObject PyVecTypeMaker() {
    PyTypeObject obj;
    obj.tp_name = "Vec2";
    obj.tp_basicsize = sizeof(PyVec);
    obj.tp_doc = "2D Vector";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_as_number = &PyVecNumber;
    obj.tp_as_mapping = &PyVecMapping;
    obj.tp_repr = reinterpret_cast< reprfunc >(Py_repr< PyVec >);
    obj.tp_hash = reinterpret_cast< hashfunc >(Py_hash< PyVec >);
    obj.tp_methods = PyVecMethods;
    obj.tp_members = PyVecMembers;
    obj.tp_init = reinterpret_cast< initproc >(Py_init< PyVec >);
    obj.tp_new = reinterpret_cast< newfunc >(Py_new< PyVec >);
    obj.tp_richcompare = reinterpret_cast< richcmpfunc >(Py_richcompare< PyVec >);
    return obj;
}

static PyTypeObject PyVec3TypeMaker() {
    PyTypeObject obj;
    obj.tp_name = "Vec3";
    obj.tp_basicsize = sizeof(PyVec3);
    obj.tp_doc = "3D Vector";
    obj.tp_flags = Py_TPFLAGS_DEFAULT;
    obj.tp_as_number = &PyVecNumber;
    obj.tp_as_mapping = &PyVec3Mapping;
    obj.tp_repr = reinterpret_cast< reprfunc >(Py_repr< PyVec3 >);
    obj.tp_hash = reinterpret_cast< hashfunc >(Py_hash< PyVec3 >);
    obj.tp_methods = PyVec3Methods;
    obj.tp_members = PyVec3Members;
    obj.tp_init = reinterpret_cast< initproc >(Py_init< PyVec3 >);
    obj.tp_new = reinterpret_cast< newfunc >(Py_new< PyVec3 >);
    obj.tp_richcompare = reinterpret_cast< richcmpfunc >(Py_richcompare< PyVec3 >);
    return obj;
};

static PyObject *Py_normalized(PyObject *, PyObject *args) {
    PyObject *vecObj = PyTuple_GetItem(args, 0);
    if (&PyVecType == vecObj->ob_type) {
        Vec v = reinterpret_cast< PyVec * >(vecObj)->v;
        gmtl::normalize(v);
        return toPython(v);
    } else if (&PyVec3Type == vecObj->ob_type) {
        Vec3 v = reinterpret_cast< PyVec3 * >(vecObj)->v;
        gmtl::normalize(v);
        return toPython(v);
    } else {
        return nullptr;
    }
}

static PyObject *Py_dot(PyObject *, PyObject *args) {
    PyObject *a = PyTuple_GetItem(args, 0);
    PyObject *b = PyTuple_GetItem(args, 1);
    if (&PyVecType == a->ob_type) {
        Vec va = reinterpret_cast< PyVec * >(a)->v;
        Vec vb = reinterpret_cast< PyVec * >(b)->v;
        double d = 0.0;
        for (size_t i = 0; i < Vec::Size; ++i) {
            d += va[i] * vb[i];
        }
        return toPython(d);
    } else if (&PyVec3Type == b->ob_type) {
        Vec3 va = reinterpret_cast< PyVec3 * >(a)->v;
        Vec3 vb = reinterpret_cast< PyVec3 * >(b)->v;
        double d = 0.0;
        for (size_t i = 0; i < Vec3::Size; ++i) {
            d += va[i] * vb[i];
        }
        return toPython(d);
    } else {
        return nullptr;
    }
}

static PyObject *Py_clampVec(PyObject *, PyObject *args) {
    double low = fromPython< double >(PyTuple_GetItem(args, 0));
    double high = fromPython< double >(PyTuple_GetItem(args, 1));
    PyObject *pv = PyTuple_GetItem(args, 2);
    if (&PyVecType == pv->ob_type) {
        Vec v = reinterpret_cast< PyVec * >(pv)->v;
        for (size_t i = 0; i < Vec::Size; ++i) {
            v[i] = clamp(low, high, v[i]);
        }
        return toPython(v);
    } else if (&PyVec3Type == pv->ob_type) {
        Vec3 v = reinterpret_cast< PyVec3 * >(pv)->v;
        for (size_t i = 0; i < Vec::Size; ++i) {
            v[i] = clamp(low, high, v[i]);
        }
        return toPython(v);
    } else {
        return nullptr;
    }
}
static PyMethodDef SorrowMethods[] = {
    { "normalized", Py_normalized, METH_VARARGS, "Return the input vector normalized" },
    { "dot", Py_dot, METH_VARARGS, "Return dot product of two vectors" },
    { "clampVec", Py_clampVec, METH_VARARGS, "Clamp each dimension of the vector to low-high" },
    { nullptr, nullptr, 0, nullptr }
};

static struct PyModuleDef SorrowModuleDef = {
    PyModuleDef_HEAD_INIT,
    "sorrow",
    "Abandon all hope",
    -1,
    SorrowMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

static PyObject *PyInit_sorrow() {
    PyType_Ready(&PyVecType);
    PyType_Ready(&PyVec3Type);
    Py_INCREF(&PyVecType);
    Py_INCREF(&PyVec3Type);
    PyObject *sorrowModule = PyModule_Create(&SorrowModuleDef);
    PyModule_AddObject(sorrowModule, "Vec2", reinterpret_cast< PyObject * >(&PyVecType));
    PyModule_AddObject(sorrowModule, "Vec3", reinterpret_cast< PyObject * >(&PyVec3Type));
    return sorrowModule;
}

RUN_STATIC(PyImport_AppendInittab("sorrow", PyInit_sorrow));

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
