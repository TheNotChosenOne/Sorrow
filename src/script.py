import time
import math
import random
import sorrow
from sorrow import *

random.seed(0x88888888)

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

def putWeapons(ent):
    log = ent.getLog()
    log["reload"] = 0.0
    log["reloadTime"] = 5.0 / ent.getPhys().rad[0]
    log["bulletForce"] = 1000
    log["blifetime"] = 1

def putNPC(ent, controller):
    ent.getPhys().gather = True
    log = ent.getLog()
    log["npc"] = True
    log["hp"] = 100
    log["maxHP"] = log["hp"]
    log["speed"] = 900 * ent.getPhys().mass
    log["controller"] = controller

def randomAroundCircle(x, y, rad):
    theta = random.random() * 2.0 * math.pi
    return (x + rad * math.cos(theta), y + rad * math.sin(theta))

def randomInCircle(x, y, rad):
    return randomAroundCircle(x, y, rad * math.sqrt(random.random()))

def putLittleDecay(core, source, x, y, rad, lifetime):
    sp = source.getPhys()
    e = core.entities.create()
    where = randomInCircle(x, y, rad)
    putPhys(e, where[0], where[1], rad, rad, False, "circle", 1.0, 0.95)
    e.getPhys().vel = Vec2( *randomAroundCircle(0, 0, 120.0 * math.log2(sp.mass)) )
    putVis(e, 0, 0, 0)
    setDecaying(core, e, lifetime, Vec3(0xFF, 0xFF, 0xFF))
    return e

def droneDeath(core, drone):
    phys = drone.getPhys()
    rad = phys.rad[0] / 4.0
    bits = math.ceil((phys.rad[0] * phys.rad[1]) / (rad * rad))
    for i in range(bits):
        putLittleDecay(core, drone, phys.pos[0], phys.pos[1], rad, 3.0 + random.uniform(0, 3.0))

def putDrone(core, x, y, hive, rad, density, elasticity):
    e = core.entities.create()
    putPhys(e, x, y, rad, rad, False, "circle", density, elasticity)
    putVis(e, 0xFF, 0, 0)
    putNPC(e, "drone")
    putWeapons(e)
    log = e.getLog()
    log["team"] = hive.getLog()["team"]
    log["hive"] = hive
    log["npc"] = True
    log["onDeath"] = droneDeath
    def refund():
        if hive.isAlive():
            hive.getLog()["budget"] += 1
    log["npcDeath"] = refund
    return e

def putHive(core, team, x, y, r, g, b):
    e = core.entities.create()
    putPhys(e, x, y, 20, 20, False, "circle")
    e.getPhys().area = 9999999
    putVis(e, r, g, b)
    putNPC(e, "hive")
    log = e.getLog()
    log["controller"] = "hive"
    log["team"] = team
    log["hp"] = 1000
    log["maxHP"] = log["hp"]
    log["budget"] = 7
    log["spawnCooldown"] = 1.0
    log["spawnCooling"] = 0.0
    log["onDeath"] = droneDeath
    return e

def setupPlayer(core):
    midY = core.renderer.getHeight() / 2.0
    midX = core.renderer.getWidth() / 2.0

    putPhys(core.player, midX, midY, 10, 10, False, "circle")
    putVis(core.player, 0, 0xFF, 0)

    phys = core.player.getPhys()
    phys.gather = False
    phys.phased = False

    putNPC(core.player, "player")
    putWeapons(core.player)
    core.player.getLog()["speed"] = 150
    core.player.getLog()["team"] = "player"
    core.player.getLog()["npc"] = False
    core.player.getLog()["reloadTime"] = 0.01

def setup(core):
    rad = 100.0
    clear = 5.0
    width = core.renderer.getWidth()
    height = core.renderer.getHeight()
    midX = width / 2.0
    midY = height / 2.0

    setupPlayer(core)

    easyWall(core, 200, 200, 250, 250)
    easyWall(core, 0 - rad, 0, width + rad, 0 - rad)
    easyWall(core, 0 - rad, height, width + rad, height + rad)
    easyWall(core, 0 - rad, 0 - rad, 0, height + rad)
    easyWall(core, width, 0 - rad, width + rad, height + rad)

    for i in range(32):
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

    aHive = putHive(core, "A", 200, midY + 45, 0xFF, 0, 0)
    bHive = putHive(core, "B", width - 200, midY - 45, 0, 0, 0xFF)

def fire(core, log, phys, direction):
    brad = 2.5
    b = core.entities.create()
    force = 50 * log["bulletForce"] * core.physics.stepsPerSecond()
    offset = 1.0001 * (phys.rad[0] + brad)
    bphys = b.getPhys()
    bphys.pos = phys.pos + direction * offset
    bphys.impulse = direction * force
    phys.impulse -= bphys.impulse / core.physics.stepsPerSecond()
    bphys.rad = Vec2(brad)
    bphys.mass = math.pi * brad * brad
    bphys.area = brad * brad
    bphys.elasticity = 0.95
    bphys.shape = "circle"
    bphys.isStatic = False
    bphys.phased = False
    bphys.gather = True
    b.getVis().draw = True
    b.getVis().colour = Vec3(125, 60, 152)
    b.getLog()["dmg"] = 1
    b.getLog()["team"] = log["team"]
    b.getLog()["lifetime"] = log["blifetime"] + random.uniform(0, 0.75)
    b.getLog()["controller"] = "bullet"
    return b

def control_bullet(core, bullet):
    phys = bullet.getPhys()
    for k in phys.contacts:
        l = core.entities.getHandle(k.which).getLog()
        if (not "controller" in l) or "bullet" != l["controller"]:
            log = bullet.getLog()
            log["lifetime"] = random.uniform(0, 0.01)
            del log["controller"]
            phys.gather = False
            break

def firing_control(core, shouldFire, log, phys, direction):
    log["reload"] = max(0.0, log["reload"] - core.physics.timestep())
    if shouldFire and 0.0 == log["reload"]:
        log["reload"] = log["reloadTime"]
        return fire(core, log, phys, direction)
    return None

def cooldownFunc(d, timeKey, coolKey, func, tick=None):
    if tick:
        d[timeKey] = max(0.0, d[timeKey] - tick)
    if 0.0 == d[timeKey]:
        d[timeKey] = d[coolKey]
        return func()
    return None

def getTarget(core, pos, team):
    ll = core.ai.findIn(pos, 512 + 256, team, False)
    if ll: return ll[0]
    return None

def setDecaying(core, ent, lifetime, to=Vec3(0, 0, 0)):
    log = ent.getLog()
    log["lifetime"] = lifetime
    log["maxLifetime"] = lifetime
    log["controller"] = "decay"
    log["originalColour"] = ent.getVis().colour
    log["targetColour"] = to

def control_decay(core, decay):
    log = decay.getLog()
    perc = log["lifetime"] / log["maxLifetime"]
    goal = log["originalColour"]
    base = log["targetColour"]
    decay.getVis().colour = base + (goal - base) * perc

def updateTarget(core, pos, log):
    replace = not "target" in log or \
              not log["target"] or \
              not log["target"].isAlive() or \
              not "npc" in log["target"].getLog()
    if replace:
        log["target"] = getTarget(core, pos, log["team"])

def contactDamage(core, e, log, phys):
    for k in phys.contacts:
        ent = core.entities.getHandle(k.which)
        elog = ent.getLog()

        dmg = 0.0
        converted = (1.0 - phys.elasticity * ent.getPhys().elasticity)
        forceDmg = (k.force * converted) / 2500

        if forceDmg > log["maxHP"] / 20:
            dmg = forceDmg

        if "dmg" in elog and elog["team"] != log["team"]:
            dmg += elog["dmg"]

        log["hp"] = max(0, log["hp"] - dmg)
        if 0.0 == log["hp"]:
            e.getVis().colour = e.getVis().colour * 0.9
            phys.gather = False
            if "npcDeath" in log: log["npcDeath"]()
            setDecaying(core, e, 0.3 + random.uniform(0, 2.0))
            break

def control_drone(core, drone):
    log = drone.getLog()
    phys = drone.getPhys()
    updateTarget(core, phys.pos, log)
    targetEnt = log["target"]
    if targetEnt:
        target = targetEnt.getPhys().pos
        direction = normalized(target - phys.pos)
        phys.impulse += direction * log["speed"]
        firing_control(core, True, log, phys, direction)

    contactDamage(core, drone, log, phys)

def control_hive(core, hive):
    phys = hive.getPhys()
    log = hive.getLog()
    def spawn():
        x, y = randomAroundCircle(phys.pos.x, phys.pos.y, phys.rad[0] + 7.1 + 10)
        drone = putDrone(core, x, y, hive, 10, 0.2, 1.0)
        log["budget"] -= 1
        drone.getVis().colour = hive.getVis().colour
    if log["budget"] > 0:
        cooldownFunc(log, "spawnCooling", "spawnCooldown", spawn, core.physics.timestep())

    contactDamage(core, hive, log, phys)

def control_player(core, player):
    log = player.getLog()
    phys = player.getPhys()

    direction = core.visuals.screenToWorld(core.input.mousePos())
    direction = normalized(direction - phys.pos)
    b = firing_control(core, core.input.mouseHeld(1), log, phys, direction)
    if b:
        b.getVis().colour = Vec3(0xFF, 0, 0xFF)
        bp = b.getPhys()
        bp.impulse = bp.impulse * 70
        bp.mass = bp.mass * 70
        bp.area = bp.area / 10.0

    # https://wiki.libsdl.org/SDLKeycodeLookup
    speed = log["speed"] * 10
    fact = 100.0
    dirs = Vec2(0, 0)
    if core.input.isHeld(97):
        dirs[0] += -1
    if core.input.isHeld(100):
        dirs[0] += 1
    if core.input.isHeld(119):
        dirs[1] += 1
    if core.input.isHeld(115):
        dirs[1] += -1
    phys.acc += clampVec(-speed, speed, phys.acc + dirs * (speed / fact))
