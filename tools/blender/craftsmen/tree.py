"""A CUSTOM CRAFTSMAN: a procedural TREE — the zoo's first ORGANIC asset, and
the first craftsman whose silhouette is GROWN (a recursion over named params +
seed) rather than dialled vertex-by-vertex like nfold_star / twisted_column.

Technique (see docs/architecture/tree-asset.md): RECURSIVE PARAMETRIC BRANCHING
(Weber-Penn lineage) reduced to the number/integer/material MANIFEST the C++
ScriptOp can carry. A depth-first recursion spawns child branches per node,
decaying length + radius by fixed ratios with seeded jitter; the pipe model sets
each child's radius from its parent. The skeleton is SKINNED to tapered-tube
(verts,faces) and the crown is clumped icospheres — ALL in pure Python, so the
proof tier (proof_mesh) and the bpy tier (build) run the SAME generator.

THIS FILE IS L0 (FORM) of the L0->L5 detail ladder. L0 builds ONLY the scaffold
the silhouette test (rubric A1+A4) needs:
  - a tapered vertical trunk (stacked ring segments, base z=0 -> z=height),
  - ONE crude crown blob (a subdivided-octahedron "icosphere") in the upper crown.
No branching, no seed jitter yet — branching arrives at L1, jitter at L5. The
FULL 24-param MANIFEST is declared NOW (most params ignored at L0) so the recipe
schema is stable across every level and the saved recipes never churn.

The contract (scanned by edi_craft.load_craftsmen):
- MANIFEST: id, label, params (key/label/type — only number/integer/material).
- proof_mesh(op) -> (verts, faces): PURE, no bpy — the OBJ/ASCII proof tier.
- build(op): the bpy twin — calls proof_mesh and ONLY instantiates via from_pydata
  (the nfold_star pattern). NO second generator, NO RNG in build (dual-tier
  parity F4 + same-seed determinism E2).

Origin convention: trunk base at the op's (x,y,z) with the local mesh starting at
z=0 and growing +z (the nfold_star convention, NOT twisted_column's centred one),
so an instance plants its base on the terrain at its placement point (rubric F3).
"""

import math

MANIFEST = {
    "id": "tree",
    "label": "Tree (procedural)",
    # The FULL Weber-Penn-reduced param set, declared up front for schema
    # stability across L0->L5. At L0 only height/trunkRadius/taper/tubeSides/
    # baseFlare/firstBranchHeight/clumpSize feed geometry; the rest are parsed
    # and IGNORED until their ladder level switches them on. Types are limited to
    # number/integer/material — the only kinds the C++ ScriptOp carries.
    "params": [
        {"key": "seed", "label": "Seed", "type": "integer", "default": 0},
        {"key": "height", "label": "Height (m)", "type": "number", "default": 6.0},
        {"key": "trunkRadius", "label": "Trunk Radius", "type": "number", "default": 0.18},
        {"key": "taper", "label": "Taper", "type": "number", "default": 0.85},
        {"key": "branchLevels", "label": "Branch Levels", "type": "integer", "default": 4},
        {"key": "childrenPerNode", "label": "Children / Node", "type": "integer", "default": 3},
        {"key": "firstBranchHeight", "label": "First Branch Height (frac)", "type": "number", "default": 0.35},
        {"key": "downAngle", "label": "Down Angle (deg)", "type": "number", "default": 45.0},
        {"key": "downAngleJitter", "label": "Down Angle Jitter (deg)", "type": "number", "default": 12.0},
        {"key": "rotateAngle", "label": "Rotate Angle (deg)", "type": "number", "default": 137.5},
        {"key": "rotateJitter", "label": "Rotate Jitter (deg)", "type": "number", "default": 20.0},
        {"key": "lengthRatio", "label": "Length Ratio", "type": "number", "default": 0.72},
        {"key": "lengthJitter", "label": "Length Jitter (frac)", "type": "number", "default": 0.15},
        {"key": "pipeExponent", "label": "Pipe Exponent", "type": "number", "default": 2.2},
        {"key": "curve", "label": "Curve (deg)", "type": "number", "default": 15.0},
        {"key": "segmentsPerBranch", "label": "Segments / Branch", "type": "integer", "default": 4},
        {"key": "tubeSides", "label": "Tube Sides", "type": "integer", "default": 6},
        {"key": "clumpCount", "label": "Clump Count", "type": "integer", "default": 14},
        {"key": "clumpSize", "label": "Clump Size", "type": "number", "default": 0.55},
        {"key": "clumpJitter", "label": "Clump Jitter (frac)", "type": "number", "default": 0.35},
        {"key": "leafSubdiv", "label": "Leaf Subdiv", "type": "integer", "default": 1},
        {"key": "baseFlare", "label": "Base Flare", "type": "number", "default": 1.6},
        # Materials default to EXISTING MATERIALS keys (aged_stone reads as weathered
        # bark; sandstone is the closest warm tone for foliage). "bark"/"leaf" are
        # NOT in the table — defaulting to them would breach additive-only. Materials
        # don't affect the OBJ proof; this only bites at the L4 material slice.
        {"key": "barkMat", "label": "Bark Material", "type": "material", "default": "aged_stone"},
        {"key": "leafMat", "label": "Leaf Material", "type": "material", "default": "sandstone"},
    ],
}


def _ring(cx, cy, cz, radius, sides):
    """A horizontal ring of `sides` verts centred at (cx,cy,cz). Returned as a
    flat list of (x,y,z); the loft below joins consecutive rings into quads."""
    out = []
    for i in range(sides):
        angle = math.tau * i / sides
        out.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius, cz))
    return out


def _trunk_mesh(height, trunk_radius, taper, base_flare, segments, sides):
    """A tapered vertical trunk: `segments` stacked ring loops from z=0 to
    z=height, each ring's radius scaled by `taper` per segment so the trunk
    narrows with height, and the very bottom ring widened by `base_flare` (root
    buttress). Closed at the bottom; the OPEN top ring is where the crown sits
    (the crown blob overlaps it — no weld needed, per the L0 'overlap, don't
    weld' rule R1). Returns (verts, faces, top_radius).

    WHY stacked segments and not a single cone: stacking lets each ring carry the
    geometric taper (radius *= taper per segment) so the profile reads as a real
    trunk that thins smoothly, and it gives L1 the ring loops to graft branches
    onto later without changing the L0 vertex layout."""
    sides = max(3, sides)
    segments = max(1, segments)

    radii = []
    radius = trunk_radius
    for seg in range(segments + 1):
        # The base ring gets the flare multiplier; every higher ring tapers
        # geometrically. taper is the fraction of radius RETAINED per segment.
        flare = base_flare if seg == 0 else 1.0
        radii.append(max(1e-4, radius * flare))
        radius *= taper

    verts = []
    for seg in range(segments + 1):
        z = height * seg / segments
        verts.extend(_ring(0.0, 0.0, z, radii[seg], sides))

    faces = []
    # Bottom cap (reversed winding so its normal points down/out).
    faces.append(list(range(sides - 1, -1, -1)))
    # Side quads bridging ring seg -> ring seg+1.
    for seg in range(segments):
        base = seg * sides
        nxt = (seg + 1) * sides
        for i in range(sides):
            ni = (i + 1) % sides
            faces.append([base + i, base + ni, nxt + ni, nxt + i])
    # NOTE: the top ring is left OPEN — the crown blob caps the silhouette there.
    return verts, faces, radii[-1]


def _icosphere(cx, cy, cz, radius, subdiv):
    """A cheap 'icosphere' as a recursively-subdivided OCTAHEDRON projected onto
    a sphere. subdiv=0 is the bare 8-face octahedron; each subdiv level splits
    every triangle into 4 (so subdiv=1 -> 32 tris). Pure Python, no bpy.

    WHY a subdivided octahedron and not a true icosahedron: the octahedron's 6
    seed verts sit on the axes, so the generator is a few lines and stays cheap;
    the projection-to-sphere step makes it read as a round blob either way. The
    crown is a placeholder mass at L0 — L3 replaces this one blob with clumpCount
    clumps at the branch tips."""
    subdiv = max(0, subdiv)

    # Octahedron seed: 6 verts on the unit axes, 8 triangles.
    base_verts = [
        (1.0, 0.0, 0.0), (-1.0, 0.0, 0.0),
        (0.0, 1.0, 0.0), (0.0, -1.0, 0.0),
        (0.0, 0.0, 1.0), (0.0, 0.0, -1.0),
    ]
    base_faces = [
        (0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4),
        (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5),
    ]

    verts = [list(v) for v in base_verts]
    faces = list(base_faces)
    # Cache midpoints by an ordered-pair key so a shared edge yields ONE shared
    # vertex (no cracks). A dict keyed by a sorted tuple is order-stable here —
    # we only LOOK UP by key, never ITERATE the dict, so determinism holds.
    midcache = {}

    def midpoint(a, b):
        key = (a, b) if a < b else (b, a)
        if key in midcache:
            return midcache[key]
        ax, ay, az = verts[a]
        bx, by, bz = verts[b]
        m = [(ax + bx) * 0.5, (ay + by) * 0.5, (az + bz) * 0.5]
        idx = len(verts)
        verts.append(m)
        midcache[key] = idx
        return idx

    for _ in range(subdiv):
        new_faces = []
        for (a, b, c) in faces:
            ab = midpoint(a, b)
            bc = midpoint(b, c)
            ca = midpoint(c, a)
            new_faces.extend([(a, ab, ca), (ab, b, bc), (ca, bc, c), (ab, bc, ca)])
        faces = new_faces

    # Project every vert onto the sphere of `radius`, then offset to the centre.
    out_verts = []
    for (vx, vy, vz) in verts:
        length = math.sqrt(vx * vx + vy * vy + vz * vz) or 1.0
        s = radius / length
        out_verts.append((cx + vx * s, cy + vy * s, cz + vz * s))
    return out_verts, [list(f) for f in faces]


def _local_mesh(params: dict):
    """The L0 tree as local (verts, faces) with the trunk base at z=0.

    L0 = tapered trunk + ONE crown blob. No branching, no RNG (seed is read for
    schema/determinism plumbing but unused until L1+)."""
    height = max(1e-3, float(params.get("height", 6.0)))
    trunk_radius = max(1e-4, float(params.get("trunkRadius", 0.18)))
    taper = float(params.get("taper", 0.85))
    base_flare = float(params.get("baseFlare", 1.6))
    tube_sides = max(3, int(float(params.get("tubeSides", 6))))
    leaf_subdiv = max(0, int(float(params.get("leafSubdiv", 1))))
    clump_size = max(1e-4, float(params.get("clumpSize", 0.55)))
    first_branch = float(params.get("firstBranchHeight", 0.35))

    # The L0 trunk uses a FIXED segment count (not segmentsPerBranch, which is a
    # per-BRANCH knob arriving at L1) — enough rings to read as a smooth taper.
    TRUNK_SEGMENTS = 4  # local: smooth-enough taper at L0 without per-level cost
    trunk_verts, trunk_faces, top_radius = _trunk_mesh(
        height, trunk_radius, taper, base_flare, TRUNK_SEGMENTS, tube_sides)

    # The crown blob fills the upper crown. Its RADIUS must read at the rubric's
    # crown-width band (A2: crown width 0.5-1.0x height). clumpSize alone (0.55 m)
    # is far too small for a 6 m tree, so derive a crown radius from height and
    # let clumpSize MODULATE it — a named relationship, no bare magic literal.
    CROWN_WIDTH_FRACTION = 0.6   # crown diameter ~0.6x height (mid of A2's 0.5-1.0 band)
    CLUMP_SIZE_REFERENCE = 0.55  # the default clumpSize: ratio to it scales the crown
    crown_radius = (height * CROWN_WIDTH_FRACTION * 0.5) * (clump_size / CLUMP_SIZE_REFERENCE)

    # Centre the blob in the upper crown: sit its CENTRE so the crown spans from
    # roughly firstBranchHeight*height up to the top, then nudge so the blob top
    # is near the tree top. Centre = midpoint of (first-branch height, height),
    # biased up toward the canopy.
    crown_band_low = first_branch * height
    crown_centre_z = (crown_band_low + height) * 0.5
    # Lift so the crown's top reaches ~height (the blob caps the silhouette).
    crown_centre_z = min(crown_centre_z + crown_radius * 0.5, height)

    crown_verts, crown_faces = _icosphere(
        0.0, 0.0, crown_centre_z, crown_radius, leaf_subdiv)

    # Concatenate: crown face indices shift by the trunk's vertex count (the two
    # meshes overlap at the trunk top — intentional, no weld at L0, per R1).
    offset = len(trunk_verts)
    verts = list(trunk_verts) + list(crown_verts)
    faces = list(trunk_faces) + [[i + offset for i in f] for f in crown_faces]
    return verts, faces


def proof_mesh(op: dict):
    """Pure (verts, faces) for the proof, placed at the op's x/y/z (base at z=0)."""
    verts, faces = _local_mesh(op.get("params", {}))
    x = float(op.get("x", 0.0))
    y = float(op.get("y", 0.0))
    z = float(op.get("z", 0.0))
    return [(vx + x, vy + y, vz + z) for vx, vy, vz in verts], faces


def build(op: dict):  # pragma: no cover — exercised in Blender
    import bpy

    verts, faces = proof_mesh(op)
    name = op.get("name", op.get("script", "tree"))
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj
