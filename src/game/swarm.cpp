#include "swarm.h"
#include "core/core.h"
#include "physics/physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "input/input.h"
#include "visual/renderer.h"

#include <map>

namespace {

void update(Core &core, std::vector< PhysBody > &pbs, const std::vector< SwarmTag > &tags) {
    const size_t drones = tags.size();

    struct SwarmInfo {
        Vec centre { 0.0, 0.0 };
        Vec heading { 0.0, 0.0 };
        size_t count { 0 };
        std::vector< size_t > indices;
    };
    std::map< uint16_t, SwarmInfo > swarms;
    for (size_t i = 0; i < drones; ++i) {
        auto &info = swarms[tags[i].tag];
        ++info.count;
        info.centre += VCast(pbs[i].body->GetPosition());
        info.heading += normalized(VCast(pbs[i].body->GetLinearVelocity()));
        info.indices.push_back(i);
    }

    for (auto &pair : swarms) {
        pair.second.centre /= pair.second.count;
        pair.second.heading = normalized(pair.second.heading);
    }

    const double favoid = core.options["avoid"].as< double >();
    const double falign = core.options["align"].as< double >();
    const double fgroup = core.options["group"].as< double >();
    double shy = core.options["bubble"].as< double >();
    shy = 4.0 * shy * shy;
    for (size_t i = 0; i < drones; ++i) {
        const auto &info = swarms[tags[i].tag];
        const Vec iAt = VCast(pbs[i].body->GetPosition());
        const Vec diff = info.centre - iAt;

        Vec avoid { 0.0, 0.0 };
        for (size_t j = 0; j < drones; ++j) {
            if (tags[j].tag != tags[i].tag || i == j) { continue; }
            const Vec diff = VCast(pbs[j].body->GetPosition()) - iAt;
            if (diff.squared_length() < shy && diff.squared_length() > 0.0) {
                avoid -= normalized(diff) * (shy - diff.squared_length()) / shy;
            }
        }

        Vec add = normalized(diff) * fgroup;
        add += normalized(info.heading) * falign;
        add += normalized(avoid) * favoid;
        pbs[i].body->ApplyForce(VCast(add), VCast(iAt), true);
    }
}

void follow(Core &core, std::vector< PhysBody > &pbs) {
    const double mousey = core.options["mouse"].as< double >();
    Point at = core.input.mousePos();
    at = Point(at.x() * core.renderer.getWidth(), at.y() * core.renderer.getHeight());
    for (size_t i = 0; i < pbs.size(); ++i) {
        const Vec iAt = VCast(pbs[i].body->GetPosition());
        const Vec diff = normalized(at - iAt);
        pbs[i].body->ApplyForce(VCast(diff * mousey),  VCast(iAt), true);
    }
}

}

void updateSwarms(Core &core) {
    Entity::ExecSimple< PhysBody, const SwarmTag >::run(core.tracker,
    [&](auto &pbs, auto &tags) {
        update(core, pbs, tags);
    });
    Entity::ExecSimple< PhysBody, const MouseFollow >::run(core.tracker,
    [&](auto &pbs, auto &) {
        follow(core, pbs);
    });
}
