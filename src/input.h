#pragma once

#include <cstdlib>

#include "utility.h"

// Held: True only if the button is currently down
// Pressed: True only if the button was just pressed
// Released: True only if the button just went from pressed to unpressed

class Input {
    public:
        Input();
        virtual ~Input();

        virtual void update();
        virtual bool shouldQuit() const;
        virtual bool isHeld(size_t key) const;
        virtual bool isPressed(size_t key) const;
        virtual bool isReleased(size_t key) const;
        virtual bool mouseHeld(uint8_t button) const;
        virtual bool mousePressed(uint8_t button) const;
        virtual bool mouseReleased(uint8_t button) const;
        virtual Vec mousePos() const;
};
