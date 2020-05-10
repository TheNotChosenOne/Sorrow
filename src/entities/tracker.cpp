#include "entities/tracker.h"
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

Signature Tracker::getDuplicates(const OrderedSignature &sig) const {
    Signature dupes;
    for (size_t i = 0; i < sig.size(); ++i) {
        const auto tid = sig[i];
        for (size_t j = i + 1; j < sig.size(); ++j) {
            if (tid == sig[j]) {
                dupes.insert(tid);
                break;
            }
        }
    }
    return dupes;
}

void Tracker::withReadLock(const std::function< void() > &func) {
    std::shared_lock lock(tex);
    func();
}

void Tracker::withWriteLock(const std::function< void() > &func) {
    std::unique_lock lock(tex);
    func();
}

Signature Tracker::getRegisteredTypes() const {
    Signature sig;
    for (const auto &pair: sources) {
        sig.insert(pair.first);
    }
    return sig;
}

void Tracker::killEntity(Core &, const EntityID id) {
    std::unique_lock lock(tex);
    doomed.insert(id);
}

void Tracker::finalizeKills(Core &core) {
    const auto killGroup = [&](Entities &ents, Sources &srcs) {
        for (auto &pair : ents) {
            auto &ids = pair.second;
            for (size_t i = 0; i < ids.size();) {
                const EntityID eid = ids[i];
                if (doomed.end() == doomed.find(eid)) {
                    ++i;
                    continue;
                }
                const Signature &sig = pair.first;
                for (const TypeID tid : sig) {
                    srcs.at(tid)->deleteComponent(core, eid);
                }
                for (const TypeID tid : sig) {
                    srcs.at(tid)->remove(eid);
                }
                ids[i] = ids[ids.size() - 1];
                ids.resize(ids.size() - 1);
            }
        }
    };

    killGroup(entities, sources);
    killGroup(nursery, nurserySources);

    doomed.clear();
}

void Tracker::killAll(Core &core) {
    std::vector< EntityID > eids = all();
    for (const auto eid : eids) {
        killEntity(core, eid);
    }
}

size_t Tracker::count() const {
    std::shared_lock lock(tex);
    size_t total = 0;
    for (const auto &group : entities) {
        total += group.second.size();
    }
    for (const auto &group : nursery) {
        total += group.second.size();
    }
    return total;
}

bool Tracker::alive(const EntityID &eid) const {
    std::shared_lock lock(tex);
    return aliveWithLock(eid);
}

bool Tracker::zombie(const EntityID &eid) const {
    std::shared_lock lock(tex);
    return doomed.end() != doomed.find(eid);
}

bool Tracker::aliveWithLock(const EntityID &eid) const {
    for (const auto &pair : entities) {
        if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
            return true;
        }
    }
    for (const auto &pair : nursery) {
        if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
            return true;
        }
    }
    return false;
}

size_t Tracker::sourceCount() const {
    return sources.size();
}

std::pair< Signature, bool > Tracker::getSignature(const EntityID &eid) const {
    std::shared_lock lock(tex);
    for (const auto &pair : entities) {
        if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
            return std::make_pair(pair.first, true);
        }
    }
    for (const auto &pair : nursery) {
        if (pair.second.end() != std::find(pair.second.begin(), pair.second.end(), eid)) {
            return std::make_pair(pair.first, false);
        }
    }
    rassert(false, "Failed to find signature for:", eid);
    return std::make_pair(Signature(), false);
}

EntityID Tracker::createSigned(Core &core, const Signature &sig, size_t count) {
    if (0 == count) { return 0; }
    for (const TypeID tid : sig) { rassert(sources.count(tid), tid, sig); }

    const EntityID id = nextID;
    nextID += count;
    auto &v = nursery[sig];
    v.reserve(v.size() + count);
    for (size_t i = 0; i < count; ++i) { v.push_back(id + i); }
    for (const TypeID tid : sig) {
        auto &v = *nurserySources.at(tid);
        v.reserve(count);
        for (size_t i = 0; i < count; ++i) { v.add(id + i); }
    }
    for (const auto tid : sig) {
        nurserySources[tid]->initComponent(core, id);
    }
    return id;
}

void Tracker::graduate() {
    std::shared_lock lock(tex);
    for (auto &[tid, srcPtr] : sources) {
        srcPtr->graduateFrom(*nurserySources[tid]);
    }

    for (auto &[sig, list] : nursery) {
        auto &graduated = entities[sig];
        graduated.reserve(graduated.size() + list.size());
        graduated.insert(graduated.end(), list.begin(), list.end());
    }
    nursery.clear();
}

std::vector< EntityID > Tracker::all() const {
    std::vector< EntityID > out;
    std::shared_lock lock(tex);
    for (auto &pair : entities) {
        for (auto &eid : pair.second) {
            out.push_back(eid);
        }
    }
    for (auto &pair : nursery) {
        for (auto &eid : pair.second) {
            out.push_back(eid);
        }
    }
    return out;
}

}
