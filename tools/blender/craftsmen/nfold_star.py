"""A CUSTOM CRAFTSMAN: an {n/k} star prism — the girih / star figure, a
deterministic compass construction the built-in op vocabulary cannot express.
Sibling of radial_petal.py; same three-part contract as twisted_column.py.

- MANIFEST: id, label, params (only the C++-known types number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof renders
  the real star prism offline, like any built-in.
- build(op): the bpy twin, from the SAME proof_mesh (Blender only).

This is a TOOL the author dials, not a generator: every vertex is a closed-form
function of the params, nothing random or auto-laid-out.
"""

import math

MANIFEST = {
    "id": "nfold_star",
    "label": "N-Fold Star {n/k}",
    "params": [
        {"key": "points", "label": "Points (n)", "type": "integer", "default": 5},
        {"key": "skip", "label": "Skip (k)", "type": "integer", "default": 2},
        {"key": "outerRadius", "label": "Outer Radius", "type": "number", "default": 1.0},
        {"key": "innerRadius", "label": "Inner Radius", "type": "number", "default": 0.4},
        {"key": "height", "label": "Height", "type": "number", "default": 0.5},
        {"key": "material", "label": "Material", "type": "material", "default": "stone"},
    ],
}


def _resolve_k(n: int, raw_k: int) -> int:
    """The {n/k} guard. C++ carries params untyped, so coerce + clamp
    defensively. Valid range is 2 <= k < n; a true single-stroke star also wants
    gcd(k, n) == 1. Rule: clamp k into [2, n-1], then if it is not coprime with n
    snap to the NEAREST coprime in that range (n-1 is always coprime to n, so a
    coprime always exists for n >= 3). A degenerate k (k <= 1, k >= n, or a
    non-coprime like {6/2}) is thus handled gracefully — it never crashes and
    never yields an empty or zero-area mesh; it lands on the closest legitimate
    star instead."""
    if n < 3:
        n = 3
    lo, hi = 2, n - 1
    k = max(lo, min(hi, raw_k))
    if math.gcd(k, n) == 1:
        return k
    for delta in range(1, n):  # search outward for the nearest coprime in range
        for cand in (k - delta, k + delta):
            if lo <= cand <= hi and math.gcd(cand, n) == 1:
                return cand
    return k  # unreachable for n >= 3, but never crash


def _local_mesh(params: dict):
    n = max(3, int(float(params.get("points", 5))))
    k = _resolve_k(n, int(float(params.get("skip", 2))))
    outer = max(1e-4, float(params.get("outerRadius", 1.0)))
    inner = max(1e-4, float(params.get("innerRadius", 0.4)))
    height = float(params.get("height", 0.5))

    # k drives the star's SHARPNESS via the valley (inner) radius: a larger skip
    # pulls the valleys inward, narrowing the points. Deterministic and bounded
    # so the outline is always well-formed (inner strictly between a tiny floor
    # and the outer radius — never collapsed, never crossing the tips).
    eff_inner = inner / (k - 1)
    eff_inner = max(1e-3 * outer, min(eff_inner, outer * 0.95))

    # The outline: a 2n-vertex alternating ring (tip / valley / tip / ...),
    # angular step pi/n — tips at 2*pi*i/n, valleys halfway between.
    outline = []
    for j in range(2 * n):
        radius = outer if j % 2 == 0 else eff_inner
        angle = math.pi * j / n
        outline.append((math.cos(angle) * radius, math.sin(angle) * radius))

    m = 2 * n
    verts = [(px, py, 0.0) for px, py in outline]          # 0..m-1   bottom
    verts += [(px, py, height) for px, py in outline]      # m..2m-1  top
    faces = []
    faces.append(list(range(m - 1, -1, -1)))               # bottom cap (reversed)
    faces.append(list(range(m, 2 * m)))                    # top cap
    for j in range(m):                                     # side quads
        nj = (j + 1) % m
        faces.append([j, nj, m + nj, m + j])
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
    name = op.get("name", op.get("script", "nfold_star"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
