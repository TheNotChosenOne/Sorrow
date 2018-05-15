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
        std::map< uint8_t, bool > mHeld;
        std::map< uint8_t, bool > mPressed;
        std::map< uint8_t, bool > mReleased;
        Vec mouse;

    public:
        InputSDL(size_t width, size_t height);
        ~InputSDL();

        void update() override;
        bool shouldQuit() const override;
        bool isHeld(size_t key) const override;
        bool isPressed(size_t key) const override;
        bool isReleased(size_t key) const override;
        bool mouseHeld(uint8_t button) const override;
        bool mousePressed(uint8_t button) const override;
        bool mouseReleased(uint8_t button) const override;
        Vec mousePos() const override;
};
