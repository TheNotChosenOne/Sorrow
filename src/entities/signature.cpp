#include "signature.h"

#include "utility/io.h"
#include "core/core.h"

#include <sstream>

std::ostream &operator<<(std::ostream &os, const Entity::TypeID &tid) {
    return (os << tid.name());
}

std::ostream &operator<<(std::ostream &os, const Entity::Signature &sig) {
    return dumpContainer(os, sig);
}

std::ostream &operator<<(std::ostream &os, const Entity::OrderedSignature &sig) {
    return dumpContainer(os, sig);
}

namespace Entity {

bool typesSubset(const Signature &super, const Signature &sub) {
    if (sub.size() > super.size()) { return false; }
    for (const auto &s : sub) {
        if (!super.count(s)) { return false; }
    }
    return true;
}

std::string signatureString(const Signature &sig) {
    std::stringstream ss;
    ss << sig;
    return ss.str();
}

std::string signatureString(const OrderedSignature &sig) {
    std::stringstream ss;
    ss << sig;
    return ss.str();
}

}

