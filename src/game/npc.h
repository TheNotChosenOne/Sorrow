#pragma once

#include "core/geometry.h"
#include "entities/data.h"

class Core;

b2Body *makeBall(Core &core, Point centre, double rad);

struct Damage {
    double dmg;
};
DeclareDataType(Damage);

struct Health {
    double hp;
    double max;
};
DeclareDataType(Health);
Health fullHealth(double hp);

struct Team {
    uint16_t team;
};
DeclareDataType(Team);

struct Lifetime {
    double seconds;
};
DeclareDataType(Lifetime);

void processDamage(Core &core);
void processLifetimes(Core &core, double seconds);
