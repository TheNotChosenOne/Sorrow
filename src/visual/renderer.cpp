#include "renderer.h"

Renderer::Renderer(size_t width, size_t height): width(width), height(height) {
}

Renderer::~Renderer() {
}

void Renderer::clear() {
}

void Renderer::update() {
}

size_t Renderer::getWidth() const {
    return width;
}

size_t Renderer::getHeight() const {
    return height;
}

void Renderer::drawPoint(Point, Point3, double, double) {
}

void Renderer::drawBox(Point, Vec, Point3, double, double) {
}

void Renderer::drawCircle(Point, Vec, Point3, double, double) {
}
