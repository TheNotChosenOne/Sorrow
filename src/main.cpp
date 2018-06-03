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
#include <ratio>

#include <valgrind/valgrind.h>

#include "renderer.h"
#include "rendererSDL.h"

#include "input.h"
#include "inputSDL.h"

#include "entityManager.h"
#include "physicsManager.h"
#include "visualManager.h"
#include "logicManager.h"

#include "timers.h"
#include "core.h"

const size_t SCREEN_WIDTH = 512;
const size_t SCREEN_HEIGHT = 512;

static void mainLoop(Core &core) {
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
    const double pps = core.options["pps"].as< size_t >();
    const double fps = core.options["fps"].as< size_t >();
    ActionTimer physTick(sprint ? 0 : 1.0 / pps);
    ActionTimer drawTick(sprint ? 0 : 1.0 / fps);
    ActionTimer infoTick(1.0);
    ActionTimer killer(std::numeric_limits< double >::infinity());

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
            const auto time = physicsUse.add([&](){ core.physics.updatePhysics(core); });
            physics.tick(time);

            // Update entity physics
            logicUse.add([&](){ core.logic.logicUpdate(core); });

            entityUse.add([&]() { core.entities.update(); });

            if (core.input.isReleased(SDLK_q)) {
                timescale *= 0.7;
                physTick.setTimeScale(timescale);
            }
            if (core.input.isReleased(SDLK_e)) {
                timescale /= 0.7;
                physTick.setTimeScale(timescale);
            }
            if (core.input.isReleased(SDLK_r)) {
                core.visuals.FOV *= 0.7;
            }
            if (core.input.isReleased(SDLK_f)) {
                core.visuals.FOV /= 0.7;
            }
        }

        if (drawTick.tick(duration)) {
            ++renderCount;
            // Update entity logic
            const auto time = visualsUse.add([&](){ core.visuals.visualUpdate(core); });
            visuals.tick(time);
        }

        if (infoTick.tick(duration)) {
            const Vec centre(core.renderer.getWidth() / 2.0, core.renderer.getHeight() / 2.0);
            double screenRad = core.renderer.getWidth() * core.renderer.getHeight();
            size_t count = 0;
            for (const Entity e : core.entities.all()) {
                const Vec diff = core.entities.getHandle(e)->getPhys().pos - centre;
                count += static_cast< size_t >(gmtl::lengthSquared(diff) > screenRad);
            }

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
                std::cout << "Entities: " << std::setw(6) << core.entities.all().size();
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

    std::unique_ptr< EntityManager > entityMan;
    entityMan = std::make_unique< EntityManager >();

    std::unique_ptr< PhysicsManager > physicsMan;
    physicsMan = std::make_unique< PhysicsManager >();
    PhysicsManager &physicsRef = *physicsMan;
    entityMan->attach(std::move(physicsMan));

    std::unique_ptr< VisualManager > visMan;
    visMan = std::make_unique< VisualManager >();
    VisualManager &visRef = *visMan;
    entityMan->attach(std::move(visMan));

    std::unique_ptr< LogicManager > logMan;
    logMan = std::make_unique< LogicManager >();
    LogicManager &logRef = *logMan;
    entityMan->attach(std::move(logMan));

    renderer->clear();
    renderer->update();

    input->update();

    Core core{ *renderer, *input, *entityMan, physicsRef, visRef, logRef, 0, options };
    entityMan->setCore(core);

    const auto putBox = [&core](double l, double b, double w, double h) {
        Entity e = core.entities.create();
        auto &phys = core.physics.get(e);
        phys.pos[0] = l;
        phys.pos[1] = b;
        phys.rad = { w / 2.0, h / 2.0 };
        phys.area = w * h;
        phys.mass = 0.0;
        phys.shape = Shape::Box;
        phys.isStatic = true;
        phys.elasticity = 0.1;
        phys.phased = false;
        phys.gather = false;
        core.visuals.get(e).draw = true;
        core.visuals.get(e).colour = Vec3(0x88, 0x88, 0x88);
    };
    const auto putCircle = [&core](double x, double y, double rad) {
        Entity e = core.entities.create();
        auto &phys = core.physics.get(e);
        phys.pos[0] = x;
        phys.pos[1] = y;
        phys.rad = { rad, rad };
        phys.area = 2 * rad;
        phys.mass = 0.0;
        phys.shape = Shape::Circle;
        phys.isStatic = true;
        phys.elasticity = 0.1;
        phys.phased = false;
        phys.gather = false;
        core.visuals.get(e).draw = true;
        core.visuals.get(e).colour = Vec3(0x88, 0x88, 0x88);
        return e;
    };

    const auto putBall = [&](double x, double y, double rad) {
        Entity e = putCircle(x, y, rad);
        auto &phys = core.physics.get(e);
        phys.isStatic = false;
        phys.mass = pi< double > * rad * rad;
        phys.elasticity = 0.99;
        core.visuals.get(e).colour = Vec3(0, 0, 0xFF);
        return e;
    };

    const auto putActor = [&](double x, double y, double rad, const std::string &control) {
        Entity e = putBall(x, y, rad);
        auto &phys = core.physics.get(e);
        phys.gather = true;
        core.visuals.get(e).colour = Vec3(0, 0, 0);
        auto &log = core.logic.get(e);
        log.setDouble("hp", 100);
        log.setDouble("speed", 900 * phys.mass);
        log.setString("controller", control);
        log.setDouble("reload", 0.0);
        log.setDouble("reloadTime", 0.05);
        log.setDouble("bulletForce", 1000);
        log.setDouble("blifetime", 5);
        return e;
    };

    const double rad = 10.0;
    const double clear = 5.0;
    const double midX = core.renderer.getWidth() / 2.0;
    const double midY = core.renderer.getHeight() / 2.0;
    putBox(midX, clear * rad, core.renderer.getWidth(), rad * 4);
    putBox(midX, core.renderer.getHeight() - clear * rad, core.renderer.getWidth(), rad * 4);
    putBox(clear * rad, midY, rad * 4, core.renderer.getHeight());
    putBox(core.renderer.getWidth() - clear * rad, midY, rad * 4, core.renderer.getHeight());
    putBox(midX, core.renderer.getHeight() / 4, core.renderer.getWidth() / 3, rad * 2);

    const size_t y = core.renderer.getHeight() / 4.0 + 5;
    Entity e = putActor(core.renderer.getWidth() / 2, y + 250, 10, "player");
    std::cout << "Player: " << e << '\n';
    auto &log = core.logic.get(e);
    log.setDouble("speed", 150);
    log.setBool("player", true);
    core.player = core.entities.getHandle(e);
    core.player->getPhys().gather =  true;

    putActor( 75, y + 250, 4, "drone");
    putActor( 90, y + 250, 4, "drone");
    putActor(105, y + 250, 4, "drone");

    std::cout << core.entities.all().size() << " entities\n";
    mainLoop(core);
}

bool getOptions(boost::program_options::variables_map &options, int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("headless",
         po::value< bool >()->default_value(RUNNING_ON_VALGRIND),
         "disable rendering and input")
        ("fps", po::value< size_t >()->default_value(STEPS_PER_SECOND), "Frames  / second")
        ("pps", po::value< size_t >()->default_value(STEPS_PER_SECOND), "Physics / second")
        ("verbose", "print more runtime info")
        ("sprint", "run as fast as possible")
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
