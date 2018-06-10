#pragma once

#include "forwardMirror.h"
#include "utility.h"

template<> PyObject *toPython< Vec > (Vec &v);
template<> PyObject *toPython< const Vec > (const Vec &v);
template<> PyObject *toPython< Vec3 > (Vec3 &v);
template<> PyObject *toPython< const Vec3 > (const Vec3 &v);
template<> void fromPython< Vec >(Vec &v, PyObject *obj);
template<> void fromPython< Vec3 >(Vec3 &v, PyObject *obj);
