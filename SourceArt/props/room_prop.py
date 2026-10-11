# Graybox room props for the PCG dressing. One script, one prop per run:
#   gen 3d build SourceArt/props/room_prop.py --set kind=crate --formats fbx glb blend --out SourceArt/props/out
# kind: crate, barrel, rocks, pillar. Origin at the base center, meters, front faces -Y.
import math
import random

# bkit, bpy, bmesh come from gen 3d build


def crate(wood, dark):
    s = 0.8
    parts = [bkit.box("body", size=(s, s, s), location=(0, 0, s / 2), material=wood, bevel=0.02)]
    # slats on the front and the sides, a little proud of the body
    for sign in (-1, 1):
        for z in (0.12, s - 0.12):
            parts.append(bkit.box("slat", size=(s + 0.02, 0.04, 0.1), location=(0, sign * s / 2, z), material=dark))
            parts.append(bkit.box("slat", size=(0.04, s + 0.02, 0.1), location=(sign * s / 2, 0, z), material=dark))
    return bkit.join(parts, "SM_PropCrate")


def barrel(wood, metal):
    h, r = 1.0, 0.36
    parts = [bkit.cylinder("staves", radius=r, depth=h, location=(0, 0, h / 2), vertices=16, material=wood, bevel=0.01)]
    for z in (0.12, h / 2, h - 0.12):
        parts.append(bkit.cylinder("hoop", radius=r + 0.015, depth=0.05, location=(0, 0, z), vertices=16, material=metal))
    return bkit.join(parts, "SM_PropBarrel")


def rocks(stone):
    rng = random.Random(7)
    parts = []
    for i, (x, y, rad) in enumerate([(0, 0, 0.35), (0.38, 0.12, 0.22), (-0.3, 0.2, 0.18), (0.1, -0.32, 0.15)]):
        bpy_obj = bkit.sphere(f"rock{i}", radius=rad, location=(x, y, rad * 0.6), segments=8, material=stone)
        bpy_obj.scale = (1.0 + rng.uniform(-0.2, 0.3), 1.0 + rng.uniform(-0.2, 0.3), 0.7 + rng.uniform(-0.1, 0.1))
        bpy_obj.rotation_euler = (0, 0, rng.uniform(0, math.pi))
        parts.append(bpy_obj)
    return bkit.join(parts, "SM_PropRocks")


def pillar(stone, dark):
    parts = [bkit.box("base", size=(0.7, 0.7, 0.2), location=(0, 0, 0.1), material=dark, bevel=0.02),
             bkit.cylinder("shaft", radius=0.25, depth=1.3, location=(0, 0, 0.85), vertices=12, material=stone)]
    # the broken top: a tilted chunk and a piece fallen next to it
    parts.append(bkit.cylinder("break", radius=0.25, depth=0.25, location=(0.03, 0, 1.55), rotation=(0.35, 0.2, 0),
                               vertices=12, material=stone))
    parts.append(bkit.cylinder("fallen", radius=0.22, depth=0.5, location=(0.55, 0.15, 0.22), rotation=(0, math.pi / 2, 0.6),
                               vertices=12, material=stone))
    return bkit.join(parts, "SM_PropPillar")


def build(params):
    kind = params.get("kind", "crate")
    wood = bkit.material("M_PropWood", (0.42, 0.29, 0.18), roughness=0.8)
    dark = bkit.material("M_PropWoodDark", (0.22, 0.15, 0.1), roughness=0.85)
    metal = bkit.material("M_PropMetal", (0.3, 0.3, 0.32), metallic=0.8, roughness=0.45)
    stone = bkit.material("M_PropStone", (0.45, 0.45, 0.47), roughness=0.9)
    makers = {"crate": lambda: crate(wood, dark), "barrel": lambda: barrel(wood, metal),
              "rocks": lambda: rocks(stone), "pillar": lambda: pillar(stone, dark)}
    makers[kind]()
