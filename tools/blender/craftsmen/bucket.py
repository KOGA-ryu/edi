"""A CUSTOM CRAFTSMAN: a bucket / pail — a truncated cone wider at the TOP than
the bottom, with a thin raised rim ring at the lip, a flat (slightly recessed)
bottom, and an optional thin arched swept-tube handle.

Same three-part contract as nfold_star.py:

- MANIFEST: id, label, params (only the C++-known types number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — renders the real bucket offline.
- build(op): the bpy twin, from the SAME proof_mesh (Blender only).

This is a background prop, not a generator: the body is a closed-form truncated
cone; a "seed" integer jitters the proportions (radii / height / rim) so a field
of buckets reads as hand-thrown rather than cloned, while staying recognizably a
bucket. Same seed = byte-identical mesh; different seed = visibly different.
"""

import math
import random

MANIFEST = {
    "id": "bucket",
    "label": "Bucket / Pail",
    "params": [
        {"key": "topRadius", "label": "Top Radius", "type": "number", "default": 0.42},
        {"key": "bottomRadius", "label": "Bottom Radius", "type": "number", "default": 0.32},
        {"key": "height", "label": "Height", "type": "number", "default": 0.55},
        {"key": "rimThickness", "label": "Rim Thickness", "type": "number", "default": 0.04},
        {"key": "wallInset", "label": "Wall Inset", "type": "number", "default": 0.03},
        {"key": "bottomRecess", "label": "Bottom Recess", "type": "number", "default": 0.03},
        {"key": "sides", "label": "Sides", "type": "integer", "default": 24},
        {"key": "handle", "label": "Handle (0/1)", "type": "integer", "default": 1},
        {"key": "handleSegments", "label": "Handle Segments", "type": "integer", "default": 12},
        {"key": "handleTubeRadius", "label": "Handle Tube Radius", "type": "number", "default": 0.018},
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _ring(cx, cy, cz, radius, sides):
    """A flat horizontal ring of `sides` vertices at height cz, radius `radius`."""
    out = []
    for i in range(sides):
        a = 2.0 * math.pi * i / sides
        out.append((cx + math.cos(a) * radius, cy + math.sin(a) * radius, cz))
    return out


def _bridge(faces, lower, upper, sides, flip=False):
    """Stitch two same-`sides` rings (index lists) into a band of quads."""
    for i in range(sides):
        j = (i + 1) % sides
        if flip:
            faces.append([lower[i], lower[j], upper[j], upper[i]])
        else:
            faces.append([lower[i], upper[i], upper[j], lower[j]])


def _local_mesh(params: dict):
    # Coerce EVERY param defensively — C++ carries them untyped.
    top = max(1e-3, float(params.get("topRadius", 0.42)))
    bottom = max(1e-3, float(params.get("bottomRadius", 0.32)))
    height = max(1e-3, float(params.get("height", 0.55)))
    rim_th = max(0.0, float(params.get("rimThickness", 0.04)))
    wall_inset = max(0.0, float(params.get("wallInset", 0.03)))
    bottom_recess = max(0.0, float(params.get("bottomRecess", 0.03)))
    sides = max(6, int(float(params.get("sides", 24))))
    handle_on = int(float(params.get("handle", 1))) != 0
    handle_seg = max(2, int(float(params.get("handleSegments", 12))))
    tube_r = max(1e-4, float(params.get("handleTubeRadius", 0.018)))

    # Seeded variation: one rng, values drawn in a FIXED order so same-seed is
    # byte-identical and different-seed visibly differs. Jitter the proportions
    # (radii, height, rim) within tame bounds — still clearly a bucket.
    rng = random.Random(int(float(params.get("seed", 0))))
    top *= 1.0 + rng.uniform(-0.12, 0.12)
    bottom *= 1.0 + rng.uniform(-0.12, 0.12)
    height *= 1.0 + rng.uniform(-0.10, 0.18)
    rim_th *= 1.0 + rng.uniform(-0.25, 0.25)

    # Keep the silhouette sane after jitter: a pail is wider at the lip, and the
    # rim/inset must not eat the whole mouth.
    top = max(top, bottom * 1.05)
    rim_th = min(rim_th, top * 0.4)
    inner_top = max(1e-3, top - rim_th - wall_inset)
    inner_bottom = max(1e-3, bottom * (inner_top / top) - wall_inset)
    floor_z = min(bottom_recess, height * 0.5)  # recessed inner floor height

    verts = []
    faces = []

    def add_ring(radius, z):
        base = len(verts)
        verts.extend(_ring(0.0, 0.0, z, radius, sides))
        return list(range(base, base + sides))

    # --- Outer body: bottom edge -> top (outer) lip ---------------------------
    outer_bot = add_ring(bottom, 0.0)          # outer wall, base on z=0 plane
    outer_top = add_ring(top, height)          # outer wall, at the lip
    _bridge(faces, outer_bot, outer_top, sides)

    # --- Rim: a thin raised ring rolling over the lip to the inner top --------
    rim_top = add_ring(inner_top, height)      # top face of rim, inner edge
    _bridge(faces, outer_top, rim_top, sides)  # the flat top annulus of the lip

    # --- Inner wall: inner top lip -> inner bottom ----------------------------
    inner_bot = add_ring(inner_bottom, floor_z)
    _bridge(faces, rim_top, inner_bot, sides, flip=True)  # faces point inward

    # --- Inner floor (recessed) + outer underside (closed bottom) -------------
    cz_in = len(verts)
    verts.append((0.0, 0.0, floor_z))          # inner floor center
    for i in range(sides):
        j = (i + 1) % sides
        faces.append([cz_in, inner_bot[j], inner_bot[i]])  # up-facing inner floor

    cz_out = len(verts)
    verts.append((0.0, 0.0, 0.0))              # outer underside center, on z=0
    for i in range(sides):
        j = (i + 1) % sides
        faces.append([cz_out, outer_bot[i], outer_bot[j]])  # down-facing base

    # --- Optional arched handle: a swept tube hinged at the two lip sides ------
    # Cheap by construction: handle_seg arch stations x a small tube ring.
    if handle_on:
        tube_sides = 6  # hexagonal tube — cheap, reads round at prop scale
        # Hinge points sit on the outer lip on the +x / -x sides.
        span = top                              # half-distance hinge-to-hinge ~ radius
        arch_h = top * 0.9                      # how high the arch bows over the mouth
        centerline = []
        for s in range(handle_seg + 1):
            t = s / handle_seg
            ang = math.pi * t                   # 0 .. pi sweeps -x lip to +x lip
            cx = -math.cos(ang) * span          # -span -> +span
            cz = height + math.sin(ang) * arch_h
            centerline.append((cx, 0.0, cz))

        # The arch lives in the x-z plane, so its binormal is always +y; the
        # in-plane normal is the tangent rotated 90 deg. Build each tube ring,
        # then bridge consecutive rings into a tube.
        prev_ring = None
        for idx, (px, py, pz) in enumerate(centerline):
            # tangent along the centerline (finite difference, in x-z)
            if idx < len(centerline) - 1:
                nx, _, nz = centerline[idx + 1]
                tx, tz = nx - px, nz - pz
            else:
                nx, _, nz = centerline[idx - 1]
                tx, tz = px - nx, pz - nz
            tlen = math.hypot(tx, tz) or 1.0
            tx, tz = tx / tlen, tz / tlen
            ux, uz = -tz, tx                    # in-plane normal (perp to tangent)
            base = len(verts)
            for k in range(tube_sides):
                ta = 2.0 * math.pi * k / tube_sides
                ca, sa = math.cos(ta), math.sin(ta)
                vx = px + tube_r * (ux * ca)
                vy = py + tube_r * (sa)         # binormal = +y
                vz = pz + tube_r * (uz * ca)
                verts.append((vx, vy, vz))
            ring = list(range(base, base + tube_sides))
            if prev_ring is not None:
                _bridge(faces, prev_ring, ring, tube_sides)
            prev_ring = ring
        # (Handle ends are left open where they tuck into the lip — invisible at
        #  prop scale, and capping would add no silhouette value.)

    # Anchor the mesh so its lowest vertex sits at local z = 0 (origin at base).
    min_z = min(v[2] for v in verts)
    if abs(min_z) > 1e-9:
        verts = [(vx, vy, vz - min_z) for vx, vy, vz in verts]

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
    name = op.get("name", op.get("script", "bucket"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
