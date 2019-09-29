#pragma once
#include "game.h"
#include "entities/systems.h"

struct Generation {
    float age;
    size_t generation;
};
DeclareDataType(Generation);

class GenerationSystem: public Entity::BaseSystem {
public:
    GenerationSystem();
    ~GenerationSystem();
    void execute(Core &core, double seconds);
    void init(Core &core);
};

class Stresser: public Game {
public:
    Stresser();
    ~Stresser();

    void registration(Core &core);
    void create(Core &core);
    void cleanup(Core &core);
    bool update(Core &core);
    void unregister(Core &core);
};
