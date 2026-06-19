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

THIS FILE IS L2 (STRUCTURE) of the L0->L5 detail ladder. L0 built ONLY the
scaffold the silhouette test needed (a tapered trunk + one placeholder crown
blob). L1 GREW the recursive self-similar SKELETON off the trunk and skinned each
branch as a bare tapered tube. L2 makes that branching STRUCTURALLY BELIEVABLE —
the three defects the L1 critique owned:

  1. HEIGHT BUDGET. L1's children grew up-and-out off the parent tip, so the
     vertical extents STACKED additively and the crown top reached z ~= 15.4 for
     height=6.0 (2.57x). The MANIFEST advertises `height` = "trunk base -> crown
     top", so that is a lie. L2 BOUNDS the recursion: the trunk carries the full
     vertical budget (trunk top = `height`), the children grow ever more OUTWARD
     than upward (down-angle widens per level + per-segment `curve` leans them
     out), and a final SAFETY SCALE uniformly squashes the whole local mesh so
     the measured crown top lands at `height` exactly. Uniform scale preserves
     every ratio (radius/radius, taper, angle), so the pipe-model bands below are
     unaffected by the fit.

  2. DISTRIBUTED PRIMARIES (kill the wishbone). L1 sprang every primary from the
     SINGLE trunk-top node -> a candelabra/bilateral symmetry that does not read
     as a trunk. L2 spaces the primaries ALONG the trunk, from
     `firstBranchHeight*height` up to the trunk top, the golden azimuth advancing
     between successive emergence points. `firstBranchHeight` (parsed-but-inert
     at L1) is now LIVE: it reserves a clear bole below the first branch.

  3. PIPE-MODEL RADII + TAPER + CURVE. The placeholder fixed-fraction decay is
     replaced by the da Vinci / pipe-model formula r_child = r_p*(1/n)^(1/e)
     (pipeExponent, doc 3.1); the trunk taper is tuned into the B3 band
     (top/base in 0.3-0.6); and each branch segment bends `curve/segments` deg in
     a consistent per-branch lean (deterministic, NO RNG — jitter is L5).

Params switched ON this level (added to L1's set): pipeExponent, curve, and a
refined taper. DEFERRED to later levels (parsed + IGNORED, so the schema is
stable): clump* CANOPY = L3; baseFlare/crown-ratio tuning = L4; the *Jitter knobs
+ seeded RNG = L5. seed is read but INERT until L5 — every angle is EXACT, there
is NO jitter, so the armature is deterministic without touching random at all.

The FULL 24-param MANIFEST is declared NOW (some params still ignored) so the
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
    # stability across L0->L5. At L2 height/trunkRadius/taper/tubeSides/baseFlare
    # feed the trunk; branchLevels/childrenPerNode/downAngle/rotateAngle/
    # lengthRatio/segmentsPerBranch/firstBranchHeight feed the armature; and
    # pipeExponent/curve feed the L2 structure refinement. The rest are parsed and
    # IGNORED until their ladder level switches them on. Types are limited to
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
        # lengthRatio scales how far each child extends vs its parent. It is the
        # crown-WIDTH lever: C3 (primary emergence angle) is geometrically
        # orthogonal to it, so shortening the children pulls the crown in without
        # touching the angle/taper/budget bands. The L2 critique found 0.72 drove
        # the crown to ~1.31× height (over the A2 0.5-1.0× band — a sprawl that
        # would amplify under the L3 canopy). 0.56 lands crown width ~1.02× the
        # mesh max-z, inside the A2 guard band, with C2/C3/B2/B3/budget unchanged.
        {"key": "lengthRatio", "label": "Length Ratio", "type": "number", "default": 0.56},
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

# --- NAMED L2 constants (no bare magic literals in the recursion logic) -------
# The DEFAULT taper from L1 (0.85 retained per segment) left the trunk too flat:
# top/base ~= 0.61, above the B3 band (0.3-0.6). L2 overrides it with a steeper
# trunk taper so the trunk visibly thins before the first split. This is the
# per-SEGMENT retention; over `segmentsPerBranch` segments it compounds to
# (TRUNK_TAPER_PER_SEGMENT ** segments) ~= top/base. 0.78^4 ~= 0.37, squarely in
# B3. We taper the TRUNK with this dedicated value rather than the param `taper`
# so the trunk taper is a deliberate structural choice; the per-branch tubes use
# the gentler BRANCH_TIP_TAPER below.
TRUNK_TAPER_PER_SEGMENT = 0.78
# A branch tip tapers to this fraction of its start radius along its own length
# (a single gentle taper per branch). Keeps tubes from ending in a blunt
# cylinder. (Per-segment `curve` bend is added on top; the radius profile is this
# linear taper.)
BRANCH_TIP_TAPER = 0.55
# The primary branches start at this fraction of the trunk-BASE radius. The pipe
# model then thins each deeper level; the primaries are the pipe model's level-1
# seed. 0.7 reads as "the limbs are clearly thinner than the trunk" at their root.
PRIMARY_RADIUS_FRAC = 0.7
# Per-level the child down-angle is WIDENED by this many degrees, so deeper
# branches lean ever more OUTWARD (less upward). This is the height-budget lever
# (b): the more outward the upper branches grow, the less they STACK +z above the
# trunk top, and the more the crown spreads horizontally (a believable broadleaf
# crown). Deterministic — purely a function of level. L2 critique: at +12°/level
# this stacked on the curve droop drove L3/L4 tips to ~-90° elevation (a weeping
# dead-tree read) and a crown 1.25× height (over the A2 0.5-1.0× band). Tempered
# to +5°/level so deep branches still spread outward without plunging vertical;
# the explicit droop CLAMP below is the hard guarantee.
DOWN_ANGLE_LEVEL_WIDEN_DEG = 5.0
# DROOP CLAMP. The accumulated down-angle (per-level widen) + the per-segment
# `curve` can drive a branch tip's heading below horizontal. A near-vertical
# plunge reads as a weeping/dead tree. We clamp the running axis in
# _curved_branch_nodes so its elevation never drops below this floor — no branch
# tip ever heads more steeply DOWN than 20° below horizontal. (Positive = above
# horizontal; the floor is negative.)
MIN_BRANCH_ELEVATION_RAD = math.radians(-20.0)
# HEIGHT BUDGET FIT — POSITION-ONLY. The additive +z of the recursion overshoots
# `height` (natural crown top ~= 2.25× height at defaults). We fit by scaling the
# skeleton's NODE POSITIONS (x,y,z) so the natural z-extent lands at `height`.
# CRITICAL (L2 critique fix): the fit scales POSITIONS ONLY, never RADII. A naive
# uniform scale of the FINAL mesh (positions AND ring radii together) silently
# thinned the trunk — base/height fell to 0.013, BELOW the B2 band (0.02-0.05) —
# so `height` was honest but the trunk proportion was a lie. By scaling only the
# skeleton positions and then assigning each tube's radii from the params/pipe-
# model AFTER, the tree fits `height` AND the trunk base radius stays exactly
# `trunkRadius` (B2 honest: 0.18/6 = 0.03, in band). Base stays at z=0 (the
# skeleton starts at the origin, so scaling about the origin keeps F3). This
# constant is the target the fit drives toward; it is `height` itself.
HEIGHT_BUDGET_TARGET_FRAC = 1.0


def _ring(cx, cy, cz, radius, sides):
    """A horizontal ring of `sides` verts centred at (cx,cy,cz). Returned as a
    flat list of (x,y,z); the loft below joins consecutive rings into quads."""
    out = []
    for i in range(sides):
        angle = math.tau * i / sides
        out.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius, cz))
    return out


def _trunk_mesh(height, trunk_radius, base_flare, segments, sides):
    """A tapered vertical trunk: `segments` stacked ring loops from z=0 to
    z=height, each ring's radius scaled by TRUNK_TAPER_PER_SEGMENT per segment so
    the trunk narrows with height, and the very bottom ring widened by
    `base_flare` (root buttress). Closed at the bottom AND the top (a cap ring) so
    the trunk reads as a solid tube; branches overlap it (no weld, per R1).
    Returns (verts, faces, top_radius).

    L2 note: the trunk uses the dedicated TRUNK_TAPER_PER_SEGMENT (steeper than
    the param `taper`) so top/base lands in the B3 band (0.3-0.6) regardless of
    how the author dials `taper` for the leaves' look — the trunk's structural
    taper is a deliberate constant, not a side effect of a foliage knob."""
    sides = max(3, sides)
    segments = max(1, segments)

    radii = []
    radius = trunk_radius
    for seg in range(segments + 1):
        # The base ring gets the flare multiplier; every higher ring tapers
        # geometrically by the trunk per-segment retention.
        flare = base_flare if seg == 0 else 1.0
        radii.append(max(1e-4, radius * flare))
        radius *= TRUNK_TAPER_PER_SEGMENT

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
# such nodes (segmentsPerBranch+1 of them) plus its level. We build the whole
# skeleton FIRST, in a FIXED depth-first child-index order, then skin it.
#
# WHY build the skeleton first (and in fixed order) rather than emit geometry
# inline during recursion: at L5 a seeded RNG will perturb each node's angle and
# length. Determinism (rubric E2 + R3) demands the RNG be drawn in the SAME order
# in both the proof tier and the bpy build. A skeleton built by a deterministic
# depth-first walk (children visited in index 0..n-1 order) gives that fixed
# traversal a stable spine to hang the (future) RNG draws on, and keeps the skin
# pass a pure function of the skeleton. At L2 there is no RNG yet, but the ORDER
# is locked in now so L5 is a drop-in.


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


def _curved_branch_nodes(start, axis, length, segments, curve_rad):
    """A branch as `segments+1` nodes, bending `curve_rad` total along its length
    (split evenly per segment). The bend turns the axis steadily AWAY from
    vertical (toward the horizontal plane), i.e. a gravity/light droop — a
    consistent per-branch lean, deterministic (no RNG; jitter is L5).

    HOW the bend direction is chosen: we rotate the running axis within the plane
    spanned by the axis and the WORLD-DOWN-projected perpendicular, so the branch
    arcs outward+down. Concretely each step rotates the axis toward the
    horizontal component of the current axis (its lean direction), accumulating a
    gentle arc rather than a straight ray. WHY this helps the height budget: an
    arcing branch covers more HORIZONTAL than VERTICAL distance than a straight
    one of the same length, so the crown spreads sideways instead of stacking +z.
    """
    nodes = [start]
    cur = axis
    step_len = length / segments
    per_seg = curve_rad / max(1, segments)
    pos = start
    for _s in range(segments):
        # Bend `cur` by per_seg toward the horizontal: the bend axis is the
        # horizontal component of `cur` rotated 90 deg in-plane. Simplest stable
        # form: rotate `cur` in the vertical plane that contains it, tilting it
        # DOWN (reducing its +z component) by per_seg.
        # Horizontal direction of travel (unit), or +x if the branch is vertical.
        hx, hy = cur[0], cur[1]
        hlen = math.hypot(hx, hy)
        if hlen < 1e-9:
            hdir = (1.0, 0.0)
        else:
            hdir = (hx / hlen, hy / hlen)
        # Current elevation angle above horizontal, then tilt it DOWN by per_seg.
        elev = math.atan2(cur[2], hlen)
        elev -= per_seg
        # DROOP CLAMP: never let the heading plunge past the floor. Without this
        # the per-level down-angle widen stacked on the curve drove deep tips to
        # ~-90° (a weeping/dead read); clamping to ~-20° keeps the arc gentle.
        if elev < MIN_BRANCH_ELEVATION_RAD:
            elev = MIN_BRANCH_ELEVATION_RAD
        ce = math.cos(elev)
        cur = (hdir[0] * ce, hdir[1] * ce, math.sin(elev))
        pos = (pos[0] + cur[0] * step_len,
               pos[1] + cur[1] * step_len,
               pos[2] + cur[2] * step_len)
        nodes.append(pos)
    return nodes


def _skeleton(params: dict):
    """Build the recursive branching skeleton as a list of BRANCHES. Each branch
    is a dict with:
        nodes : [(x, y, z), ...]   (segments_per_branch + 1 points, start->tip)
        radii : [r0, ..., r_tip]   (radius at each node, gently tapering)
        level : int                (0 = trunk, 1 = primary, ...)
    The trunk is branch 0 (level 0). PRIMARY branches are DISTRIBUTED along the
    trunk from firstBranchHeight*height up to the trunk top; each branch tip then
    spawns `childrenPerNode` children to depth branchLevels.

    Pure + deterministic: the only angle source is the EXACT golden azimuth
    (rotateAngle advancing per primary + per child) — no RNG at L2. Depth-first,
    children in index order, so L5's seeded jitter has a fixed traversal."""
    height = max(1e-3, float(params.get("height", 6.0)))
    trunk_radius = max(1e-4, float(params.get("trunkRadius", 0.18)))
    branch_levels = max(0, int(float(params.get("branchLevels", 4))))
    children_per_node = max(1, int(float(params.get("childrenPerNode", 3))))
    first_branch = min(0.95, max(0.0, float(params.get("firstBranchHeight", 0.35))))
    down_angle = math.radians(float(params.get("downAngle", 45.0)))
    rotate_angle = math.radians(float(params.get("rotateAngle", 137.5)))
    # Fallback mirrors the MANIFEST default (0.56): the sample recipe doesn't pin
    # lengthRatio, so this .get() fallback IS the rendered value — it must track
    # the manifest default or the proof mesh would render a different crown width
    # than the inspector advertises. 0.56 keeps the crown in the A2 0.5-1.0× band.
    length_ratio = float(params.get("lengthRatio", 0.56))
    segments = max(1, int(float(params.get("segmentsPerBranch", 4))))
    pipe_exponent = max(1e-3, float(params.get("pipeExponent", 2.2)))
    curve = math.radians(float(params.get("curve", 15.0)))

    # PIPE MODEL (da Vinci's rule): a parent of radius r_p splitting into n
    # children gives each child r_p * (1/n)^(1/e). With n=3, e=2.2 -> ~0.62.
    # Computed ONCE here; applied at every split. This is the L2 substance: child
    # radius is DERIVED from the parent, not a fixed magic fraction.
    pipe_factor = (1.0 / children_per_node) ** (1.0 / pipe_exponent)

    branches = []

    def add_branch(start, axis, length, radius, level):
        """Append one CURVED branch of `segments` steps from `start` along `axis`
        (bending `curve` total toward horizontal), tapering radius from `radius`
        to radius*BRANCH_TIP_TAPER, then — if we are not yet at branchLevels —
        recurse children off its TIP. Returns the branch's index."""
        nodes = _curved_branch_nodes(start, axis, length, segments, curve)
        radii = []
        for s in range(segments + 1):
            t = s / segments
            # Linear taper along the branch from `radius` down to the tip frac.
            radii.append(radius * (1.0 - (1.0 - BRANCH_TIP_TAPER) * t))
        idx = len(branches)
        branches.append({"nodes": nodes, "radii": radii, "level": level})

        if level < branch_levels:
            tip = nodes[-1]
            tip_axis = _node_direction(nodes, len(nodes) - 1)
            child_len = length * length_ratio
            # Pipe model: each child keeps pipe_factor of THIS branch's start
            # radius (the splitting cross-section is conserved per da Vinci).
            child_rad = radius * pipe_factor
            # Deeper levels lean ever more OUTWARD (height-budget lever (b)).
            child_down = down_angle + math.radians(DOWN_ANGLE_LEVEL_WIDEN_DEG) * level
            for c in range(children_per_node):
                azimuth = rotate_angle * c
                caxis = _child_axis(tip_axis, child_down, azimuth)
                add_branch(tip, caxis, child_len, child_rad, level + 1)
        return idx

    # The TRUNK is branch 0: a vertical branch from z=0 to z=height. Its skinned
    # geometry is the dedicated flared/capped _trunk_mesh; the skeleton trunk is
    # used only as the spawn AXIS + emergence heights for primaries.
    trunk_axis = (0.0, 0.0, 1.0)
    branches.append({
        "nodes": [(0.0, 0.0, 0.0), (0.0, 0.0, height)],
        "radii": [trunk_radius, trunk_radius * (TRUNK_TAPER_PER_SEGMENT ** segments)],
        "level": 0,
    })

    # DISTRIBUTED PRIMARIES (kill the wishbone). Instead of all primaries
    # springing from the single trunk-top node, they emerge at SUCCESSIVE heights
    # from first_branch*height up to the trunk top, the golden azimuth advancing
    # between emergence points so consecutive primaries spiral around the trunk
    # (C4). firstBranchHeight reserves a clear bole below the first primary.
    if branch_levels >= 1 and children_per_node >= 1:
        primary_len = height * length_ratio
        primary_rad = trunk_radius * PRIMARY_RADIUS_FRAC
        n_prim = children_per_node
        z_lo = first_branch * height
        z_hi = height
        for c in range(n_prim):
            # Spread emergence heights evenly in (z_lo, z_hi]; with n primaries
            # the c-th sits at z_lo + (c+1)/n * (z_hi - z_lo) so the topmost is at
            # the trunk top and none sits exactly at the bole cut.
            frac = (c + 1) / n_prim
            zc = z_lo + frac * (z_hi - z_lo)
            start = (0.0, 0.0, zc)
            azimuth = rotate_angle * c
            caxis = _child_axis(trunk_axis, down_angle, azimuth)
            add_branch(start, caxis, primary_len, primary_rad, 1)

    return branches


def _node_direction(nodes, i):
    """Unit direction of the branch AT node i: toward the next node (or from the
    previous at the tip). Used to spawn children off a CURVED branch's tip along
    the tip's true heading, and to orient each ring in the skin pass."""
    if i < len(nodes) - 1:
        d = (nodes[i + 1][0] - nodes[i][0],
             nodes[i + 1][1] - nodes[i][1],
             nodes[i + 1][2] - nodes[i][2])
    else:
        d = (nodes[i][0] - nodes[i - 1][0],
             nodes[i][1] - nodes[i - 1][1],
             nodes[i][2] - nodes[i - 1][2])
    dlen = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) or 1.0
    return (d[0] / dlen, d[1] / dlen, d[2] / dlen)


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
    it leans far off vertical or arcs through the per-segment curve."""
    sides = max(3, sides)
    nodes = branch["nodes"]
    radii = branch["radii"]
    verts = []
    for i, (cx, cy, cz) in enumerate(nodes):
        axis = _node_direction(nodes, i)
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
    """The L2 tree as local (verts, faces) with the trunk base at z=0.

    L2 = tapered trunk + the recursive branching ARMATURE skinned as bare tubes,
    now with pipe-model radii, per-segment curve, and DISTRIBUTED primaries — and
    fitted to the declared `height` budget. A bare winter-tree silhouette is the
    correct L2 read; canopy returns at L3. seed is read for schema/determinism
    plumbing but UNUSED until L5 (every angle is exact, no jitter)."""
    height = max(1e-3, float(params.get("height", 6.0)))
    trunk_radius = max(1e-4, float(params.get("trunkRadius", 0.18)))
    base_flare = float(params.get("baseFlare", 1.6))
    tube_sides = max(3, int(float(params.get("tubeSides", 6))))
    segments = max(1, int(float(params.get("segmentsPerBranch", 4))))

    # HEIGHT BUDGET FIT — POSITION-ONLY, computed BEFORE any skinning.
    #
    # The recursion's natural z-extent overshoots `height` (~2.25× at defaults
    # because the children stack +z off each parent tip). We must land the crown
    # top at `height`. The KEY DESIGN DECISION (the L2 critique fix): scale the
    # skeleton NODE POSITIONS, never the RADII.
    #
    # WHY positions-only and not a uniform whole-mesh scale: a uniform scale of
    # the finished mesh shrinks the ring RADII along with the positions, so the
    # trunk silently thins — base/height dropped to 0.013, BELOW the B2 band
    # (0.02-0.05). `height` was honest but the trunk PROPORTION was a lie, and
    # the smoke test masked it by reading the pre-fit param ratio. By scaling
    # only the spine here and assigning every tube's radii from the params/pipe-
    # model afterwards, BOTH invariants hold: the tree fits `height` AND the
    # trunk base radius is exactly `trunkRadius` (0.18/6 = 0.03, in B2 band).
    skel = _skeleton(params)
    natural_zmax = max((n[2] for b in skel for n in b["nodes"]), default=height)
    target = HEIGHT_BUDGET_TARGET_FRAC * height
    pos_scale = target / natural_zmax if natural_zmax > 1e-9 else 1.0
    # Scale POSITIONS only (about the origin → base stays at z=0, F3). Radii are
    # left exactly as the pipe model / taper produced them.
    for b in skel:
        b["nodes"] = [(nx * pos_scale, ny * pos_scale, nz * pos_scale)
                      for (nx, ny, nz) in b["nodes"]]

    # The visible trunk is the dedicated flared/capped _trunk_mesh, built to the
    # SCALED trunk-top height (so it shares the spine's vertical compression) but
    # with the PARAM radii (so its base radius is untouched by the fit). The
    # trunk skeleton node (level 0) carries the scaled top; read it back so the
    # trunk mesh and the branch emergence heights agree.
    trunk_branch = next(b for b in skel if b["level"] == 0)
    scaled_trunk_height = trunk_branch["nodes"][-1][2]
    trunk_verts, trunk_faces, _top_radius = _trunk_mesh(
        scaled_trunk_height, trunk_radius, base_flare, segments, tube_sides)

    verts = list(trunk_verts)
    faces = list(trunk_faces)

    # Skin every skeleton branch (level >= 1; the trunk's own tube is the
    # dedicated flared/capped _trunk_mesh above). Each tube's indices shift by
    # the running vertex count; tubes OVERLAP at joins (R1, no weld). Positions
    # are already height-fitted; radii are honest (never scaled).
    for branch in skel:
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
