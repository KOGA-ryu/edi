"""A CUSTOM CRAFTSMAN: a treasure chest — a rectangular BODY box topped by a
curved half-cylinder LID (segmented across N steps), wrapped by a couple of metal
BANDS and a small front LATCH nub. Same three-part contract as nfold_star.py:

- MANIFEST: id, label, params (only the C++-known types number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real chest offline, like any built-in.
- build(op): the bpy twin, from the SAME proof_mesh (Blender only).

Every dimension is a named param; nothing is hardcoded in the geometry. A seed
integer jitters the proportions (a believable chest is never perfectly square),
drawn in a FIXED order so same-seed is byte-identical and different-seed differs.
The base sits on z=0 so the chest plants on the ground when instanced.
"""

import math
import random


# Silhouette-defining proportion, named rather than inline (the tree precedent).
BAND_LID_CLIMB_FRAC = 0.6   # straps climb this fraction up the lid past the body top

MANIFEST = {
    "id": "chest",
    "label": "Treasure Chest",
    "params": [
        {"key": "width", "label": "Width (X)", "type": "number", "default": 1.0},
        {"key": "depth", "label": "Depth (Y)", "type": "number", "default": 0.6},
        {"key": "bodyHeight", "label": "Body Height", "type": "number", "default": 0.5},
        {"key": "lidHeight", "label": "Lid Height", "type": "number", "default": 0.32},
        {"key": "lidSegments", "label": "Lid Segments", "type": "integer", "default": 8},
        {"key": "bandCount", "label": "Band Count", "type": "integer", "default": 2},
        {"key": "bandWidth", "label": "Band Width", "type": "number", "default": 0.08},
        {"key": "bandBulge", "label": "Band Bulge", "type": "number", "default": 0.02},
        {"key": "latchSize", "label": "Latch Size", "type": "number", "default": 0.14},
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _add_box(verts, faces, cx, cy, x0, x1, y0, y1, z0, z1):
    """Append an axis-aligned box spanning [x0,x1]x[y0,y1]x[z0,z1], centred in
    X/Y about (cx,cy) by the CALLER (we take absolute extents here). Returns
    nothing; outward-facing quad faces are appended with a consistent winding so
    the proof and the bpy mesh agree. cx/cy are unused beyond documenting intent
    — kept explicit so the geometry reads as 'a box at this place'."""
    base = len(verts)
    # 8 corners: bottom 0..3 (CCW from -X-Y), top 4..7 stacked above.
    verts.extend([
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    ])
    b = base
    faces.extend([
        [b + 0, b + 3, b + 2, b + 1],   # bottom (-z), reversed for outward normal
        [b + 4, b + 5, b + 6, b + 7],   # top (+z)
        [b + 0, b + 1, b + 5, b + 4],   # front (-y)
        [b + 1, b + 2, b + 6, b + 5],   # right (+x)
        [b + 2, b + 3, b + 7, b + 6],   # back (+y)
        [b + 3, b + 0, b + 4, b + 7],   # left (-x)
    ])


def _local_mesh(params: dict):
    # Coerce EVERY param defensively — C++ carries them untyped.
    width = max(1e-3, float(params.get("width", 1.0)))
    depth = max(1e-3, float(params.get("depth", 0.6)))
    body_h = max(1e-3, float(params.get("bodyHeight", 0.5)))
    lid_h = max(1e-3, float(params.get("lidHeight", 0.32)))
    lid_segs = max(2, int(float(params.get("lidSegments", 8))))
    band_count = max(0, int(float(params.get("bandCount", 2))))
    band_w = max(0.0, float(params.get("bandWidth", 0.08)))
    band_bulge = max(0.0, float(params.get("bandBulge", 0.02)))
    latch_size = max(0.0, float(params.get("latchSize", 0.14)))
    seed = int(float(params.get("seed", 0)))

    # Seeded variation: a believable chest is never perfectly regular. Draw a
    # FIXED sequence of multipliers so same-seed -> byte-identical, and a
    # different seed visibly nudges the proportions while staying a chest.
    rng = random.Random(seed)
    width *= 1.0 + (rng.random() - 0.5) * 0.30   # +/-15% width
    depth *= 1.0 + (rng.random() - 0.5) * 0.30   # +/-15% depth
    body_h *= 1.0 + (rng.random() - 0.5) * 0.30  # +/-15% body height
    lid_h *= 1.0 + (rng.random() - 0.5) * 0.40   # +/-20% lid height (curvature)
    latch_size *= 1.0 + (rng.random() - 0.5) * 0.30

    hx = width * 0.5
    hy = depth * 0.5

    verts = []
    faces = []

    # --- BODY: a box from z=0 up to body_h, centred on X/Y. ---
    _add_box(verts, faces, 0.0, 0.0, -hx, hx, -hy, hy, 0.0, body_h)

    # --- LID: a half-cylinder lying along X, curved across Y, segmented in N. ---
    # The barrel axis runs along X at the body's top; the cross-section is a
    # half-disc of radius hy, swept from -hy to +hy and rising to +lid_h at apex.
    # We scale the unit half-circle's z by (lid_h / hy) so lidHeight controls the
    # curvature height independently of the depth radius.
    z_scale = lid_h / hy
    # Cross-section profile points (y, z) from -hy (theta=pi) to +hy (theta=0),
    # along the top half of a circle. lid_segs facets across the arc.
    profile = []
    for i in range(lid_segs + 1):
        theta = math.pi * (1.0 - i / lid_segs)   # pi -> 0
        py = math.cos(theta) * hy                # -hy -> +hy
        pz = math.sin(theta) * hy * z_scale      # 0 -> apex -> 0
        profile.append((py, pz + body_h))        # ride on top of the body
    npts = len(profile)

    lid_base = len(verts)
    # End ring at x = -hx, then at x = +hx.
    for (py, pz) in profile:
        verts.append((-hx, py, pz))
    for (py, pz) in profile:
        verts.append((hx, py, pz))
    left0 = lid_base
    right0 = lid_base + npts

    # Barrel side quads between the two end rings.
    for i in range(npts - 1):
        faces.append([left0 + i, left0 + i + 1, right0 + i + 1, right0 + i])

    # Flat back walls of the lid (the two semicircular end caps), each closed by
    # a fan to a base-line midpoint so the half-disc is filled. The chord runs
    # along the body top from -hy to +hy at z=body_h, already the profile's
    # endpoints (i=0 and i=npts-1), so a simple fan over the arc closes it.
    left_mid = len(verts)
    verts.append((-hx, 0.0, body_h))             # left cap fan centre
    for i in range(npts - 1):
        faces.append([left_mid, left0 + i + 1, left0 + i])   # inward (-x) normal
    right_mid = len(verts)
    verts.append((hx, 0.0, body_h))              # right cap fan centre
    for i in range(npts - 1):
        faces.append([right_mid, right0 + i, right0 + i + 1])  # outward (+x)

    # --- BANDS: thin raised straps wrapping the body + lid in the Y/Z plane,
    # spaced evenly across the width. Each band is a slim box bulging out past
    # the body faces in +/-Y; modeled as one box wider in Y than the body. ---
    if band_count > 0 and band_w > 0.0:
        band_top = body_h + lid_h * BAND_LID_CLIMB_FRAC  # bands climb partway up the lid
        for j in range(band_count):
            # Centre each band at an even fraction across the width.
            frac = (j + 1) / (band_count + 1)
            cx = -hx + frac * width
            bx0 = cx - band_w * 0.5
            bx1 = cx + band_w * 0.5
            by0 = -hy - band_bulge
            by1 = hy + band_bulge
            _add_box(verts, faces, cx, 0.0, bx0, bx1, by0, by1, 0.0, band_top)

    # --- LATCH: a small nub on the front face (-y), centred in X, straddling
    # the body/lid seam so it reads as a clasp. A little box poking out in -Y. ---
    if latch_size > 0.0:
        ls = latch_size
        lx0 = -ls * 0.5
        lx1 = ls * 0.5
        lz0 = body_h - ls * 0.5
        lz1 = body_h + ls * 0.5
        ly1 = -hy                  # flush with front face
        ly0 = -hy - ls * 0.5       # pokes out in -Y
        _add_box(verts, faces, 0.0, -hy, lx0, lx1, ly0, ly1, lz0, lz1)

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
    name = op.get("name", op.get("script", "chest"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
