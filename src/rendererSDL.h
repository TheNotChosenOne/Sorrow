#pragma once

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <array>

#include "utility.h"
#include "renderer.h"

class RendererSDL: public Renderer {
    private:
        size_t width;
        size_t height;

        SDL_Window *window;
        SDL_GLContext context;

        std::array< GLuint, 3 > programs;
        GLuint vbo;
        GLuint ibo;
        GLuint vao;
        GLuint posBuffer;
        GLuint radBuffer;
        GLuint colBuffer;
        GLuint hsdLoc;

        struct DrawCommand {
            Vec3 col;
            Vec pos;
            Vec rad;
            double alpha;
            double depth;
        };

        std::array< std::vector< DrawCommand >, 3 > commands;

    public:
        RendererSDL(size_t width, size_t height);
        ~RendererSDL();

        void clear() override;
        void update() override;
        void drawPoint(Vec pos, Vec3 col, double alpha, double depth) override;
        void drawBox(Vec pos, Vec rad, Vec3 col, double alpha, double depth) override;
        void drawCircle(Vec pos, Vec rad, Vec3 col, double alpha, double depth) override;

        size_t getWidth() const override;
        size_t getHeight() const override;
};
