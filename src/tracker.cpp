#include "tracker.h"
#include "utility.h"

#include <sstream>

std::string sigToStr(const Signature &sig, const Tracker &track) {
    std::stringstream ss;
    ss << '<';
    for (const TypeID tid : sig) {
        rassert(track.sources.count(tid), tid);
        ss << ' ' << track.sources.at(tid)->TypeName();
    }
    ss << " >";
    return ss.str();
}

EntityID Tracker::create(const Signature &sig) {
    for (const TypeID tid : sig) { rassert(sources.count(tid), tid, sig); }

    const EntityID id = nextID++;
    if (!entities.count(sig)) {
        std::cout << "Creating entity group: " << sigToStr(sig, *this) << '\n';
    }
    entities[sig].push_back(id);
    for (const TypeID tid : sig) {
        sources.at(tid)->add(id);
    }
    return id;
}
