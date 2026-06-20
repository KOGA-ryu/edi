"""A CUSTOM CRAFTSMAN: a wooden shipping crate — a rectangular box framed by
raised corner posts and edge rails, with each of the six faces sunk to an INSET
panel so the silhouette reads as planks-in-a-frame, not a plain cube.

Same three-part contract as nfold_star.py / twisted_column.py:
- MANIFEST: id, label, params (only C++-known types: number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real crate offline, exactly like a built-in op.
- build(op): the bpy twin, fed by the SAME proof_mesh (Blender only).

A crate is a frame, not a smooth solid: we model it as a set of axis-aligned
boxes — twelve edge rails (the wooden frame members) plus one recessed panel
slab per face. The recessed panels sit a hair INSIDE the rail cage, so from
any angle you see a raised border around a sunken plank face. Seeded variation
jitters width/depth/height a few percent so a row of crates does not read as
clones, while staying recognizably the same crate.
"""

import math
import random

MANIFEST = {
    "id": "crate",
    "label": "Wooden Crate",
    "params": [
        {"key": "width", "label": "Width", "type": "number", "default": 1.0},
        {"key": "depth", "label": "Depth", "type": "number", "default": 1.0},
        {"key": "height", "label": "Height", "type": "number", "default": 1.0},
        {"key": "frameThickness", "label": "Frame Thickness", "type": "number", "default": 0.12},
        {"key": "panelInset", "label": "Panel Inset", "type": "number", "default": 0.04},
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _box(x0, y0, z0, x1, y1, z1, verts, faces):
    """Append one axis-aligned box [x0,x1]x[y0,y1]x[z0,z1] to verts/faces.

    Each call adds 8 fresh corner verts and 6 quad faces. We deliberately do NOT
    weld shared corners between boxes — the rails and panels are distinct wooden
    members that merely touch, so independent verts keep each box a closed,
    self-contained manifold (no T-junctions to reason about). Winding is
    outward-facing (CCW seen from outside) so normals bake correctly.
    """
    base = len(verts)
    verts.extend([
        (x0, y0, z0),  # 0
        (x1, y0, z0),  # 1
        (x1, y1, z0),  # 2
        (x0, y1, z0),  # 3
        (x0, y0, z1),  # 4
        (x1, y0, z1),  # 5
        (x1, y1, z1),  # 6
        (x0, y1, z1),  # 7
    ])
    b = base
    faces.extend([
        [b + 0, b + 3, b + 2, b + 1],  # bottom (-z), CCW from below
        [b + 4, b + 5, b + 6, b + 7],  # top    (+z)
        [b + 0, b + 1, b + 5, b + 4],  # front  (-y)
        [b + 2, b + 3, b + 7, b + 6],  # back   (+y)
        [b + 1, b + 2, b + 6, b + 5],  # right  (+x)
        [b + 3, b + 0, b + 4, b + 7],  # left   (-x)
    ])


def _local_mesh(params: dict):
    # C++ carries params untyped, so coerce EVERY value defensively.
    width = max(1e-3, float(params.get("width", 1.0)))
    depth = max(1e-3, float(params.get("depth", 1.0)))
    height = max(1e-3, float(params.get("height", 1.0)))
    frame = max(1e-3, float(params.get("frameThickness", 0.12)))
    inset = max(0.0, float(params.get("panelInset", 0.04)))

    # Seeded variation: one Random, values drawn in a FIXED order so same-seed is
    # byte-identical and different-seed visibly differs. We jitter the three
    # outer dimensions a few percent — enough to break the clone look, never
    # enough to stop reading as the same crate.
    rng = random.Random(int(float(params.get("seed", 0))))
    width *= 1.0 + rng.uniform(-0.06, 0.06)
    depth *= 1.0 + rng.uniform(-0.06, 0.06)
    height *= 1.0 + rng.uniform(-0.06, 0.06)

    # The frame member is a fraction of the smallest dimension so a thin crate
    # never gets rails thicker than half the box. Clamp to keep two opposite
    # rails from overlapping in the middle (leaves room for a real panel).
    smallest = min(width, depth, height)
    t = min(frame, smallest * 0.45)

    # Centre the box on x/y, base on z=0 so it plants on the ground when placed.
    hx, hy = width * 0.5, depth * 0.5
    x0, x1 = -hx, hx
    y0, y1 = -hy, hy
    z0, z1 = 0.0, height

    verts: list = []
    faces: list = []

    # --- Twelve edge rails: the wooden frame cage ----------------------------
    # 4 vertical corner posts (full height, t x t footprint at each corner).
    for cx in (x0, x1 - t):
        for cy in (y0, y1 - t):
            _box(cx, cy, z0, cx + t, cy + t, z1, verts, faces)

    # 4 bottom horizontal rails + 4 top horizontal rails. Each runs BETWEEN the
    # corner posts so it does not double up with them: x-rails span [x0+t, x1-t],
    # y-rails span [y0+t, y1-t].
    for rz in (z0, z1 - t):
        # rails running along x (front and back edges)
        _box(x0 + t, y0, rz, x1 - t, y0 + t, rz + t, verts, faces)
        _box(x0 + t, y1 - t, rz, x1 - t, y1, rz + t, verts, faces)
        # rails running along y (left and right edges)
        _box(x0, y0 + t, rz, x0 + t, y1 - t, rz + t, verts, faces)
        _box(x1 - t, y0 + t, rz, x1, y1 - t, rz + t, verts, faces)

    # --- Six recessed face panels: the sunken plank fields -------------------
    # Each panel fills the window framed by the rails on that face, pulled inward
    # by `inset` so a raised border surrounds a sunken face. Panel slab is `t`
    # thick so it has real depth (not a zero-volume sheet). Inset is clamped so a
    # panel never inverts on a small crate.
    panel_inset = min(inset, t * 0.6)

    # Span of the framed window on each axis (between the rails).
    px0, px1 = x0 + t, x1 - t
    py0, py1 = y0 + t, y1 - t
    pz0, pz1 = z0 + t, z1 - t

    # Bottom panel (-z) and top panel (+z): horizontal slabs in the x/y window,
    # sunk down from / up to the rail plane by panel_inset.
    _box(px0, py0, z0 + panel_inset, px1, py1, z0 + panel_inset + t, verts, faces)
    _box(px0, py0, z1 - panel_inset - t, px1, py1, z1 - panel_inset, verts, faces)

    # Front (-y) and back (+y) panels: slabs in the x/z window, recessed in y.
    _box(px0, y0 + panel_inset, pz0, px1, y0 + panel_inset + t, pz1, verts, faces)
    _box(px0, y1 - panel_inset - t, pz0, px1, y1 - panel_inset, pz1, verts, faces)

    # Left (-x) and right (+x) panels: slabs in the y/z window, recessed in x.
    _box(x0 + panel_inset, py0, pz0, x0 + panel_inset + t, py1, pz1, verts, faces)
    _box(x1 - panel_inset - t, py0, pz0, x1 - panel_inset, py1, pz1, verts, faces)

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
    name = op.get("name", op.get("script", "crate"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
