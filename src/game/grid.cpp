#include "grid.h"

#include "utility/utility.h"

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

uint64_t Grid::set(size_t row, size_t col, uint64_t ent) {
    const uint64_t old = grid[row][col];
    grid[row][col] = ent;
    return old;
}

uint64_t Grid::set(Point point, uint64_t ent) {
    const auto coord = getCoord(point);
    const uint64_t old = grid[coord.first][coord.second];
    grid[coord.first][coord.second] = ent;
    return old;
}

Point Grid::gridOrigin(size_t row, size_t col) const {
    return origin + Vec(row * size, col * size);
}

Point Grid::gridOrigin(Point point) const {
    const auto coord = getCoord(point);
    return origin + Vec(coord.first * size, coord.second * size);
}
