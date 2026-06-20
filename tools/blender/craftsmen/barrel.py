"""A CUSTOM CRAFTSMAN: a lathed wooden cask — the classic barrel.

Sibling of nfold_star.py / twisted_column.py; same three-part contract:

- MANIFEST: id, label, params (only the C++-known types number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real barrel offline, like any built-in.
- build(op): the bpy twin, from the SAME proof_mesh (Blender only).

The body is a vertical tube of N staves whose radius bulges (a cosine-shaped
profile) — widest at mid-height, narrowing toward the top/bottom end rings.
2-3 thin raised metal BANDS (slightly larger-radius rings, each a short
cylindrical sleeve) clamp the staves at fractions of the height. A "seed"
integer jitters the bulge amount, the end/mid proportions, and the band
positions, so the same seed is byte-identical and different seeds give a
visibly different — but always recognizable — cask.
"""

import math
import random

MANIFEST = {
    "id": "barrel",
    "label": "Barrel (Wooden Cask)",
    "params": [
        {"key": "height", "label": "Height", "type": "number", "default": 1.2},
        {"key": "midRadius", "label": "Mid Radius", "type": "number", "default": 0.55},
        {"key": "endRadius", "label": "End Radius", "type": "number", "default": 0.42},
        {"key": "sides", "label": "Staves (sides)", "type": "integer", "default": 20},
        {"key": "rings", "label": "Profile Rings", "type": "integer", "default": 8},
        {"key": "bandCount", "label": "Metal Bands", "type": "integer", "default": 3},
        {"key": "bandBulge", "label": "Band Bulge", "type": "number", "default": 0.04},
        {"key": "bandHeight", "label": "Band Height", "type": "number", "default": 0.06},
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _ring(cx_count, radius, z):
    """One horizontal ring of `cx_count` points at the given radius and height z.
    Returns a list of (x, y, z) tuples on a circle in the XY plane."""
    pts = []
    for s in range(cx_count):
        ang = 2.0 * math.pi * s / cx_count
        pts.append((math.cos(ang) * radius, math.sin(ang) * radius, z))
    return pts


def _bridge(verts, faces, lower_start, upper_start, n):
    """Quad-bridge two same-size rings already appended to `verts`.
    lower_start / upper_start are the base vertex indices of each ring; n is the
    point count. Winding is outward-facing (CCW seen from outside)."""
    for s in range(n):
        ns = (s + 1) % n
        faces.append([
            lower_start + s,
            lower_start + ns,
            upper_start + ns,
            upper_start + s,
        ])


def _local_mesh(params: dict):
    # Coerce EVERY param defensively — C++ carries them untyped.
    height = max(1e-3, float(params.get("height", 1.2)))
    mid_radius = max(1e-3, float(params.get("midRadius", 0.55)))
    end_radius = max(1e-3, float(params.get("endRadius", 0.42)))
    sides = max(6, int(float(params.get("sides", 20))))
    rings = max(3, int(float(params.get("rings", 8))))
    band_count = max(0, int(float(params.get("bandCount", 3))))
    band_bulge = max(0.0, float(params.get("bandBulge", 0.04)))
    band_height = max(1e-4, float(params.get("bandHeight", 0.06)))

    # Seeded variation: ONE rng, drawn in a FIXED order so same-seed is
    # byte-identical and different-seed visibly differs.
    rng = random.Random(int(float(params.get("seed", 0))))

    # 1) Jitter the bulge depth: how much fatter the middle is than the ends.
    #    Base bulge = (mid - end); scale it by a seeded factor in [0.75, 1.25].
    bulge_scale = 0.75 + rng.random() * 0.5
    base_bulge = (mid_radius - end_radius)
    eff_mid = end_radius + base_bulge * bulge_scale
    # 2) Jitter the end taper a touch so top/bottom openings differ run-to-run.
    end_jitter = 1.0 + (rng.random() - 0.5) * 0.12  # [0.94, 1.06]
    eff_end = max(1e-3, end_radius * end_jitter)
    # Keep the silhouette a genuine bulge: mid strictly wider than the ends.
    eff_mid = max(eff_mid, eff_end * 1.04)

    # --- BODY: a stack of `rings` profile loops, radius from a bulge curve. ---
    # radius_at(t) for t in [0,1] over the height: sin(pi*t) is 0 at the two ends
    # (t=0, t=1) and 1 at the middle (t=0.5), so the radius is `eff_end` at the
    # caps and `eff_mid` at the waist — a smooth, symmetric barrel bulge.
    def radius_at(t):
        return eff_end + (eff_mid - eff_end) * math.sin(math.pi * t)

    verts = []
    faces = []

    body_ring_starts = []
    for r in range(rings):
        t = r / (rings - 1)          # 0 .. 1 inclusive
        z = t * height
        rad = radius_at(t)
        start = len(verts)
        verts.extend(_ring(sides, rad, z))
        body_ring_starts.append((start, z, rad))

    # Bridge consecutive body rings into the stave wall.
    for r in range(rings - 1):
        lo = body_ring_starts[r][0]
        up = body_ring_starts[r + 1][0]
        _bridge(verts, faces, lo, up, sides)

    # Caps: bottom (z=0) and top (z=height). Add a center vertex and fan so the
    # mesh is closed/manifold — the cask reads as a solid, not an open tube.
    bot_center = len(verts)
    verts.append((0.0, 0.0, 0.0))
    bot_start = body_ring_starts[0][0]
    for s in range(sides):
        ns = (s + 1) % sides
        # Reversed winding so the bottom faces down (outward).
        faces.append([bot_center, bot_start + ns, bot_start + s])

    top_center = len(verts)
    verts.append((0.0, 0.0, height))
    top_start = body_ring_starts[-1][0]
    for s in range(sides):
        ns = (s + 1) % sides
        faces.append([top_center, top_start + s, top_start + ns])

    # --- BANDS: thin raised metal hoops at fractions of the height. ---
    # Each band is a short cylindrical sleeve (two rings bridged + an outer skin)
    # at radius = body-radius-at-that-height + band_bulge. We seed-jitter each
    # band's center fraction so the hoop spacing varies run-to-run.
    if band_count > 0:
        half_bh = band_height * 0.5
        # Evenly spaced nominal fractions, inset from the very ends so a band
        # never pokes past a cap. Then nudge each by a seeded jitter.
        for b in range(band_count):
            if band_count == 1:
                frac = 0.5
            else:
                frac = (b + 1) / (band_count + 1)
            # Seeded nudge in fixed order (one draw per band).
            frac += (rng.random() - 0.5) * 0.10
            # Clamp so the whole sleeve stays within the body height.
            lo_frac = (half_bh + 1e-4) / height
            frac = max(lo_frac, min(1.0 - lo_frac, frac))

            z_mid = frac * height
            z_lo = z_mid - half_bh
            z_hi = z_mid + half_bh
            t_lo = max(0.0, min(1.0, z_lo / height))
            t_hi = max(0.0, min(1.0, z_hi / height))
            r_lo = radius_at(t_lo) + band_bulge
            r_hi = radius_at(t_hi) + band_bulge

            lo_start = len(verts)
            verts.extend(_ring(sides, r_lo, z_lo))
            hi_start = len(verts)
            verts.extend(_ring(sides, r_hi, z_hi))
            # Outer skin of the hoop.
            _bridge(verts, faces, lo_start, hi_start, sides)

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
    name = op.get("name", op.get("script", "barrel"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
