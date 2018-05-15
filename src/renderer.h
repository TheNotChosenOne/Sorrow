#pragma once

#include <cstddef>

#include "utility.h"

class Renderer {
    private:
        size_t width;
        size_t height;

    public:
        Renderer(size_t width, size_t height);
        virtual ~Renderer();

        virtual void clear();
        virtual void update();
        virtual size_t getWidth() const;
        virtual size_t getHeight() const;
        virtual void drawPoint(Vec pos, Vec3 col);
        virtual void drawBox(Vec pos, Vec rad, Vec3 col);
        virtual void drawCircle(Vec pos, Vec rad, Vec3 col);
};
