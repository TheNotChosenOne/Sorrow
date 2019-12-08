#pragma once

#include "game/game.h"

class HallGame: public Game {
public:
    HallGame();
    ~HallGame();

    void registration(Core &core);
    void create(Core &core);
    void cleanup(Core &core);
    bool update(Core &core);
    void unregister(Core &core);
    double gravity() const;
};
