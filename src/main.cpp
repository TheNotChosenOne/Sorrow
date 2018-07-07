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

#include "renderer.h"
#include "rendererSDL.h"
#include "input.h"
#include "inputSDL.h"

#include "tracker.h"
#include "physics.h"
#include "visuals.h"

#include "timers.h"
#include "core.h"

const size_t SCREEN_WIDTH = 1024;
const size_t SCREEN_HEIGHT = 1024;

static const size_t STEPS_PER_SECOND = 25;

static void mainLoop(Core &core) {
    const Signature speed = getSignature< Position, Shape, Colour, Direction, Speed >();
    const Signature noSpeed = getSignature< Position, Shape, Direction, Colour >();
    core.tracker.create(speed, 4096);
    core.tracker.create(noSpeed, 128);

    std::mt19937_64 rng(0x88888888);
    std::uniform_real_distribution< double > distro(0.0, 1.0);

    core.tracker.exec< Colour >([&](auto &colours) {
        for (size_t i = 0; i < colours.size(); ++i) {
            colours[i].colour = Point3(0, 0xFF, 0);
        }
    });

    core.tracker.exec< Speed, Colour >([&](auto &speeds, auto &colours) {
        for (size_t i = 0; i < speeds.size(); ++i) {
            speeds[i].d = distro(rng);
            colours[i].colour = Point3(0xFF, 0, 0);
        }
    });

    core.tracker.exec< Position, Shape, Direction >([&](
                auto &positions, auto &shapes, auto &directions) {
        const auto rnd = [&](const double x){ return x * 2.0 * (distro(rng) - 0.5); };
        for (size_t i = 0; i < directions.size(); ++i) {
            positions[i].v = Point(512, 512) + Vec(rnd(256), rnd(256));
            shapes[i].type = ShapeType::Box;
            shapes[i].rad = Vec(10, 10);
            directions[i].v = Dir(rnd(1), rnd(1));
        }
    });

    core.tracker.exec< Speed, Shape >([&](auto &, auto &shapes) {
        for (auto &shape : shapes) { shape.type = ShapeType::Circle; }
    });
    std::cout << "Finished setup\n";

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

    Tracker tracker;
    tracker.addSource(std::make_unique< PositionData >());
    tracker.addSource(std::make_unique< DirectionData >());
    tracker.addSource(std::make_unique< ShapeData >());
    tracker.addSource(std::make_unique< SpeedData >());
    tracker.addSource(std::make_unique< ColourData >());
    Core core{ *input, tracker, *renderer, options };

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
    std::cout << "Finished\n";
    return 0;
}
