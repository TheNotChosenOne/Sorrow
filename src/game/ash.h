#pragma once

#include "game/game.h"

class ASHGame: public Game {
public:
    ASHGame();
    ~ASHGame();

    void registration(Core &core);
    void create(Core &core);
    void cleanup(Core &core);
    bool update(Core &core);
    void unregister(Core &core);
};
