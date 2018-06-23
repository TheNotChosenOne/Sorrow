#include "test.h"

std::ostream &operator<<(std::ostream &os, const Location &l) {
    return (os << l.v);
}

std::ostream &operator<<(std::ostream &os, const Direction &d) {
    return (os << d.v);
}
