#include "data.h"

std::ostream &operator<<(std::ostream &os, const TypeID &tid) {
    return (os << tid.name());
}

std::ostream &operator<<(std::ostream &os, const BaseData &bd) {
    return (os << '(' << bd.type() << ", " << bd.type().name() << ')');
}
