#include <SDL2/SDL.h>

#include <functional>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>
#include <thread>
#include <memory>
#include <limits>
#include <chrono>
#include <random>
#include <ratio>

#include <valgrind/valgrind.h>

#include "visual/renderer.h"
#include "visual/rendererSDL.h"
#include "visual/camera.h"
#include "input/input.h"
#include "input/inputSDL.h"

#include "entities/tracker.h"
#include "entities/exec.h"
#include "entities/systems.h"
#include "physics/physics.h"
#include "visual/visuals.h"
#include "game/swarm.h"
#include "game/controller.h"
#include "game/grid.h"
#include "game/game.h"
#include "game/npc.h"
#include "game/gol.h"

#include "utility/timers.h"
#include "core/core.h"

#include "Box2D.h"

static const size_t WORLD_SIZE = 1000.0;
static const size_t STEPS_PER_SECOND = 60;

namespace {

std::mt19937_64 rng(0x88888888);
std::uniform_real_distribution< double > distro(0.0, 1.0);

// x, y is a corner, w, h are dimensions, can be negative to work with corner
b2Body *makeWall(b2World *world, double x, double y, double w, double h) {
    b2BodyDef def;
    def.type = b2_staticBody;

    b2PolygonShape box;

    b2FixtureDef fixture;
    fixture.density = 15.0f;
    fixture.friction = 0.7f;
    fixture.shape = &box;

    if (w < 0.0) {
        w = -w;
        x -= w;
    }
    if (h < 0.0) {
        h = -h;
        y -= h;
    }
    w /= 2.0;
    h /= 2.0;
    x += w;
    y += h;

    box.SetAsBox(w, h);
    def.position.Set(x, y);

    b2Body *body = world->CreateBody(&def);
    body->CreateFixture(&fixture);
    return body;
}

void gridWalls(Core &core) {
    const size_t size = 64;
    const double prob = core.options["walls"].as< double >();
    Grid grid(size, Point(0, 0), 1000 / size, 1000 / size);
    for (size_t y = 0; y < grid.getHeight(); ++y) {
        for (size_t x = 0; x < grid.getWidth(); ++x) {
            if (distro(rng) < prob) {
                const auto corner = grid.gridOrigin(y, x);
                auto wall = makeWall(core.b2world.b2w.get(), corner.x(), corner.y(), grid.getSize(), grid.getSize());
                core.tracker.createWith< PhysBody, Colour >(core, { wall }, { { 0xFF, 0, 0xFF } });
            }
        }
    }
}

void randomWalls(Core &core) {
    const double width = WORLD_SIZE;
    const double height = WORLD_SIZE;
    const double hw = width / 2.0;
    const double hh = height / 2.0;
    const Colour wallCol { { 0xFF, 0, 0xFF } };

    for (size_t i = 0; i < core.options["walls"].as< size_t >(); ++i) {
        core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.b2w.get(), hw + rnd(40), hh + rnd(40), 16.0 * distro(rng), 16.0 * distro(rng)) },
            wallCol);
    }
}

void makeBox(Core &core, const double left, const double rite, const double top, const double bot, const double size, const Colour wallCol) {
    const double height = std::abs(top  - bot)  + 2 * size;
    const double width  = std::abs(rite - left) + 2 * size;
    // left
    core.tracker.createWith< PhysBody, Colour >(core, { makeWall( core.b2world.b2w.get(),
            left, top, -size, -height
    ) }, wallCol);
    // right
    core.tracker.createWith< PhysBody, Colour >(core, { makeWall( core.b2world.b2w.get(),
            rite, top, size, -height
    ) }, wallCol);
    // top
    core.tracker.createWith< PhysBody, Colour >(core, { makeWall( core.b2world.b2w.get(),
            left, top, width, -size
    ) }, wallCol);
    // bot
    core.tracker.createWith< PhysBody, Colour >(core, { makeWall( core.b2world.b2w.get(),
            left, bot, width, size
    ) }, wallCol);
}

void createWalls(Core &core) {
    const double wallRad = 10.0;
    const double rad = WORLD_SIZE / 2.0;
    const Colour wallCol { { 0xFF, 0, 0xFF } };

    const double border = 75.0;

    makeBox(core, -(rad + border), rad + border, rad + border, -(rad + border), wallRad, wallCol);
    /*
    // top
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.b2w.get(), -border, -(wallRad + border), -wallRad, height + 2.0 * (wallRad + border)) }, wallCol);
    // right
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.b2w.get(), width + border, -wallRad, wallRad, height + 2.0 * (wallRad + border)) }, wallCol);
    // top
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.b2w.get(), -wallRad, 0.0, width + 2.0 * wallRad, -wallRad) }, wallCol);
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.b2w.get(), -wallRad, height, width + 2.0 * wallRad, wallRad) }, wallCol);
            */
}

Entity::EntityID makePlayer(Core &core) {
    const Colour playerCol { { 0xAA, 0xAA, 0xAA } };
    const auto playerID = core.tracker.createWith(core,
        PhysBody{ randomBall(core, WORLD_SIZE / 2.0) }, playerCol, HitData{}, Controller{ KeyboardController }, Team{ 0 }, fullHealth( std::numeric_limits< double >::infinity())
    );
    rassert(core.tracker.optComponent< Team >(playerID));
    std::cout << "Player ID: " << playerID << '\n';
    return playerID;
}

Entity::EntityID makeCamera(Core &core) {
    b2BodyDef def;
    def.type = b2_staticBody;
    def.position.Set(0.0, 0.0);

    b2CircleShape circle;
    circle.m_radius = 0.0;
    b2FixtureDef fixture;
    fixture.shape = &circle;
    fixture.filter.categoryBits = 0;
    fixture.filter.maskBits = 0;

    b2Body *body = core.b2world.b2w->CreateBody(&def);
    body->CreateFixture(&fixture);
    const auto cid = core.tracker.createWith(core, PhysBody{ body }, Camera{ 1.0 });
    std::cout << "Camera ID: " << cid << '\n';
    return cid;
}

uint64_t createPlant(Core &core, double size) {
    b2Body* body = makeBall(core, Point(0.0, 0.0), size / 8.0);
    body->SetGravityScale(0.0f);
    return core.tracker.createWith(core, PhysBody{ body }, Colour{ { 0, 0xFF, 0 } });
}

}

static void mainLoop(Core &core, Game &game) {
    AccumulateTimer entityUse;
    Entity::k_entity_timer = &entityUse;

    AccumulateTimer visualsUse;
    AccumulateTimer inputUse;
    AccumulateTimer logicUse;
    AccumulateTimer actual;
    AccumulateTimer spare;

    DurationTimer visuals;
    DurationTimer logic;

    const bool sprint = core.options.count("sprint");
    const double lps = core.options["lps"].as< double >();
    const double fps = core.options["fps"].as< double >();

    ActionTimer logiTick((sprint && core.options["lps"].defaulted()) ? 0 : 1.0 / lps);
    ActionTimer drawTick((sprint && core.options["fps"].defaulted()) ? 0 : 1.0 / fps);
    ActionTimer infoTick(1.0);
    ActionTimer killer(core.options["runfor"].as< double >());

    double timescale = 1.0;

    size_t renderCount = 0;
    size_t logicCount = 0;

    std::chrono::duration< double > busyTime(0);
    auto start = std::chrono::high_resolution_clock::now();
    while (!core.input.shouldQuit()) {

    game.cleanup(core);
    game.create(core);

    //createWalls(core);
    //gridWalls(core);
    const auto playerID = makePlayer(core);
    const auto cameraID = makeCamera(core);

    while (!core.input.shouldQuit()) {
        if (game.update(core)) { break; }
        const auto now = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration< double >(now - start);
        start = now;
        spare.add(duration - busyTime);
        actual.add(duration);

        const auto busyStart = std::chrono::high_resolution_clock::now();
        if (logiTick.tick(duration)) {
            ++logicCount;
            // Update input
            inputUse.add([&](){ core.input.update(); });

            if (core.input.isReleased(SDLK_q)) {
                timescale *= 0.7;
                logiTick.setTimeScale(timescale);
            }
            if (core.input.isReleased(SDLK_e)) {
                timescale /= 0.7;
                logiTick.setTimeScale(timescale);
            }
            const auto time = logicUse.add([&](){
                core.systems.execute(core, timescale / lps);
            });
            logic.tick(time);
        }

        if (drawTick.tick(duration)) {
            ++renderCount;
            const auto time = visualsUse.add([&](){
                // Draw time kids!
                const auto &cam = core.tracker.getComponent< Camera >(cameraID);
                const auto &bod = core.tracker.getComponent< PhysBody >(cameraID);
                core.radius = cam.radius;
                core.camera = VPC< Point >(bod.body->GetPosition());
                draw(core.tracker, core.renderer, core.camera, core.scale());
                core.renderer.update();
                core.renderer.clear();
            });
            visuals.tick(time);
        }

        if (infoTick.tick(duration)) {
            const double vis = visualsUse.empty();
            const double act = actual.empty();
            const double sp = spare.empty();
            const double busy = act - sp;
            if (core.options.count("verbose") ||
                (lps * timescale) - 3.0 > logicCount ||
                fps - 3.0 > renderCount) {
                std::cout << "LPS: " << std::setw(6) << logicCount;
                std::cout << " / " << std::setw(14) << logic.perSecond() << '\n';
                std::cout << "FPS: " << std::setw(6) << renderCount;
                std::cout << " / " << std::setw(14) << visuals.perSecond() << '\n';
                std::cout << "Spare: " << sp << " Busy: " << busy;
                std::cout << ' ' << std::setw(10) << 100 * (busy / act) << "%";
                std::cout << " (" << act << ")";
                std::cout << " Timescale: " << timescale << '\n';
                std::cout << "Vis: " << vis;
                std::cout << " EM: " << entityUse.empty();
                std::cout << " IO: " << inputUse.empty() << '\n';
                std::cout << "Systems: " << logicUse.empty() << '\n';
                core.systems.dumpTimes();
                std::cout << '\n';
            }

            logicCount = 0;
            renderCount = 0;
        }

        if (killer.tick(duration)) {
            return;
        }

        busyTime = std::chrono::high_resolution_clock::now() - busyStart;

        double minSleep = std::min(logiTick.estimate(), drawTick.estimate());
        minSleep = std::min(minSleep, infoTick.estimate());
        minSleep = std::min(minSleep, killer.estimate());
        std::this_thread::sleep_for(std::chrono::duration< double >(minSleep));
    }
    }
}

static void run(boost::program_options::variables_map &options) {
    std::unique_ptr< Renderer > renderer;
    std::unique_ptr< Input > input;
    if (options["headless"].as< bool >()) {
        renderer = std::make_unique< Renderer >(options["width"].as< size_t >(), options["height"].as< size_t >());
        input = std::make_unique< Input >();
    } else {
        renderer = std::make_unique< RendererSDL >(options["width"].as< size_t >(), options["height"].as< size_t >());
        input = std::make_unique< InputSDL >(options["width"].as< size_t >(), options["height"].as< size_t >());
    }

    renderer->clear();
    renderer->update();

    input->update();

    Entity::Tracker tracker;

    std::unique_ptr< Entity::SystemManager > systems = std::make_unique< Entity::SystemManager >(options);

    b2Vec2 gravity(0.0f, -options["gravity"].as< double >());
    std::unique_ptr< b2World > world = std::make_unique< b2World >(gravity);
    Core core{ *input, tracker, *renderer, *systems, { std::mutex(), std::move(world) }, options, 10.0, Point(0.0, 0.0) };

    SwarmGame game = SwarmGame();
    game.registration(core);
    core.systems.init(core);

    mainLoop(core, game);

    game.cleanup(core);
}

bool getOptions(boost::program_options::variables_map &options, int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("headless",
         po::value< bool >()->default_value(RUNNING_ON_VALGRIND),
         "disable rendering and input")
        ("fps", po::value< double >()->default_value(STEPS_PER_SECOND), "Frames  / second")
        ("lps", po::value< double >()->default_value(STEPS_PER_SECOND), "Logic / second")
        ("j", po::value< size_t >()->default_value(0), "Thread count")
        ("width", po::value< size_t >()->default_value(1024), "Screen width")
        ("height", po::value< size_t >()->default_value(1024), "Screen height")
        ("verbose", "print more runtime info")
        ("sprint", "run as fast as possible")
        ("pyperf", po::value< double >()->default_value(infty< double >()),
                   "Show script performance monitoring information every x seconds")
        ("runfor", po::value< double >()->default_value(infty< double >()),
                   "Run only for x seconds")
        ("c", po::value< size_t >()->default_value(1024), "Dynamic object count")
        // Boid values
        ("avoid",  po::value< double >()->default_value(40.0), "Boid avoidance factor")
        ("align",  po::value< double >()->default_value(10.0), "Boid aligning factor")
        ("group",  po::value< double >()->default_value(10.0), "Boid grouping factor")
        ("bubble", po::value< double >()->default_value( 2.0), "Boid personal space")
        ("mouse",  po::value< double >()->default_value( 0.0), "Boid mouse magnetism")

        //("gravity", po::value< double >()->default_value(98.0), "Strength of gravity")
        ("gravity", po::value< double >()->default_value(0.0), "Strength of gravity")
        ("walls", po::value< double >()->default_value(0.0), "Percentage of tiles that should be walls")
        ("help", "Ask and ye shall receive");
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);
    if (options.count("help")) {
        std::cout << desc << '\n';
        return false;
    }
    return true;
}

int main(int argc, char **argv) {
    std::cout << std::fixed;
    boost::program_options::variables_map options;
    if (!getOptions(options, argc, argv)) { return 0; }
    run(options);
    return 0;
}
