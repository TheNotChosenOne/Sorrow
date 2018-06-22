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
    e.getVis().depth = -0.9
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

def putSwarm(core, xx, yy, name, k, attractor, ran, post=None):
    sr = 4
    spawned = 0
    for x in range(-ran, ran):
        for y in range(-ran, ran):
            spawned += 1
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
            putPhys(e, xx + x * sr * 2.1, yy + y * sr * 2.1, sr * 0.02, sr * 0.02, False, "circle")
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

    def pre_update(core, data):
        swarm = core.logic.getGroup(name)
        if not swarm: return

        c = Vec2()
        for e in swarm: c += e.getPhys().pos
        data["c"] = c / len(swarm)

        data["a"] = swarm[0].getLog()["attractor"](core, name)

    def post_update(core, data):
        swarm = core.logic.getGroup(name)
        for e in swarm:
            e.getPhys().rad = Vec2(e.getLog()["size"])
        if post: post(core, swarm)

    def group_death(core, data):
        print("Swarm", name, "has been vanquished!")

    main = __import__(__name__)
    setattr(main, "control_pre_" + name, pre_update)
    setattr(main, "control_post_" + name, post_update)
    setattr(main, "control_death_" + name, group_death)
    print("Created swarm %s: %d" % (name, spawned))

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

    spacing = 400 * 0.2401
    wallRad = 30 * 0.2401
    xx = 100
    while xx < width - 100:
        yy = 100
        while yy < height - 100:
            easyWall(core, xx - wallRad, yy - wallRad, xx + wallRad, yy + wallRad)
            yy += spacing
        xx += spacing

    def mouseAttract(core, name):
        return core.visuals.screenToWorld(core.input.mousePos())

    def targetPlayer(core, name):
        if core.logic.hasGroup("player"):
            swarm = core.logic.getGroup("player")
            if swarm:
                com = Vec2()
                for d in swarm:
                    com += d.getPhys().pos
                com /= len(swarm)
                return com
        return Vec2(core.renderer.getWidth(), core.renderer.getHeight()) / 2.0

    def cameraMover(core, swarm):
        if not swarm:
            cent = Vec2(core.renderer.getWidth(), core.renderer.getHeight()) / 2.0
            cent -= core.visuals.getFOV() / 2.0
            core.visuals.setCam(cent)
            return
        com = Vec2()
        for d in swarm:
            com += d.getPhys().pos
        com /= len(swarm)
        com -= core.visuals.getFOV() / 2.0
        interp = 0.1
        core.visuals.setCam(core.visuals.getCam() * (1 - interp) + com * interp)

    #putSwarm(core, 40, 40, "A", Vec3(0xFF, 0, 0), targetPlayer, 8)
    putSwarm(core, midX, midY, "player", Vec3(0, 0xFF, 0), mouseAttract, 16, cameraMover)

def droneDamage(core, ent, log, phys, group):
    for k in phys.contacts:
        other = core.entities.getHandle(k.which)
        elog = other.getLog()
        if (not "controller" in elog) or (elog["controller"] != "drone"): continue
        if elog["group"] == group: continue

        log["size"] *= min(1, log["size"] / elog["size"]) * 0.99
        if log["size"] < 0.2:
            core.entities.kill(ent)

multiplier = 10
def droneSeek(core, phys, group):
    data = core.logic.getGroupData(group)
    phys.acc += (data["c"] - phys.pos) * multiplier * 10
    phys.acc += (data["a"] - phys.pos) * multiplier * 10

def droneAlign(core, phys, group):
    pos = phys.pos
    localSwarm = core.ai.findIn(pos, 50, group, True)
    if len(localSwarm) > 1:
        avoid = Vec2()
        match = Vec2()
        shy2 = pow(phys.rad[0] * 3, 2)

        for d in localSwarm:
            p = d.getPhys()
            diff = p.pos - pos
            if diff.length2() < shy2:
                avoid -= diff
            match += p.vel

        phys.acc += avoid * multiplier * 2000
        phys.acc += (match / (len(localSwarm) - 1)) * multiplier

def control_drone(core, ent):
    log = ent.getLog()
    phys = ent.getPhys()

    group = log["group"]
    droneSeek(core, phys, group)
    droneAlign(core, phys, group)
    droneDamage(core, ent, log, phys, group)

if __name__ == '__main__':
    random.seed(0x88888888)

