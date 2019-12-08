#pragma once

struct Core;

class Game {
public:
    Game();
    virtual ~Game();

    virtual void registration(Core &core) = 0;
    virtual void create(Core &core) = 0;
    virtual void cleanup(Core &core) = 0;
    virtual void unregister(Core &core) = 0;
    virtual bool update(Core &core) = 0;
    virtual double gravity() const;
};

class SwarmGame: public Game {
public:
    SwarmGame();
    ~SwarmGame();

    void registration(Core &core);
    void create(Core &core);
    void cleanup(Core &core);
    bool update(Core &core);
    void unregister(Core &core);
};
