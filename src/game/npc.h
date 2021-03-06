#pragma once

#include <functional>
#include <optional>

#include "core/geometry.h"
#include "entities/data.h"
#include "entities/tracker.h"
#include "entities/systems.h"

struct Core;

b2Body *makeBall(Core &core, Point centre, double rad);
b2Body *randomBall(Core &core, double rad);
b2Body *randomBall(Core &core, Point centre, double rad);

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

typedef uint16_t TeamNumber;
struct Team {
    uint16_t team;
};
DeclareDataType(Team);

struct Lifetime {
    double seconds;
};
DeclareDataType(Lifetime);

typedef std::function< Entity::EntityID(Core &, Entity::EntityID, Point at, Vec to, std::optional< Entity::EntityID > target) > BulletCreator;
struct Turret {
    std::string name;
    BulletCreator bullet;
    double cooldown_length;
    double cooldown;
    double range;
    bool automatic;

    bool trigger();
};
DeclareMultiDataType(Turret);

struct BulletInfo {
    double lifetime;
    double radius;
    double health;
    double dmg;
    bool seeking;
};
BulletCreator getStandardBulletCreator(BulletInfo bi);


struct Seeker {
    Entity::EntityID target;
    double retargetingRange = 0.0;
    bool retargeting = false;
};
DeclareDataType(Seeker);

struct TargetValue {
    double value;
};
DeclareDataType(TargetValue);

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
