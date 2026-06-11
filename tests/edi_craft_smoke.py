#!/usr/bin/env python3
"""ctest smoke for the craftsmen library's Blender-free half.

Asserts the dry-run plan for the compiled doric column matches the
committed golden byte for byte, and that the strict reader refuses the
classic offenders. The bpy half is exercised in Blender (and reviewed
against bpy semantics); everything testable without Blender is tested
here so a refactor cannot silently bend the plan.
"""

import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))

import edi_craft  # noqa: E402

SAMPLES = os.path.join(ROOT, "samples", "doric_column")


def main() -> int:
    ops = edi_craft.parse_ops(os.path.join(SAMPLES, "doric_column_ops_compiled.toml"))
    plan = "\n".join(edi_craft.plan_lines(ops)) + "\n"
    with open(os.path.join(SAMPLES, "doric_dry_run.txt"), encoding="utf-8") as f:
        golden = f.read()
    assert plan == golden, "dry-run plan drifted from samples/doric_column/doric_dry_run.txt"

    def refuses(toml_text: str, needle: str) -> None:
        path = tempfile.mktemp(suffix=".toml")
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(toml_text)
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
    refuses('op.0.type = "AddDodecahedron"\n', "unknown op type")

    print("edi_craft smoke: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
