import time
import math
import random

random.seed(0x88888888)

def normalize(v):
    length = math.sqrt(v[0] * v[0] + v[1] * v[1])
    if length > 0.0: v = Vec2( (v[0] / length, v[1] / length) )
    return v

def putPhys(ent, x, y, w, h, static, shape):
    phys = ent.getPhys()
    phys.pos = Vec2( (x, y) )
    phys.rad = Vec2( (w / 2.0, h / 2.0) ) if "box" == shape else Vec2( (w, h) )
    phys.area = w + h
    phys.elasticity = 0.1
    phys.phased = False
    phys.gather = False
    phys.isStatic = static
    phys.shape = shape
    if static:
        phys.mass = 0.0
    else:
        phys.mass = w * h if "box" == shape else math.pi * w * h

def putVis(ent, r, g, b):
    vis = ent.getVis()
    vis.colour = Vec3( (r, g, b) )
    vis.draw = True

def putWall(core, x, y, w, h):
    e = core.entities.create()
    putPhys(e, x, y, w, h, True, "box")
    putVis(e, 0x88, 0x88, 0x88)
    return e

def putWeapons(ent):
    log = ent.getLog()
    log["reload"] = 0.0
    log["reloadTime"] = 0.05
    log["bulletForce"] = 1000
    log["blifetime"] = 5

def putNPC(ent, controller):
    ent.getPhys().gather = True
    log = ent.getLog()
    log["hp"] = 100
    log["speed"] = 900 * ent.getPhys().mass
    log["controller"] = controller

def putDrone(core, x, y, rad):
    e = core.entities.create()
    putPhys(e, x, y, rad, rad, False, "circle")
    putVis(e, 0xFF, 0, 0)
    putNPC(e, "drone")
    putWeapons(e)
    return e

def setupPlayer(core):
    height = core.renderer.getHeight()
    midX = core.renderer.getWidth() / 2.0

    putPhys(core.player, midX, height / 4.0 + 250, 10, 10, False, "circle")
    putVis(core.player, 0xFF, 0, 0)

    phys = core.player.getPhys()
    phys.gather = True

    putNPC(core.player, "player")
    putWeapons(core.player)
    core.player.getLog()["speed"] = 150

def setup(core):
    rad = 10.0
    clear = 5.0
    width = core.renderer.getWidth()
    height = core.renderer.getHeight()
    midX = width / 2.0
    midY = height / 2.0

    setupPlayer(core)

    putWall(core, midX, clear * rad, width, rad * 4)
    putWall(core, midX, height - clear * rad, width, rad * 4)
    putWall(core, clear * rad, midY, rad * 4, height)
    putWall(core, width - clear * rad, midY, rad * 4, height)
    putWall(core, midX, height / 4.0, width / 3.0, rad * 2)

    putDrone(core,  75, height / 4.0 + 250, 4)
    putDrone(core,  90, height / 4.0 + 250, 4)
    putDrone(core, 105, height / 4.0 + 250, 4)

def fire(core, log, phys, direction):
    brad = 0.5
    b = core.entities.create()
    force = log["bulletForce"] * core.physics.stepsPerSecond()
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
    b.getVis().colour = Vec3( (255, 0, 0) )
    b.getLog()["dmg"] = 1
    b.getLog()["team"] = log["team"]
    b.getLog()["bullet"] = 1
    b.getLog()["debris_lifetime"] = log["blifetime"]
    b.getLog()["controller"] = "bullet"

def control_bullet(core, bullet):
    phys = bullet.getPhys()
    if 0 != len(phys.contacts):
        log = bullet.getLog()
        log["lifetime"] = log["debris_lifetime"] + random.uniform(0, 0.75)
        del log["controller"]
        phys.gather = False

def rotate_colour(vis):
    black = Vec3( (0, 0, 0) )
    red = Vec3( (255, 0, 0) )
    if vis.colour == black:
        vis.colour = red
    k = vis.colour
    vis.colour = Vec3( (k[2], k[0], k[1]) )

def firing_control(core, shouldFire, log, phys, direction):
    log["reload"] = max(0.0, log["reload"] - core.physics.timestep())
    if shouldFire and 0.0 == log["reload"]:
        log["reload"] = log["reloadTime"]
        fire(core, log, phys, direction)

def control_drone(core, drone):
    log = drone.getLog()
    log["team"] = "enemy"
    phys = drone.getPhys()
    target = core.player.getPhys().pos
    diff = Vec2( (target[0] - phys.pos[0], target[1] - phys.pos[1]) )
    diff = normalize(diff)
    speed = log["speed"]
    phys.impulse = Vec2( (phys.impulse[0] + diff[0] * speed, phys.impulse[1] + diff[1] * speed) )

    direction = Vec2( (target[0] - phys.pos[0], target[1] - phys.pos[1]) )
    firing_control(core, True, log, phys, normalize(direction))

    for k in phys.contacts:
        ent = core.entities.getHandle(k.which)
        elog = ent.getLog()
        if "dmg" in elog and elog["team"] != log["team"]:
            dmg = elog["dmg"]
            log["hp"] = max(0, log["hp"] - dmg)
            if 0.0 == log["hp"]:
                print("Rip drone", drone)
                drone.getVis().colour = Vec3( (0x77, 0x77, 0x77) )
                del log["controller"]
                phys.gather = False
                break

def control_player(core, player):
    rotate_colour(player.getVis())

    log = player.getLog()
    phys = player.getPhys()
    log["team"] = "player"

    direction = core.visuals.screenToWorld(core.input.mousePos())
    direction = Vec2( (direction[0] - phys.pos[0], direction[1] - phys.pos[1]) )
    firing_control(core, core.input.mouseHeld(1), log, phys, normalize(direction))

    # https://wiki.libsdl.org/SDLKeycodeLookup
    speed = log["speed"]
    if core.input.isHeld(97) and phys.vel[0] > -speed:
        phys.acc = Vec2( (phys.acc[0] + max(-speed, phys.acc[0] - speed), phys.acc[1]) )
    if core.input.isHeld(100) and phys.vel[0] < speed:
        phys.acc = Vec2( (phys.acc[0] + min(speed, phys.acc[0] + speed), phys.acc[1]) )
    if core.input.isHeld(119) and phys.surface[1] > 0.75:
        phys.acc = Vec2( (phys.acc[0], phys.acc[1] + 0.011 * core.physics.stepsPerSecond() * speed) )
