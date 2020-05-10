#include "game/ash.h"

#include "game/npc.h"
#include "core/core.h"
#include "visual/camera.h"
#include "visual/visuals.h"
#include "physics/physics.h"
#include "input/controller.h"
#include "entities/tracker.h"
#include "physics/geometry.h"

namespace {

std::vector< Entity::EntityID > findScores(Core &core) {
    std::vector< Entity::EntityID > out;
    Entity::Exec< Entity::Packs< const Score > >::run(core.tracker,
    [&](const auto &scores) {
        out = scores.second;
    });
    return out;
}

}

ASHGame::ASHGame() { }
ASHGame::~ASHGame() { }

void ASHGame::registration(Core &core) {
    core.tracker.addSource< ScoreData >();
    core.tracker.addSource< ColourData >();
    core.systems.addSystem(std::make_unique< ControllerSystem >());
    core.systems.addSystem(std::make_unique< PhysicsSystem >());
    core.systems.addSystem(std::make_unique< DamageSystem >());
    core.systems.addSystem(std::make_unique< SeekerSystem >());
    core.systems.addSystem(std::make_unique< TurretSystem >());
    core.systems.addSystem(std::make_unique< LifetimeSystem >());
}

void ASHGame::unregister(Core &) {
}

void ASHGame::create(Core &core) {
    if (findScores(core).empty()) {
        core.tracker.createWith(core,
            Team{ 1 },
            Score{ 0 }
        );

        core.tracker.createWith(core,
            Team{ 2 },
            Score{ 0 }
        );
    }

    const auto bul = getStandardBulletCreator(BulletInfo{
        2.0, 0.50, 0.25, 1.0, false
    });

    const auto mis = getStandardBulletCreator(BulletInfo{
        10.0, 1.0, 0.5, 2.0, true
    });

    player_1 = core.tracker.createWith(core,
        PhysBody{ makeCircle(core, Point(-64.0, 0.0), 3.0, PhysProperties{
            .dynamic = true, .rotates = false, .category = 0x0011
        } ) },
        Colour{ { 0x00, 0xAA, 0x00 } },
        HitData{},
        Controller{ KeyboardController, Layout{
            {"up", SDLK_w},
            {"down", SDLK_s},
            {"left", SDLK_a},
            {"rite", SDLK_d},
        } },
        TurretController{ KeyboardTurretController, Layout{
            { "fire", SDLK_e },
            { "altFire", SDLK_q },
        } },
        Team{ 1 },
        Damage{ std::numeric_limits< double >::infinity() },
        TargetValue{ 1.0 },
        fullHealth(3.0),
        Turret{ "secondary", mis, 2.0, 0.0, 0.0, false },
        Turret{ "primary", bul, 0.33, 0.0, 0.0, false }
    );

    player_2 = core.tracker.createWith(core,
        PhysBody{ makeCircle(core, Point(64.0, 0.0), 3.0, PhysProperties{
            .dynamic = true, .rotates = false, .category = 0x0011
        } ) },
        Colour{ { 0x00, 0x00, 0xAA } },
        HitData{},
        Controller{ KeyboardController, Layout{
            {"up", SDLK_i},
            {"down", SDLK_k},
            {"left", SDLK_j},
            {"rite", SDLK_l},
        } },
        TurretController{ KeyboardTurretController, Layout{
            { "fire", SDLK_o },
            { "altFire", SDLK_u },
        } },
        Team{ 2 },
        Damage{ std::numeric_limits< double >::infinity() },
        TargetValue{ 1.0 },
        fullHealth(3.0),
        Turret{ "secondary", mis, 2.0, 0.0, 0.0, false },
        Turret{ "primary", bul, 0.33, 0.0, 0.0, false }
    );

    const double widths = 4.0;

    PhysProperties divider {
        .dynamic = false,
        .rotates = false,
        .category = 0x0010,
        .mask = 0x0010
    };
    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0, 0), 2, 256 + widths, divider) },
        Colour{ { 0x33, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(-128, 0), widths, 256 + widths, PhysProperties{ .dynamic = false, .rotates = false}) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(128, 0), widths, 256 + widths, PhysProperties{ .dynamic = false, .rotates = false}) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0, 128.0), 256 + widths, widths, PhysProperties{ .dynamic = false, .rotates = false}) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

    core.tracker.createWith(core,
        PhysBody{ makeRect(core, Point(0, -128.0), 256 + widths, widths, PhysProperties{ .dynamic = false, .rotates = false}) },
        Colour{ { 0xFF, 0, 0 } },
        Damage{ std::numeric_limits< double >::infinity() }
    );

}

void ASHGame::cleanup(Core &core) {
    std::vector< Entity::EntityID > ids = core.tracker.all();

    for (const auto &scoreID : findScores(core)) {
        ids.erase(std::remove(ids.begin(), ids.end(), scoreID), ids.end());
    }

    for (const auto &eid : ids) {
        core.tracker.killEntity(core, eid);
    }
}

bool ASHGame::update(Core &core) {
    const bool p1 = core.tracker.alive(player_1);
    const bool p2 = core.tracker.alive(player_2);
    if (!p1 || !p2) {
        Entity::ExecSimple< const Team, Score >::run(core.tracker,
        [&](const auto &, const auto &teams, auto &scores) {
            for (size_t i = 0; i < teams.size(); i++) {
                if (teams[i].team == 1 && p1) {
                    ++scores[i].score;
                    std::cout << "Player 1: " << scores[i].score << std::endl;
                }
                if (teams[i].team == 2 && p2) {
                    ++scores[i].score;
                    std::cout << "Player 2: " << scores[i].score << std::endl;
                }
            }
        });
        cleanup(core);
        create(core);
    }
    return false;
}
