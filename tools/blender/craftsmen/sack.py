"""A CUSTOM CRAFTSMAN: a cloth sack / bag — a bulging rounded body that narrows
to a cinched neck gathered closed at the top, slightly lumpy and irregular.

Same three-part contract as nfold_star.py / twisted_column.py:

- MANIFEST: id, label, params (only the C++-known types number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real sack offline, exactly like any built-in.
- build(op): the bpy twin, from the SAME proof_mesh (Blender only).

Construction: a surface of revolution (lathe). A smooth base profile gives the
radius as a function of normalized height t in [0, 1] — small foot, mid bulge,
pinch to a small neck at the top. That profile is sampled at `rings` heights and
revolved over `sides` columns. Seeded noise then deforms it a LOT: a per-ring
radius wobble, a per-ring lean (lateral offset, so the sack slumps), and a
per-(ring, column) lumpiness so no two seeds look alike. Same seed is
byte-identical (one rng drawn in a fixed order); different seed is visibly a
different sack, but always recognizably a sack.
"""

import math
import random


MANIFEST = {
    "id": "sack",
    "label": "Cloth Sack",
    "params": [
        {"key": "height", "label": "Height", "type": "number", "default": 1.2},
        {"key": "bodyRadius", "label": "Body Radius", "type": "number", "default": 0.55},
        {"key": "neckRadius", "label": "Neck Radius", "type": "number", "default": 0.14},
        {"key": "baseRadius", "label": "Base Radius", "type": "number", "default": 0.34},
        {"key": "sides", "label": "Sides", "type": "integer", "default": 16},
        {"key": "rings", "label": "Rings", "type": "integer", "default": 14},
        {"key": "lumpiness", "label": "Lumpiness", "type": "number", "default": 0.32},
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _profile_radius(t: float, base_r: float, body_r: float, neck_r: float) -> float:
    """The base silhouette: radius as a smooth function of normalized height t.

    Three control radii (foot / belly / neck) interpolated so the sack reads as
    a bulging body that pinches to a gathered neck:
      - t in [0, 0.5]   foot  -> belly   (the bulge grows)
      - t in [0.5, 1.0] belly -> neck     (sharp pinch up to the cinch)
    A smoothstep on each leg keeps the silhouette rounded rather than conical.
    The belly is pushed a touch ABOVE the linear midpoint so the widest part
    sits low (heavy-bottomed, like a filled sack), via the 0.45 belly anchor.
    """
    belly_t = 0.45  # where the widest part of the body sits (below mid = sags low)
    if t <= belly_t:
        u = t / belly_t
        s = u * u * (3.0 - 2.0 * u)  # smoothstep
        return base_r + (body_r - base_r) * s
    u = (t - belly_t) / (1.0 - belly_t)
    # bias the pinch so most of the narrowing happens near the very top (gather)
    s = u * u  # quadratic ease-in: stays fat then cinches hard
    return body_r + (neck_r - body_r) * s


def _local_mesh(params: dict):
    # C++ carries params untyped, so coerce EVERY one defensively.
    height = max(1e-3, float(params.get("height", 1.2)))
    body_r = max(1e-3, float(params.get("bodyRadius", 0.55)))
    neck_r = max(1e-4, float(params.get("neckRadius", 0.14)))
    base_r = max(1e-3, float(params.get("baseRadius", 0.34)))
    sides = max(5, int(float(params.get("sides", 16))))
    rings = max(4, int(float(params.get("rings", 14))))
    lump = max(0.0, float(params.get("lumpiness", 0.32)))
    seed = int(float(params.get("seed", 0)))

    rng = random.Random(seed)

    # --- Seeded per-ring deformation tables, drawn in a FIXED order. ---
    # ring_scale: overall radius wobble per ring (the sack bulges unevenly).
    # lean_x / lean_y: the ring's centre drifts sideways, accumulating up the
    #   stack so the sack slumps/leans like soft cloth rather than a rigid lathe.
    # The neck (top ring) is pulled back toward centre so the cinch stays tight.
    ring_scale = []
    lean_x = []
    lean_y = []
    drift_x = 0.0
    drift_y = 0.0
    for i in range(rings):
        t = i / (rings - 1)
        # wobble shrinks toward the neck so the gather reads clean
        wob = lump * (1.0 - 0.7 * t)
        ring_scale.append(1.0 + rng.uniform(-wob, wob))
        # lateral drift step, scaled by body radius and lumpiness
        step = lump * body_r * 0.18
        drift_x += rng.uniform(-step, step)
        drift_y += rng.uniform(-step, step)
        # taper the accumulated lean back toward 0 at the very top (tight cinch)
        pull = 1.0 - (t ** 3)
        lean_x.append(drift_x * pull)
        lean_y.append(drift_y * pull)

    # --- Seeded per-(ring, column) lumpiness, drawn in a FIXED order. ---
    # A small radial bump on each vertex makes the cloth surface irregular.
    bumps = []
    for i in range(rings):
        t = i / (rings - 1)
        amp = lump * 0.35 * (1.0 - 0.6 * t)  # less bumpy near the neck
        row = [rng.uniform(-amp, amp) for _ in range(sides)]
        bumps.append(row)

    # --- Lathe the profile. ---
    verts = []
    for i in range(rings):
        t = i / (rings - 1)
        z = t * height
        r = _profile_radius(t, base_r, body_r, neck_r) * ring_scale[i]
        r = max(1e-4, r)
        cx = lean_x[i]
        cy = lean_y[i]
        for j in range(sides):
            ang = 2.0 * math.pi * j / sides
            rr = max(1e-4, r * (1.0 + bumps[i][j]))
            x = cx + math.cos(ang) * rr
            y = cy + math.sin(ang) * rr
            verts.append((x, y, z))

    # --- Caps. The neck gathers to a single pinched apex (gathered closed). ---
    # Bottom cap centre (closes the foot flat-ish), top apex (the cinch knot).
    bottom_center_idx = len(verts)
    verts.append((lean_x[0], lean_y[0], 0.0))
    top_apex_idx = len(verts)
    # apex sits slightly above the top ring: the gathered cloth tied off
    verts.append((lean_x[-1], lean_y[-1], height + neck_r * 0.6))

    faces = []

    # bottom cap: triangle fan, wound so the normal faces down (outward at base)
    base = 0
    for j in range(sides):
        nj = (j + 1) % sides
        faces.append([bottom_center_idx, base + nj, base + j])

    # side quads between consecutive rings
    for i in range(rings - 1):
        a = i * sides
        b = (i + 1) * sides
        for j in range(sides):
            nj = (j + 1) % sides
            faces.append([a + j, a + nj, b + nj, b + j])

    # neck gather: triangle fan from the top ring up to the single apex
    top = (rings - 1) * sides
    for j in range(sides):
        nj = (j + 1) % sides
        faces.append([top + j, top + nj, top_apex_idx])

    # --- Shift so the lowest vertex sits at local z == 0 (origin at base). ---
    min_z = min(v[2] for v in verts)
    if abs(min_z) > 1e-12:
        verts = [(x, y, z - min_z) for (x, y, z) in verts]

    return verts, faces


def proof_mesh(op: dict):
    """Pure (verts, faces) for the proof, placed at the op's x/y/z."""
    verts, faces = _local_mesh(op.get("params", {}))
    x = float(op.get("x", 0.0))
    y = float(op.get("y", 0.0))
    z = float(op.get("z", 0.0))
    return [(vx + x, vy + y, vz + z) for vx, vy, vz in verts], faces


def build(op: dict):  # pragma: no cover — exercised in Blender
    import bpy

    verts, faces = proof_mesh(op)
    name = op.get("name", op.get("script", "sack"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
