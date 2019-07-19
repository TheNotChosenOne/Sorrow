#pragma once

#include <boost/program_options.hpp>
#include <cstddef>
#include <memory>

#include <Box2D.h>

class Input;
namespace Entity { class Tracker; class SystemManager; }
class Renderer;

struct Core {
    Input &input;
    Entity::Tracker &tracker;
    Renderer &renderer;
    Entity::SystemManager &systems;
    std::unique_ptr< b2World > b2world;
    boost::program_options::variables_map options;
    double scale;
};
