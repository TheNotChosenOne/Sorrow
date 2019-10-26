#pragma once

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <array>

#include "utility/utility.h"
#include "visual/renderer.h"

class RendererSDL: public Renderer {
    private:
        const static size_t POINT = 0;
        const static size_t LINE = 1;
        const static size_t CIRCLE = 2;
        const static size_t BOX = 3;
        size_t width;
        size_t height;

        SDL_Window *window;
        SDL_GLContext context;

        std::array< GLuint, 4 > programs;
        GLuint vbo;
        GLuint line_vbo;
        GLuint ibo;
        GLuint vao;
        GLuint posBuffer;
        GLuint radBuffer;
        GLuint colBuffer;
        GLuint hsdLoc;

        struct DrawCommand {
            Point3 col;
            Point pos;
            Vec rad;
            double alpha;
            double depth;
        };

        std::array< std::vector< DrawCommand >, 4 > commands;

    public:
        RendererSDL(size_t width, size_t height);
        ~RendererSDL();

        void clear() override;
        void update() override;
        void drawPoint(Point pos, Point3 col, double alpha, double depth) override;
        void drawBox(Point pos, Vec rad, Point3 col, double alpha, double depth) override;
        void drawCircle(Point pos, Vec rad, Point3 col, double alpha, double depth) override;
        void drawLine(Point pos1, Point pos2, Point3 col, double alpha, double depth) override;

        size_t getWidth() const override;
        size_t getHeight() const override;
};
