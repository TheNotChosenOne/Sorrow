#pragma once

#include <boost/program_options.hpp>
#include <cstddef>
#include <memory>

#include <Box2D.h>

class Input;
namespace Entity { class Tracker; }
class Renderer;

struct Core {
    Input &input;
    Entity::Tracker &tracker;
    Renderer &renderer;
    std::unique_ptr< b2World > b2world;
    boost::program_options::variables_map options;
};
