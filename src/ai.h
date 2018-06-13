#pragma once

#include <gmtl/Vec.h>
#include <string>
#include <memory>
#include <vector>

#include "forwardMirror.h"

class Core;
class EntityView;
typedef std::shared_ptr< EntityView > EntityHandle;
typedef gmtl::Vec2d Vec;

class AI {
    private:
        Core *core=nullptr;

    public:
        void setCore(Core &core);
        
        std::vector< EntityHandle > findIn(Vec pos, double radius, const std::string &team, bool match=true);
};

template<>
PyObject *toPython< AI >(AI &ai);
