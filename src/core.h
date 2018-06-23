#pragma once

#include <boost/program_options.hpp>
#include <cstddef>
#include <memory>

#include "forwardMirror.h"


class Input;
class Tracker;
class Renderer;

struct Core {
    Input &input;
    Tracker &tracker;
    Renderer &renderer;
    boost::program_options::variables_map options;
};

template<>
PyObject *toPython< Core >(Core &core);
