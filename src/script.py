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

def randomAroundCircle(x, y, rad):
    theta = random.random() * 2.0 * math.pi
    return (x + rad * math.cos(theta), y + rad * math.sin(theta))

def randomInCircle(x, y, rad):
    return randomAroundCircle(x, y, rad * math.sqrt(random.random()))

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
