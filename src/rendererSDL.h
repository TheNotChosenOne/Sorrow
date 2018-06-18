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

        enum DrawType {
            Point = 0,
            Circle = 1,
            Box = 2
        };
        struct DrawCommand {
            Vec3 col;
            Vec pos;
            Vec rad;
            double depth;
            DrawType type;
        };

        std::vector< DrawCommand > commands;

        void drawPoint(Vec pos, Vec, Vec3 col);
        void drawBox(Vec pos, Vec rad, Vec3 col);
        void drawCircle(Vec pos, Vec rad, Vec3 col);

    public:
        RendererSDL(size_t width, size_t height);
        ~RendererSDL();

        void clear() override;
        void update() override;
        void drawPoint(Vec pos, Vec3 col, double depth) override;
        void drawBox(Vec pos, Vec rad, Vec3 col, double depth) override;
        void drawCircle(Vec pos, Vec rad, Vec3 col, double depth) override;

        size_t getWidth() const override;
        size_t getHeight() const override;
};
