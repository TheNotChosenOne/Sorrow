#include "game/swarm.h"
#include "input/controller.h"
#include "game/npc.h"
#include "core/core.h"
#include "physics/physics.h"
#include "entities/tracker.h"
#include "entities/exec.h"
#include "input/input.h"
#include "visual/visuals.h"
#include "visual/renderer.h"

#include <map>

namespace {

void update(Core &core, std::vector< PhysBody > &pbs, const std::vector< SwarmTag > &tags,
        const Entity::IDMap &, std::vector< Entity::EntityID > &) {
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
        info.centre += VPC< Vec >(pbs[i].body->GetPosition());
        info.heading += normalized(VPC< Vec >(pbs[i].body->GetLinearVelocity()));
        info.indices.push_back(i);
    }

    for (auto &pair : swarms) {
        pair.second.centre /= pair.second.count;
        pair.second.heading = normalized(pair.second.heading);
    }

    const Point centre(0.0, 0.0);
    const double tether_length = (100 + 100) / 6.0;
    const double favoid = core.options["avoid"].as< double >();
    const double falign = core.options["align"].as< double >();
    const double fgroup = core.options["group"].as< double >();
    double shy = core.options["bubble"].as< double >();
    shy = 4.0 * shy * shy;
    for (size_t i = 0; i < drones; ++i) {
        const auto &info = swarms[tags[i].tag];
        const Vec iAt = VPC< Vec >(pbs[i].body->GetPosition());
        const Vec diff = info.centre - iAt;

        Vec avoid { 0.0, 0.0 };
        for (size_t j = 0; j < drones; ++j) {
            if (tags[j].tag != tags[i].tag || i == j) { continue; }
            const Vec diff = VPC< Vec >(pbs[j].body->GetPosition()) - iAt;
            if (diff.squared_length() < shy && diff.squared_length() > 0.0) {
                avoid -= normalized(diff) * (shy - diff.squared_length()) / shy;
            }
        }

        const Vec centre_diff = VPC< Vec >(centre - iAt);
        const double centre_dist = centre_diff.squared_length();
        double centre_pull = centre_dist / tether_length;
        if (centre_dist < tether_length * tether_length) { centre_pull = 0.0; }
        //centre_pull *= centre_pull;
        //centre_pull *= centre_pull;
        Vec add = normalized(diff) * fgroup;
        add += normalized(info.heading) * falign;
        add += normalized(avoid) * favoid;
        add += normalized(centre_diff) * centre_pull * 10.0;
        pbs[i].body->ApplyForce(VPC< b2Vec2 >(add), VPC< b2Vec2 >(iAt), false);
    }
}

void follow(Core &core, std::vector< PhysBody > &pbs, std::vector< Entity::EntityID > &) {
    const double mousey = core.options["mouse"].as< double >();
    Point at = core.input.mousePos();
    at = Point(at.x() * core.renderer.getWidth(), at.y() * core.renderer.getHeight());
    for (size_t i = 0; i < pbs.size(); ++i) {
        const Vec iAt = VPC< Vec >(pbs[i].body->GetPosition());
        const Vec diff = normalized(at - iAt);
        pbs[i].body->ApplyForce(VPC< b2Vec2 >(diff * mousey),  VPC< b2Vec2 >(iAt), true);
    }
}

}

SwarmSystem::SwarmSystem()
    : BaseSystem("Swarm", Entity::getConstySignature< const SwarmTag, const MouseFollow, PhysBody, const HitData >()) {
}

SwarmSystem::~SwarmSystem() { }

void SwarmSystem::init(Core &core) {
    core.tracker.addSource< SwarmTagData >();
    core.tracker.addSource< MouseFollowData >();
}

void SwarmSystem::execute(Core &core, double) {
    std::vector< Entity::EntityID > kill;
    Entity::Exec< Entity::Packs< PhysBody, const SwarmTag, const HitData > >::run(core.tracker,
    [&](auto &pack) {
        update(core, pack.first.template get< PhysBody >(),
                     pack.first.template get< const SwarmTag >(), pack.second, kill);
    });
    Entity::ExecSimple< PhysBody, const MouseFollow >::run(core.tracker,
    [&](const auto &, auto &pbs, auto &) {
        follow(core, pbs, kill);
    });
}

HiveTrackerSystem::HiveTrackerSystem()
    : BaseSystem("Hive Tracker", Entity::getConstySignature< Hive, const SwarmTag >()) {
}

HiveTrackerSystem::~HiveTrackerSystem() { }

void HiveTrackerSystem::init(Core &core) {
    core.tracker.addSource< HiveData >();
}

void HiveTrackerSystem::execute(Core &core, double) {
    std::map< uint16_t, size_t > tag_counts;
    Entity::ExecSimple< const SwarmTag >::run(core.tracker,
    [&](const auto &, const auto &tags) {
        for (const auto &tag : tags) {
            ++tag_counts[tag.tag];
        }
    });

    Entity::ExecSimple< Hive >::run(core.tracker,
    [&](const auto &, auto &hives) {
        for (auto &hive : hives) {
            hive.actual = tag_counts[hive.tag];
        }
    });
}

Entity::EntityID makeSwarmer(Core &core, uint16_t tag, Point3 colour) {
    b2Body *body = randomBall(core, 500.0);
    return core.tracker.createWith(core,
        PhysBody{ body },
        Colour{ colour },
        HitData{},
        SwarmTag{ tag },
        fullHealth(10.0),
        Team{ tag },
        Damage{ 0.2 },
        Turret{ 2.0, rnd_range(0.0, 2.0), 60.0, 0.4, 2.0, 0.25, 0.01, true },
        Turret{ 0.2, rnd_range(0.0, 0.2), 15.0, 0.1, 0.25, 0.05, 0.0, false }
    );
}

HiveSpawnerSystem::HiveSpawnerSystem()
    : BaseSystem("Hive Spawner",
        Entity::getConstySignature< Hive >()) {
}

HiveSpawnerSystem::~HiveSpawnerSystem() { }

void HiveSpawnerSystem::init(Core &core) {
    core.tracker.addSource< HiveData >();
}

void HiveSpawnerSystem::execute(Core &core, double seconds) {
    Entity::ExecSimple< Hive >::run(core.tracker,
    [&](const auto &, auto &hives) {
        for (auto &hive : hives) {
            hive.cooldown = std::max(0.0, hive.cooldown - seconds);
            if (hive.cooldown > 0.0 || hive.actual >= hive.target) { continue; }

            makeSwarmer(core, hive.tag, hive.colour);
            hive.cooldown = hive.cooldown_length;
        }
    });
}
