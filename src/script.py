import time
import math
import random
import sorrow
from sorrow import *

def clamp(low, high, x):
    return min(high, max(low, x))

def putPhys(ent, x, y, w, h, static, shape, density=1.0, elasticity=None):
    phys = ent.getPhys()
    phys.pos = Vec2(x, y)
    phys.rad = Vec2(w, h) / 2.0 if "box" == shape else Vec2(w, h)
    phys.area = w + h
    phys.elasticity = elasticity if elasticity else (0.1 if static else 0.95)
    phys.phased = False
    phys.gather = False
    phys.isStatic = static
    phys.shape = shape
    if static:
        phys.mass = 0.0
    else:
        phys.mass = density * (w * h if "box" == shape else math.pi * w * h)

def putVis(ent, r, g, b):
    vis = ent.getVis()
    vis.colour = Vec3(r, g, b)
    vis.draw = True

def putWall(core, x, y, w, h):
    e = core.entities.create()
    putPhys(e, x, y, w, h, True, "box")
    putVis(e, 0x88, 0x88, 0x88)
    return e

def easyWall(core, left, top, right, bot):
    left, right = min(left, right), max(left, right)
    top, bot = max(top, bot), min(top, bot)
    x = (left + right) / 2.0
    y = (top + bot) / 2.0
    return putWall(core, x, y, abs(right - left), abs(top - bot))

def randomAroundCircle(x, y, rad):
    theta = random.random() * 2.0 * math.pi
    return (x + rad * math.cos(theta), y + rad * math.sin(theta))

def randomInCircle(x, y, rad):
    return randomAroundCircle(x, y, rad * math.sqrt(random.random()))

def putSwarm(core, xx, yy, name, k, attractor, post=None):
    sr = 8
    ran = 2
    for x in range(-ran, ran):
        for y in range(-ran, ran):
            a = core.entities.create()
            kk = k * 0.5
            putVis(a, kk.x, kk.y, kk.z)
            putPhys(a, xx + x * sr * 2.1, yy + y * sr * 2.1, sr * 10.1, sr * 10.1, False, "circle")
            phys = a.getPhys()
            phys.phased = True
            a.getVis().depth = -1.0
            a.getVis().transparency = 0.5

            e = core.entities.create()
            putVis(e, k.x, k.y, k.z)
            putPhys(e, xx + x * sr * 2.1, yy + y * sr * 2.1, sr, sr, False, "circle")
            e.getPhys().gather = True
            log = e.getLog()
            log["group"] = name
            log["controller"] = "drone"
            log["attractor"] = attractor
            log["size"] = sr
            log["aura"] = a

            def death(core, d):
                aura = d.getLog()["aura"]
                core.entities.kill(aura)
                core.physics.unbind(d.id(), aura.id())
            log["onDeath"] = death

            core.physics.bind(e.id(), a.id(), Vec2())

    def post_update(core, swarm):
        for e in swarm:
            e.getPhys().rad = Vec2(e.getLog()["size"])
        if post: post(core, swarm)

    def group_death(core, name):
        print("Swarm", name, "has been vanquished!")

    main = __import__(__name__)
    setattr(main, "control_post_" + name, post_update)
    setattr(main, "control_death_" + name, group_death)

def setup(core):
    rad = 100.0
    clear = 5.0
    width = core.renderer.getWidth()
    height = core.renderer.getHeight()
    midX = width / 2.0
    midY = height / 2.0

    easyWall(core, 200, 200, 250, 250)
    easyWall(core, 0 - rad, 0, width + rad, 0 - rad)
    easyWall(core, 0 - rad, height, width + rad, height + rad)
    easyWall(core, 0 - rad, 0 - rad, 0, height + rad)
    easyWall(core, width, 0 - rad, width + rad, height + rad)

    for i in range(0):
        left = 250 + i * 5
        top = midY + i * 7 - 7 * 31
        easyWall(core, left, top, left + 10, top + 10)
        top = midY - i * 7 + 7 * 31
        easyWall(core, left, top, left + 10, top + 10)

        left = width - 250 - i * 5
        top = midY - i * 7 + 7 * 31
        easyWall(core, left, top, left + 10, top + 10)
        top = midY + i * 7 - 7 * 31
        easyWall(core, left, top, left + 10, top + 10)

    def mouseAttract(core, name):
        return core.visuals.screenToWorld(core.input.mousePos())

    def targetPlayer(core, name):
        swarm = core.logic.getGroup("player")
        com = Vec2()
        for d in swarm:
            com += d.getPhys().pos
        com /= len(swarm)
        return com

    def cameraMover(core, swarm):
        if not swarm: return
        com = Vec2()
        for d in swarm:
            com += d.getPhys().pos
        com /= len(swarm)
        com -= core.visuals.getFOV() / 2.0
        interp = 0.1
        core.visuals.setCam(core.visuals.getCam() * (1 - interp) + com * interp)

    putSwarm(core, midX, midY, "A", Vec3(0xFF, 0, 0), targetPlayer)
    putSwarm(core, midX, midY, "player", Vec3(0, 0xFF, 0), mouseAttract, cameraMover)

def control_drone(core, ent):
    log = ent.getLog()
    phys = ent.getPhys()

    swarm = core.logic.getGroup(log["group"])

    averad = phys.rad[0]
    for d in swarm:
        averad += d.getPhys().rad[0]

    averad /= len(swarm)
    com = Vec2()
    avoid = Vec2()
    match = Vec2()
    if 1 != len(swarm):
        for d in swarm:
            if d.id() == ent.id(): continue
            p = d.getPhys()
            com += p.pos
            shy = phys.rad[0] * 3
            if (p.pos - phys.pos).length2() < shy * shy:
                avoid -= (p.pos - phys.pos)
            match += p.vel

        com /= len(swarm) - 1
        com = com - phys.pos
        match /= len(swarm) - 1

    attract = log["attractor"](core, log["group"]) - phys.pos
    phys.acc += com * 0.01
    phys.acc += avoid
    phys.acc += match * pow(1.0 / 8.0, 3)
    phys.acc += attract * 0.05

    for k in phys.contacts:
        other = core.entities.getHandle(k.which)
        elog = other.getLog()
        if (not "controller" in elog) or (elog["controller"] != "drone"): continue
        if elog["group"] == log["group"]: continue

        log["size"] *= min(1, log["size"] / elog["size"]) * 0.99
        if log["size"] < 1:
            core.entities.kill(ent)

if __name__ == '__main__':
    random.seed(0x88888888)

