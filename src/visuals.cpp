#include "visuals.h"
#include "tracker.h"
#include "renderer.h"
#include "physics.h"

namespace {

struct MinVis {
    Vec p;
    Vec r;
    Vec3 c;
};

}

void draw(Tracker &tracker, Renderer &renderer) {
    tracker.exec< const Position, const Shape, const Colour >([&](
            auto &positions, auto &shapes, auto &colours) {
        std::array< std::vector< MinVis >, 2 > arr;
        for (size_t i = 0; i < positions.size(); ++i) {
            arr[shapes[i].type].push_back({ positions[i].v, shapes[i].rad, colours[i].colour });
        }
        for (const MinVis &mv : arr[0]) {
            renderer.drawCircle(mv.p, mv.r, mv.c);
        }
        for (const MinVis &mv : arr[0]) {
            renderer.drawBox(mv.p, mv.r, mv.c);
        }
    });
}
