#pragma once

#include "game/game.h"

class ASHGame: public Game {
public:
    ASHGame();
    ~ASHGame();

    uint64_t player_1;
    uint64_t player_2;

    void registration(Core &core);
    void create(Core &core);
    void cleanup(Core &core);
    bool update(Core &core);
    void unregister(Core &core);
};
