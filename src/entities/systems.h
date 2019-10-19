#pragma once

#include "utility/typelist.h"
#include "tracker.h"
#include "exec.h"
#include "pack.h"
#include "data.h"
#include "signature.h"
#include "utility/timers.h"

#include <boost/program_options.hpp>
#include <condition_variable>
#include <functional>
#include <utility>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <map>
#include <set>

struct Core;

namespace Entity {

class BaseSystem {
public:
    const std::string name;
    const Signature signature;
    BaseSystem(const std::string &name, const Signature &sig);
    virtual ~BaseSystem();
    virtual void execute(Core &core, double seconds) = 0;
    virtual void init(Core &core);
};

class SystemManager {
    std::map< const BaseSystem*, AccumulateTimer > timers;
    std::vector< std::unique_ptr< BaseSystem > > systems;
    std::vector< std::vector< BaseSystem* > > stages;

    std::vector< std::thread > threads;
    std::condition_variable cv;
    bool terminating;
    std::mutex tex;

    std::queue< std::function< void() > > work_queue;
    size_t processed;

    void threadJob();

public:
    SystemManager(boost::program_options::variables_map &options);
    ~SystemManager();
    void addSystem(std::unique_ptr< BaseSystem > system);
    void execute(Core &core, double seconds);
    void init(Core &core);
    void dumpTimes();
};

}
