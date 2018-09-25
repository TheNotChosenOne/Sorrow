#pragma once

#include "core/geometry.h"

#include <vector>
#include <utility>

class Grid {
    double size;
    Point origin;
    size_t height, width;

    // row, column
    std::vector< std::vector< uint64_t > > grid;

public:
    Grid(double size, Point origin, size_t height, size_t width);

    Point getOrigin() const;
    double getSize() const;
    size_t getHeight() const;
    size_t getWidth() const;

    std::pair< size_t, size_t > getCoord(Point point) const;

    uint64_t get(size_t row, size_t col) const;
    uint64_t get(Point point) const;

    uint64_t set(size_t row, size_t col, uint64_t ent);
    uint64_t set(Point point, uint64_t ent);

    Point gridOrigin(size_t row, size_t col) const;
    Point gridOrigin(Point point) const;
};
