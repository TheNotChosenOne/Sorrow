#pragma once

#include <boost/program_options.hpp>
#include <cstddef>
#include <memory>

class Input;
namespace Entity { class Tracker; }
class Renderer;

struct Core {
    Input &input;
    Entity::Tracker &tracker;
    Renderer &renderer;
    boost::program_options::variables_map options;
};
