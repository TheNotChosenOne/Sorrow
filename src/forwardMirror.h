#pragma once

#include <vector>

#define Py_RETURN_BOOL(x) { if (x) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }

typedef struct _object PyObject;
class Mirror;

template< typename T >
class HanaMirror;

PyObject *PyMirrorMake(Mirror *mirror);

template< typename T >
Mirror *getMirrorFor(T &t);

template< typename T >
PyObject *toPython(T &t);

template< typename T >
void fromPython(T& t, PyObject *);

template< typename T >
PyObject *toPython(std::vector< T > &v);

template< typename T >
PyObject *toPython(const std::vector< T > &v);

template< typename T >
void fromPython(std::vector< T > &v, PyObject *obj);

template< typename T >
T fromPython(PyObject *);
