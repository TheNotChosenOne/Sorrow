#pragma once

#include "input.h"

#include <vector>
#include <map>

class InputSDL: public Input {
    private:
        bool quit;
        size_t width;
        size_t height;
        std::vector< bool > held;
        std::vector< bool > pressed;
        std::vector< bool > released;
        std::map< size_t, bool > mHeld;
        std::map< size_t, bool > mPressed;
        std::map< size_t, bool > mReleased;
        Point mouse;

    public:
        InputSDL(size_t width, size_t height);
        ~InputSDL();

        void update() override;
        bool shouldQuit() const override;
        bool isHeld(size_t key) const override;
        bool isPressed(size_t key) const override;
        bool isReleased(size_t key) const override;
        bool mouseHeld(size_t button) const override;
        bool mousePressed(size_t button) const override;
        bool mouseReleased(size_t button) const override;
        Point mousePos() const override;
};
