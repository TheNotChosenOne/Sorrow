#pragma once

#include "componentManager.h"
#include "utility.h"
#include "core.h"

struct VisualComponent {
    Vec3 colour;
    bool draw;
};

class VisualManager: public ComponentManager< VisualComponent > {
    private:
        void updater(Core &core, const Components &l, Components &n);

    public:
        VisualManager();

        Vec cam;
        Vec FOV;

        void visualUpdate(Core &core);
        Vec screenToWorld(const Vec v) const;
};
