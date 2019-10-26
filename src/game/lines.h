#pragma once
#include "game/game.h"

class Liner: public Game {
public:
    Liner();
    ~Liner();

    void registration(Core &core);
    void create(Core &core);
    void cleanup(Core &core);
    bool update(Core &core);
    void unregister(Core &core);
};
