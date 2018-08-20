#pragma once

#include <cstddef>

#include "core/geometry.h"
#include "utility/utility.h"

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
        virtual void drawPoint(Point pos, Point3 col, double alpha=1.0, double depth=0.0);
        virtual void drawBox(Point pos, Vec rad, Point3 col, double alpha=1.0, double depth=0.0);
        virtual void drawCircle(Point pos, Vec rad, Point3 col, double alpha=1.0, double depth=0.0);
};
