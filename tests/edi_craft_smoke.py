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

    # Custom craftsmen (the foundation): the scanner finds the sample script, the
    # registry exposes its manifest as TOML (what the C++ lab reads), and a
    # Script op renders in the proof through the craftsman's proof_mesh.
    registry = edi_craft.load_craftsmen(edi_craft.default_craftsmen_dir())
    assert "twisted_column" in registry, "twisted_column craftsman not discovered"
    assert "radial_petal" in registry, "radial_petal craftsman not discovered"
    assert "nfold_star" in registry, "nfold_star craftsman not discovered"
    manifest_toml = edi_craft.craftsmen_manifest_toml(edi_craft.default_craftsmen_dir())
    # Craftsmen are emitted sorted by id: nfold_star (n) < radial_petal (r) <
    # twisted_column (t) — so 0/1/2 respectively.
    assert 'craftsman.0.id = "nfold_star"' in manifest_toml
    assert 'craftsman.1.id = "radial_petal"' in manifest_toml
    assert 'craftsman.2.id = "twisted_column"' in manifest_toml
    assert 'param.2.key = "sides"' in manifest_toml, "craftsman param schema not emitted"
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

    print("edi_craft smoke: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
