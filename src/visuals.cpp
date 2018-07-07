#include "visuals.h"
#include "tracker.h"
#include "renderer.h"
#include "physics.h"

namespace {

struct MinVis {
    Point p;
    Vec r;
    Point3 c;
};

}

void draw(Tracker &tracker, Renderer &renderer) {
    tracker.exec< const Position, const Shape, const Colour >([&](
            auto &positions, auto &shapes, auto &colours) {
        std::array< std::vector< MinVis >, 2 > arr;
        std::array< size_t, 2 > counts = { 0, 0 };
        for (auto &v : arr) { v.resize(positions.size()); }
        for (size_t i = 0; i < positions.size(); ++i) {
            arr[shapes[i].type][counts[shapes[i].type]++] = {
                positions[i].v, shapes[i].rad, colours[i].colour
            };
        }
        for (size_t i = 0; i < counts.size(); ++i) {
            arr[i].resize(counts[i]);
        }
        for (const MinVis &mv : arr[0]) {
            renderer.drawCircle(mv.p, mv.r, mv.c);
        }
        for (const MinVis &mv : arr[1]) {
            renderer.drawBox(mv.p, mv.r, mv.c);
        }
    });
}
