#pragma once

#include "data.h"
#include "utility.h"

#include <iostream>

struct Location {
    Vec v;
};
DeclareDataType(Location);
std::ostream &operator<<(std::ostream &os, const Location &l);

struct Direction {
    Vec v;
};
DeclareDataType(Direction);
std::ostream &operator<<(std::ostream &os, const Direction &d);
