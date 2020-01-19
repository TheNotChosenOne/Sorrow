#pragma once

#include "core/geometry.h"
#include "entities/data.h"
#include "entities/systems.h"
#include "game/npc.h"

struct SwarmTag {
    uint16_t tag;
};
DeclareDataType(SwarmTag);
struct MouseFollow { };
DeclareDataType(MouseFollow);

class SwarmSystem: public Entity::BaseSystem {
    public:
    SwarmSystem();
    ~SwarmSystem();
    void init(Core &core);
    void execute(Core &core, double seconds);
};

struct Hive {
    uint16_t tag;
    Point3 colour;
    size_t target;
    size_t actual;
    float cooldown;
    float cooldown_length;
};
DeclareDataType(Hive);

class HiveTrackerSystem: public Entity::BaseSystem {
    public:
    HiveTrackerSystem();
    ~HiveTrackerSystem();
    void init(Core &core);
    void execute(Core &core, double seconds);
};

class HiveSpawnerSystem: public Entity::BaseSystem {
    public:
    BulletCreator bulleter;
    BulletCreator missiler;
    HiveSpawnerSystem();
    ~HiveSpawnerSystem();
    void init(Core &core);
    void execute(Core &core, double seconds);
    Entity::EntityID makeSwarmer(Core &core, uint16_t tag, Point3 Colour) const;
};
