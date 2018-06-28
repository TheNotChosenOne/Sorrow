#pragma once

#include "data.h"

#include <gmtl/Vec.h>

typedef gmtl::Vec3d Vec3;

struct Colour { Vec3 colour; };
DeclareDataType(Colour);
