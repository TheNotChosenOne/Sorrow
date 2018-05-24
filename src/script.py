def control(d):
    if "player" in d and d["player"]:
        phys = d["phys"]
        phys.pos = Vec2((phys.pos.x, 200))
