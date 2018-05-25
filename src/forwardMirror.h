#pragma once

#include <vector>

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
void FromPython(T& t, PyObject *);

template< typename T >
PyObject *toPython(std::vector< T > &v);

template< typename T >
void fromPython(std::vector< T > &v, PyObject *obj);

