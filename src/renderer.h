#pragma once

#include <cstddef>

#include "utility.h"
#include "forwardMirror.h"

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
        virtual void drawPoint(Vec pos, Vec3 col, double alpha=1.0, double depth=0.0);
        virtual void drawBox(Vec pos, Vec rad, Vec3 col, double alpha=1.0, double depth=0.0);
        virtual void drawCircle(Vec pos, Vec rad, Vec3 col, double alpha=1.0, double depth=0.0);
};

template<>
PyObject *toPython< Renderer >(Renderer &r);
