#!/usr/bin/env python3
"""ctest smoke for the craftsmen library's Blender-free half.

Asserts the dry-run plan for the compiled doric column matches the
committed golden byte for byte, that the strict reader refuses the
classic offenders, that every plan/default path the doric never walks
is pinned by an inline fixture, and that the python material table
mirrors the C++ validator's. The bpy half is exercised in Blender (and
reviewed against bpy semantics); everything testable without Blender is
tested here so a refactor cannot silently bend the plan.
"""

import math
import os
import re
import sys
import tempfile

# Keep ctest from littering tools/blender/__pycache__ into the source tree.
sys.dont_write_bytecode = True

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))

import edi_craft  # noqa: E402

SAMPLES = os.path.join(ROOT, "samples", "doric_column")

# A complete, valid cylinder to graft one bad field onto per refusal case.
MINIMAL_CYLINDER = (
    'op.0.type = "AddCylinder"\nop.0.name = "c"\nop.0.radius = "1"\n'
    'op.0.height = "2"\nop.0.z = "0"\n'
)

# Exercises every plan/default path the doric sample never reaches:
# sphere/ring/label plan lines, omitted-key defaults (ring overhang 0,
# material stone, axis z, z_mode center; sphere/ring vertices asserted on
# the parsed dicts — no plan line prints them), the x-axis cylinder at
# x=2 (bounds_of's x branch moves the rig's camera x to 1.5 — at x=0 the
# branch's bounds are symmetric and the rig can't see it), z_mode="base"
# (_center_z's base branch), entasis="false" (_flag's false branch), and
# CutFlutes without width_ratio/z-range (the 0.28 default, no z suffix),
# and a CutFlutes WITH explicit cutter geometry (the cutter_r/at_r plan
# path, R1-B04b).
PLAN_FIXTURE = """\
recipe.id = "smoke.plan"
recipe.name = "Plan Fixture"

op.0.type = "AddSphere"
op.0.name = "finial"
op.0.radius = "1.5"
op.0.z = "10"

op.1.type = "AddRing"
op.1.name = "collar"
op.1.radius = "2"
op.1.tube_height = "0.5"
op.1.z = "8"

op.2.type = "AddLabel"
op.2.name = "tag"
op.2.text = "north face"
op.2.x = "1"
op.2.y = "2"
op.2.z = "3"

op.3.type = "AddCylinder"
op.3.name = "beam"
op.3.radius = "0.5"
op.3.height = "6"
op.3.z = "4"
op.3.x = "2"
op.3.axis = "x"

op.4.type = "AddBox"
op.4.name = "pedestal"
op.4.width = "4"
op.4.depth = "4"
op.4.height = "2"
op.4.z = "0"
op.4.z_mode = "base"

op.5.type = "AddCylinder"
op.5.name = "drum"
op.5.radius = "1"
op.5.height = "4"
op.5.z = "2"
op.5.entasis = "false"

op.6.type = "CutFlutes"
op.6.target = "drum"
op.6.count = "6"
op.6.depth = "0.1"

op.7.type = "CutFlutes"
op.7.target = "drum"
op.7.count = "20"
op.7.depth = "0.12"
op.7.cutter_radius = "0.16"
op.7.at_radius = "1.056"
"""

# Derived by hand from the module's own arithmetic: bounds x [-2, 5]
# (beam at x=2 lying along x: [2-3, 2+3]; pedestal/ring give the -2),
# y [-2, 2], z [0, 11.5] (finial top) -> span 11.5, ortho 11.5 * 1.2 =
# 13.8, camera x = (-2+5)/2 = 1.5, y = -2 - 11.5 * 1.6 = -20.4,
# center 5.75. Camera x is the killable trace of bounds_of's x branch.
PLAN_EXPECTED = [
    "# edi_craft dry run — 8 ops",
    "AddSphere finial r=1.5 z=10 material=stone",
    "AddRing collar r=2 tube_h=0.5 z=8 material=stone",
    'AddLabel tag "north face" at (1, 2, 3)',
    "AddCylinder beam r=0.5 h=6 center_z=4 axis=x vertices=96 material=stone",
    "AddBox pedestal 4x4x2 center_z=1 material=stone",
    "AddCylinder drum r=1 h=4 center_z=2 axis=z vertices=96 material=stone",
    "CutFlutes drum count=6 depth=0.1 width_ratio=0.28",
    "CutFlutes drum count=20 depth=0.12 cutter_r=0.16 at_r=1.056",
    "preview rig: ortho_scale=13.8 camera=(1.5, -20.4, 5.75) target_z=5.75",
]


def write_temp(toml_text: str) -> str:
    with tempfile.NamedTemporaryFile(
        "w", suffix=".toml", encoding="utf-8", delete=False
    ) as f:
        f.write(toml_text)
        return f.name


def assert_manifold(verts, faces, label):
    """A closed orientable manifold: every UNDIRECTED edge bounds exactly 2
    faces, and no face is degenerate (< 3 distinct verts). Codifies the BL-06
    reviewer's hand-check so a future regression fails loudly. Apply only to
    meshes that SHOULD be watertight — NOT the self-intersecting swept solid
    (BL-08, valid faces but not 2-manifold by design). The helix (BL-07) IS
    now closed (P3 added axis spine + inner/outer quads + end caps) so it IS
    covered here."""
    edge_faces = {}
    for face in faces:
        assert len(set(face)) >= 3, f"{label}: degenerate face {face}"
        k = len(face)
        for i in range(k):
            a, b = face[i], face[(i + 1) % k]
            edge = (min(a, b), max(a, b))
            edge_faces[edge] = edge_faces.get(edge, 0) + 1
    bad = {e: c for e, c in edge_faces.items() if c != 2}
    assert not bad, f"{label}: non-manifold edges (incidence != 2): {list(bad.items())[:5]}"


def assert_oriented(faces, label):
    """A consistently oriented closed surface: every DIRECTED edge (a→b)
    appears in exactly one face, with its reverse (b→a) in exactly one other.

    Why this matters: in a closed 2-manifold every undirected edge has exactly
    2 adjacent faces.  Those two faces can traverse the edge in the SAME
    direction — meaning both 'own' (a→b) and neither owns (b→a) — or in
    OPPOSITE directions, meaning one owns (a→b) and the other owns (b→a).
    Only the latter is a consistently oriented surface (outward normals all
    pointing the same way).  The former is 2-manifold but INVERTED on that
    edge — the standard symptom of a sector fan or bridging quad wound the
    same way as its neighbour instead of opposite.

    Precondition: call assert_manifold first (undirected count=2 is assumed).
    Apply to meshes that are BOTH closed AND should be consistently oriented
    (partial revolve, helix).  Do NOT apply to the self-intersecting BL-08
    swept solid."""
    directed: dict = {}
    for face in faces:
        k = len(face)
        for i in range(k):
            a, b = face[i], face[(i + 1) % k]
            directed[(a, b)] = directed.get((a, b), 0) + 1
    # Every directed edge should appear exactly once (given manifold, if (a→b)
    # count=1 then (b→a) must also be count=1 — there are exactly 2 traversals
    # of the undirected edge and we've used one for each direction).
    bad = {e: c for e, c in directed.items() if c != 1}
    assert not bad, (
        f"{label}: non-unit directed edges (same direction in 2 faces): "
        f"{list(bad.items())[:5]}"
    )


def main() -> int:
    # The two material vocabularies must stay one vocabulary: a key only in
    # C++ validates and previews but hard-errors in the Blender driver; one
    # only here is unreachable. Brittle-to-reformatting on purpose — a
    # failure forces a human to look at both tables.
    with open(os.path.join(ROOT, "src", "recipe", "RecipeOps.cpp"), encoding="utf-8") as f:
        cpp_src = f.read()
    block = re.search(r"static const std::vector<std::string> table = \{(.*?)\};", cpp_src, re.S)
    assert block, "material table block not found in RecipeOps.cpp (reformat? update this regex)"
    cpp_materials = set(re.findall(r'"([^"]+)"', block.group(1)))
    assert cpp_materials == set(edi_craft.MATERIALS), (
        f"material tables drifted: C++ only {sorted(cpp_materials - set(edi_craft.MATERIALS))}, "
        f"python only {sorted(set(edi_craft.MATERIALS) - cpp_materials)}"
    )

    ops = edi_craft.parse_ops(os.path.join(SAMPLES, "doric_column_ops_compiled.toml"))
    plan = "\n".join(edi_craft.plan_lines(ops)) + "\n"
    with open(os.path.join(SAMPLES, "doric_dry_run.txt"), encoding="utf-8") as f:
        golden = f.read()
    assert plan == golden, "dry-run plan drifted from samples/doric_column/doric_dry_run.txt"

    # The mesh proof (R2): the OBJ of the compiled doric is byte-stable and
    # carries every op as a named object — the flute cutters included. The
    # byte-golden pins all 6928 verts / 6192 faces; these extra checks name the
    # semantics so a mutation (a dropped cutter, a renamed object) reads clearly.
    obj = "\n".join(edi_craft.obj_lines(ops)) + "\n"
    with open(os.path.join(SAMPLES, "doric_column.obj"), encoding="utf-8") as f:
        obj_golden = f.read()
    assert obj == obj_golden, "mesh proof OBJ drifted from samples/doric_column/doric_column.obj"
    obj_names = [line[2:] for line in obj.splitlines() if line.startswith("o ")]
    assert len(obj_names) == 28, f"OBJ object count drifted: {len(obj_names)}"
    cutters = [name for name in obj_names if ".flute_cutter_" in name]
    assert len(cutters) == 20, f"flute cutter count drifted: {len(cutters)}"
    assert obj.count("\nv ") + (1 if obj.startswith("v ") else 0) == 6928, "OBJ vertex count drifted"
    assert obj.count("\nf ") + (1 if obj.startswith("f ") else 0) == 6192, "OBJ face count drifted"

    # The extrude spine (BL-04): the lowered AddPrism builds a height-bearing
    # prism PURELY (no bpy), so its OBJ is a byte-golden too. The L-bracket
    # footprint (6 points) lofts to 12 verts (bottom + top cap) and 8 faces
    # (2 caps + 6 side quads); the z-extent equals the op's height — the mesh is
    # honest about dimension, never abs()'d. This is why the OBJ proof needs no
    # Blender: _prism_world is pure, and add_prism (bpy) builds from the SAME
    # mesh, so the headless proof and the Blender build cannot drift.
    prism_dir = os.path.join(ROOT, "samples", "extruded_figure")
    prism_ops = edi_craft.parse_ops(os.path.join(prism_dir, "extruded_figure_ops_compiled.toml"))
    prism_obj = "\n".join(edi_craft.obj_lines(prism_ops)) + "\n"
    with open(os.path.join(prism_dir, "extruded_figure.obj"), encoding="utf-8") as f:
        prism_golden = f.read()
    assert prism_obj == prism_golden, "prism OBJ drifted from samples/extruded_figure/extruded_figure.obj"
    prism_zs = [float(line.split()[3]) for line in prism_obj.splitlines() if line.startswith("v ")]
    assert len(prism_zs) == 12, f"prism vertex count drifted: {len(prism_zs)}"
    assert max(prism_zs) - min(prism_zs) == 3.0, "prism z-extent must equal the op height (3)"
    assert prism_obj.count("\nf ") == 8, "prism face count drifted (2 caps + 6 sides)"
    # The straight prism is a closed solid (caps + side quads) — watertight.
    prism_objs = edi_craft.obj_objects(prism_ops)
    assert_manifold(prism_objs[0][1], prism_objs[0][2], "straight prism")

    # BL-08: a Follow-Me sweep — an AddPrism WITH a path lofts the footprint
    # along the path into a closed swept solid. Footprint n=4, path m=3:
    # verts = n*m = 12, faces = (m-1)*n side quads + 2 end caps = 10. The solid
    # bends out of a flat box — it spans x, y AND z — and every face is well-formed.
    swept_ops = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "swept_profile", "swept_profile_ops_compiled.toml"))
    swept_obj = "\n".join(edi_craft.obj_lines(swept_ops)) + "\n"
    with open(os.path.join(ROOT, "samples", "swept_profile", "swept_profile.obj"),
              encoding="utf-8") as f:
        swept_golden = f.read()
    assert swept_obj == swept_golden, "swept OBJ drifted from samples/swept_profile/swept_profile.obj"
    swept_objs = edi_craft.obj_objects(swept_ops)
    sw_verts, sw_faces = swept_objs[0][1], swept_objs[0][2]
    assert len(sw_verts) == 12, f"swept vert count: {len(sw_verts)}"
    assert len(sw_faces) == 10, f"swept face count: {len(sw_faces)}"
    for face in sw_faces:
        assert len(set(face)) >= 3, "degenerate swept face"
        assert all(0 <= i < len(sw_verts) for i in face), "swept face references a missing vertex"
    assert len({v[0] for v in sw_verts}) > 1 and len({v[1] for v in sw_verts}) > 1 \
        and len({v[2] for v in sw_verts}) > 1, "swept solid must span x, y and z (the path bends it)"

    # BL-09: a taper_end < 1 narrows the swept profile toward the end. The cross-
    # section is the first n verts vs the last n verts; the end loop's extent is
    # ~taper_end of the start. taper_end == 1.0 is byte-identical to BL-08.
    n_fp = 4  # the swept_profile footprint has 4 points
    def extent(loop):
        return (max(p[2] for p in loop) - min(p[2] for p in loop))  # z-span of the cross-section
    no_taper = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "swept_profile", "swept_profile_ops_compiled.toml"))[0]
    base_verts = edi_craft._prism_world(no_taper)[0]
    tapered = dict(no_taper, taper_end=0.5)
    tap_verts = edi_craft._prism_world(tapered)[0]
    assert edi_craft._prism_world(dict(no_taper, taper_end=1.0))[0] == base_verts, \
        "taper_end=1.0 must be byte-identical to the no-taper sweep"
    start_extent = extent(tap_verts[:n_fp])
    end_extent = extent(tap_verts[-n_fp:])
    assert abs(end_extent - 0.5 * start_extent) < 1e-9, \
        f"taper_end=0.5 should halve the end cross-section: start {start_extent}, end {end_extent}"

    # P4: taper_curve remaps the linear path-fraction t → t^taper_curve before
    # the lerp. taper_curve=1.0 is t^1=t = LINEAR = byte-identical to BL-09.
    # taper_curve=2.0 BACK-LOADS the narrowing: at the mid-path point (t=0.5)
    # the curved fraction is 0.5^2=0.25, so scale there is lerp(1,0.5,0.25)=0.875
    # vs the linear's lerp(1,0.5,0.5)=0.75 — the mid-loop is WIDER for curve=2.
    linear_at_mid = 1.0 + (0.5 - 1.0) * 0.5    # lerp(1,0.5,0.5) = 0.75
    curved_at_mid = 1.0 + (0.5 - 1.0) * (0.5 ** 2.0)  # lerp(1,0.5,0.25) = 0.875
    assert curved_at_mid > linear_at_mid, "curve=2 should give a wider mid-section than linear"
    # Confirm via actual vertex spans: path has 2 points (index 0 and 1); the
    # sweep interpolates at k=0 (t=0) and k=1 (t=1). With a single-segment
    # path the ONLY interior point is k=0 (scale=1, identical for both), so use
    # a 3-point path to expose the mid-segment. Build a synthetic op directly.
    import math as _math
    fp3 = [{"x": 0.0, "y": 0.0}, {"x": 0.0, "y": 1.0},
           {"x": 1.0, "y": 1.0}, {"x": 1.0, "y": 0.0}]
    path3 = [{"x": 0.0, "y": 0.0}, {"x": 1.0, "y": 0.0}, {"x": 2.0, "y": 0.0}]
    base_op = {"footprint": fp3, "path": path3, "x": 0.0, "y": 0.0,
               "base_z": 0.0, "taper_end": 0.5, "inset": 0.0, "normal_offset": 0.0}
    n_fp3 = 4  # 4 footprint points → 4 verts per loop
    lin_verts = edi_craft._prism_world(dict(base_op, taper_curve=1.0))[0]
    cur_verts = edi_craft._prism_world(dict(base_op, taper_curve=2.0))[0]
    # Loop at k=1 (mid-path) is verts [n_fp3 .. 2*n_fp3-1]; extent by z-span.
    lin_mid = extent(lin_verts[n_fp3: 2 * n_fp3])
    cur_mid = extent(cur_verts[n_fp3: 2 * n_fp3])
    assert cur_mid > lin_mid + 1e-9, \
        f"curve=2.0 mid-loop must be wider than linear: linear={lin_mid}, curved={cur_mid}"
    # taper_curve=1.0 exactly matches linear (byte-identical to BL-09 linear).
    lin_verts2 = edi_craft._prism_world(dict(base_op, taper_curve=1.0))[0]
    lin_ref   = edi_craft._prism_world(dict(base_op))[0]  # no taper_curve key → default 1.0
    assert lin_verts2 == lin_ref, "taper_curve=1.0 must be byte-identical to omitting the key"

    # P4b: per-axis (asymmetric) taper. taper_end=1.0 (no local-X taper) +
    # taper_end_y=0.5 (local-Y narrows to half) → the cross-section is anisotropic.
    # In _swept_prism_world the footprint's local-X (fx) maps to the in-plane
    # normal direction and local-Y (fy) maps to WORLD Z (the height dimension).
    # So taper_end_y=0.5 halves the world-Z span of the end loop while
    # taper_end=1.0 leaves the normal-direction span unchanged.
    asym_op = {"footprint": fp3, "path": path3, "x": 0.0, "y": 0.0,
               "base_z": 0.0, "taper_end": 1.0, "taper_end_y": 0.5,
               "inset": 0.0, "normal_offset": 0.0}
    asym_verts = edi_craft._prism_world(asym_op)[0]
    end_loop = asym_verts[-n_fp3:]
    start_loop = asym_verts[:n_fp3]
    # extent() measures world-Z span (= local-Y of the footprint, controlled by sy).
    # taper_end_y=0.5 → sy=0.5 at t=1 → end Z span ≈ half of start Z span.
    assert abs(extent(end_loop) - 0.5 * extent(start_loop)) < 1e-9, \
        f"taper_end_y=0.5 should halve Z-extent at end: start={extent(start_loop)}, end={extent(end_loop)}"
    # World-Y span (= footprint local-X, controlled by sx).
    # taper_end=1.0 → sx=1.0 → end world-Y span == start world-Y span.
    def wy_extent(loop):
        return max(v[1] for v in loop) - min(v[1] for v in loop)
    assert abs(wy_extent(end_loop) - wy_extent(start_loop)) < 1e-9, \
        f"taper_end=1.0 should leave normal-dir span unchanged: start={wy_extent(start_loop)}, end={wy_extent(end_loop)}"
    # Sanity: taper_end_y does change the Z span (asymmetry is real).
    uniform_taper = {"footprint": fp3, "path": path3, "x": 0.0, "y": 0.0,
                     "base_z": 0.0, "taper_end": 0.5, "taper_end_y": 0.0,
                     "inset": 0.0, "normal_offset": 0.0}
    unif_verts = edi_craft._prism_world(uniform_taper)[0]
    unif_end = unif_verts[-n_fp3:]
    # Uniform taper (taper_end_y=0 → sentinel → follow taper_end=0.5):
    # both Z and Y spans halved; asymmetric op has only Z halved.
    assert abs(wy_extent(unif_end) - 0.5 * wy_extent(unif_verts[:n_fp3])) < 1e-9, \
        "uniform taper should also halve the normal-direction span"
    # taper_end_y=0 (default sentinel) is byte-identical to omitting the key.
    uniform_op = dict(base_op, taper_end=1.0)  # no taper → base reference
    sentinel_op = dict(uniform_op, taper_end_y=0.0)  # explicit sentinel
    uniform_verts = edi_craft._prism_world(uniform_op)[0]
    sentinel_verts = edi_craft._prism_world(sentinel_op)[0]
    assert sentinel_verts == uniform_verts, \
        "taper_end_y=0 (sentinel) must be byte-identical to no taper_end_y key"

    # BL-10: inset shrinks the (straight) extrude's footprint; normal_offset
    # fattens the shell. Both 0 are byte-identical to the existing mesh.
    straight = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "extruded_figure", "extruded_figure_ops_compiled.toml"))[0]
    base = edi_craft._prism_world(straight)[0]
    assert edi_craft._prism_world(dict(straight, inset=0.0, normal_offset=0.0))[0] == base, \
        "inset=0 normal_offset=0 must be byte-identical to the plain prism"

    def xy_extent(verts):
        xs = [v[0] for v in verts]
        ys = [v[1] for v in verts]
        return (max(xs) - min(xs)) + (max(ys) - min(ys))
    inset_verts = edi_craft._prism_world(dict(straight, inset=0.3))[0]
    assert xy_extent(inset_verts) < xy_extent(base) - 1e-9, "inset>0 must shrink the footprint"

    def bbox_vol(verts):
        xs = [v[0] for v in verts]; ys = [v[1] for v in verts]; zs = [v[2] for v in verts]
        return (max(xs) - min(xs)) * (max(ys) - min(ys)) * (max(zs) - min(zs))
    fat_verts = edi_craft._prism_world(dict(straight, normal_offset=0.5))[0]
    assert bbox_vol(fat_verts) > bbox_vol(base) + 1e-9, "normal_offset>0 must fatten the shell"

    # P6: robust non-convex inset (edge-offset-then-intersect).
    # v2 replaces the per-vertex bisector-amplification (1/cos_half → ∞ at
    # reflex corners) with a per-EDGE offset + line-intersection approach that
    # is numerically stable at all corner types, plus post-hoc area-flip and
    # edge-crossing checks that return None on bad results.

    # Helper: signed shoelace area of a 2D polygon.
    def signed_area_p6(pts):
        n2 = len(pts)
        a = 0.0
        for i in range(n2):
            x0, y0 = pts[i]; x1, y1 = pts[(i + 1) % n2]
            a += x0 * y1 - x1 * y0
        return a * 0.5

    # ---- Case 1: convex footprint with inset > 0 produces a smaller,
    # well-formed polygon (same as v1 — both exact for convex corners). ----
    p6_convex = [(0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)]  # 4×4 square CCW
    p6_result = edi_craft._inset_polygon(p6_convex, 0.5)
    assert p6_result is not None, "convex inset must succeed"
    # The inset polygon has SMALLER signed area.
    assert signed_area_p6(p6_result) < signed_area_p6(p6_convex) - 1e-9, \
        "inset > 0 must produce a smaller polygon"
    assert len(p6_result) == 4, "convex inset must keep vertex count"
    # All result vertices must be strictly inside the original bbox.
    p6_xs = [p[0] for p in p6_result]; p6_ys = [p[1] for p in p6_result]
    assert min(p6_xs) > 0.0 + 1e-9 and max(p6_xs) < 4.0 - 1e-9, \
        "inset polygon must be strictly inside original x-range"
    assert min(p6_ys) > 0.0 + 1e-9 and max(p6_ys) < 4.0 - 1e-9, \
        "inset polygon must be strictly inside original y-range"

    # ---- Case 2: identity — inset = 0.0 is byte-identical. ----
    assert edi_craft._inset_polygon(p6_convex, 0.0) is p6_convex, \
        "inset=0 must return the same pts object (identity)"

    # ---- Case 3: non-convex (L-shape) footprint with a MODERATE inset that
    # v1 would handle poorly at the reflex vertex — v2 produces a valid,
    # non-self-intersecting inset. ----
    # L-shape (CCW): one reflex vertex at (2,2); adjacent edges each length 2.
    # Pinch limit = 0.5*min(2,2) = 1.0; inset=0.5 is safely below it.
    p6_lshape = [
        (0.0, 0.0), (4.0, 0.0), (4.0, 2.0),
        (2.0, 2.0), (2.0, 4.0), (0.0, 4.0)]
    p6_nc = edi_craft._inset_polygon(p6_lshape, 0.5)
    assert p6_nc is not None, \
        "L-shape with moderate inset should produce a valid result, not None"
    assert len(p6_nc) == 6, "vertex count must be preserved"
    # The area of the inset polygon must be smaller (it was shrunk in).
    assert signed_area_p6(p6_nc) < signed_area_p6(p6_lshape) - 1e-9, \
        "non-convex inset must produce a smaller polygon"
    # Sanity: verify all result edges have positive length (no degenerate edges).
    for i in range(len(p6_nc)):
        ex0, ey0 = p6_nc[i]; ex1, ey1 = p6_nc[(i + 1) % len(p6_nc)]
        assert math.hypot(ex1 - ex0, ey1 - ey0) > 1e-9, "degenerate edge in inset result"

    # ---- Case 4: oversized inset on the non-convex footprint returns None
    # (the post-hoc area-flip or edge-crossing check fires) and the caller
    # falls back to the original un-inset footprint. ----
    # inset=1.5 > pinch limit=1.0 — the C++ guard refuses this; if it somehow
    # reaches Python (defense-in-depth), None is returned.
    p6_over = edi_craft._inset_polygon(p6_lshape, 1.5)
    assert p6_over is None, \
        "oversized inset on non-convex footprint must return None (self-intersection detected)"

    # Confirm the caller (_prism_world) falls back to the original footprint
    # when _inset_polygon returns None: build two ops — one with the safe
    # inset (0.5) and one that is oversized (1.5, which will be None) — and
    # verify the fallback op's XY footprint matches the original.
    p6_fp_dicts = [{"x": x, "y": y} for x, y in p6_lshape]
    p6_op_safe = {"type": "AddPrism", "name": "l_safe",
                  "footprint": p6_fp_dicts, "x": 0.0, "y": 0.0,
                  "base_z": 0.0, "height": 1.0, "inset": 0.5,
                  "normal_offset": 0.0}
    p6_op_over = {"type": "AddPrism", "name": "l_over",
                  "footprint": p6_fp_dicts, "x": 0.0, "y": 0.0,
                  "base_z": 0.0, "height": 1.0, "inset": 1.5,
                  "normal_offset": 0.0}
    p6_verts_safe, _ = edi_craft._prism_world(p6_op_safe)
    p6_verts_over, _ = edi_craft._prism_world(p6_op_over)
    # Fallback: the oversized-inset op's bottom cap verts must match the
    # ORIGINAL footprint (no inset applied), not the failed inset result.
    p6_n = len(p6_lshape)
    p6_orig_xy = set(p6_lshape)
    p6_over_xy = set((v[0], v[1]) for v in p6_verts_over[:p6_n])
    assert p6_orig_xy == p6_over_xy, \
        "oversized-inset fallback: bottom cap must match the original footprint"
    # Safe inset: bottom cap verts differ from original (they were shrunk).
    p6_safe_xy = set((v[0], v[1]) for v in p6_verts_safe[:p6_n])
    assert p6_safe_xy != p6_orig_xy, \
        "safe inset: bottom cap must differ from original (inset was applied)"

    # ---- Case 5: obj_lines with the oversized-inset op includes a # WARNING
    # comment line — and the warning does not contain '{' or '}' (no JSON). ----
    p6_warn_lines = edi_craft.obj_lines([p6_op_over])
    p6_wcs = [l for l in p6_warn_lines if l.startswith("# WARNING:")]
    assert p6_wcs, "obj_lines must emit # WARNING for a rejected inset"
    for wc in p6_wcs:
        assert '{' not in wc and '}' not in wc, "OBJ warning must not contain JSON syntax"
    # obj_lines with the safe op must NOT emit a warning.
    p6_safe_lines = edi_craft.obj_lines([p6_op_safe])
    assert not any(l.startswith("# WARNING:") for l in p6_safe_lines), \
        "obj_lines must not emit a warning for a successful inset"

    # BL-11 / P2: an AddBoolean's PROOF emits its two operands (by name) tagged
    # with the boolean intent — it does NOT compute the CSG. P2: an operand
    # CONSUMED by the boolean is no longer emitted STANDALONE (the real build
    # consumes it), so the box-minus-cylinder sample carries ONLY the two tagged
    # operands — `block` and `bore` do NOT appear standalone.
    bool_ops = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "boolean_op", "boolean_op_ops_compiled.toml"))
    bool_obj = "\n".join(edi_craft.obj_lines(bool_ops)) + "\n"
    with open(os.path.join(ROOT, "samples", "boolean_op", "boolean_op.obj"),
              encoding="utf-8") as f:
        bool_golden = f.read()
    assert bool_obj == bool_golden, "boolean OBJ drifted from samples/boolean_op/boolean_op.obj"
    bool_names = [line[2:] for line in bool_obj.splitlines() if line.startswith("o ")]
    assert bool_names == ["block.minus.bore.subtract.a", "block.minus.bore.subtract.b"], \
        f"boolean proof should be ONLY the two tagged operands, got {bool_names}"
    assert "block" not in bool_names and "bore" not in bool_names, \
        "consumed operands must NOT appear standalone (the boolean consumes them)"

    # BL-06: a partial-angle revolve (sweep_degrees < 360) yields a DIFFERENT,
    # well-formed mesh — the rings span only the arc (no wrap, fewer wall quads)
    # and the two radial open ends are capped, so it is a closed solid, not a
    # shell. A full revolve (the default 360) keeps the original face topology.
    # (N=2 rings, V=8 verts/ring: full = (N-1)*V wall + 2 z-caps = 10 faces;
    # partial = (N-1)*(V-1) wall + 2 z-caps + 2 radial caps = 11 faces.)
    def moulding(sweep=360.0, screw_rise=0.0, screw_turns=1.0):
        return {"type": "AddMoulding", "name": "band", "base_z": 0.0, "x": 0.0, "y": 0.0,
                "vertices": 8, "material": "stone", "sweep_degrees": sweep,
                "screw_rise": screw_rise, "screw_turns": screw_turns,
                "profile": [{"term": "a", "z": 0.0, "radius": 1.0},
                            {"term": "b", "z": 1.0, "radius": 1.0}]}
    full_verts, full_faces = edi_craft.moulding_rings(moulding(360.0))
    part_verts, part_faces = edi_craft.moulding_rings(moulding(180.0))
    assert len(full_faces) == 10, f"full-revolve face topology drifted: {len(full_faces)}"
    assert len(part_faces) == 11, f"partial-revolve face count: {len(part_faces)}"
    assert part_faces != full_faces, "a <360 sweep must change the mesh"
    # Well-formed: every face index references a real vertex, both meshes.
    for verts, faces in ((full_verts, full_faces), (part_verts, part_faces)):
        for face in faces:
            assert all(0 <= i < len(verts) for i in face), "face references a missing vertex"
            assert len(face) >= 3, "degenerate face"
    # The partial revolve is a closed "cake slice" solid (axis spine + caps) —
    # watertight, every edge bounding exactly 2 faces. The FULL revolve is also
    # closed, so assert both. The self-intersecting swept solid (BL-08) stays
    # excluded — see assert_manifold's note.
    assert_manifold(full_verts, full_faces, "full revolve")
    assert_manifold(part_verts, part_faces, "partial revolve")
    # P3b: the partial revolve must also be CONSISTENTLY ORIENTED — every
    # directed edge (a→b) in exactly one face, its reverse (b→a) in exactly one
    # other.  The sector-fan winding fix (P3b) corrects the 18 directed edges
    # that the original code traversed in the same direction as the swept skin.
    assert_oriented(part_faces, "partial revolve")

    # BL-07 / P3: a screw_rise>0 moulding produces a measurably RISING, CLOSED
    # solid — the sweep spirals up by ~screw_rise per full turn over screw_turns
    # turns, and P3's axis spine + inner/outer quads + end caps seal it into a
    # 2-manifold (every edge bounds exactly 2 faces). The default (rise 0)
    # matches the non-helix mesh EXACTLY — behavior-preserving, byte-identical.
    flat_verts, _ = edi_craft.moulding_rings(moulding(screw_rise=0.0))
    helix_verts, helix_faces = edi_craft.moulding_rings(
        moulding(screw_rise=2.0, screw_turns=3.0))
    flat_z = [v[2] for v in flat_verts]
    helix_z = [v[2] for v in helix_verts]
    assert max(flat_z) - min(flat_z) == 1.0, "non-helix z-extent is just the profile (1.0)"
    # rise 2.0 * 3 turns = 6.0 global lift, plus the profile's own 1.0 span.
    assert (max(helix_z) - min(helix_z)) >= 6.0, "helix must rise by ~screw_rise*screw_turns"
    for face in helix_faces:  # well-formed
        assert all(0 <= i < len(helix_verts) for i in face), "helix face references a missing vertex"
        assert len(face) >= 3, "degenerate helix face"
    # The helix is now a closed 2-manifold solid (P3). Previously excluded
    # because it was an open ribbon; now asserted.
    assert_manifold(helix_verts, helix_faces, "helix")
    # P3b: the helix must also be consistently oriented — reversing the outer
    # quads (P3b) ensures the shared axis-spine edge is traversed in OPPOSITE
    # directions by the inner and outer quads, giving each directed edge a
    # unique owner.
    assert_oriented(helix_faces, "helix")
    # The default (rise 0) is byte-identical to the plain non-helix mesh.
    assert edi_craft.moulding_rings(moulding(screw_rise=0.0)) == \
        edi_craft.moulding_rings(moulding()), "rise=0 must match the non-helix mesh"

    # P7: bounds_of tightness for helix + swept prism.
    #
    # WHY this matters: bounds_of feeds the dry-run / snapshot preview rig.
    # A loose frame makes the camera pull back too far (the helix lifts out of
    # the preview box), giving a bad composition.  OBJ output is unaffected —
    # bounds_of is preview-framing only, not part of obj_lines.
    #
    # APPROACH: for helix mouldings, bounds_of now calls _moulding_world and
    # frames from the actual mesh verts — the same approach the swept-prism arm
    # (already correct since BL-08) uses.  For straight (non-helix) mouldings
    # and straight prisms the formula path is unchanged.

    # ---- Helix moulding: z-max must cover the screw_rise * screw_turns lift.
    # With rise=2, turns=3: lift=6; profile z in [0,1]; base_z=0.
    # Old frame: z_max = 1.  New (correct) frame: z_max = 7 (= 1 + 6).
    p7_helix_op = moulding(screw_rise=2.0, screw_turns=3.0)
    p7_hx0, p7_hx1, p7_hy0, p7_hy1, p7_hz0, p7_hz1 = edi_craft.bounds_of([p7_helix_op])
    # z-max must reach or exceed base_z + max_profile_z + screw_rise*screw_turns = 7.
    assert p7_hz1 >= 7.0 - 1e-9, \
        f"helix bounds_of z-max must cover the lift (got {p7_hz1}, expected ≥ 7.0)"
    # z-min must cover the base (profile starts at z=0).
    assert p7_hz0 <= 0.0 + 1e-9, \
        f"helix bounds_of z-min must cover the base (got {p7_hz0})"
    # x/y radius must still cover the profile's max_radius (=1.0 here).
    assert p7_hx1 >= 1.0 - 1e-9, "helix bounds_of x-max must cover max profile radius"
    assert p7_hx0 <= -1.0 + 1e-9, "helix bounds_of x-min must cover -max profile radius"

    # ---- Straight (non-helix) moulding: bounds UNCHANGED (formula path stays). ----
    p7_flat_op = moulding(screw_rise=0.0)          # rise=0 → formula path
    p7_fx0, p7_fx1, p7_fy0, p7_fy1, p7_fz0, p7_fz1 = edi_craft.bounds_of([p7_flat_op])
    assert p7_fz0 == 0.0 and p7_fz1 == 1.0, \
        f"straight moulding bounds_of z must stay [0,1] (got [{p7_fz0},{p7_fz1}])"
    assert p7_fx0 == -1.0 and p7_fx1 == 1.0, \
        f"straight moulding bounds_of x must stay [-1,1] (got [{p7_fx0},{p7_fx1}])"

    # ---- Negative screw_rise: helix goes DOWN — z-min must cover the drop. ----
    # rise=-2, turns=3 → lift=-6; profile z in [0,1]; z-min = 0 + (-6) = -6.
    p7_down_op = moulding(screw_rise=-2.0, screw_turns=3.0)
    _, _, _, _, p7_dz0, p7_dz1 = edi_craft.bounds_of([p7_down_op])
    assert p7_dz0 <= -6.0 + 1e-9, \
        f"descending helix bounds_of z-min must cover the drop (got {p7_dz0})"
    assert p7_dz1 >= 1.0 - 1e-9, \
        f"descending helix bounds_of z-max must still cover the profile top (got {p7_dz1})"

    # ---- Swept prism: x/y bounds cover the FULL path extent (already correct
    # since BL-08 — this test guards against future regression). ----
    # Footprint is a unit square at origin; path extends 10 units along +x.
    p7_sfp = [{"x": 0.0, "y": 0.0}, {"x": 1.0, "y": 0.0},
              {"x": 1.0, "y": 1.0}, {"x": 0.0, "y": 1.0}]
    p7_spath = [{"x": 0.0, "y": 0.0}, {"x": 10.0, "y": 0.0}]
    p7_swept_op = {"type": "AddPrism", "name": "long_run",
                   "footprint": p7_sfp, "path": p7_spath,
                   "x": 0.0, "y": 0.0, "base_z": 0.0, "height": 0.0,
                   "inset": 0.0, "normal_offset": 0.0}
    p7_sx0, p7_sx1, p7_sy0, p7_sy1, p7_sz0, p7_sz1 = edi_craft.bounds_of([p7_swept_op])
    # The path runs from x=0 to x=10; the bounds must reach x=10 (the far end).
    assert p7_sx1 >= 10.0 - 1e-9, \
        f"swept prism bounds_of x-max must cover path extent (got {p7_sx1})"
    # Footprint is 1×1; some y-extent must be present.
    assert p7_sy1 > p7_sy0 + 1e-9, "swept prism must have non-zero y extent"

    # ---- Straight prism (no path): bounds UNCHANGED by P7. ----
    p7_sprism_op = {"type": "AddPrism", "name": "block",
                    "footprint": p7_sfp, "x": 2.0, "y": 3.0,
                    "base_z": 1.0, "height": 4.0, "inset": 0.0, "normal_offset": 0.0}
    p7_px0, p7_px1, p7_py0, p7_py1, p7_pz0, p7_pz1 = edi_craft.bounds_of([p7_sprism_op])
    # Footprint xs in [0,1] offset by x=2 → [2, 3]; ys in [0,1] offset by y=3 → [3, 4].
    assert abs(p7_px0 - 2.0) < 1e-9 and abs(p7_px1 - 3.0) < 1e-9, \
        f"straight prism x bounds must be [2,3] (got [{p7_px0},{p7_px1}])"
    assert abs(p7_pz0 - 1.0) < 1e-9 and abs(p7_pz1 - 5.0) < 1e-9, \
        f"straight prism z bounds must be [1,5] (got [{p7_pz0},{p7_pz1}])"

    # Custom craftsmen (the foundation): the scanner finds the sample script, the
    # registry exposes its manifest as TOML (what the C++ lab reads), and a
    # Script op renders in the proof through the craftsman's proof_mesh.
    registry = edi_craft.load_craftsmen(edi_craft.default_craftsmen_dir())
    assert "twisted_column" in registry, "twisted_column craftsman not discovered"
    assert "radial_petal" in registry, "radial_petal craftsman not discovered"
    assert "nfold_star" in registry, "nfold_star craftsman not discovered"
    manifest_toml = edi_craft.craftsmen_manifest_toml(edi_craft.default_craftsmen_dir())
    # Craftsmen are emitted sorted by id. The CONTAINER BATCH (game-asset library
    # batch 1) added barrel/bucket/chest/crate/pot/sack, which interleave with the
    # original four by alphabetical id: barrel(0) bucket(1) chest(2) crate(3)
    # nfold_star(4) pot(5) radial_petal(6) sack(7) tree(8) twisted_column(9).
    # We pin the original four at their POST-BATCH indices (so a re-sort/load drift
    # is still caught) plus assert each new container id is present by NAME (the
    # positional index of containers is not the contract — their presence is).
    assert 'craftsman.4.id = "nfold_star"' in manifest_toml
    assert 'craftsman.6.id = "radial_petal"' in manifest_toml
    assert 'craftsman.8.id = "tree"' in manifest_toml
    assert 'craftsman.9.id = "twisted_column"' in manifest_toml
    for container in ("barrel", "bucket", "chest", "crate", "pot", "sack"):
        assert f'id = "{container}"' in manifest_toml, \
            f"container craftsman {container} not in manifest"
    assert 'param.2.key = "sides"' in manifest_toml, "craftsman param schema not emitted"

    # CONTAINER BATCH GEOMETRY: presence in the manifest is not enough — each
    # container's pure proof_mesh must render a well-formed, closed-enough mesh,
    # so this block mirrors the tree/nfold_star geometry checks below. The
    # containers are seeded, so a FIXED default op (manifest defaults, seed 0 at
    # the origin) is deterministic. For each: non-empty verts AND faces, no
    # degenerate face, every index in range, and origin-at-base (z-min ≈ 0 — the
    # contract every game asset honours so an instance plants on terrain).
    for container in ("barrel", "bucket", "chest", "crate", "pot", "sack"):
        craftsman = registry[container]
        default_op = {
            "params": {p["key"]: p["default"] for p in craftsman.MANIFEST["params"]},
            "x": 0.0, "y": 0.0, "z": 0.0,
        }
        c_verts, c_faces = craftsman.proof_mesh(default_op)
        assert c_verts and c_faces, f"{container} proof_mesh returned an empty mesh"
        for face in c_faces:
            assert len(set(face)) >= 3, f"degenerate {container} face"
            assert all(0 <= i < len(c_verts) for i in face), \
                f"{container} face references a missing vertex"
        c_zmin = min(v[2] for v in c_verts)
        assert abs(c_zmin) < 1e-6, \
            f"{container} base must sit at z=0 (origin-at-base), got z-min {c_zmin}"
    script_op = {"type": "Script", "script": "twisted_column", "name": "twist",
                 "x": 0.0, "y": 0.0, "z": 0.0,
                 "params": {"radius": "1", "height": "4", "sides": "6", "turns": "1", "rings": "8"}}
    objects = edi_craft.obj_objects([script_op])
    assert len(objects) == 1 and objects[0][0] == "twist", "Script op did not render one object"
    assert len(objects[0][1]) == 9 * 6, f"twisted column vert count: {len(objects[0][1])}"  # (rings+1)*sides
    unknown = {"type": "Script", "script": "nope", "name": "x",
               "x": 0.0, "y": 0.0, "z": 0.0, "params": {}}
    assert edi_craft.obj_objects([unknown]) == [], "an unknown craftsman should be skipped, not crash"

    # BL-12: the radial-petal bloom renders a deterministic, well-formed mesh
    # through its pure proof_mesh — a hub fan + one kite lobe per petal. With P
    # petals: verts = 1 (hub center) + 4*P, faces = 2*P (P hub triangles + P
    # petal kites). Pin the committed sample (P=10) and assert no degenerate face.
    petal_ops = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "radial_petal", "radial_petal_ops_compiled.toml"))
    petal_objs = edi_craft.obj_objects(petal_ops)
    assert len(petal_objs) == 1 and petal_objs[0][0] == "rose.window"
    petal_verts, petal_faces = petal_objs[0][1], petal_objs[0][2]
    assert len(petal_verts) == 1 + 4 * 10, f"radial_petal vert count: {len(petal_verts)}"
    assert len(petal_faces) == 2 * 10, f"radial_petal face count: {len(petal_faces)}"
    for face in petal_faces:
        assert len(set(face)) >= 3, "degenerate petal face"
        assert all(0 <= i < len(petal_verts) for i in face), "petal face references a missing vertex"
    # zRise lifts the petal tips above the flat hub (a gentle dome).
    assert max(v[2] for v in petal_verts) > 0.0, "zRise should lift the petal tips"

    # BL-13: the {n/k} star prism renders a deterministic, CLOSED mesh through
    # its pure proof_mesh — a 2n-vertex alternating outline extruded into a
    # prism. With n points: verts = 4n (bottom + top outline), faces = 2 + 2n
    # (two caps + 2n side quads). Pin the committed sample (n=8).
    star_ops = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "nfold_star", "nfold_star_ops_compiled.toml"))
    star_objs = edi_craft.obj_objects(star_ops)
    assert len(star_objs) == 1 and star_objs[0][0] == "girih.star"
    star_verts, star_faces = star_objs[0][1], star_objs[0][2]
    assert len(star_verts) == 4 * 8, f"nfold_star vert count: {len(star_verts)}"
    assert len(star_faces) == 2 + 2 * 8, f"nfold_star face count: {len(star_faces)}"
    star_zs = [v[2] for v in star_verts]
    assert max(star_zs) - min(star_zs) == 0.6, "star prism z-extent must equal height (0.6)"
    for face in star_faces:
        assert len(set(face)) >= 3, "degenerate star face"
        assert all(0 <= i < len(star_verts) for i in face), "star face references a missing vertex"
    # A DEGENERATE skip (k=1, k>=n, or non-coprime {6/2}) must still render a
    # well-formed, non-empty mesh — the coprime/clamp guard, not a crash. The
    # loaded registry hands back the craftsman module itself.
    nfold = registry["nfold_star"]
    for points, skip in (("6", "2"), ("7", "1"), ("7", "99"), ("3", "2")):
        op = {"params": {"points": points, "skip": skip}, "x": 0.0, "y": 0.0, "z": 0.0}
        verts, faces = nfold.proof_mesh(op)
        n = max(3, int(points))
        assert len(verts) == 4 * n and len(faces) == 2 + 2 * n, f"degenerate-k mesh malformed: {points}/{skip}"
        for face in faces:
            assert len(set(face)) >= 3, f"degenerate face for {points}/{skip}"

    # TREE L4 (BARK / BASE): the first ORGANIC craftsman, now PROPORTIONED. L0 was
    # the scaffold; L1 grew the recursive branching SKELETON; L2 made it
    # structurally believable (pipe-model radii, height budget, distributed
    # primaries); L3 draped a clumped, gapped foliage MASS. L4 fixes the L3 read
    # (short/thin trunk under a dominant crown): firstBranchHeight rises to 0.6 to
    # grow a believable bole (B1 live crown ratio into the healthy 0.40-0.65 band),
    # the root flare eases over a short multi-ring buttress (B4 >= 1.3, no abrupt
    # step), and build() assigns a BARK/LEAF material split on the known face
    # boundary (geometry still from proof_mesh — parity F4 intact). The L4 gate is
    # PROPORTION B1+B4; the L3 gate (CANOPY D1+D2+D4) and all of L2's (C2+C3,
    # B2+B3, height budget) and L1's (C1+C4) stay green on the unchanged skeleton.
    # Everything checkable offline from proof_mesh/canopy_clumps/face_split is
    # pinned here (renders cover the silhouette read); dual-tier parity stays on
    # the checklist.
    assert "tree" in registry, "tree craftsman not discovered"
    # The tree manifest must declare ALL 24 params up front (schema stability
    # across L0->L5) and use only the C++-known types.
    tree = registry["tree"]
    tree_params = tree.MANIFEST["params"]
    assert len(tree_params) == 24, f"tree manifest must declare all 24 params, got {len(tree_params)}"
    for p in tree_params:
        assert p["type"] in ("number", "integer", "material"), \
            f"tree param {p['key']!r} has non-C++ type {p['type']!r}"
    # Bark/leaf materials must default to EXISTING MATERIALS keys (additive-only:
    # "bark"/"leaf" are NOT in the table).
    tree_defaults = {p["key"]: p["default"] for p in tree_params}
    assert tree_defaults["barkMat"] in edi_craft.MATERIALS, \
        f"barkMat default {tree_defaults['barkMat']!r} not an existing material"
    assert tree_defaults["leafMat"] in edi_craft.MATERIALS, \
        f"leafMat default {tree_defaults['leafMat']!r} not an existing material"

    tree_ops = edi_craft.parse_ops(
        os.path.join(ROOT, "samples", "tree", "tree_ops_compiled.toml"))
    tree_objs = edi_craft.obj_objects(tree_ops)
    assert len(tree_objs) == 1 and tree_objs[0][0] == "tree", "tree sample did not render one object"
    tree_verts, tree_faces = tree_objs[0][1], tree_objs[0][2]
    assert tree_verts and tree_faces, "tree proof_mesh returned an empty mesh"
    for face in tree_faces:
        assert len(set(face)) >= 3, "degenerate tree face"
        assert all(0 <= i < len(tree_verts) for i in face), "tree face references a missing vertex"

    # F3 — origin at base: the lowest vertex sits at z≈0 (the sample places the
    # op at z=0), so an instance plants its base on terrain.
    tree_zs = [v[2] for v in tree_verts]
    assert abs(min(tree_zs)) < 1e-6, f"tree base must sit at z=0 (F3), got z-min {min(tree_zs)}"

    # F2 — poly budget: one tree at L2 defaults is FAR under the 25k-tri forest
    # target (read the tri count from the proof, not a render). The branch fanout
    # is the explosion risk (childrenPerNode^branchLevels) and the per-segment
    # curve adds rings, so this guard matters after L2.
    tree_tris = sum(len(face) - 2 for face in tree_faces)
    assert tree_tris < 25000, f"tree exceeds the 25k-tri budget (F2): {tree_tris}"

    # A1 (sharpened) — trunk vertical: fit a line to the trunk-ring centroids and
    # measure its angle from +z (< ~15°). The dedicated _trunk_mesh is built FIRST
    # in _local_mesh, so the trunk's own ring verts are exactly the first
    # (segments+1)*tubeSides verts — isolate THEM (the L2 branch tubes now droop
    # with `curve` and would pollute a "below trunk_cut" slice with off-axis
    # verts). Their per-z-layer centroids must climb straight up the +z axis.
    tree_height = float(tree_ops[0]["params"]["height"])
    # firstBranchHeight default is 0.35 (not in the sample → use the manifest default).
    first_branch_frac = float(tree_defaults["firstBranchHeight"])
    trunk_cut = first_branch_frac * tree_height
    a1_tube_sides = int(float(tree_ops[0]["params"].get("tubeSides", 6)))
    a1_segments = int(float(tree_ops[0]["params"].get("segmentsPerBranch", 4)))
    n_trunk_verts = (a1_segments + 1) * a1_tube_sides
    trunk_verts = tree_verts[:n_trunk_verts]
    assert len(trunk_verts) >= 2, "no trunk verts to fit the trunk axis"
    # Group by z-layer, take each layer's xy-centroid, then fit a direction from
    # the lowest to the highest centroid (the trunk axis).
    layers = {}
    for (vx, vy, vz) in trunk_verts:
        key = round(vz, 6)
        cx, cy, n = layers.get(key, (0.0, 0.0, 0))
        layers[key] = (cx + vx, cy + vy, n + 1)
    centroids = sorted(
        ((z, sx / n, sy / n) for z, (sx, sy, n) in layers.items()), key=lambda t: t[0])
    assert len(centroids) >= 2, "need >=2 trunk z-layers to fit an axis"
    z_lo, x_lo, y_lo = centroids[0]
    z_hi, x_hi, y_hi = centroids[-1]
    dx, dy, dz = x_hi - x_lo, y_hi - y_lo, z_hi - z_lo
    horiz = math.hypot(dx, dy)
    trunk_angle_deg = math.degrees(math.atan2(horiz, abs(dz)))
    assert trunk_angle_deg < 15.0, \
        f"trunk axis must be within 15° of vertical (A1), got {trunk_angle_deg:.1f}°"

    # Branch mass must exist ABOVE the first-branch height (the armature, not a
    # bare pole). At L1 these are the recursive branch tubes, not a crown blob.
    crown_verts = [v for v in tree_verts if v[2] > trunk_cut + 1e-9]
    assert len(crown_verts) >= 6, \
        f"a branch mass must exist above firstBranchHeight (got {len(crown_verts)} verts)"
    # The armature must spread WIDE off the trunk axis (not a thin broom): the
    # branch tips reach out, so the xy-extent up top must be a real fraction of
    # the tree height.
    crown_xs = [v[0] for v in crown_verts]
    crown_width = max(crown_xs) - min(crown_xs)
    assert crown_width >= 0.4 * tree_height, \
        f"armature too narrow to read (A2): width {crown_width:.2f} vs height {tree_height}"

    # A2 (UPPER bound) — the crown must not SPRAWL either. The L2 critique found
    # lengthRatio=0.72 drove the FULL-mesh crown XY width to ~1.31× the mesh
    # max-z (over the 0.5-1.0× band — a lollipop that the L3 canopy would only
    # amplify). lengthRatio=0.56 pulls it into band; this guard LOCKS the band
    # going into L3 so the canopy builder can't silently re-widen the armature.
    # Width = the larger of the x-span and y-span over ALL tree_verts (the full
    # mesh bbox, trunk included); divide by the mesh max-z. C3 (emergence angle)
    # is orthogonal to lengthRatio, so this band is held purely by the length
    # lever — no collateral on the angle/taper/budget bands measured below.
    all_xs = [v[0] for v in tree_verts]
    all_ys = [v[1] for v in tree_verts]
    crown_xy_width = max(max(all_xs) - min(all_xs), max(all_ys) - min(all_ys))
    a2_max_z = max(tree_zs)  # tree_zs already collected above (F3 check)
    crown_w_over_h = crown_xy_width / a2_max_z
    assert crown_w_over_h <= 1.05, \
        f"A2: full-mesh crown XY width/max-z {crown_w_over_h:.3f} must be <= 1.05 " \
        f"(over-wide sprawl — width {crown_xy_width:.3f}, max-z {a2_max_z:.3f})"

    # ---- L3 (CANOPY) gate: D1 + D2 + D4. The canopy is `clumpCount` discrete
    # icosphere blobs at the OUTER tips — a foliage MASS with gaps, NOT a swept
    # lollipop. canopy_clumps() returns the clump centres + the PINNED fill proxy
    # off the SAME fitted skeleton the skin uses, so the gate reads the rendered
    # canopy without re-parsing the mesh.
    clump_centres, clump_size, d2_fill = tree.canopy_clumps(tree_ops[0]["params"])

    # D1 — >=6 distinct clumps at the outer tips. The default clumpCount is 72.
    clump_count_default = int(float(tree_defaults["clumpCount"]))
    assert clump_count_default >= 6, \
        f"D1: clumpCount default {clump_count_default} must be >=6"
    assert len(clump_centres) >= 6, \
        f"D1: need >=6 distinct canopy clumps, got {len(clump_centres)}"
    # Distinct centres (no two clumps stacked on one tip) — the gaps are real.
    assert len({(round(c[0], 6), round(c[1], 6), round(c[2], 6)) for c in clump_centres}) \
        == len(clump_centres), "D1: canopy clumps must sit at DISTINCT tips (no overlap-stack)"

    # D2 — the PINNED fill proxy in the reads-as-MASS BAND 0.15 <= fill < 0.6
    # (doc 7). The lower bound is the L3 critique's fix: 14 clumps @0.55 gave
    # fill~0.045 (~4.5%) — a near-EMPTY crown that read as decorated tips, the
    # OPPOSITE failure from a lollipop, yet it slipped past the old <0.6-only
    # gate. The upper bound keeps gaps (not a solid lollipop). The chosen defaults
    # (72 clumps @0.64) land ~0.27, mid-band. The band locks BOTH failure modes
    # offline: a future sparseness regression trips the floor, an inflation the
    # ceiling.
    assert 0.15 <= d2_fill < 0.6, \
        f"D2: canopy fill proxy {d2_fill:.3f} must be in [0.15, 0.6) — reads as a " \
        f"foliage MASS (above empty) AND clumped+gapped (below a lollipop)"

    # D4 — foliage in the UPPER crown only: every clump centroid sits ABOVE
    # firstBranchHeight*height so the bare bole is never skirted by leaves.
    d4_cut = first_branch_frac * tree_height
    min_clump_z = min(c[2] for c in clump_centres)
    assert min_clump_z > d4_cut, \
        f"D4: lowest clump centroid z {min_clump_z:.3f} must be ABOVE the bole cut {d4_cut:.3f}"

    # The clumps must actually appear in the RENDERED mesh, not just the helper:
    # icosphere blobs add verts ABOVE the bole cut. (The skeleton-only checks
    # below would pass even if the canopy were never skinned — this guards the
    # wiring in _local_mesh.) leafSubdiv=1 → 32 tris × clumpCount ≈ 450 tris.
    canopy_min_tris = 8 * len(clump_centres)  # >=8 tris/clump even at subdiv=0
    tris_with_canopy = sum(len(face) - 2 for face in tree_faces)
    assert tris_with_canopy >= canopy_min_tris, \
        f"canopy not skinned into the mesh: {tris_with_canopy} tris < {canopy_min_tris} expected from clumps"

    # ---- L4 (BARK / BASE) gate: B1 (live crown ratio) + B4 (base flare). L3
    # gave a coherent canopy but the crown DOMINATED — the trunk read short/thin
    # (LCR ~0.73, above the healthy band). L4 raises firstBranchHeight (0.6) to
    # grow a believable bole, pulling the foliage MASS up so the LCR lands in the
    # healthy 0.40-0.65 band, and eases the root flare over a short multi-ring
    # buttress (rather than one abrupt step) while keeping B4 >= 1.3.

    # B1 — LIVE CROWN RATIO: foliage vertical extent / total tree height in
    # 0.40-0.65 (a healthy crown over a present bole). Foliage extent = the z-span
    # of the canopy clumps (centre ± clump_size), off the SAME helper the skin
    # uses. The default tunes toward a healthy mid-band ~0.50.
    foliage_zs = [c[2] for c in clump_centres]
    foliage_zmin = min(foliage_zs) - clump_size
    foliage_zmax = max(foliage_zs) + clump_size
    lcr = (foliage_zmax - foliage_zmin) / a2_max_z  # a2_max_z = max(tree_zs)
    assert 0.40 <= lcr <= 0.65, \
        f"B1: live crown ratio {lcr:.3f} must be in 0.40-0.65 (foliage extent " \
        f"[{foliage_zmin:.3f}, {foliage_zmax:.3f}] / height {a2_max_z:.3f})"

    # B4 — BASE FLARE: bottom ring radius / next ring radius >= 1.3 (root
    # buttress). _trunk_mesh now eases the flare over BUTTRESS_RINGS rings; the
    # steepest drop is bottom->next, so that ratio gates the buttress. Rebuild the
    # trunk to read its first two ring radii (radius = max horizontal distance
    # from the trunk axis x=y=0). This assertion was BUILT but never gated at L3.
    b4_tube_sides = int(float(tree_ops[0]["params"].get("tubeSides", 6)))
    b4_segments = int(float(tree_ops[0]["params"].get("segmentsPerBranch", 4)))
    b4_base_flare = float(tree_ops[0]["params"].get("baseFlare", tree_defaults["baseFlare"]))
    b4_trunk_radius = float(tree_ops[0]["params"]["trunkRadius"])
    b4_tv, _b4_tf, _b4_tr = tree._trunk_mesh(
        tree_height, b4_trunk_radius, b4_base_flare, b4_segments, b4_tube_sides)
    b4_ring0 = max(math.hypot(v[0], v[1]) for v in b4_tv[:b4_tube_sides])
    b4_ring1 = max(math.hypot(v[0], v[1]) for v in b4_tv[b4_tube_sides:2 * b4_tube_sides])
    base_flare_ratio = b4_ring0 / b4_ring1
    assert base_flare_ratio >= 1.3, \
        f"B4: base flare (bottom ring {b4_ring0:.4f} / next ring {b4_ring1:.4f}) " \
        f"= {base_flare_ratio:.3f} must be >= 1.3 (root buttress)"

    # MATERIAL HINT (the bark/leaf half of L4): the pure face_split helper reports
    # the structure->canopy face boundary build() slices on to assign material
    # slots WITHOUT a second generator (geometry stays from proof_mesh — F4). The
    # two counts must sum to the total face count (no face unclassified), and both
    # halves must be non-empty (a real bark mass AND a real leaf mass).
    struct_faces, canopy_faces = tree.face_split(tree_ops[0]["params"])
    assert struct_faces + canopy_faces == len(tree_faces), \
        f"face_split must partition all faces: {struct_faces}+{canopy_faces} != {len(tree_faces)}"
    assert struct_faces > 0 and canopy_faces > 0, \
        f"face_split halves must both be non-empty: bark {struct_faces}, leaf {canopy_faces}"
    # The canopy half must match the icospheres actually skinned (>=8 faces/clump).
    assert canopy_faces >= 8 * len(clump_centres), \
        f"face_split canopy count {canopy_faces} < {8 * len(clump_centres)} (one ico per clump)"

    # C1 (BRANCHING gate) — >=3 visible branch LEVELS (trunk -> primary ->
    # secondary -> tertiary). skeleton_levels() returns the distinct levels; the
    # trunk is level 0, so >=3 levels BEYOND it means the max level >= 3. The
    # pure helper lets us check the hierarchy offline without parsing the skin.
    levels = tree.skeleton_levels(tree_ops[0]["params"])
    assert levels[0] == 0, f"skeleton must start at the trunk (level 0), got {levels}"
    assert max(levels) >= 3, \
        f"C1: need >=3 branch levels beyond the trunk, got levels {levels}"

    # C4 (BRANCHING gate) — successive children SPIRAL around the parent at the
    # golden azimuth (rotateAngle), not coplanar / not all one side. Take the
    # primary branches (level 1) sprouting off the trunk top and measure each
    # one's azimuth around the trunk's +z axis; successive primaries must step by
    # ~rotateAngle. L5a NOTE: rotateJitter now perturbs each primary's azimuth by
    # ± rotateJitter, so the step is no longer EXACT — it is golden ± (up to)
    # 2*rotateJitter (each endpoint jittered independently). The band keeps the
    # gate's intent (a phyllotactic spiral, NOT coplanar) while admitting the seed.
    rotate_angle = float(tree_defaults["rotateAngle"])  # 137.5 (golden)
    rotate_jitter_deg = float(tree_defaults["rotateJitter"])  # 20.0 (the L5a bound)
    skel = tree._skeleton(tree_ops[0]["params"])
    primaries = [b for b in skel if b["level"] == 1]
    assert len(primaries) >= 2, f"need >=2 primaries to measure spiral, got {len(primaries)}"

    def _branch_azimuth(branch):
        # Azimuth (deg, around +z) of the branch's start->tip direction.
        s, t = branch["nodes"][0], branch["nodes"][-1]
        return math.degrees(math.atan2(t[1] - s[1], t[0] - s[0])) % 360.0

    prim_az = [_branch_azimuth(b) for b in primaries]
    # Not coplanar / not all one side: the azimuths must actually differ.
    assert len(set(round(a, 3) for a in prim_az)) == len(prim_az), \
        f"C4: primaries are coplanar (duplicate azimuths): {prim_az}"
    for i in range(len(prim_az) - 1):
        step = (prim_az[i + 1] - prim_az[i]) % 360.0
        # Tolerance = base golden step ± 2*rotateJitter (both endpoints jittered).
        assert abs(step - rotate_angle) <= 2.0 * rotate_jitter_deg + 1e-6, \
            f"C4: successive primaries must step ~the golden angle {rotate_angle} " \
            f"(±2*rotateJitter), got {step:.3f}"

    # ---- L2 (STRUCTURE) gate: height budget + pipe model + taper + distributed
    # primaries. These are the defects the L1 critique owned; each is checkable
    # offline from proof_mesh/skeleton, so pin the measured value to the rubric
    # band so a regression names which band it broke.

    # HEIGHT BUDGET (the headline L1 defect): the MANIFEST says `height` = "trunk
    # base -> crown top". L1's crown top reached z~=15.4 for height=6 (2.57x). L2
    # bounds the recursion (outward lean + curve) and uniformly fits the crown top
    # to `height`, so the WHOLE tree fits its declared height within tolerance.
    tree_zmax = max(tree_zs)
    assert tree_zmax <= 1.15 * tree_height, \
        f"height budget: crown top {tree_zmax:.3f} must be <= 1.15*height ({1.15*tree_height:.3f})"
    # The fit should not crush the tree to a stump either — the crown should reach
    # most of the declared height.
    assert tree_zmax >= 0.85 * tree_height, \
        f"height budget: crown top {tree_zmax:.3f} should reach >= 0.85*height ({0.85*tree_height:.3f})"

    # C2 (pipe model) — child radius / parent radius for a 3-way split must land in
    # 0.55-0.75 (matches (1/n)^(1/e), e~=2.0-2.3). Measure a secondary (level 2)
    # start radius against its parent primary (level 1) start radius. They are NOT
    # the same thickness as the parent (the placeholder fixed-fraction is gone).
    secondaries = [b for b in skel if b["level"] == 2]
    assert primaries and secondaries, "need primaries + secondaries to check the pipe model"
    pipe_ratio = secondaries[0]["radii"][0] / primaries[0]["radii"][0]
    assert 0.55 <= pipe_ratio <= 0.75, \
        f"C2: pipe-model child/parent radius {pipe_ratio:.4f} must be in 0.55-0.75"

    # B2 (HONEST) — trunk base radius / height in 0.02-0.05, MEASURED ON THE FINAL
    # RENDERED MESH. The L1->L2 critique found the old gate read the raw PARAM
    # ratio (trunkRadius/height), which the height fit then quietly violated: a
    # uniform whole-mesh scale shrank the trunk radius along with the positions, so
    # the RENDERED base/height was 0.013 (below band) while the gate passed on
    # 0.030. The position-only fit keeps radii honest, but the gate must still read
    # the MESH, not the param. The trunk base ring is the first `tubeSides` verts
    # (the z≈0 ring of _trunk_mesh, which is built first in _local_mesh); its
    # radius = the max horizontal distance from the trunk axis (x=y=0). Divide by
    # the mesh's actual max z. This is the number that describes the rendered tree.
    tree_trunk_radius = float(tree_ops[0]["params"]["trunkRadius"])
    b2_tube_sides = int(float(tree_ops[0]["params"].get("tubeSides", 6)))
    base_ring = tree_verts[:b2_tube_sides]
    base_ring_r = max(math.hypot(v[0], v[1]) for v in base_ring)
    base_r_over_h = base_ring_r / tree_zmax
    assert 0.02 <= base_r_over_h <= 0.05, \
        f"B2: FINAL-mesh trunk base-ring radius/max-z {base_r_over_h:.4f} " \
        f"must be in 0.02-0.05 (base ring r={base_ring_r:.4f}, max z={tree_zmax:.3f})"

    # B3 — trunk visibly TAPERS: top-of-trunk radius / base radius in 0.3-0.6. The
    # trunk mesh's own radii carry this; rebuild it to read base + top radii. L1's
    # was 0.61 (too flat); L2 steepens the trunk per-segment taper into band.
    tree_tube_sides = int(float(tree_ops[0]["params"].get("tubeSides", 6)))
    tree_segments = int(float(tree_ops[0]["params"].get("segmentsPerBranch", 4)))
    tree_base_flare = float(tree_ops[0]["params"].get("baseFlare", tree_defaults["baseFlare"]))
    _tv, _tf, trunk_top_r = tree._trunk_mesh(
        tree_height, tree_trunk_radius, tree_base_flare, tree_segments, tree_tube_sides)
    trunk_taper_ratio = trunk_top_r / tree_trunk_radius
    assert 0.3 <= trunk_taper_ratio <= 0.6, \
        f"B3: trunk top/base radius {trunk_taper_ratio:.4f} must be in 0.3-0.6"

    # C3 — primary branch elevation (down) angle off the trunk in 30-55° (not
    # horizontal shelves, not vertical brooms). Measure the angle of a primary's
    # FIRST segment off the trunk's +z axis (the emergence angle — the per-segment
    # curve bends the later segments further down, but C3 is about how the limb
    # LEAVES the trunk).
    def _down_angle_deg(branch):
        s, t = branch["nodes"][0], branch["nodes"][1]
        d = (t[0] - s[0], t[1] - s[1], t[2] - s[2])
        dl = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) or 1.0
        return math.degrees(math.acos(max(-1.0, min(1.0, d[2] / dl))))
    # L5a: downAngleJitter perturbs each primary's emergence angle by ± its bound,
    # so a single primary can sit up to downAngleJitter outside the structural
    # 30-55° band while the DIAL (downAngle=45) stays centred in it. Gate the band
    # widened by the jitter bound (the structural intent survives the seed), and
    # separately pin that the base dial itself is in the rubric band.
    down_angle_base = float(tree_defaults["downAngle"])       # 45.0
    down_jitter_deg = float(tree_defaults["downAngleJitter"])  # 12.0 (the L5a bound)
    assert 30.0 <= down_angle_base <= 55.0, \
        f"C3: base downAngle dial {down_angle_base}° must be in 30-55°"
    prim_down = _down_angle_deg(primaries[0])
    assert 30.0 - down_jitter_deg <= prim_down <= 55.0 + down_jitter_deg, \
        f"C3: primary down-angle {prim_down:.2f}° must be in 30-55° (±downAngleJitter)"

    # DROOP CLAMP (L2 curve responsibility) — no branch tip may plunge near
    # vertical. The L1->L2 critique found the per-level down-angle widen stacked on
    # the curve droop drove L3/L4 tips to ~-90° elevation (a weeping/dead-tree
    # read). L2 tempers the widen AND clamps the running axis so NO tip heads more
    # steeply DOWN than ~20° below horizontal. Measure the elevation (angle above
    # horizontal, negative = below) of every branch's final segment; the minimum
    # must be >= -20° (a small epsilon for float error). Elevation is invariant
    # under the uniform position fit, so the skeleton tips describe the rendered
    # tips exactly.
    def _tip_elev_deg(branch):
        n = branch["nodes"]
        d = (n[-1][0] - n[-2][0], n[-1][1] - n[-2][1], n[-1][2] - n[-2][2])
        dl = math.sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) or 1.0
        return math.degrees(math.asin(max(-1.0, min(1.0, d[2] / dl))))
    tip_elevs = [_tip_elev_deg(b) for b in skel if b["level"] >= 1]
    min_tip_elev = min(tip_elevs)
    assert min_tip_elev >= -20.0 - 1e-6, \
        f"droop clamp: min branch-tip elevation {min_tip_elev:.2f}° plunges below " \
        f"-20° (a weeping/dead-tree artifact)"

    # DISTRIBUTED primaries (kill the wishbone): the primaries must emerge at MORE
    # THAN ONE distinct height along the trunk (L1 sprang them all from the single
    # trunk-top node). firstBranchHeight is now LIVE — the lowest primary sits at
    # or above firstBranchHeight*height, leaving a clear bole below.
    prim_start_z = sorted({round(b["nodes"][0][2], 4) for b in primaries})
    assert len(prim_start_z) > 1, \
        f"distributed primaries: need >1 distinct start height, got {prim_start_z}"
    assert min(prim_start_z) >= first_branch_frac * tree_height - 1e-6, \
        f"firstBranchHeight: lowest primary {min(prim_start_z):.3f} below the bole cut " \
        f"{first_branch_frac * tree_height:.3f}"

    # F4 (dual-tier parity proxy): proof_mesh is deterministic — two calls give
    # identical (verts,faces). build() calls proof_mesh, so the bpy mesh shares
    # the count; same-seed determinism (E2) holds for free at L2 (every angle is
    # exact, NO RNG — jitter arrives at L5).
    v2, f2 = tree.proof_mesh(tree_ops[0])
    assert v2 == tree_verts and f2 == tree_faces, "tree proof_mesh is not deterministic"

    # ---- TREE L5a (SEEDED JITTER / VARIATION): E gate = E1 + E2 + E3, plus the
    # must-not-break bands re-checked across SEVERAL seeds.
    #
    # L5a turns the seed ON: ONE random.Random(int(float(seed))) is threaded
    # through the generation in fixed depth-first order, drawing every per-node /
    # per-clump jitter UNCONDITIONALLY before any keep/clip decision. The whole
    # value of the asset is REPRODUCIBILITY, so the killer gate is E2: same seed →
    # byte-identical mesh (proof tier == bpy tier, since build() calls proof_mesh);
    # different seed → a visibly different tree (E1). The jitter breaks the
    # candelabra/bilateral regularity (E3) — verified offline as a real angular
    # spread among sibling primaries.
    def _tree_op(seed):
        # Same params as the committed sample, only the seed varies.
        p = dict(tree_ops[0]["params"], seed=str(seed))
        return {"type": "Script", "script": "tree", "name": "tree",
                "x": 0.0, "y": 0.0, "z": 0.0, "params": p}

    # E2 — DETERMINISM: proof_mesh(seed=K) called twice is byte-identical (the
    # dual-tier killer; build() reuses proof_mesh so the bpy tier inherits it).
    for k in (0, 1, 2, 3):
        a_v, a_f = tree.proof_mesh(_tree_op(k))
        b_v, b_f = tree.proof_mesh(_tree_op(k))
        assert a_v == b_v and a_f == b_f, \
            f"E2: tree proof_mesh(seed={k}) is not byte-identical across two calls"

    # E1 — VARIATION: seed=1 vs seed=2 (all else equal) produce DIFFERENT meshes
    # (different branch placement/angles + clump layout). The vertex lists must
    # not be equal, and the vertex COUNT typically differs too (jitter changes how
    # many clumps survive the D4 cut).
    s1_v, _ = tree.proof_mesh(_tree_op(1))
    s2_v, _ = tree.proof_mesh(_tree_op(2))
    assert s1_v != s2_v, "E1: seed=1 and seed=2 must produce different trees"
    # And seed=1 differs from the default seed=0 sample too (the seed actually bites).
    assert s1_v != tree_verts, "E1: seed=1 must differ from the default seed=0 tree"

    # E3 — NO SYMMETRY: the per-child downAngleJitter makes sibling primaries show
    # a real angular SPREAD (not a mirror/candelabra). Measure the primary
    # down-angles at a jittered seed; their spread must exceed a noise floor (the
    # EXACT tree would have near-identical primary down-angles).
    e3_skel = tree._skeleton(_tree_op(1)["params"])
    e3_prims = [b for b in e3_skel if b["level"] == 1]
    e3_downs = [_down_angle_deg(b) for b in e3_prims]
    assert (max(e3_downs) - min(e3_downs)) > 1.0, \
        f"E3: jittered primaries must spread in down-angle (mirror/candelabra), got {e3_downs}"

    # MUST-NOT-BREAK across seeds 0..3: A2 <= 1.05, D2 fill band, height budget,
    # F2 tri budget, D4 clumps above the bole. A seed that knocks any band out
    # must fail here (the A2 XY-safety clamp is what keeps the crown width in band
    # for every seed despite the jitter splaying the branches).
    for k in range(4):
        kp = _tree_op(k)["params"]
        kv, kf = tree.proof_mesh(_tree_op(k))
        kzs = [v[2] for v in kv]
        kxs = [v[0] for v in kv]
        kys = [v[1] for v in kv]
        k_maxz = max(kzs)
        k_a2 = max(max(kxs) - min(kxs), max(kys) - min(kys)) / k_maxz
        assert k_a2 <= 1.05, f"L5a A2: seed={k} crown width/max-z {k_a2:.3f} > 1.05"
        assert abs(min(kzs)) < 1e-6, f"L5a F3: seed={k} base must sit at z=0"
        assert 0.85 * tree_height <= k_maxz <= 1.15 * tree_height, \
            f"L5a height budget: seed={k} max-z {k_maxz:.3f} out of [0.85,1.15]*height"
        k_tris = sum(len(face) - 2 for face in kf)
        assert k_tris < 25000, f"L5a F2: seed={k} {k_tris} tris >= 25k"
        k_clumps, _k_cs, k_fill = tree.canopy_clumps(kp)
        assert 0.15 <= k_fill < 0.6, f"L5a D2: seed={k} fill {k_fill:.3f} out of [0.15,0.6)"
        assert len(k_clumps) >= 6, f"L5a D1: seed={k} only {len(k_clumps)} clumps"
        k_d4_cut = first_branch_frac * tree_height
        assert min(c[2] for c in k_clumps) > k_d4_cut, \
            f"L5a D4: seed={k} a clump centre fell below the bole cut {k_d4_cut:.3f}"
        # D3 (clump size variation) — clumpJitter makes the per-clump sizes spread.
        k_sizes = [c[3] for c in k_clumps]
        assert (max(k_sizes) - min(k_sizes)) > 1e-6, \
            f"L5a D3: seed={k} clump sizes must vary (clumpJitter)"

    # P5: bisector miter joints on _swept_prism_world.
    #
    # BL-08 v1 placed each cross-section loop perpendicular to ONE adjacent
    # segment's tangent; at corners the quads on the tight side overlapped while
    # the quads on the wide side left a gap. P5 v2 replaces that with a bisector
    # miter frame — the cross-section is placed in the MITER PLANE, which is
    # perpendicular to the normalised sum (t_in + t_out) of the adjacent unit
    # tangents. The cross-section width is pre-scaled by 2/|t_in+t_out| (the
    # miter-width compensation) so it keeps the same apparent width through the
    # corner. STRAIGHT paths: t_in = t_out → b = 2·t_in → |b| = 2 → scale = 1 →
    # byte-identical to v1.
    #
    # The swept_profile sample has a 90° corner at (4,0):
    #   t_in = (1,0), t_out = (0,1), b = (1,1), |b| = √2, miter_scale = √2.
    # Its golden was REGENERATED as part of P5 — the 4 corner-loop verts shift
    # by ±0.5 in y (= 0.5 * 1/√2 * √2 = ±0.5) to account for the oblique cut.

    # ---- Case 1: the new swept_profile golden matches.
    # (The byte-exact comparison above already asserts this — this comment
    # is the P5 semantic annotation on that assertion.)
    # swept_obj == swept_golden is already asserted above. ✓

    # ---- Case 2: a 90° corner sweep produces a mitered (non-collapsed) join.
    # Build a synthetic sweep with the SAME footprint + path as swept_profile.
    # The corner loop (k=1) should be the middle n_fp=4 verts of the 12-vert mesh.
    p5_fp = [{"x": -0.5, "y": 0.0}, {"x": 0.5, "y": 0.0},
             {"x": 0.5, "y": 1.0}, {"x": -0.5, "y": 1.0}]
    p5_path = [{"x": 0.0, "y": 0.0}, {"x": 4.0, "y": 0.0}, {"x": 4.0, "y": 3.0}]
    p5_op = {"footprint": p5_fp, "path": p5_path, "x": 0.0, "y": 0.0,
             "base_z": 0.0, "inset": 0.0, "normal_offset": 0.0}
    p5_verts, _ = edi_craft._prism_world(p5_op)
    # 4 footprint points × 3 path points = 12 verts; k=1 → middle 4.
    assert len(p5_verts) == 12, f"90° sweep vert count: {len(p5_verts)}"
    p5_start_loop = p5_verts[:4]      # k=0 endpoint loop
    p5_corner_loop = p5_verts[4:8]    # k=1 mitered corner loop
    # The footprint's fy maps to world Z only, so each loop has n_fp/2 distinct
    # XY positions (one per unique fx value: fx=-0.5 and fx=0.5 here).
    # Key miter geometry at the 90° corner (t_in=(1,0), t_out=(0,1)):
    #   b=(1,1), |b|=√2, miter_scale=√2.
    #   nx_eff = (-1/√2)·√2 = -1; ny_eff = (1/√2)·√2 = 1.
    #   fx=-0.5 → world=(4+0.5, 0-0.5, 0/1) = (4.5, -0.5, ·)
    #   fx= 0.5 → world=(4-0.5, 0+0.5, 0/1) = (3.5,  0.5, ·)
    # Confirm the miter is active: the corner loop spans BOTH X and Y (the
    # oblique miter places verts at different x AND y, unlike the start loop
    # which is purely Y-spread).
    p5_start_xs = {v[0] for v in p5_start_loop}  # start: all verts at x=0
    p5_corner_xs = {v[0] for v in p5_corner_loop}
    p5_corner_ys = {v[1] for v in p5_corner_loop}
    # Start loop: the path heads along +x (ny_eff=1, nx_eff=0) → all verts x=0.
    assert len(p5_start_xs) == 1, "start endpoint loop should have no x-spread (path along +x)"
    # Corner loop: the miter rotates the frame → verts shift in both X and Y.
    assert len(p5_corner_xs) > 1, "mitered corner loop must span x (miter tilts the frame)"
    assert len(p5_corner_ys) > 1, "mitered corner loop must span y (miter tilts the frame)"
    # Confirm the x-span at the corner matches the golden: |4.5 - 3.5| = 1.0.
    # (The miter_scale=√2 compensates so the tube width is preserved.)
    p5_corner_x_span = max(p5_corner_xs) - min(p5_corner_xs)
    assert abs(p5_corner_x_span - 1.0) < 1e-9, \
        f"corner x-span should be 1.0 (width-compensated): got {p5_corner_x_span}"
    # Confirm the y-extent: |-0.5 - 0.5| = 1.0 (same scale).
    p5_corner_y_span = max(p5_corner_ys) - min(p5_corner_ys)
    assert abs(p5_corner_y_span - 1.0) < 1e-9, \
        f"corner y-span should be 1.0 (miter projects footprint width onto y): got {p5_corner_y_span}"

    # ---- Case 3: a STRAIGHT-path sweep is byte-identical to BL-08 v1.
    # With collinear path points t_in == t_out at every interior point, so
    # b = 2·t_in, |b| = 2, miter_scale = 1.0 — exactly the v1 frame.
    # Use the P4 fixture (3-point collinear path along +x) as the reference.
    #
    # For a collinear +x path: tx=1, ty=0 at all points → nx_eff=0, ny_eff=1
    # (no miter amplification). The footprint's fx maps to world-y via ny_eff=1
    # and fy maps to world-z. Across loops, the ONLY changing coordinate is the
    # path's x — so y and z of corresponding verts are IDENTICAL for all loops.
    p5_collinear_op = {"footprint": fp3, "path": path3, "x": 0.0, "y": 0.0,
                       "base_z": 0.0, "inset": 0.0, "normal_offset": 0.0}
    p5_straight_verts = edi_craft._prism_world(p5_collinear_op)[0]
    n_fp3_loops = n_fp3  # 4 verts per loop
    # The 3-loop collinear sweep has 12 verts (3 × 4).
    assert len(p5_straight_verts) == 3 * n_fp3_loops, \
        f"straight 3-point sweep should have 12 verts (got {len(p5_straight_verts)})"
    # Check: y and z of corresponding verts are identical across all 3 loops.
    # (Only x changes = the path moves along +x; no miter deviation in y/z.)
    for i in range(n_fp3_loops):
        v0 = p5_straight_verts[i]          # loop k=0
        v1 = p5_straight_verts[n_fp3_loops + i]     # loop k=1
        v2 = p5_straight_verts[2 * n_fp3_loops + i]  # loop k=2
        assert abs(v0[1] - v1[1]) < 1e-12 and abs(v0[1] - v2[1]) < 1e-12, (
            f"straight path: y of vert {i} should not change between loops "
            f"(miter_scale=1 for collinear): {v0[1]}, {v1[1]}, {v2[1]}")
        assert abs(v0[2] - v1[2]) < 1e-12 and abs(v0[2] - v2[2]) < 1e-12, (
            f"straight path: z of vert {i} should not change between loops: "
            f"{v0[2]}, {v1[2]}, {v2[2]}")
    # Full vert-list equality: two calls produce the same result (no randomness).
    p5_straight_v2 = edi_craft._prism_world(dict(p5_collinear_op))[0]
    assert p5_straight_verts == p5_straight_v2, \
        "straight-path sweep: two calls must produce identical vert lists"

    # The doric writes every field explicitly and uses no sphere/ring/label,
    # so the defaults and the remaining plan lines need their own fixture.
    path = write_temp(PLAN_FIXTURE)
    try:
        fixture_ops = edi_craft.parse_ops(path)
    finally:
        os.unlink(path)
    fixture_plan = edi_craft.plan_lines(fixture_ops)
    assert fixture_plan == PLAN_EXPECTED, (
        "plan fixture drifted:\n got      %r\n expected %r" % (fixture_plan, PLAN_EXPECTED)
    )
    # Vertex defaults are bpy-side inputs no plan line prints — pin them on
    # the parsed dicts so the 24/96 constants stay asserted somewhere.
    assert fixture_ops[0]["vertices"] == 24, "AddSphere vertices default drifted"
    assert fixture_ops[1]["vertices"] == 96, "AddRing vertices default drifted"

    def refuses(toml_text: str, needle: str) -> None:
        path = write_temp(toml_text)
        try:
            try:
                edi_craft.parse_ops(path)
            except ValueError as exc:
                assert needle in str(exc), f"wrong refusal: {exc}"
                return
            raise AssertionError(f"accepted bad recipe (wanted {needle!r})")
        finally:
            os.unlink(path)

    refuses('op.0.type = "AddBox"\nop.0.name = "b"\n', "missing required key")
    refuses(
        'op.0.type = "AddBox"\nop.0.name = "b"\nop.0.width = "1"\n'
        'op.0.depth = "1"\nop.0.height = "1"\nop.0.z = "0"\n'
        'op.0.material = "plastic"\n',
        "unknown material",
    )
    refuses(
        'op.0.type = "AddProfileMoulding"\nop.0.name = "m"\nop.0.base_z = "0"\n',
        "must be compiled",
    )
    # The lathe reference (R1-B04): only resolved streams reach the
    # craftsmen — parity with the C++ compile/preview refusals.
    refuses(
        'op.0.type = "AddRevolvedProfile"\nop.0.name = "m"\n',
        "AddRevolvedProfile must be resolved before building",
    )
    # The extrude reference (BL-01/04): only the lowered AddPrism reaches a
    # build — a raw extrude is refused by name, parity with C++ compile/resolve.
    refuses(
        'op.0.type = "AddExtrudedProfile"\nop.0.name = "m"\n',
        "AddExtrudedProfile must be resolved before building",
    )
    # The Follow-Me sweep reference (BL-08/09 fold-in): a raw AddSweepProfile is
    # refused by name too, parity with the C++ compile/resolve refusals.
    refuses(
        'op.0.type = "AddSweepProfile"\nop.0.name = "m"\n',
        "AddSweepProfile must be resolved before building",
    )
    refuses('op.0.type = "AddDodecahedron"\n', "unknown op type")

    # The consumption audit, by name: a typo'd field, a gapped op index,
    # and a stray table must each be rejected exactly like the C++ store.
    refuses(
        MINIMAL_CYLINDER + 'op.0.entasis_ration = "0.05"\n',
        "op.0.entasis_ration: unknown recipe key",
    )
    refuses(
        MINIMAL_CYLINDER + 'op.2.type = "AddBox"\n',
        "op.2: gapped or unknown op index",
    )
    refuses('ops.0.type = "AddBox"\n', "ops: unknown recipe key")

    # The edi TOML dialect: every value is a quoted string. Native numbers
    # and booleans parse in tomllib but must be refused here, or a file the
    # dry run accepts would bounce off the C++ store on re-import.
    refuses(
        'op.0.type = "AddBox"\nop.0.name = "b"\nop.0.width = 1.5\n',
        "expected quoted string value",
    )
    refuses(MINIMAL_CYLINDER + "op.0.entasis = true\n", "expected quoted string value")
    refuses(MINIMAL_CYLINDER + 'op.0.entasis = "yes"\n', "expected true or false")
    refuses(MINIMAL_CYLINDER + 'op.0.axis = "w"\n', "must be one of")

    # Flutes are vertical grooves: a non-z-axis target is geometric nonsense
    # the reader refuses (mirrors the C++ flute_target_not_vertical error).
    refuses(
        MINIMAL_CYLINDER + 'op.0.axis = "x"\n'
        'op.1.type = "CutFlutes"\nop.1.target = "c"\nop.1.count = "6"\nop.1.depth = "0.1"\n',
        "cuts vertical grooves",
    )

    # Explicit cutter geometry (R1-B04b): both or neither, and never beside a
    # width_ratio — the C++ store reader's refusals and wordings, mirrored.
    refuses(
        'op.0.type = "CutFlutes"\nop.0.target = "drum"\nop.0.count = "20"\n'
        'op.0.depth = "0.12"\nop.0.cutter_radius = "0.16"\n',
        "a cutter needs both .cutter_radius and .at_radius",
    )
    refuses(
        'op.0.type = "CutFlutes"\nop.0.target = "drum"\nop.0.count = "20"\n'
        'op.0.depth = "0.12"\nop.0.cutter_radius = "0.16"\nop.0.at_radius = "1.056"\n'
        'op.0.width_ratio = "0.34"\n',
        "has both an explicit cutter (.cutter_radius/.at_radius) and a .width_ratio",
    )

    # R1b: the --asset-out routing. The bake itself needs Blender (it imports
    # bpy via build() and _bake_asset()), so ctest can only test that main()
    # RUNS the build and THEN calls _bake_asset with the requested path — not the
    # actual .blend write. Stub both bpy-bound functions: build → no-op, and
    # _bake_asset → record the path it was handed.
    real_build = edi_craft.build
    real_bake = edi_craft._bake_asset
    built = []
    baked = []
    edi_craft.build = lambda ops: built.append(ops)
    edi_craft._bake_asset = lambda path: baked.append(path)
    try:
        rc = edi_craft.main(
            ["edi_craft.py", "--asset-out=/tmp/x.blend",
             os.path.join(SAMPLES, "doric_column_ops_compiled.toml")])
    finally:
        edi_craft.build = real_build
        edi_craft._bake_asset = real_bake
    assert rc == 0, f"--asset-out run should return 0, got {rc}"
    assert len(built) == 1, "--asset-out must run the build (build stub did not fire)"
    assert baked == ["/tmp/x.blend"], f"_bake_asset must be called with the asset path, got {baked}"

    # The usage string advertises --asset-out (the lab/CLI reads it). A no-path
    # invocation returns 2 and prints usage to stderr.
    import io
    import contextlib
    err = io.StringIO()
    with contextlib.redirect_stderr(err):
        usage_rc = edi_craft.main(["edi_craft.py"])
    assert usage_rc == 2, f"no-path invocation should return 2, got {usage_rc}"
    assert "--asset-out" in err.getvalue(), "usage string must advertise --asset-out"

    print("edi_craft smoke: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
