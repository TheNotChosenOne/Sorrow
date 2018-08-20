#include "swarm.h"
#include "core/core.h"
#include "physics/physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "input/input.h"
#include "visual/renderer.h"

#include <map>

void updateSwarms(Core &core) {
    Entity::ExecSimple< const Position, Direction, Speed, const SwarmTag >::run(core.tracker,
    [&](auto &positions, auto &directions, auto &speeds, auto &tags) {
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
            info.centre += Vec(positions[i].v.x(), positions[i].v.y());
            info.heading += normalized(directions[i].v.vector());
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
            const Vec diff = info.centre - Vec(positions[i].v.x(), positions[i].v.y());

            Vec avoid { 0.0, 0.0 };
            for (size_t j = 0; j < drones; ++j) {
                if (tags[j].tag != tags[i].tag || i == j) { continue; }
                const Vec diff = positions[j].v - positions[i].v;
                if (diff.squared_length() < shy && diff.squared_length() > 0.0) {
                    avoid -= normalized(diff) * (shy - diff.squared_length()) / shy;
                }
            }

            Vec add = normalized(diff) * fgroup;
            add += normalized(info.heading) * falign;
            add += normalized(avoid) * favoid;
            directions[i].v = Dir(add);
            speeds[i].d = std::sqrt(add.squared_length());
        }
    });
    Entity::ExecSimple< const Position, Direction, const MouseFollow >::run(core.tracker,
    [&](auto &positions, auto &directions, auto &) {
        Point at = core.input.mousePos();
        at = Point(at.x() * core.renderer.getWidth(), at.y() * core.renderer.getHeight());
        for (size_t i = 0; i < directions.size(); ++i) {
            const Vec diff = normalized(at - positions[i].v);
            directions[i].v = Dir(normalized(directions[i].v.vector()) + diff * 0.5);
        }
    });
}
