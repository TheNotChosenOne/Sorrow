#include "renderer.h"

#include <Python.h>
#include <structmember.h>

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

void Renderer::drawPoint(Vec, Vec3, double, double) {
}

void Renderer::drawBox(Vec, Vec, Vec3, double, double) {
}

void Renderer::drawCircle(Vec, Vec, Vec3, double, double) {
}
