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
#include "input/input.h"
#include "input/inputSDL.h"

#include "entities/tracker.h"
#include "entities/exec.h"
#include "physics/physics.h"
#include "visual/visuals.h"
#include "game/swarm.h"
#include "game/controller.h"
#include "game/grid.h"

#include "utility/timers.h"
#include "core/core.h"

#include "Box2D.h"

const size_t SCREEN_WIDTH = 1024;
const size_t SCREEN_HEIGHT = 1024;

static const size_t STEPS_PER_SECOND = 60;

namespace {

std::mt19937_64 rng(0x88888888);
std::uniform_real_distribution< double > distro(0.0, 1.0);

double rnd(const double x) {
    return x * 2.0 * (distro(rng) - 0.5);
}

b2Body *randomBall(b2World *world, const double scale) {
    const Point p = Point(512, 512) + Vec(rnd(256), rnd(256));

    b2BodyDef def;
    def.type = b2_dynamicBody;
    def.position.Set(p.x() / scale, p.y() / scale);

    b2CircleShape circle;
    circle.m_radius = 1.0f;

    b2FixtureDef fixture;
    fixture.density = 15.0f;
    fixture.friction = 0.7f;
    fixture.shape = &circle;

    b2Body *body = world->CreateBody(&def);
    body->SetLinearVelocity(100.0 * distro(rng) * b2Vec2(rnd(1), rnd(1)));
    body->CreateFixture(&fixture);
    return body;
}

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

void createSwarms(Core &core) {
    static const size_t groupRoot = 2;
    const size_t bugs = core.options["c"].as< size_t >();
    for (size_t i = 0; i < bugs; ++i) {
        b2Body *body = randomBall(core.b2world.get(), core.scale);

        const double x = core.scale * body->GetPosition().x; // 256
        const double y = core.scale * body->GetPosition().y; // 768
        double easyX = (x - core.renderer.getWidth() / 2.0) + 256.0;
        double easyY = (y - core.renderer.getHeight() / 2.0) + 256.0;
        easyX /= core.renderer.getWidth() / 2.0;
        easyY /= core.renderer.getHeight() / 2.0;
        easyX /= 1.0 / groupRoot;
        easyY /= 1.0 / groupRoot;
        const int64_t gridX = clamp(int64_t(0), int64_t(groupRoot - 1), static_cast< int64_t >(easyX));
        const int64_t gridY = clamp(int64_t(0), int64_t(groupRoot - 1), static_cast< int64_t >(easyY));
        const uint16_t tag = gridX * groupRoot + gridY;

        const Point3 colours[] = { { 0, 0, 0 }, { 0xFF, 0, 0 }, { 0, 0xFF, 0 }, { 0, 0, 0xFF } };
        const auto colour = colours[tag];

        core.tracker.createWith(core, PhysBody{ body }, Colour{ colour }, HitData{}, SwarmTag{ tag });
    }
}

void gridWalls(Core &core) {
    const size_t size = 64;
    Grid grid(size / core.scale, Point(0, 0), SCREEN_WIDTH / size, SCREEN_HEIGHT / size);
    for (size_t y = 0; y < grid.getHeight(); ++y) {
        for (size_t x = 0; x < grid.getWidth(); ++x) {
            if (distro(rng) < 0.3) {
                const auto corner = grid.gridOrigin(y, x);
                auto wall = makeWall(core.b2world.get(), corner.x(), corner.y(), grid.getSize(), grid.getSize());
                core.tracker.createWith< PhysBody, Colour >(core, { wall }, { { 0xFF, 0, 0xFF } });
            }
        }
    }
}

void randomWalls(Core &core) {
    const double width = core.renderer.getWidth() / core.scale;
    const double height = core.renderer.getHeight() / core.scale;
    const double hw = width / 2.0;
    const double hh = height / 2.0;
    const Colour wallCol { { 0xFF, 0, 0xFF } };

    for (size_t i = 0; i < core.options["walls"].as< size_t >(); ++i) {
        core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.get(), hw + rnd(40), hh + rnd(40), 16.0 * distro(rng), 16.0 * distro(rng)) },
            wallCol);
    }
}

void createWalls(Core &core) {
    const double wallRad = 10.0;
    const double width = core.renderer.getWidth() / core.scale;
    const double height = core.renderer.getHeight() / core.scale;
    const Colour wallCol { { 0xFF, 0, 0xFF } };

    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.get(), 0.0, -wallRad, -wallRad, height + 2.0 * wallRad) }, wallCol);
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.get(), width, -wallRad, wallRad, height + 2.0 * wallRad) }, wallCol);
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.get(), -wallRad, 0.0, width + 2.0 * wallRad, -wallRad) }, wallCol);
    core.tracker.createWith< PhysBody, Colour >(core,
            { makeWall(core.b2world.get(), -wallRad, height, width + 2.0 * wallRad, wallRad) }, wallCol);
}

Entity::EntityID makePlayer(Core &core) {
    const Colour playerCol { { 0xFF, 0xFF, 0xFF } };
    const auto playerID = core.tracker.createWith< PhysBody, Colour, HitData, Controller >(core,
        { randomBall(core.b2world.get(), core.scale) }, playerCol, {}, { KeyboardController }
    );
    return playerID;
}

}

static void mainLoop(Core &core) {
    AccumulateTimer entityUse;
    Entity::k_entity_timer = &entityUse;

    initPhysics(core);

    AccumulateTimer visualsUse;
    AccumulateTimer physicsUse;
    AccumulateTimer inputUse;
    AccumulateTimer logicUse;
    AccumulateTimer actual;
    AccumulateTimer spare;

    DurationTimer visuals;
    DurationTimer physics;

    const bool sprint = core.options.count("sprint");
    const double pps = core.options["pps"].as< double >();
    const double fps = core.options["fps"].as< double >();

    ActionTimer physTick((sprint && core.options["pps"].defaulted()) ? 0 : 1.0 / pps);
    ActionTimer drawTick((sprint && core.options["fps"].defaulted()) ? 0 : 1.0 / fps);
    ActionTimer infoTick(1.0);
    ActionTimer killer(core.options["runfor"].as< double >());

    double timescale = 1.0;

    size_t renderCount = 0;
    size_t logicCount = 0;

    std::chrono::duration< double > busyTime(0);
    auto start = std::chrono::high_resolution_clock::now();
    while (!core.input.shouldQuit()) {

    core.tracker.killAll(core);
    createSwarms(core);
    createWalls(core);
    gridWalls(core);
    const auto playerID = makePlayer(core);

    while (!core.input.shouldQuit()) {
        if (!core.tracker.alive(playerID)) { break; }
        const auto now = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration< double >(now - start);
        start = now;
        spare.add(duration - busyTime);
        actual.add(duration);

        const auto busyStart = std::chrono::high_resolution_clock::now();
        if (physTick.tick(duration)) {
            ++logicCount;
            // Update input
            inputUse.add([&](){ core.input.update(); });

            if (core.input.isReleased(SDLK_q)) {
                timescale *= 0.7;
                physTick.setTimeScale(timescale);
            }
            if (core.input.isReleased(SDLK_e)) {
                timescale /= 0.7;
                physTick.setTimeScale(timescale);
            }
            const auto time = physicsUse.add([&](){
                applyControllers(core);
                updatePhysics(core);
                updateSwarms(core);
            });
            physics.tick(time);
        }

        if (drawTick.tick(duration)) {
            ++renderCount;
            // Update entity logic
            const auto time = visualsUse.add([&](){
                draw(core.tracker, core.renderer, core.scale);
                core.renderer.update();
                core.renderer.clear();
            });
            visuals.tick(time);
        }

        if (infoTick.tick(duration)) {
            size_t count = 0;

            const double phys = physicsUse.empty();
            const double vis = visualsUse.empty();
            const double act = actual.empty();
            const double sp = spare.empty();
            const double busy = act - sp;
            if (core.options.count("verbose") || STEPS_PER_SECOND - 3 > logicCount) {
                std::cout << "PPS: " << std::setw(6) << logicCount;
                std::cout << " / " << std::setw(14) << physics.perSecond() << '\n';
                std::cout << "FPS: " << std::setw(6) << renderCount;
                std::cout << " / " << std::setw(14) << visuals.perSecond() << '\n';
                std::cout << " Phys: " << phys << "  Vis: " << vis;
                std::cout << " Logic: " << logicUse.empty();
                std::cout << " EM: " << entityUse.empty();
                std::cout << " IO: " << inputUse.empty() << '\n';
                std::cout << "Spare: " << sp << " Busy: " << busy;
                std::cout << ' ' << std::setw(10) << 100 * (busy / act) << "%";
                std::cout << " (" << act << ")\n";
                std::cout << " Timescale: " << timescale;
                std::cout << " Escaped: " << count << '\n';
                std::cout << '\n';
            }

            logicCount = 0;
            renderCount = 0;
        }

        if (killer.tick(duration)) {
            return;
        }

        busyTime = std::chrono::high_resolution_clock::now() - busyStart;

        double minSleep = std::min(physTick.estimate(), drawTick.estimate());
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
        renderer = std::make_unique< Renderer >(SCREEN_WIDTH, SCREEN_HEIGHT);
        input = std::make_unique< Input >();
    } else {
        renderer = std::make_unique< RendererSDL >(SCREEN_WIDTH, SCREEN_HEIGHT);
        input = std::make_unique< InputSDL >(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    renderer->clear();
    renderer->update();

    input->update();

    Entity::Tracker tracker;
    tracker.addSource(std::make_unique< ControllerData >());
    tracker.addSource(std::make_unique< PhysBodyData >());
    tracker.addSource(std::make_unique< ColourData >());
    tracker.addSource(std::make_unique< SwarmTagData >());
    tracker.addSource(std::make_unique< MouseFollowData >());
    tracker.addSource(std::make_unique< HitDataData >());

    b2Vec2 gravity(0.0f, -options["gravity"].as< double >());
    std::unique_ptr< b2World > world = std::make_unique< b2World >(gravity);
    Core core{ *input, tracker, *renderer, std::move(world), options, 10.0 };

    mainLoop(core);
}

bool getOptions(boost::program_options::variables_map &options, int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("headless",
         po::value< bool >()->default_value(RUNNING_ON_VALGRIND),
         "disable rendering and input")
        ("fps", po::value< double >()->default_value(STEPS_PER_SECOND), "Frames  / second")
        ("pps", po::value< double >()->default_value(STEPS_PER_SECOND), "Physics / second")
        ("verbose", "print more runtime info")
        ("sprint", "run as fast as possible")
        ("pyperf", po::value< double >()->default_value(infty< double >()),
                   "Show script performance monitoring information every x seconds")
        ("runfor", po::value< double >()->default_value(infty< double >()),
                   "Run only for x seconds")
        ("c", po::value< size_t >()->default_value(64), "Dynamic object count")
        // Boid values
        ("avoid", po::value< double >()->default_value(   10.0), "Boid avoidance factor")
        ("align", po::value< double >()->default_value(10000.0), "Boid aligning factor")
        ("group", po::value< double >()->default_value(10000.0), "Boid grouping factor")
        ("bubble", po::value< double >()->default_value(  15.0), "Boid personal space")
        ("mouse", po::value< double >()->default_value( 5000.0), "Boid mouse magnetism")

        ("gravity", po::value< double >()->default_value(98.0), "Strength of gravity")
        ("walls", po::value< size_t >()->default_value(32), "Number of walls")
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
