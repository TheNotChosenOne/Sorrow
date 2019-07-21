#include "visuals.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "renderer.h"
#include "physics/physics.h"

#include <Box2D.h>

#include <utility>

namespace {

struct MinVis {
    Point p;
    Vec r;
    Point3 c;
};

}

using DrawPack = Entity::Packs< const PhysBody, const Colour >;
void draw(Entity::Tracker &tracker, Renderer &renderer, const Point position, const double scale) {
    Entity::Exec< DrawPack >::run(tracker, [&](auto &p) {
        const auto &pbs = p.first.template get< const PhysBody >();
        const auto &colours = p.first.template get< const Colour >();
        std::array< std::vector< MinVis >, 2 > arr;
        std::array< size_t, 2 > counts = { 0, 0 };
        for (auto &v : arr) { v.resize(pbs.size()); }

        const auto shift = VPC< b2Vec2 >(position);
        for (size_t i = 0; i < pbs.size(); ++i) {
            b2Body *body = pbs[i].body;
            const b2Fixture *fixes = body->GetFixtureList();
            const b2Shape *shape = fixes->GetShape();
            const size_t index = b2Shape::Type::e_polygon == shape->GetType();
            MinVis &mv = arr[index][counts[index]++];
            mv.p = VPC< Point >(scale * (body->GetPosition() - shift));
            mv.r = scale * Vec(shape->m_radius, shape->m_radius);
            mv.c = colours[i].colour;
            if (1 == index) {
                const b2PolygonShape *box = dynamic_cast< const b2PolygonShape * >(shape);
                mv.r = scale * VPC< Vec >(box->GetVertex(2));
            }
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
