#include "inputSDL.h"

#include <SDL2/SDL.h>

InputSDL::InputSDL(size_t width, size_t height)
    : quit(false)
    , width(width)
    , height(height)
    , held(512, false)
    , pressed(512, false)
    , released(512, false)
{
}

InputSDL::~InputSDL() {
}

void InputSDL::update() {
    SDL_Event e;
    released.clear();
    released.resize(held.size(), false);
    pressed.clear();
    pressed.resize(held.size(), false);
    mPressed.clear();
    mReleased.clear();

    while (0 != SDL_PollEvent(&e)) {
        if (SDL_QUIT == e.type) {
            quit = true;
        } else if (SDL_KEYDOWN == e.type) {
            size_t key = e.key.keysym.sym;
            if (key < pressed.size()) {
                pressed[key] = true;
                held[key] = true;
            }
        } else if (SDL_KEYUP == e.type) {
            size_t key = e.key.keysym.sym;
            if (key < pressed.size()) {
                held[key] = false;
            }
            if (key < released.size()) {
                released[key] = true;
            }
        } else if (SDL_MOUSEMOTION == e.type) {
            mouse = { 1.0 * e.motion.x, 1.0 * e.motion.y };
        } else if (SDL_MOUSEBUTTONDOWN == e.type) {
            mHeld[e.button.button] = true;
            mPressed[e.button.button] = true;
        } else if (SDL_MOUSEBUTTONUP == e.type) {
            mHeld[e.button.button] = false;
            mReleased[e.button.button] = true;
        }
    }
}

bool InputSDL::shouldQuit() const {
    return quit || isReleased(SDLK_x);
}

bool InputSDL::isHeld(size_t key) const {
    return (key < pressed.size()) ? held[key] : false;
}

bool InputSDL::isPressed(size_t key) const {
    return (key < pressed.size()) ? pressed[key] : false;
}

bool InputSDL::isReleased(size_t key) const {
    return (key < released.size()) ? released[key] : false;
}

bool InputSDL::mouseHeld(size_t button) const {
    const auto loc = mHeld.find(button);
    return mHeld.end() == loc ? false : loc->second;
}

bool InputSDL::mousePressed(size_t button) const {
    const auto loc = mPressed.find(button);
    return mPressed.end() == loc ? false : loc->second;
}

bool InputSDL::mouseReleased(size_t button) const {
    const auto loc = mReleased.find(button);
    return mReleased.end() == loc ? false : loc->second;
}

Vec InputSDL::mousePos() const {
    return Vec( mouse[0] / width, (height - mouse[1] - 1) / height );
}
