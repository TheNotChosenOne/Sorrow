#pragma once

#include "componentManager.h"
#include "utility.h"
#include "input.h"
#include "core.h"

#include <string>
#include <map>

typedef std::map< std::string, double > LogicComponent;

class LogicManager: public ComponentManager< LogicComponent > {
    private:
        void updater(Core &core, const Components &last, Components &next);

    public:
        void logicUpdate(Core &core);
};
