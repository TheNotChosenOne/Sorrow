#include "data.h"

std::ostream &operator<<(std::ostream &os, const BaseData &bd) {
    return (os << '(' << bd.type() << ", " << bd.TypeName() << ')');
}
