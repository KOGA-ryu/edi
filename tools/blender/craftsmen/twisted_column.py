"""A sample CUSTOM CRAFTSMAN: a column whose N-gon cross-section twists with
height — a recipe step the built-in vocabulary cannot express. Drop more .py
files beside this one (each with a MANIFEST + proof_mesh + build) and they
appear in the lab's palette automatically.

The contract (scanned by edi_craft.load_craftsmen):
- MANIFEST: id, label, and params (key/label/type/default) — the palette offers
  the craftsman and the inspector renders its fields from this.
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the ASCII/OBJ proof renders
  the real shape, so a custom step is proved like any built-in one.
- build(op): the bpy execution (only ever called inside Blender).
"""

import math

MANIFEST = {
    "id": "twisted_column",
    "label": "Twisted Column",
    "params": [
        {"key": "radius", "label": "Radius", "type": "number", "default": 1.0},
        {"key": "height", "label": "Height", "type": "number", "default": 5.0},
        {"key": "sides", "label": "Sides", "type": "integer", "default": 4},
        {"key": "turns", "label": "Turns", "type": "number", "default": 1.0},
        {"key": "rings", "label": "Rings", "type": "integer", "default": 24},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _local_mesh(params: dict):
    radius = float(params.get("radius", 1.0))
    height = float(params.get("height", 5.0))
    sides = max(3, int(float(params.get("sides", 4))))
    turns = float(params.get("turns", 1.0))
    rings = max(1, int(float(params.get("rings", 24))))
    verts = []
    for ring in range(rings + 1):
        t = ring / rings
        z = -height * 0.5 + height * t
        twist = math.tau * turns * t
        for i in range(sides):
            angle = math.tau * i / sides + twist
            verts.append((math.cos(angle) * radius, math.sin(angle) * radius, z))
    faces = []
    for ring in range(rings):
        current = ring * sides
        nxt_ring = (ring + 1) * sides
        for i in range(sides):
            ni = (i + 1) % sides
            faces.append([current + i, current + ni, nxt_ring + ni, nxt_ring + i])
    faces.append(list(range(sides - 1, -1, -1)))
    top = rings * sides
    faces.append(list(range(top, top + sides)))
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
    name = op.get("name", op.get("script", "twisted_column"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
