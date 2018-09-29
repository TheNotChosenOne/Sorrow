#include "tracker.h"
#include "utility/utility.h"

#include <sstream>

namespace Entity {

std::string sigToStr(const Signature &sig, const Tracker &track) {
    std::stringstream ss;
    ss << '<';
    for (const TypeID tid : sig) {
        rassert(track.sources.count(tid), tid);
        ss << ' ' << track.sources.at(tid)->type().name();
    }
    ss << " >";
    return ss.str();
}

void Tracker::addSource(SourcePtr &&ptr) {
    sources[ptr->type()] = std::move(ptr);
}

void Tracker::killEntity(Core &core, const EntityID id) {
    for (auto &pair : entities) {
        auto &ids = pair.second;
        for (size_t i = 0; i < ids.size(); ++i) {
            if (id != ids[i]) { continue; }
            const Signature &sig = pair.first;
            for (const TypeID tid : sig) {
                sources.at(tid)->deleteComponent(core, id);
            }
            for (const TypeID tid : sig) {
                sources.at(tid)->remove(id);
            }
            ids.erase(ids.begin() + i);
            return;
        }
    }
}

void Tracker::killAll(Core &core) {
    std::vector< EntityID > all;
    for (auto &pair : entities) {
        for (auto &eid : pair.second) {
            all.push_back(eid);
        }
    }
    for (const auto eid : all) {
        killEntity(core, eid);
    }
}

bool Tracker::alive(const EntityID &eid) const {
    for (const auto &pair : entities) {
        if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
            return true;
        }
    }
    return false;
}

Signature Tracker::getSignature(const EntityID &eid) const {
    for (const auto &pair : entities) {
        if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
            return pair.first;
        }
    }
    rassert(false, "Failed to find signature for:", eid);
    return Signature();
}

EntityID Tracker::createSigned(Core &core, const Signature &sig, size_t count) {
    if (0 == count) { return 0; }
    for (const TypeID tid : sig) { rassert(sources.count(tid), tid, sig); }

    const EntityID id = nextID;
    nextID += count;
    auto &v = entities[sig];
    v.reserve(v.size() + count);
    for (size_t i = 0; i < count; ++i) { v.push_back(id + i); }
    for (const TypeID tid : sig) {
        auto &v = *sources.at(tid);
        v.reserve(count);
        for (size_t i = 0; i < count; ++i) { v.add(id + i); }
    }
    for (const auto tid : sig) {
        sources[tid]->initComponent(core, id);
    }
    return id;
}

}
