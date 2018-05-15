#pragma once

#include <SDL2/SDL.h>

#include "utility.h"
#include "renderer.h"

class RendererSDL: public Renderer {
    private:
        size_t width;
        size_t height;

        SDL_Window *window;
        SDL_Surface *surface;
        SDL_Renderer *renderer;

    public:
        RendererSDL(size_t width, size_t height);
        ~RendererSDL();

        void clear() override;
        void update() override;
        void drawPoint(Vec pos, Vec3 col) override;
        void drawBox(Vec pos, Vec rad, Vec3 col) override;
        void drawCircle(Vec pos, Vec rad, Vec3 col) override;

        size_t getWidth() const override;
        size_t getHeight() const override;
};
