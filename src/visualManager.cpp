#include "physicsManager.h"
#include "visualManager.h"
#include "renderer.h"

#include <gmtl/MatrixOps.h>
#include <gmtl/CoordOps.h>
#include <gmtl/Generate.h>
#include <gmtl/Matrix.h>
#include <gmtl/Xforms.h>
#include <gmtl/Coord.h>

#include <algorithm>
#include <vector>

namespace {

struct MinVis {
    Vec3 col;
    Vec pos;
    Vec rad;
};

typedef std::vector< std::vector< MinVis > > DrawLists;

static void gather(DrawLists &dl, const VisualManager::Components &comps,
                   const Core &core, const gmtl::Matrix33d view) {
    dl.clear();
    dl.resize(3);
    for (auto &l : dl) { l.reserve(comps.size()); }

    for (size_t i = 0; i < comps.size(); ++i) {
        const auto &phys = core.physics.get(i);
        const auto &vis = comps[i];
        const size_t which = static_cast< size_t >(phys.shape);
        dl[vis.draw * (which + 1)].push_back({
            vis.colour, view * gmtl::Point2d(phys.pos), view * phys.rad
        });
    }
    const auto &contacts = core.physics.get(core.player).contacts;
    for (const auto &k : contacts) {
        const double d = std::max(k.depth, 0.5);
        const Vec rad(d, d);
        dl[2].push_back({
            Vec3(0xFF, 0, 0), view * gmtl::Point2d(k.where), view * rad
        });
    }
}

};

void VisualManager::updater(Core &core, const Components &lasts, Components &next) {
    next = lasts;
    static std::vector< std::vector< MinVis > > shaped;

    Vec scaler = Vec(core.renderer.getWidth(), core.renderer.getHeight());
    scaler[0] /= FOV[0];
    scaler[1] /= FOV[1];
    gmtl::Matrix33d viewMat;
    gmtl::identity(viewMat);
    gmtl::setScale(viewMat, scaler);
    gmtl::setTrans(viewMat, -(viewMat * cam));

    gather(shaped, next, core, viewMat);
    core.renderer.clear();
    for (const MinVis &mv : shaped[1]) { // Boxes
        core.renderer.drawBox(mv.pos, mv.rad, mv.col);
    }
    for (const MinVis &mv : shaped[2]) { // Circles
        core.renderer.drawCircle(mv.pos, mv.rad, mv.col);
    }
    core.renderer.update();
}

VisualManager::VisualManager()
    : cam(0, 0)
    , FOV(256, 256) {
}

void VisualManager::visualUpdate(Core &core) {
    ComponentManager::update([this, &core](const Components &l, Components &r) {
        updater(core, l, r);
    });
}

Vec VisualManager::screenToWorld(const Vec v) const {
    const Vec scaled(FOV[0] * v[0], FOV[1] * v[1]);
    return cam + scaled;
}
