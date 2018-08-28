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

#include "utility/timers.h"
#include "core/core.h"

#include "Box2D.h"

const size_t SCREEN_WIDTH = 1024;
const size_t SCREEN_HEIGHT = 1024;

static const size_t STEPS_PER_SECOND = 25;

static void mainLoop(Core &core) {
    core.tracker.create< Position, Shape, Colour, Direction, Speed, const SwarmTag, HitData >(
            core.options["c"].as< size_t >());

    std::mt19937_64 rng(0x88888888);
    std::uniform_real_distribution< double > distro(0.0, 1.0);

    std::map< size_t, size_t > tagCount;
    const auto rnd = [&](const double x){ return x * 2.0 * (distro(rng) - 0.5); };
    Entity::ExecSimple< Position, Shape, Colour, Direction, Speed, SwarmTag >::run(core.tracker,
    [&](auto &positions, auto &shapes, auto &colours, auto &directions,
        auto &speeds, auto &tags) {
        for (size_t i = 0; i < tags.size(); ++i) {
            colours[i].colour = Point3(0xFF, 0, 0);
            speeds[i].d = 5.0 * distro(rng);
            positions[i].v = Point(512, 512) + Vec(rnd(256), rnd(256));
            shapes[i].rad = Vec(10, 10);
            shapes[i].type = ShapeType::ShapeCircle;
            directions[i].v = Dir(rnd(1), rnd(1));

            int64_t gridX = static_cast< int64_t >(positions[i].v.x()) / 128;
            int64_t gridY = static_cast< int64_t >(positions[i].v.y()) / 128;
            tags[i].tag = gridX * 8 + gridY;
            ++tagCount[tags[i].tag];
        }
    });
    size_t ave = 0;
    for (auto &p : tagCount) {
        ave += p.second;
    }
    core.tracker.create< Position, Shape, Colour, Direction, Speed, SwarmTag, HitData, MouseFollow >(ave / tagCount.size());
    Entity::ExecSimple< Position, Shape, Colour, Direction, Speed, SwarmTag, const MouseFollow >::run(core.tracker,
    [&](auto &positions, auto &shapes, auto &colours, auto &directions,
        auto &speeds, auto &tags, const auto &) {
        rassert(tags.size() > 0);
        for (size_t i = 0; i < tags.size(); ++i) {
            colours[i].colour = Point3(0, 0, 0xFF);
            speeds[i].d = 5.0 * distro(rng);
            positions[i].v = Point(512, 512) + Vec(rnd(256), rnd(256));
            shapes[i].rad = Vec(10, 10);
            shapes[i].type = ShapeType::ShapeCircle;
            directions[i].v = Dir(rnd(1), rnd(1));
            tags[i].tag = 0;
        }
    });

    const double wallRad = 10.0;

    const double width = core.renderer.getWidth();
    const double height = core.renderer.getHeight();
    const double hw = width / 2.0;
    const double hh = height / 2.0;
    const Colour wallCol { { 0xFF, 0, 0xFF } };
    core.tracker.createWith< Position, Shape, Colour >(
            {{ -wallRad, hh }}, { ShapeType::ShapeBox, { wallRad, hh } }, wallCol);
    core.tracker.createWith< Position, Shape, Colour >(
            {{ width + wallRad, hh }}, { ShapeType::ShapeBox, { wallRad, hh } }, wallCol);
    core.tracker.createWith< Position, Shape, Colour >(
            {{ hw, -wallRad }}, { ShapeType::ShapeBox, { hw, wallRad } }, wallCol);
    core.tracker.createWith< Position, Shape, Colour >(
            {{ hw, height + wallRad }}, { ShapeType::ShapeBox, { hw, wallRad } }, wallCol);
    for (size_t i = 0; i < 1; ++i) {
        core.tracker.createWith< Position, Shape, Colour >(
            { Point(hw, hh) + Vec(rnd(400), rnd(400)) },
            { ShapeType::ShapeBox, { 64.0 * distro(rng), 64.0 * distro(rng) } },
            wallCol);
    }

    AccumulateTimer visualsUse;
    AccumulateTimer physicsUse;
    AccumulateTimer entityUse;
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
                updatePhysics(core);
                updateSwarms(core);
            });
            physics.tick(time);
        }

        if (drawTick.tick(duration)) {
            ++renderCount;
            // Update entity logic
            const auto time = visualsUse.add([&](){
                draw(core.tracker, core.renderer);
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
            break;
        }

        busyTime = std::chrono::high_resolution_clock::now() - busyStart;

        double minSleep = std::min(physTick.estimate(), drawTick.estimate());
        minSleep = std::min(minSleep, infoTick.estimate());
        minSleep = std::min(minSleep, killer.estimate());
        std::this_thread::sleep_for(std::chrono::duration< double >(minSleep));
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
    tracker.addSource(std::make_unique< PositionData >());
    tracker.addSource(std::make_unique< DirectionData >());
    tracker.addSource(std::make_unique< ShapeData >());
    tracker.addSource(std::make_unique< SpeedData >());
    tracker.addSource(std::make_unique< ColourData >());
    tracker.addSource(std::make_unique< SwarmTagData >());
    tracker.addSource(std::make_unique< MouseFollowData >());
    tracker.addSource(std::make_unique< HitDataData >());

    b2Vec2 gravity(0.0f, 0.0f);
    std::unique_ptr< b2World > world = std::make_unique< b2World >(gravity);
    Core core{ *input, tracker, *renderer, std::move(world), options };

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
        ("c", po::value< size_t >()->default_value(256), "Dynamic object count")
        ("avoid", po::value< double >()->default_value(2.0), "Boid avoidance factor")
        ("align", po::value< double >()->default_value(2.0), "Boid aligning factor")
        ("group", po::value< double >()->default_value(1.0), "Boid grouping factor")
        ("bubble", po::value< double >()->default_value(15.0), "Boid personal space")
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
