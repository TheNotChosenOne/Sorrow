#pragma once

#include "core/geometry.h"
#include "entities/data.h"

#include <vector>
#include <utility>

class Grid;

struct GridBind {
    Grid *grid;
    size_t row;
    size_t col;
};
DeclareDataType(GridBind);
template<>
void Entity::deleteComponent< GridBind >(Core &core, uint64_t id, GridBind &gb);

class Grid {
    double size;
    Point origin;
    size_t height, width;

    // row, column
    std::vector< std::vector< uint64_t > > grid;

public:
    Grid(double size, Point origin, size_t height, size_t width);
    Grid(double size, Point origin, double height, double width);

    Point getOrigin() const;
    double getSize() const;
    size_t getHeight() const;
    size_t getWidth() const;

    std::pair< size_t, size_t > getCoord(Point point) const;

    uint64_t get(size_t row, size_t col) const;
    uint64_t get(Point point) const;

    uint64_t set(Core &core, size_t row, size_t col, uint64_t ent);
    uint64_t set(Core &core, Point point, uint64_t ent);

    Point gridOrigin(size_t row, size_t col) const;
    Point gridOrigin(Point point) const;

    friend void Entity::deleteComponent< GridBind >(Core &core, uint64_t id, GridBind &gb);
};
