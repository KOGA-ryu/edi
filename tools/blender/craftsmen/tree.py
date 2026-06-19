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

THIS FILE IS L1 (ARMATURE) of the L0->L5 detail ladder. L0 built ONLY the
scaffold the silhouette test needed (a tapered trunk + one placeholder crown
blob). L1 GROWS the recursive self-similar SKELETON off the trunk and skins each
branch as a bare tapered tube — the crown blob is GONE; a bare winter-tree
armature is the correct L1 silhouette (foliage returns at L3). The gate is
BRANCHING C1+C4:
  - C1: >=3 visible branch levels (trunk -> primary -> secondary -> tertiary),
  - C4: successive children SPIRAL around the parent at the golden azimuth
    (rotateAngle, default 137.5 deg), not coplanar / not all one side.

Params switched ON this level: branchLevels, childrenPerNode, downAngle,
rotateAngle, lengthRatio, segmentsPerBranch, firstBranchHeight. DEFERRED to later
levels (parsed + IGNORED for now, so the schema is stable): pipeExponent + per-
segment curve + taper refinement = L2; clump* CANOPY = L3; baseFlare/crown-ratio
tuning = L4; the *Jitter knobs + seeded RNG = L5. seed is read but INERT at L1 —
the golden angle is EXACT, there is NO jitter, so the armature is deterministic
without touching random at all (RNG arrives at L5).

The FULL 24-param MANIFEST is declared NOW (most params still ignored) so the
recipe schema is stable across every level and saved recipes never churn.

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
    # stability across L0->L5. At L1 height/trunkRadius/taper/tubeSides/baseFlare
    # feed the trunk, and branchLevels/childrenPerNode/downAngle/rotateAngle/
    # lengthRatio/segmentsPerBranch/firstBranchHeight feed the armature; the rest
    # are parsed and IGNORED until their ladder level switches them on. Types are
    # limited to number/integer/material — the only kinds the C++ ScriptOp carries.
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

# --- NAMED L1 constants (no bare magic literals in the recursion logic) -------
# Crude per-level radius decay. The PIPE MODEL (r_child = r_p*(1/n)^(1/e), the
# pipeExponent formula) is L2's gated refinement — at L1 we just thin each child
# by a fixed fraction of its parent so the hierarchy READS as thinning without
# pre-empting L2's critique. 0.62 is the value the default pipe model (n=3,
# e=2.2) would give, chosen so L2 can swap in the real formula with minimal
# silhouette shift.
CHILD_RADIUS_DECAY = 0.62
# The trunk top (where the first branch ring of children sprouts) keeps this
# fraction of the trunk-base radius as the seed radius for the primary branches —
# a believable "the limbs are thinner than the trunk" start, refined by the pipe
# model at L2.
TRUNK_TIP_RADIUS_FRAC = 0.7
# A branch tip tapers to this fraction of its start radius along its own length
# (a single gentle taper per branch — the per-segment `taper`/`curve` profile is
# L2). Keeps tubes from ending in a blunt cylinder.
BRANCH_TIP_TAPER = 0.6


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
    buttress). Closed at the bottom AND the top (a cap ring) so the trunk reads
    as a solid tube; branches overlap it at the top (no weld, per R1). Returns
    (verts, faces, top_radius).

    WHY stacked segments and not a single cone: stacking lets each ring carry the
    geometric taper (radius *= taper per segment) so the profile reads as a real
    trunk that thins smoothly, and it gives the armature the trunk-top ring to
    sprout primary branches from."""
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
    # Top cap (so the trunk is a closed tube now that no crown blob covers it).
    top = segments * sides
    faces.append(list(range(top, top + sides)))
    return verts, faces, radii[-1]


# --- The recursive ARMATURE skeleton -----------------------------------------
#
# A SKELETON node carries (position, radius, level). A BRANCH is a polyline of
# such nodes (segmentsPerBranch+1 of them) plus its parent's index. We build the
# whole skeleton FIRST, in a FIXED depth-first child-index order, then skin it.
#
# WHY build the skeleton first (and in fixed order) rather than emit geometry
# inline during recursion: at L5 a seeded RNG will perturb each node's angle and
# length. Determinism (rubric E2 + R3) demands the RNG be drawn in the SAME order
# in both the proof tier and the bpy build. A skeleton built by a deterministic
# depth-first walk (children visited in index 0..n-1 order) gives that fixed
# traversal a stable spine to hang the (future) RNG draws on, and keeps the skin
# pass a pure function of the skeleton. At L1 there is no RNG yet, but the
# ORDER is locked in now so L5 is a drop-in.


def _axis_frame(axis):
    """Given a unit `axis` direction, return two unit vectors (u, v) spanning the
    plane perpendicular to it. Children are rotated within this frame by the
    azimuth angle, then tilted toward `axis`. A stable choice of `up` avoids a
    degenerate cross product when the axis is near-vertical."""
    ax, ay, az = axis
    # Pick a reference not parallel to the axis: +x unless the axis is ~+x/-x.
    if abs(ax) < 0.9:
        ref = (1.0, 0.0, 0.0)
    else:
        ref = (0.0, 1.0, 0.0)
    # u = normalize(axis x ref)
    ux = ay * ref[2] - az * ref[1]
    uy = az * ref[0] - ax * ref[2]
    uz = ax * ref[1] - ay * ref[0]
    ulen = math.sqrt(ux * ux + uy * uy + uz * uz) or 1.0
    u = (ux / ulen, uy / ulen, uz / ulen)
    # v = axis x u (already unit since axis and u are orthonormal)
    vx = ay * u[2] - az * u[1]
    vy = az * u[0] - ax * u[2]
    vz = ax * u[1] - ay * u[0]
    return u, (vx, vy, vz)


def _child_axis(parent_axis, down_angle_rad, azimuth_rad):
    """A child branch axis: take the parent axis, tilt it by `down_angle` toward a
    direction in the perpendicular plane chosen by `azimuth`, and renormalise.

    The azimuth picks a compass direction (u*cos + v*sin) in the plane normal to
    the parent; tilting blends `cos(down)` of the parent axis with `sin(down)` of
    that perpendicular direction. Successive children share the same down_angle
    but step their azimuth by the golden angle, so they SPIRAL (C4)."""
    u, v = _axis_frame(parent_axis)
    # Perpendicular "lean" direction for this child's azimuth.
    px = u[0] * math.cos(azimuth_rad) + v[0] * math.sin(azimuth_rad)
    py = u[1] * math.cos(azimuth_rad) + v[1] * math.sin(azimuth_rad)
    pz = u[2] * math.cos(azimuth_rad) + v[2] * math.sin(azimuth_rad)
    c, s = math.cos(down_angle_rad), math.sin(down_angle_rad)
    ax = parent_axis[0] * c + px * s
    ay = parent_axis[1] * c + py * s
    az = parent_axis[2] * c + pz * s
    alen = math.sqrt(ax * ax + ay * ay + az * az) or 1.0
    return (ax / alen, ay / alen, az / alen)


def _skeleton(params: dict):
    """Build the recursive branching skeleton as a list of BRANCHES. Each branch
    is a dict with:
        nodes : [(x, y, z), ...]   (segments_per_branch + 1 points, start->tip)
        radii : [r0, ..., r_tip]   (radius at each node, gently tapering)
        level : int                (0 = trunk, 1 = primary, ...)
    The trunk is branch 0 (level 0). Primary branches sprout from the trunk top;
    each branch tip then spawns `childrenPerNode` children to depth branchLevels.

    Pure + deterministic: the only angle source is the EXACT golden azimuth
    (rotateAngle * child_index) — no RNG at L1. Depth-first, children in index
    order, so L5's seeded jitter has a fixed traversal to perturb."""
    height = max(1e-3, float(params.get("height", 6.0)))
    trunk_radius = max(1e-4, float(params.get("trunkRadius", 0.18)))
    branch_levels = max(0, int(float(params.get("branchLevels", 4))))
    children_per_node = max(1, int(float(params.get("childrenPerNode", 3))))
    first_branch = float(params.get("firstBranchHeight", 0.35))
    down_angle = math.radians(float(params.get("downAngle", 45.0)))
    rotate_angle = math.radians(float(params.get("rotateAngle", 137.5)))
    length_ratio = float(params.get("lengthRatio", 0.72))
    segments = max(1, int(float(params.get("segmentsPerBranch", 4))))

    branches = []

    def add_branch(start, axis, length, radius, level):
        """Append one straight branch of `segments` even steps from `start` along
        `axis`, tapering radius from `radius` to radius*BRANCH_TIP_TAPER, then —
        if we are not yet at branchLevels — recurse children off its TIP. Returns
        the branch's index in `branches`."""
        nodes = []
        radii = []
        for s in range(segments + 1):
            t = s / segments
            nodes.append((start[0] + axis[0] * length * t,
                          start[1] + axis[1] * length * t,
                          start[2] + axis[2] * length * t))
            # Linear taper along the branch from `radius` down to the tip frac.
            radii.append(radius * (1.0 - (1.0 - BRANCH_TIP_TAPER) * t))
        idx = len(branches)
        branches.append({"nodes": nodes, "radii": radii, "level": level})

        if level < branch_levels:
            tip = nodes[-1]
            child_len = length * length_ratio
            child_rad = radius * CHILD_RADIUS_DECAY
            # Children spawn off the tip, each stepped by the golden azimuth so
            # they spiral around the parent axis (C4). Fixed index order 0..n-1.
            for c in range(children_per_node):
                azimuth = rotate_angle * c
                caxis = _child_axis(axis, down_angle, azimuth)
                add_branch(tip, caxis, child_len, child_rad, level + 1)
        return idx

    # The TRUNK is branch 0: a vertical branch from z=0 to z=height. Its radius
    # tapers like a branch; primary branches sprout from its TOP. We model the
    # trunk's skinned geometry separately (the flared, capped _trunk_mesh) so the
    # skeleton trunk is used only as the SPAWN POINT for primaries — its tube is
    # the dedicated trunk mesh, not a generic branch tube.
    trunk_axis = (0.0, 0.0, 1.0)
    trunk_top = (0.0, 0.0, height)
    # The primary branches grow from the trunk top. (firstBranchHeight reserves a
    # clear bole; at L1 all primaries spring from the single trunk-top node — the
    # bole is the bare trunk below it. Distributing primaries ALONG the trunk
    # above firstBranchHeight is an L2+ refinement once curve/taper land.)
    if branch_levels >= 1:
        primary_len = height * length_ratio
        primary_rad = trunk_radius * TRUNK_TIP_RADIUS_FRAC
        for c in range(children_per_node):
            azimuth = rotate_angle * c
            caxis = _child_axis(trunk_axis, down_angle, azimuth)
            add_branch(trunk_top, caxis, primary_len, primary_rad, 1)

    return branches


def _branch_tube(branch, sides):
    """Skin one skeleton branch (a polyline of nodes + per-node radii) as a
    closed, capped tapered tube: a ring of `sides` verts at each node, side quads
    bridging consecutive rings, and a triangle fan cap at BOTH ends.

    R1 (no weld): each branch is its own closed tube; where a child meets its
    parent the tubes simply OVERLAP/interpenetrate (the overlap is hidden and
    welding is an L4+ nicety). So every tube being individually closed keeps the
    mesh 'manifold-ish' (F1) without the hard join topology.

    Each ring is built in the plane perpendicular to the LOCAL branch direction
    (the segment leaving that node) so the tube follows the branch axis even when
    it leans far off vertical."""
    sides = max(3, sides)
    nodes = branch["nodes"]
    radii = branch["radii"]
    verts = []
    for i, (cx, cy, cz) in enumerate(nodes):
        # Local direction at this node: toward the next node (or from the prev at
        # the tip). The ring lies in the plane perpendicular to it.
        if i < len(nodes) - 1:
            d = (nodes[i + 1][0] - cx, nodes[i + 1][1] - cy, nodes[i + 1][2] - cz)
        else:
            d = (cx - nodes[i - 1][0], cy - nodes[i - 1][1], cz - nodes[i - 1][2])
        dlen = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) or 1.0
        axis = (d[0] / dlen, d[1] / dlen, d[2] / dlen)
        u, v = _axis_frame(axis)
        r = radii[i]
        for k in range(sides):
            ang = math.tau * k / sides
            ex = u[0] * math.cos(ang) + v[0] * math.sin(ang)
            ey = u[1] * math.cos(ang) + v[1] * math.sin(ang)
            ez = u[2] * math.cos(ang) + v[2] * math.sin(ang)
            verts.append((cx + ex * r, cy + ey * r, cz + ez * r))

    faces = []
    # Base cap (reversed so its normal points back down the branch).
    faces.append(list(range(sides - 1, -1, -1)))
    rings = len(nodes)
    for seg in range(rings - 1):
        base = seg * sides
        nxt = (seg + 1) * sides
        for k in range(sides):
            nk = (k + 1) % sides
            faces.append([base + k, base + nk, nxt + nk, nxt + k])
    # Tip cap.
    top = (rings - 1) * sides
    faces.append(list(range(top, top + sides)))
    return verts, faces


def _local_mesh(params: dict):
    """The L1 tree as local (verts, faces) with the trunk base at z=0.

    L1 = tapered trunk + the recursive branching ARMATURE skinned as bare tubes.
    The L0 crown blob is REMOVED — a bare winter-tree silhouette is the correct
    armature read; canopy returns at L3. seed is read for schema/determinism
    plumbing but UNUSED until L5 (the golden angle is exact, no jitter)."""
    height = max(1e-3, float(params.get("height", 6.0)))
    trunk_radius = max(1e-4, float(params.get("trunkRadius", 0.18)))
    taper = float(params.get("taper", 0.85))
    base_flare = float(params.get("baseFlare", 1.6))
    tube_sides = max(3, int(float(params.get("tubeSides", 6))))
    segments = max(1, int(float(params.get("segmentsPerBranch", 4))))

    # The trunk uses segmentsPerBranch rings now (L0 used a fixed 4) so the
    # trunk and the branches share one segment knob.
    trunk_verts, trunk_faces, _top_radius = _trunk_mesh(
        height, trunk_radius, taper, base_flare, segments, tube_sides)

    verts = list(trunk_verts)
    faces = list(trunk_faces)

    # Skin every skeleton branch (level >= 1; the trunk's own tube is the
    # dedicated flared/capped _trunk_mesh above). Each tube's indices shift by
    # the running vertex count; tubes OVERLAP at joins (R1, no weld).
    for branch in _skeleton(params):
        if branch["level"] == 0:
            continue  # the trunk is skinned by _trunk_mesh, not as a generic tube
        b_verts, b_faces = _branch_tube(branch, tube_sides)
        offset = len(verts)
        verts.extend(b_verts)
        faces.extend([[i + offset for i in f] for f in b_faces])

    return verts, faces


def skeleton_levels(params: dict):
    """A small PURE helper the smoke test uses to assert the armature structure
    offline (C1: branch-level count) WITHOUT parsing the skinned mesh. Returns
    the sorted list of distinct branch levels present (e.g. [0, 1, 2, 3, 4]).

    Level 0 is the trunk spawn-axis (skinned as the dedicated trunk mesh); levels
    1.. are the recursive branches. So len(...) - 1 >= 3 means >=3 branch levels
    beyond the trunk (trunk -> primary -> secondary -> tertiary), the C1 gate."""
    levels = {0}  # the trunk spawn axis always exists
    for branch in _skeleton(params):
        levels.add(branch["level"])
    return sorted(levels)


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
