#include "grid.h"

#include "utility/utility.h"
#include "core/core.h"
#include "entities/tracker.h"

namespace {

void setBinding(Core &core, uint64_t eid, Grid *grid, size_t row, size_t col) {
    auto optGB = core.tracker.optComponent< GridBind >(eid);
    if (optGB) {
        optGB->get().grid = grid;
        optGB->get().row = row;
        optGB->get().col = col;
    } else {
        core.tracker.addComponent(core, eid, GridBind{ grid, row, col });
    }
}

void removeBinding(Core &core, uint64_t id) {
    if (0 == id) { return; }
    if (core.tracker.hasComponent< GridBind >(id)) {
        core.tracker.removeComponent< GridBind >(core, id);
    }
}

}

Grid::Grid(double size, Point origin, size_t height, size_t width) {
    this->size = size;
    this->origin = origin;
    this->height = height;
    this->width = width;
    grid.resize(height);
    for (auto &row : grid) {
        row.resize(width, 0);
    }
}

Grid::Grid(double size, Point origin, double height, double width) {
    this->size = size;
    this->origin = origin;
    this->height = static_cast< size_t >(height / size);
    this->width  = static_cast< size_t >(width / size);
    grid.resize(this->height);
    for (auto &row : grid) {
        row.resize(this->width, 0);
    }
}

Point Grid::getOrigin() const {
    return origin;
}

double Grid::getSize() const {
    return size;
}

size_t Grid::getHeight() const {
    return height;
}

size_t Grid::getWidth() const {
    return width;
}

std::pair< size_t, size_t > Grid::getCoord(Point point) const {
    const Vec offset = point - origin;
    const size_t row = static_cast< size_t >(offset.y() / size);
    const size_t col = static_cast< size_t >(offset.x() / size);
    return std::make_pair(row, col);
}

uint64_t Grid::get(size_t row, size_t col) const {
    rassert(row < height && col < width, row, col, height, width);
    return grid[row][col];
}

uint64_t Grid::get(Point point) const {
    const auto coord = getCoord(point);
    return get(coord.first, coord.second);
}

uint64_t Grid::set(Core &core, size_t row, size_t col, uint64_t ent) {
    const uint64_t old = grid[row][col];
    removeBinding(core, old);
    grid[row][col] = ent;
    setBinding(core, ent, this, row, col);
    return old;
}

uint64_t Grid::set(Core &core, Point point, uint64_t ent) {
    const auto coord = getCoord(point);
    return set(core, coord.first, coord.second, ent);
}

Point Grid::gridOrigin(size_t row, size_t col) const {
    return origin + Vec(row * size, col * size);
}

Point Grid::gridOrigin(Point point) const {
    const auto coord = getCoord(point);
    return origin + Vec(coord.first * size, coord.second * size);
}

template<>
void Entity::deleteComponent(Core &, uint64_t id, GridBind &gb) {
    if (gb.grid) {
        rassert(id == gb.grid->grid[gb.row][gb.col], id, gb.grid->grid[gb.row][gb.col]);
        gb.grid->grid[gb.row][gb.col] = 0;
    }
}
