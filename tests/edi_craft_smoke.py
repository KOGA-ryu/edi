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

    # Custom craftsmen (the foundation): the scanner finds the sample script, the
    # registry exposes its manifest as TOML (what the C++ lab reads), and a
    # Script op renders in the proof through the craftsman's proof_mesh.
    registry = edi_craft.load_craftsmen(edi_craft.default_craftsmen_dir())
    assert "twisted_column" in registry, "twisted_column craftsman not discovered"
    manifest_toml = edi_craft.craftsmen_manifest_toml(edi_craft.default_craftsmen_dir())
    assert 'craftsman.0.id = "twisted_column"' in manifest_toml
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
