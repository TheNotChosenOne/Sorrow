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
#include "entities/systems.h"
#include "physics/physics.h"
#include "visual/visuals.h"
#include "game/swarm.h"
#include "game/controller.h"
#include "game/grid.h"
#include "game/npc.h"
#include "game/gol.h"

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

double rnd_range(const double l, const double h) {
    return distro(rng) * (h - l) + l;
}


b2Body *randomBall(Core &core) {
    const Point p = Point(512, 512) + Vec(rnd(256), rnd(256));
    return makeBall(core, Point(p.x() / core.scale, p.y() / core.scale), 1.0);
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
        b2Body *body = randomBall(core);

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

        const Point3 colours[] = { { 0xFF, 0xFF, 0xFF }, { 0xFF, 0, 0 }, { 0, 0xAA, 0 }, { 0, 0, 0xFF } };
        const auto colour = colours[tag];

        const auto pusher = 10000.0 * (body->GetPosition() - (b2Vec2(512.0 / core.scale, 512.0 / core.scale)));
        body->ApplyForceToCenter(pusher, true);

        core.tracker.createWith(core,
            PhysBody{ body },
            Colour{ colour },
            HitData{},
            SwarmTag{ tag },
            fullHealth(10.0),
            Team{tag},
            Damage{ 0.2 },
            Turret{ 2.0, rnd_range(0.0, 2.0), 60.0, 0.4, 3.0 },
            Turret2{ 0.2, rnd_range(0.0, 0.2), 15.0, 0.1, 0.5 },
            MouseFollow{}
        );
    }
}

void gridWalls(Core &core) {
    const size_t size = 64;
    const double prob = core.options["walls"].as< double >();
    Grid grid(size / core.scale, Point(0, 0), SCREEN_WIDTH / size, SCREEN_HEIGHT / size);
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
    const double width = core.renderer.getWidth() / core.scale;
    const double height = core.renderer.getHeight() / core.scale;
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
            left, top, -size, height
    ) }, wallCol);
    // right
    core.tracker.createWith< PhysBody, Colour >(core, { makeWall( core.b2world.b2w.get(),
            rite, top, size, height
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
    const double width = core.renderer.getWidth() / core.scale;
    const double height = core.renderer.getHeight() / core.scale;
    const Colour wallCol { { 0xFF, 0, 0xFF } };

    const double border = 75.0;

    makeBox(core, -border, width + border, -border, height + border, wallRad, wallCol);
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
        PhysBody{ randomBall(core) }, playerCol, HitData{}, Controller{ KeyboardController }, Team{ 0 }, fullHealth( std::numeric_limits< double >::infinity())
    );
    rassert(core.tracker.optComponent< Team >(playerID));
    std::cout << "Player ID: " << playerID << '\n';
    return playerID;
}

uint64_t createPlant(Core &core, double size) {
    b2Body* body = makeBall(core, Point(0.0, 0.0), size / 8.0);
    body->SetGravityScale(0.0f);
    return core.tracker.createWith(core, PhysBody{ body }, Colour{ { 0, 0xFF, 0 } });
}

}

static void mainLoop(Core &core) {
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

    core.tracker.killAll(core);
    createSwarms(core);
    createWalls(core);
    gridWalls(core);
    const auto playerID = makePlayer(core);

    //GOL gol(core, Grid(20 / core.scale, Point(0, 0), SCREEN_WIDTH / core.scale, SCREEN_HEIGHT / core.scale), createPlant);

    while (!core.input.shouldQuit()) {
        if (!core.tracker.alive(playerID)) { break; }
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
            // Update entity logic
            const auto time = visualsUse.add([&](){
                draw(core.tracker, core.renderer, core.scale);
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
            if (core.options.count("verbose") || STEPS_PER_SECOND - 3 > logicCount) {
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
    tracker.addSource(std::make_unique< ColourData >());

    std::unique_ptr< Entity::SystemManager > systems = std::make_unique< Entity::SystemManager >(options);
    systems->addSystem(std::make_unique< ControllerSystem >());
    systems->addSystem(std::make_unique< PhysicsSystem >());
    systems->addSystem(std::make_unique< DamageSystem >());
    systems->addSystem(std::make_unique< SeekerSystem >());
    systems->addSystem(std::make_unique< TurretSystem >());
    systems->addSystem(std::make_unique< SwarmSystem >());
    systems->addSystem(std::make_unique< LifetimeSystem >());

    b2Vec2 gravity(0.0f, -options["gravity"].as< double >());
    std::unique_ptr< b2World > world = std::make_unique< b2World >(gravity);
    Core core{ *input, tracker, *renderer, *systems, { std::mutex(), std::move(world) }, options, 10.0 };

    core.systems.init(core);

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
        ("lps", po::value< double >()->default_value(STEPS_PER_SECOND), "Logic / second")
        ("j", po::value< size_t >()->default_value(std::thread::hardware_concurrency()), "Thread count")
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
