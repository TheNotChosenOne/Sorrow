#pragma once

#include <Python.h>
#include <iostream>
#include <utility>
#include <string>
#include <vector>
#include <memory>

#include <boost/hana.hpp>

#include "utility.h"
#include "forwardMirror.h"

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

void PyTypesInit();

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

template<> PyObject *toPython< Vec > (Vec &v);
template<> PyObject *toPython< bool >(bool &b);
template<> PyObject *toPython< Vec3 > (Vec3 &v);
template<> PyObject *toPython< size_t >(size_t &v);
template<> PyObject *toPython< double >(double &v);
template<> PyObject *toPython< int64_t >(int64_t &v);
template<> PyObject *toPython< std::string >(std::string &v);
template<> PyObject *toPython< const Vec > (const Vec &v);
template<> PyObject *toPython< const bool >(const bool &b);
template<> PyObject *toPython< const Vec3 > (const Vec3 &v);
template<> PyObject *toPython< const size_t >(const size_t &v);
template<> PyObject *toPython< const double >(const double &v);
template<> PyObject *toPython< const int64_t >(const int64_t &v);
template<> PyObject *toPython< const std::string >(const std::string &v);
template<> void fromPython< Vec >(Vec &v, PyObject *obj);
template<> void fromPython< bool >(bool &v, PyObject *obj);
template<> void fromPython< Vec3 >(Vec3 &v, PyObject *obj);
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
                const std::string attr = boost::hana::to< const char * >(key);
                if (name == attr) {
                    obj = toPython(boost::hana::at_key(*mirroring, key));
                }
            });
            return obj;
        }

        void set(const std::string &name, PyObject *obj) override {
            boost::hana::for_each(boost::hana::keys(*mirroring), [&](auto key) {
                const std::string attr = boost::hana::to< const char * >(key);
                if (name == attr) {
                    auto &member = boost::hana::at_key(*mirroring, key);
                    fromPython(member, obj);
                }
            });
        }
};
