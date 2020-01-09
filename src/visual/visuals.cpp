#include "visual/visuals.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "visual/renderer.h"
#include "physics/physics.h"
#include "core/core.h"
#include "game/npc.h"

#include <Box2D.h>

#include <utility>

namespace {

struct MinVis {
    Point p;
    Vec r;
    Point3 c;
};

}

void draw(Core &core, Entity::Tracker &tracker, Renderer &renderer, const Point position, const double scale) {
    const auto shift = VPC< b2Vec2 >(position);
    Entity::ExecSimple< const PhysBody, const Colour >::run(tracker,
    [&](const auto &, const auto &pbs, const auto &colours) {
        std::array< std::vector< MinVis >, 2 > arr;
        std::array< size_t, 2 > counts = { 0, 0 };
        for (auto &v : arr) { v.resize(pbs.size()); }

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

    const auto flag = core.getFlag< SeekerLinesFlag >();
    if (flag && flag->get().drawSeekerLines) {
        Entity::ExecSimple< const PhysBody, const Colour, const Seeker >::run(tracker,
        [&](const auto &, const auto &bodies, const auto &colours, const auto &seekers) {
            for (size_t i = 0; i < bodies.size(); ++i) {
                const auto tid = seekers[i].target;
                const auto optBody = tracker.optComponent< PhysBody >(tid);
                if (!optBody) { continue; }
                const auto src = scale * (bodies[i].body->GetPosition() - shift);
                const auto dst = scale * (optBody->get().body->GetPosition() - shift);
                renderer.drawLine(VPC< Point >(src), VPC< Point >(dst), colours[i].colour);
            }
        });
    }
}
