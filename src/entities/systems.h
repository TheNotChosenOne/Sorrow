#pragma once

#include "utility/typelist.h"
#include "tracker.h"
#include "exec.h"
#include "pack.h"
#include "data.h"
#include "signature.h"
#include "utility/timers.h"

#include <utility>
#include <memory>
#include <chrono>
#include <vector>
#include <map>

struct Core;

namespace Entity {

class BaseSystem {
public:
    const std::string name;
    const Signature signature;
    BaseSystem(const std::string &name, const Signature sig);
    virtual ~BaseSystem();
    virtual void execute(Core &core, double seconds) = 0;
    virtual void init(Core &core);
};

class SystemManager {
    std::vector< std::unique_ptr< BaseSystem > > systems;
    std::vector< AccumulateTimer > timers;

public:
    void addSystem(std::unique_ptr< BaseSystem > system);
    void execute(Core &core, double seconds);
    void init(Core &core);
    void dumpTimes();
};

}
