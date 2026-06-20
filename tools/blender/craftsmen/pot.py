"""A CUSTOM CRAFTSMAN: a lathed ceramic pot / urn.

Same three-part contract as nfold_star.py / twisted_column.py:

- MANIFEST: id, label, params (only the C++-known types number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real lathed pot offline, like any built-in.
- build(op): the bpy twin, from the SAME proof_mesh (Blender only).

The shape is a SURFACE OF REVOLUTION: a radius-vs-height silhouette (foot ring ->
bulbous body -> narrow neck -> flared rim) sampled at fixed fractions of the
height, then spun around the z-axis over `sides` segments. Seed jitters the
profile PROPORTIONS only (each control radius / its height fraction nudged by a
bounded factor), so every seed is still recognizably a pot but with its own
belly, waist and lip. Nothing is hardcoded: every dimension is a named param.
"""

import math
import random


# Silhouette-defining proportion, named rather than inline (the tree precedent).
FOOT_INSET_FRAC = 0.82   # the underside base radius is this fraction of the foot radius


MANIFEST = {
    "id": "pot",
    "label": "Ceramic Pot / Urn",
    "params": [
        {"key": "height", "label": "Height", "type": "number", "default": 1.2},
        {"key": "footRadius", "label": "Foot Radius", "type": "number", "default": 0.30},
        {"key": "bodyRadius", "label": "Body Radius", "type": "number", "default": 0.55},
        {"key": "neckRadius", "label": "Neck Radius", "type": "number", "default": 0.28},
        {"key": "rimRadius", "label": "Rim Radius", "type": "number", "default": 0.40},
        {"key": "sides", "label": "Sides", "type": "integer", "default": 24},
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _profile(params: dict, rng: random.Random):
    """Return the lathe silhouette as a list of (radius, height_fraction) rings
    from base (frac 0) to rim (frac 1), strictly increasing in height fraction.

    The profile is built from the named control radii. Each control point is
    nudged by a bounded seeded factor so the belly/waist/lip vary per seed while
    staying a believable pot. rng values are drawn in a FIXED order so the same
    seed is byte-identical and a different seed visibly differs."""

    foot = max(1e-3, float(params.get("footRadius", 0.30)))
    body = max(1e-3, float(params.get("bodyRadius", 0.55)))
    neck = max(1e-3, float(params.get("neckRadius", 0.28)))
    rim = max(1e-3, float(params.get("rimRadius", 0.40)))

    # Seeded jitter: each factor in a fixed draw order. +/-12% on radii and a
    # small +/-0.05 nudge on the interior height fractions. Bounded so the shape
    # never degenerates (radii stay positive, fractions stay ordered).
    def jr():  # radius factor in [0.88, 1.12]
        return 1.0 + (rng.random() - 0.5) * 0.24

    def jf():  # height-fraction nudge in [-0.05, 0.05]
        return (rng.random() - 0.5) * 0.10

    foot_r = foot * jr()
    base_r = foot_r * FOOT_INSET_FRAC  # the underside of the foot, slightly inset
    waist_r = max(1e-3, (foot + body) * 0.5 * jr())  # transition foot->belly
    body_r = body * jr()
    shoulder_r = max(1e-3, (body + neck) * 0.5 * jr())  # belly->neck taper
    neck_r = neck * jr()
    rim_r = rim * jr()

    # Interior height fractions (base=0, rim=1 are fixed). Defaults give the
    # classic urn cadence: low foot, belly around 0.35, waist neck up high.
    f_foot = 0.06 + jf() * 0.3
    f_waist = 0.18 + jf()
    f_belly = 0.38 + jf()
    f_shoulder = 0.66 + jf()
    f_neck = 0.86 + jf()

    # Each (radius, fraction) ring, base -> rim. The foot ring is repeated near
    # the bottom (base_r then foot_r) so the pot reads as standing on a ring.
    rings = [
        (base_r, 0.0),
        (foot_r, f_foot),
        (waist_r, f_waist),
        (body_r, f_belly),
        (shoulder_r, f_shoulder),
        (neck_r, f_neck),
        (rim_r, 1.0),
    ]

    # Enforce strictly increasing height fractions (guards the seeded nudges and
    # any pathological param combos). Clamp each interior frac into an open gap
    # between its neighbours so the lathe never folds back on itself.
    fixed = [rings[0]]
    eps = 1e-3
    for i in range(1, len(rings) - 1):
        r, f = rings[i]
        lo = fixed[-1][1] + eps
        hi = rings[-1][1] - eps * (len(rings) - 1 - i)
        f = max(lo, min(f, hi))
        fixed.append((r, f))
    fixed.append(rings[-1])
    return fixed


def _local_mesh(params: dict):
    height = max(1e-3, float(params.get("height", 1.2)))
    sides = max(3, int(float(params.get("sides", 24))))
    rng = random.Random(int(float(params.get("seed", 0))))

    profile = _profile(params, rng)
    rings = len(profile)

    # Lathe: each profile ring becomes a circle of `sides` verts at z = frac*height.
    verts = []
    for radius, frac in profile:
        z = frac * height
        for s in range(sides):
            ang = 2.0 * math.pi * s / sides
            verts.append((math.cos(ang) * radius, math.sin(ang) * radius, z))

    # A single apex vertex at the centre of the bottom ring (z=0) and a single
    # centre vertex at the top ring give clean closed caps without a hole; the
    # top centre sits slightly below the rim so the mouth reads as a vessel.
    bottom_center_idx = len(verts)
    verts.append((0.0, 0.0, 0.0))
    top_inner_drop = (profile[-1][1] - profile[-2][1]) * height * 0.5
    top_center_idx = len(verts)
    verts.append((0.0, 0.0, height - max(0.0, top_inner_drop)))

    faces = []

    # Side quads between consecutive rings.
    for r in range(rings - 1):
        base0 = r * sides
        base1 = (r + 1) * sides
        for s in range(sides):
            ns = (s + 1) % sides
            faces.append([base0 + s, base0 + ns, base1 + ns, base1 + s])

    # Bottom cap: fan from the bottom centre, wound so the normal points down.
    for s in range(sides):
        ns = (s + 1) % sides
        faces.append([bottom_center_idx, ns, s])

    # Top cap: fan from the top inner centre, wound so the normal points up.
    top_base = (rings - 1) * sides
    for s in range(sides):
        ns = (s + 1) % sides
        faces.append([top_center_idx, top_base + s, top_base + ns])

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
    name = op.get("name", op.get("script", "pot"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
