#pragma once

#include "core/geometry.h"
#include "entities/data.h"
#include "entities/tracker.h"
#include "entities/systems.h"

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

struct Turret2 {
    double cooldown_length;
    double cooldown;
    double range;
    double dmg;
    double lifetime;
};
DeclareDataType(Turret2);

struct Seeker {
    Entity::EntityID target;
};
DeclareDataType(Seeker);

class DamageSystem: public Entity::BaseSystem {
    public:
    DamageSystem();
    ~DamageSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};

class SeekerSystem: public Entity::BaseSystem {
    public:
    SeekerSystem();
    ~SeekerSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};

class LifetimeSystem: public Entity::BaseSystem {
    public:
    LifetimeSystem();
    ~LifetimeSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};

class TurretSystem: public Entity::BaseSystem {
    public:
    TurretSystem();
    ~TurretSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};
