#include "quadtree.h"

#include <gmtl/MatrixOps.h>
#include <gmtl/CoordOps.h>
#include <gmtl/Generate.h>
#include <gmtl/Matrix.h>
#include <gmtl/Xforms.h>
#include <gmtl/Coord.h>

#include "physicsManager.h"
#include "visualManager.h"
#include "renderer.h"
#include "core.h"

namespace {

// 0 1
// 2 3
// inside is 4
static inline size_t partition(const Vec mid, const Vec p, const Vec r) {
    const bool bot = p[1] + r[1] < mid[1];
    const bool top = p[1] - r[1] > mid[1];
    const bool left = p[0] + r[0] < mid[0];
    const bool rite = p[0] - r[0] > mid[0];
    if ((bot || top) && (left || rite)) { 
        return static_cast< size_t >(rite) + 2 * static_cast< size_t >(bot);
    }
    return 4;
}

static inline bool contains(const Quadtree &qt, const Vec p, const Vec r) {
    return !(p[0] + r[0] < qt.mid[0] - qt.rad[0] ||
             p[0] - r[0] > qt.mid[0] + qt.rad[0] ||
             p[1] + r[1] < qt.mid[1] - qt.rad[1] ||
             p[1] - r[1] > qt.mid[1] + qt.rad[1]);
}

}

Quadtree::Quadtree(std::vector< TaggedPhys > &phys) {
    Vec mmin( infty< double >(),  infty< double >());
    Vec mmax(-infty< double >(), -infty< double >());
    for (const auto p : phys) {
        const auto pos = p.second->pos;
        const auto rad = p.second->rad;
        mmin[0] = std::min(mmin[0], pos[0] - rad[0] - 1.0);
        mmin[1] = std::min(mmin[1], pos[1] - rad[1] - 1.0);
        mmax[0] = std::max(mmax[0], pos[0] + rad[0] + 1.0);
        mmax[1] = std::max(mmax[1], pos[1] + rad[1] + 1.0);
    }

    mid = (mmax + mmin) / 2.0;
    rad = (mmax - mmin) / 2.0;

    if (phys.size() < 8) {
        for (const auto p : phys) {
            inside.push_back(p.first);
        }
        return;
    }

    const size_t pre = phys.size();
    std::vector< std::vector< TaggedPhys > > fits;
    fits.resize(4);
    for (auto f : fits) { f.reserve(phys.size()); }
    for (size_t i = 0; i < phys.size(); ++i) {
        const auto &p = phys[i];
        const auto pos = p.second->pos;
        const auto rad = p.second->rad;
        const size_t part = partition(mid, pos, rad);
        if (part < 4) {
            fits[part].push_back(std::move(p));
            phys[i--] = std::move(phys.back());
            phys.resize(phys.size() - 1);
        }
    }
    size_t count = phys.size();
    for (size_t i = 0; i < 4; ++i) {
        count += fits[i].size();
        std::vector< TaggedPhys > &sub = fits[i];
        if (!sub.empty()) {
            children[i] = std::make_unique< Quadtree >(sub);
        }
    }
    rassert(count == pre, count, pre);
    for (const auto p : phys) {
        inside.push_back(p.first);
    }
}

void Quadtree::suggest(const Vec pos, const Vec rad, std::vector< size_t > &into) const {
    if (!contains(*this, pos, rad)) { return; }
    std::copy(inside.begin(), inside.end(), std::back_inserter(into));
    for (const auto &qt : children) {
        if (qt) { qt->suggest(pos, rad, into); }
    }
}

size_t count(const Quadtree &qt) {
    size_t c = qt.inside.size();
    for (const auto &q : qt.children) {
        if (q) { c += count(*q); }
    }
    return c;
}

void Quadtree::draw(Core &core, size_t density) const {
    if (0 == density) { density = count(*this); }
    double percent = inside.size() / static_cast< double >(density);
    const gmtl::Matrix33d view = core.visuals.getViewMatrix(core.renderer);
    core.renderer.drawBox(view * gmtl::Point2d(mid), view * rad,
            Vec3(0, 127 + 128 * percent, 0));
    for (const auto &q : children) {
        if (q) { q->draw(core, density); }
    }
}

static void plumb(std::ostream &os, size_t depth) {
    for (size_t i = 0; i < depth; ++i) { os << "  "; }
}

static void dumper(std::ostream &os, const Quadtree &qt, size_t depth) {
   plumb(os, depth);
   os << qt.mid << ' ' << qt.rad << ' ' << qt.inside.size() << '\n';
   for (const auto &k : qt.children) {
        if (k) { dumper(os, *k, depth + 1); }
        else   { plumb(os, depth + 1); os << "empty\n"; }
   }
}

std::ostream &operator<<(std::ostream &os, const Quadtree &qt) {
    dumper(os, qt, 0);
    return os;
}
