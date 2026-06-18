"""A CUSTOM CRAFTSMAN: a radial-petal bloom — N petals arrayed around a central
hub, the rose-window / flower move. Like every craftsman it is a recipe step the
built-in op vocabulary cannot express, dropped beside twisted_column.py and
discovered automatically by edi_craft.load_craftsmen.

The contract (identical to twisted_column.py):
- MANIFEST: id, label, params (key/label/type/default) — only the param types
  the C++ side knows (number / integer / material), so the registry TOML
  round-trips through parseCraftsmanRegistryToml.
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real bloom, so this custom step is provable offline like any built-in.
- build(op): the bpy execution (Blender only), from the SAME proof_mesh verts/
  faces, so the headless proof and the Blender build cannot drift.

This is a TOOL the author dials, not a generator: every vertex is a closed-form
function of the params — nothing random, nothing auto-laid-out.
"""

import math

MANIFEST = {
    "id": "radial_petal",
    "label": "Radial Petal Bloom",
    "params": [
        {"key": "petals", "label": "Petals", "type": "integer", "default": 8},
        {"key": "petalLength", "label": "Petal Length", "type": "number", "default": 1.0},
        {"key": "petalWidth", "label": "Petal Width", "type": "number", "default": 0.4},
        {"key": "centerRadius", "label": "Center Radius", "type": "number", "default": 0.3},
        {"key": "zRise", "label": "Z Rise", "type": "number", "default": 0.15},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _local_mesh(params: dict):
    petals = max(3, int(float(params.get("petals", 8))))
    petal_length = float(params.get("petalLength", 1.0))
    petal_width = float(params.get("petalWidth", 0.4))
    center_radius = max(1e-4, float(params.get("centerRadius", 0.3)))
    z_rise = float(params.get("zRise", 0.15))

    # Vertex 0 is the hub center; each petal then contributes a 4-vertex kite
    # (a leaf): a base on the hub ring, two mid-flanks half-way out, and a lifted
    # tip. The hub is a fan of triangles from the center to the petal bases.
    verts = [(0.0, 0.0, 0.0)]
    mid_radius = center_radius + petal_length * 0.5
    tip_radius = center_radius + petal_length
    # Tangential half-width expressed as an angle, measured at the mid radius so
    # petalWidth is the petal's real width there.
    half_angle = (petal_width * 0.5) / mid_radius

    for i in range(petals):
        a = math.tau * i / petals
        # base on the hub ring
        verts.append((math.cos(a) * center_radius, math.sin(a) * center_radius, 0.0))
        # left flank
        verts.append((math.cos(a - half_angle) * mid_radius,
                      math.sin(a - half_angle) * mid_radius, z_rise * 0.5))
        # lifted tip
        verts.append((math.cos(a) * tip_radius, math.sin(a) * tip_radius, z_rise))
        # right flank
        verts.append((math.cos(a + half_angle) * mid_radius,
                      math.sin(a + half_angle) * mid_radius, z_rise * 0.5))

    faces = []
    # Hub fan: center to consecutive petal bases (wrapping shut).
    for i in range(petals):
        base = 1 + i * 4
        nxt_base = 1 + ((i + 1) % petals) * 4
        faces.append([0, base, nxt_base])
    # Each petal kite: base -> left flank -> tip -> right flank.
    for i in range(petals):
        base = 1 + i * 4
        faces.append([base, base + 1, base + 2, base + 3])
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
    name = op.get("name", op.get("script", "radial_petal"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
