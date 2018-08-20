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

EntityID Tracker::createSigned(const Signature &sig, size_t count) {
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
    return id;
}

}
