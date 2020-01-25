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
#include <coz.h>

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
#include "game/ash.h"
#include "input/controller.h"
#include "game/grid.h"
#include "game/game.h"
#include "game/npc.h"
#include "game/stress.h"
#include "game/lines.h"
#include "game/hall.h"

#include "utility/timers.h"
#include "core/core.h"

static const size_t WORLD_SIZE = 1000.0;
static const size_t STEPS_PER_SECOND = 60;

namespace {

std::mt19937_64 rng(0x88888888);
std::uniform_real_distribution< double > distro(0.0, 1.0);

}

static size_t mainLoop(Core &core, Game &game) {
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
    size_t logic_steps = 0;

    std::cout << "Core types: " << core.tracker.getRegisteredTypes() << std::endl;

    std::chrono::duration< double > busyTime(0);
    auto start = std::chrono::high_resolution_clock::now();
    while (!core.input.shouldQuit()) {

    game.cleanup(core);
    core.tracker.killAll(core);
    game.create(core);
    core.tracker.graduate();

    //createWalls(core);
    //gridWalls(core);
    //makePlayer(core);
    //const auto cameraID = makeCamera(core);
    const Entity::EntityID cameraID = 0;

    while (!core.input.shouldQuit()) {
        if (game.update(core)) { break; }
        const auto now = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration< double >(now - start);
        start = now;
        spare.add(duration - busyTime);
        actual.add(duration);

        const auto busyStart = std::chrono::high_resolution_clock::now();
        if (logiTick.tick(duration)) {
            COZ_BEGIN("LOGIC");
            ++logicCount;
            // Update input
            inputUse.add([&](){ core.input.update(); });

            if (core.input.isReleased(SDLK_t)) {
                timescale *= 0.7;
                logiTick.setTimeScale(timescale);
            }
            if (core.input.isReleased(SDLK_g)) {
                timescale /= 0.7;
                logiTick.setTimeScale(timescale);
            }
            const auto time = logicUse.add([&](){
                core.systems.execute(core, 1.0 / lps);
                core.tracker.graduate();
            });
            logic.tick(time);
            ++logic_steps;
            COZ_END("LOGIC");
        }

        if (drawTick.tick(duration)) {
            ++renderCount;
            const auto time = visualsUse.add([&](){
                // Draw time kids!
                if (cameraID > 0) {
                    const auto &cam = core.tracker.getComponent< Camera >(cameraID);
                    const auto &bod = core.tracker.getComponent< PhysBody >(cameraID);
                    core.radius = cam.radius;
                    core.camera = VPC< Point >(bod.body->GetPosition());
                }
                draw(core, core.tracker, core.renderer, core.camera, core.scale());
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
                std::cout << "Systems: " << logicUse.empty();
                std::cout << " for " << core.tracker.count() << " entites\n";
                core.systems.dumpTimes();
                std::cout << '\n';
            }

            logicCount = 0;
            renderCount = 0;
        }

        if (killer.tick(duration)) {
            return logic_steps;
        }

        busyTime = std::chrono::high_resolution_clock::now() - busyStart;

        double minSleep = std::min(logiTick.estimate(), drawTick.estimate());
        minSleep = std::min(minSleep, infoTick.estimate());
        minSleep = std::min(minSleep, killer.estimate());
        std::this_thread::sleep_for(std::chrono::duration< double >(minSleep));
    }
    }
    return logic_steps;
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

    std::unique_ptr< Game > game;
    const auto gameChoice = options["game"].as< std::string >();
    if ("hall" == gameChoice)  {
        game = std::make_unique< HallGame >();
    } else if ("lines" == gameChoice) {
        game = std::make_unique< Liner >();
    } else if ("swarm" == gameChoice) {
        game = std::make_unique< SwarmGame >();
    } else if ("stress" == gameChoice) {
        game = std::make_unique< Stresser >();
    } else if ("ash" == gameChoice) {
        game = std::make_unique< ASHGame >();
    } else {
        std::cerr << "Not a known game: " << gameChoice << std::endl;
        return;
    }

    b2Vec2 gravity(0.0f, game->gravity());
    std::unique_ptr< b2World > world = std::make_unique< b2World >(gravity);
    Core core{ *input, tracker, *renderer, *systems, { std::mutex(), std::move(world) }, options, 128, Point(0.0, 0.0), Core::FlagMap() };

    if (core.options.count("showSeeking")) {
        core.setFlag(SeekerLinesFlag{ true });
    }

    game->registration(core);
    core.systems.init(core);
    if (core.options.count("verbose")) {
        std::cout << "Using " << core.tracker.sourceCount() << " sources" << std::endl;
    }
    const auto steps = mainLoop(core, *game);
    std::cout << "Ran " << steps << " logical steps." << std::endl;

    game->cleanup(core);
}

bool getOptions(boost::program_options::variables_map &options, int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("game", po::value< std::string >()->default_value("hall"), "What game to use")
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
        ("showSeeking", "Show seeker targets")

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
