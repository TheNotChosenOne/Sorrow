import time
import math
import random

random.seed(0x88888888)

def clamp(low, high, x):
    return min(high, max(low, x))

def normalize(v):
    length = math.sqrt(v[0] * v[0] + v[1] * v[1])
    if length > 0.0: v = Vec2( (v[0] / length, v[1] / length) )
    return v

def putPhys(ent, x, y, w, h, static, shape, density=1.0, elasticity=None):
    phys = ent.getPhys()
    phys.pos = Vec2( (x, y) )
    phys.rad = Vec2( (w / 2.0, h / 2.0) ) if "box" == shape else Vec2( (w, h) )
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
    vis.colour = Vec3( (r, g, b) )
    vis.draw = True

def putWall(core, x, y, w, h):
    e = core.entities.create()
    putPhys(e, x, y, w, h, True, "box")
    putVis(e, 0x88, 0x88, 0x88)
    return e

def easyWall(core, left, top, right, bot):
    x = (left + right) / 2.0
    y = (top + bot) / 2.0
    return putWall(core, x, y, abs(right - left), abs(top - bot))

def putWeapons(ent):
    log = ent.getLog()
    log["reload"] = 0.0
    log["reloadTime"] = 5.0 / ent.getPhys().rad[0]
    log["bulletForce"] = 1000
    log["blifetime"] = 1

def putNPC(ent, controller,):
    ent.getPhys().gather = True
    log = ent.getLog()
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
    e.getPhys().vel = Vec2( randomAroundCircle(0, 0, 10.0 * sp.mass) )
    putVis(e, 0, 0, 0)
    setDecaying(core, e, lifetime, (0xFF, 0xFF, 0xFF))
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
    return e

def putHive(core, team, x, y, r, g, b):
    e = core.entities.create()
    putPhys(e, x, y, 20, 20, True, "circle")
    putVis(e, r, g, b)
    log = e.getLog()
    log["controller"] = "hive"
    log["team"] = team
    log["budget"] = 7
    log["spawnCooldown"] = 1.0
    log["spawnCooling"] = log["spawnCooldown"]
    return e

def setupPlayer(core):
    height = core.renderer.getHeight()
    midX = core.renderer.getWidth() / 2.0

    putPhys(core.player, midX, height / 4.0 + 250, 10, 10, False, "circle")
    putVis(core.player, 0, 0xFF, 0)

    phys = core.player.getPhys()
    phys.gather = False
    phys.phased = True

    putNPC(core.player, "player")
    putWeapons(core.player)
    core.player.getLog()["speed"] = 150
    core.player.getLog()["team"] = "player"
    core.player.getLog()["npc"] = False

def setup(core):
    rad = 10.0
    clear = 5.0
    width = core.renderer.getWidth()
    height = core.renderer.getHeight()
    midX = width / 2.0
    midY = height / 2.0

    setupPlayer(core)

    easyWall(core, 100, 100, 150, 150)
    putWall(core, midX, clear * rad, width, rad * 4)
    putWall(core, midX, height - clear * rad, width, rad * 4)
    putWall(core, clear * rad, midY, rad * 4, height)
    putWall(core, width - clear * rad, midY, rad * 4, height)
    putWall(core, midX, height / 4.0, width / 3.0, rad * 2)

    aHive = putHive(core, "A", 150, midY + 45, 0xFF, 0, 0)
    bHive = putHive(core, "B", width - 150, midY - 45, 0, 0, 0xFF)
    #putDrone(core,  75, height / 4.0 + 250 + 5, aHive, 10, 1.0, 0.97)
    #putDrone(core,  90, height / 4.0 + 250 + 0, bHive, 7.5, 0.5, 0.6)
    #putDrone(core, 105, height / 4.0 + 250 - 5, bHive, 5, 0.5, 0.6)

def fire(core, log, phys, direction):
    brad = 2.5
    b = core.entities.create()
    force = 50 * log["bulletForce"] * core.physics.stepsPerSecond()
    offset = 1.0001 * (phys.rad[0] + brad)
    bphys = b.getPhys()
    bphys.pos = Vec2( (phys.pos[0] + direction[0] * offset, phys.pos[1] + direction[1] * offset) )
    bphys.vel = Vec2( (0, 0) )
    bphys.acc = Vec2( (0, 0) )
    bphys.impulse = Vec2( (direction[0] * force, direction[1] * force) )
    phys.impulse = Vec2( (phys.impulse[0] - bphys.impulse[0] / 250, phys.impulse[1] - bphys.impulse[1] / 250) )
    bphys.rad = Vec2( (brad, brad) )
    bphys.surface = Vec2( (0, 1) )
    bphys.mass = math.pi * brad * brad
    bphys.area = brad * brad
    bphys.elasticity = 0.95
    bphys.shape = "circle"
    bphys.isStatic = False
    bphys.phased = False
    bphys.gather = True
    b.getVis().draw = True
    b.getVis().colour = Vec3( (125, 60, 152) )
    b.getLog()["dmg"] = 0
    b.getLog()["team"] = log["team"]
    b.getLog()["bullet"] = 1
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
    #return None
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

def getTarget(core, team):
    ll = core.entities.all();
    random.shuffle(ll)
    random.shuffle(ll)
    for e in ll:
        h = core.entities.getHandle(e)
        log = h.getLog()
        if "npc" in log and log["npc"] and "team" in log and team != log["team"]:
            return h
    return None

def setDecaying(core, ent, lifetime, to=(0, 0, 0)):
    log = ent.getLog()
    log["lifetime"] = lifetime
    log["maxLifetime"] = lifetime
    log["controller"] = "decay"
    log["originalColour"] = ent.getVis().colour
    log["targetColour"] = to

def clampColour(r, g, b):
    return Vec3( ( clamp(0, 0xFF, r), clamp(0, 0xFF, g), clamp(0, 0xFF, b) ) )

def control_decay(core, decay):
    log = decay.getLog()
    perc = log["lifetime"] / log["maxLifetime"]
    vis = decay.getVis()
    goal = log["originalColour"]
    base = log["targetColour"]
    erp = lambda i: base[i] + (goal[i] - base[i]) * perc
    vis.colour = clampColour(erp(0), erp(1), erp(2))

def control_drone(core, drone):
    log = drone.getLog()
    phys = drone.getPhys()
    replaceTarget = not "target" in log
    replaceTarget = replaceTarget or not log["target"]
    replaceTarget = replaceTarget or not log["target"].isAlive()
    replaceTarget = replaceTarget or not "controller" in log["target"].getLog()
    replaceTarget = replaceTarget or not "npc" in log["target"].getLog()
    if replaceTarget:
        log["target"] = getTarget(core, log["team"])
    targetEnt = log["target"]
    if targetEnt:
        target = targetEnt.getPhys().pos
        diff = Vec2( (target[0] - phys.pos[0], target[1] - phys.pos[1]) )
        diff = normalize(diff)
        speed = log["speed"]
        phys.impulse = Vec2( (phys.impulse[0] + diff[0] * speed, phys.impulse[1] + diff[1] * speed) )
        direction = Vec2( (target[0] - phys.pos[0], target[1] - phys.pos[1]) )
        firing_control(core, True, log, phys, normalize(direction))

    if "big" not in log: log["big"] = 0
    for k in phys.contacts:
        ent = core.entities.getHandle(k.which)
        elog = ent.getLog()
        dmg = 0.0
        el = (1.0 - phys.elasticity * ent.getPhys().elasticity)
        forceDmg = (k.force * el) / 2500
        if forceDmg > log["maxHP"] / 20:
            dmg = forceDmg
            #print(k.force, el, dmg, log["hp"])
        if "dmg" in elog and elog["team"] != log["team"]:
            dmg += elog["dmg"]
        log["hp"] = max(0, log["hp"] - dmg)
        if 0.0 == log["hp"]:
            #print("Rip drone", drone)
            drone.getVis().colour = Vec3( (0x77, 0x77, 0x77) )
            phys.gather = False
            log["hive"].getLog()["budget"] += 1
            setDecaying(core, drone, 3.0 + random.uniform(0, 2.0))
            del log["npc"]
            break

def control_hive(core, hive):
    phys = hive.getPhys()
    log = hive.getLog()
    def spawn():
        x = phys.pos[0] + random.uniform(-1, 1)
        y = phys.pos[1] + random.uniform(-1, 1)
        drone = putDrone(core, x, y, hive, 10, 0.2, 1.0)
        log["budget"] -= 1
        drone.getVis().colour = hive.getVis().colour
    if log["budget"] > 0:
        cooldownFunc(log, "spawnCooling", "spawnCooldown", spawn, core.physics.timestep())
    pass

def control_player(core, player):
    log = player.getLog()
    phys = player.getPhys()

    direction = core.visuals.screenToWorld(core.input.mousePos())
    direction = Vec2( (direction[0] - phys.pos[0], direction[1] - phys.pos[1]) )
    b = firing_control(core, core.input.mouseHeld(1), log, phys, normalize(direction))
    if b:
        b.getVis().colour = Vec3( (0, 0, 0xFF) )

    # https://wiki.libsdl.org/SDLKeycodeLookup
    speed = log["speed"] * 10
    fact = 100.0
    dirs = [0, 0]
    if core.input.isHeld(97):
        dirs[0] += -1
    if core.input.isHeld(100):
        dirs[0] += 1
    if core.input.isHeld(119):
        dirs[1] += 1
    if core.input.isHeld(115):
        dirs[1] += -1
    phys.acc = Vec2((
        phys.acc[0] + clamp(-speed, speed, phys.acc[0] + dirs[0] * (speed / fact)),
        phys.acc[1] + clamp(-speed, speed, phys.acc[1] + dirs[1] * (speed / fact)),
    ))
