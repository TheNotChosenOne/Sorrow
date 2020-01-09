#include "core/flags.h"

std::ostream &operator<<(std::ostream &os, const BaseFlag &flag) {
    flag.dump(os);
    return os;
}
