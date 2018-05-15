#include "input.h"

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

bool Input::mouseHeld(uint8_t) const {
    return false;
}

bool Input::mousePressed(uint8_t) const {
    return false;
}

bool Input::mouseReleased(uint8_t) const {
    return false;
}

Vec Input::mousePos() const {
    return Vec();
}
