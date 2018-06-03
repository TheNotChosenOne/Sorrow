#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <array>

#include "utility.h"

class Core;
struct PhysicsComponent;
typedef std::pair< size_t, PhysicsComponent * > TaggedPhys;

class Quadtree {
    public:
        Vec mid;
        Vec rad;
        std::vector< size_t > inside;
        std::array< std::unique_ptr< Quadtree >, 4 > children;

    Quadtree(std::vector< TaggedPhys > &phys);

    void suggest(const Vec pos, const Vec rad, std::vector< size_t > &into) const;
    void draw(Core &core, size_t density=0) const;
};
std::ostream &operator<<(std::ostream &os, const Quadtree &qt);
