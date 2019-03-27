#pragma once

#include "core/geometry.h"
#include "entities/data.h"
#include "entities/tracker.h"

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

struct Turret {
    double cooldown_length;
    double cooldown;
    double range;
    double dmg;
    double lifetime;
};
DeclareDataType(Turret);

struct Seeker {
    Entity::EntityID target;
};
DeclareDataType(Seeker);

void processDamage(Core &core);
void processSeeker(Core &core);
void processLifetimes(Core &core, double seconds);
void processTurrets(Core &core, double seconds);
