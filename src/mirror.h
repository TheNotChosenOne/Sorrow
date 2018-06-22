#pragma once

#include <functional>
#include <Python.h>
#include <iostream>
#include <utility>
#include <string>
#include <vector>
#include <memory>

#include <boost/hana.hpp>

#include "utility.h"
#include "forwardMirror.h"
#include "pyVec.h"

class Mirror {
    public:
        virtual ~Mirror();
        virtual PyObject *get(const std::string &name);
        virtual void set(const std::string &name, PyObject *obj);
};

struct PyMirror {
    PyObject_HEAD
    Mirror *mirror;
};

template< typename T >
class HanaMirror;

// Takes ownership of pointer memory
PyObject *PyMirrorMake(Mirror *mirror);

void addPyTypeInitializer(const std::function< void() > &func);
void PyTypesInit();

#define PyStructSequence_NewType static_assert(false); // This function doesn't work

template< typename T >
Mirror *getMirrorFor(T &t) {
    static_assert(boost::hana::Struct< T >::value,
                  "This type does not have custom or Hana mirroring support");
    return new HanaMirror< T >(&t);
}

template< typename T >
PyObject *toPython(T &t) {
    return PyMirrorMake(getMirrorFor(t));
}

template< typename T >
void fromPython(T& t, PyObject *) {
    std::cout << "Cannot find assignment for: " << t << '\n';
}

template< typename T >
T fromPython(PyObject *obj) {
    T t;
    fromPython(t, obj);
    return t;
}

template<> PyObject *toPython< bool >(bool &b);
template<> PyObject *toPython< size_t >(size_t &v);
template<> PyObject *toPython< double >(double &v);
template<> PyObject *toPython< int64_t >(int64_t &v);
template<> PyObject *toPython< std::string >(std::string &v);
template<> PyObject *toPython< const bool >(const bool &b);
template<> PyObject *toPython< const size_t >(const size_t &v);
template<> PyObject *toPython< const double >(const double &v);
template<> PyObject *toPython< const int64_t >(const int64_t &v);
template<> PyObject *toPython< const std::string >(const std::string &v);
template<> void fromPython< bool >(bool &v, PyObject *obj);
template<> void fromPython< size_t >(size_t &v, PyObject *obj);
template<> void fromPython< double >(double &v, PyObject *obj);
template<> void fromPython< int64_t >(int64_t &v, PyObject *obj);
template<> void fromPython< std::string >(std::string &v, PyObject *obj);

template< typename T >
PyObject *toPython(T &&t) {
    T &v = t;
    return toPython< T >(v);
}

template< typename T >
PyObject *toPython(const T &&t) {
    T &v = t;
    return toPython< const T >(v);
}

PyObject *toPython(const char *s);

template< typename T >
PyObject *toPython(std::vector< T > &v) {
    PyObject *listy = PyList_New(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        PyList_SetItem(listy, i, toPython(v[i]));
    }
    return listy;
}

template< typename T >
PyObject *toPython(const std::vector< T > &v) {
    PyObject *listy = PyList_New(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        PyList_SetItem(listy, i, toPython(v[i]));
    }
    return listy;
}

template< typename T >
void fromPython(std::vector< T > &v, PyObject *obj) {
    v.resize(PyList_Size(obj));
    for (size_t i = 0; i < v.size(); ++i) {
        fromPython(v[i], PyList_GetItem(obj, i));
    }
}

template< typename T >
class HanaMirror: public Mirror {
    private:
        T *mirroring;

    public:
        HanaMirror(T *mirroring): mirroring(mirroring) { }

        PyObject *get(const std::string &name) override {
            PyObject *obj = nullptr;
            boost::hana::for_each(boost::hana::keys(*mirroring), [&](auto key) {
                if (boost::hana::to< const char * >(key) == name) {
                    obj = toPython(boost::hana::at_key(*mirroring, key));
                }
            });
            return obj;
        }

        void set(const std::string &name, PyObject *obj) override {
            boost::hana::for_each(boost::hana::keys(*mirroring), [&](auto key) {
                if (boost::hana::to< const char * >(key) == name) {
                    fromPython(boost::hana::at_key(*mirroring, key), obj);
                }
            });
        }
};
