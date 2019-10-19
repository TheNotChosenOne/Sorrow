#pragma once

#include <cstdlib>

#include "core/geometry.h"
#include "utility/utility.h"

// Held: True only if the button is currently down
// Pressed: True only if the button was just pressed
// Released: True only if the button just went from pressed to unpressed

struct Core;

class Input {
    public:
        Input();
        virtual ~Input();

        virtual void update();
        virtual bool shouldQuit() const;
        virtual bool isHeld(size_t key) const;
        virtual bool isPressed(size_t key) const;
        virtual bool isReleased(size_t key) const;
        virtual bool mouseHeld(size_t button) const;
        virtual bool mousePressed(size_t button) const;
        virtual bool mouseReleased(size_t button) const;
        virtual Point mousePos() const;
        virtual Point mouseToWorld(Core &core) const;
};
