#pragma once

#include "componentManager.h"
#include "utility.h"
#include "core.h"
#include "forwardMirror.h"

#include <boost/hana.hpp>

struct VisualComponent {
    BOOST_HANA_DEFINE_STRUCT(VisualComponent,
        (Vec3, colour),
        (double, depth),
        (bool, draw));
};

class Renderer;
class VisualManager: public ComponentManager< VisualComponent > {
    private:
        void updater(Core &core, const Components &l, Components &n);

    public:
        VisualManager();

        Vec cam;
        Vec FOV;

        void visualUpdate(Core &core);
        Vec screenToWorld(const Vec v) const;
        gmtl::Matrix33d getViewMatrix(Renderer &rend) const;
};

template<>
PyObject *toPython< VisualManager >(VisualManager &visMan);
