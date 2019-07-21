#pragma once

#include <boost/program_options.hpp>
#include <functional>
#include <cstddef>
#include <memory>
#include <mutex>

#include <Box2D.h>

class Input;
namespace Entity { class Tracker; class SystemManager; }
class Renderer;

struct ThreadedWorld {
    std::mutex tex;
    std::unique_ptr< b2World > b2w;

    void locked(const std::function< void() > &func);
};

struct Core {
    Input &input;
    Entity::Tracker &tracker;
    Renderer &renderer;
    Entity::SystemManager &systems;
    ThreadedWorld b2world;
    boost::program_options::variables_map options;
    double scale;
};
