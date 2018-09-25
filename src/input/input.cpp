#include "input.h"

#include <Python.h>
#include <functional>
#include <structmember.h>

#include "core/core.h"

Input::Input() {
}

Input::~Input() {
}

void Input::update() {
}

bool Input::shouldQuit() const {
    return false;
}

bool Input::isPressed(size_t) const {
    return false;
}

bool Input::isHeld(size_t) const {
    return false;
}

bool Input::isReleased(size_t) const {
    return false;
}

bool Input::mouseHeld(size_t) const {
    return false;
}

bool Input::mousePressed(size_t) const {
    return false;
}

bool Input::mouseReleased(size_t) const {
    return false;
}

Point Input::mousePos() const {
    return Point(0.0, 0.0);
}

Point Input::mouseToWorld(Core &) const {
    return Point(0.0, 0.0);
}
