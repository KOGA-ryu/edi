#!/usr/bin/env python3
"""edi_craft — the craftsmen library.

The scripts are the craftsmen; the recipe documents are the dimension
sheets. This library holds the durable build techniques (ported from the
working prototype, ascii_blender_dryrun_v0/blender_backend.py); a COMPILED
edi op-stream TOML tells them how much to cut, bevel, and chamfer.

    Recipe is truth. ASCII preview is proof. Blender script is execution.

Usage:
    blender --python edi_craft.py -- /path/to/doric_column_ops_compiled.toml
    python3 edi_craft.py --dry-run /path/to/doric_column_ops_compiled.toml

The dry run needs no Blender: it prints the deterministic build plan, one
line per op, so the plan can be diffed before any mesh exists (and is
golden-tested against samples/doric_column/doric_dry_run.txt).

Port fidelity and divergences (each deliberate):
- Mesh-building math is v0's exactly: tapered cylinder rings, the entasis
  bulge sin(pi*t) * radius * ratio (ratio now comes from the op; v0
  hardcoded 0.045), moulding ring lofts with capped ends, flute cutters at
  centre distance radius + cutter_radius - depth with the EXACT solver.
- Materials are a strict table. v0 silently repainted unknown materials to
  stone; here an unknown material is a hard error naming the op.
- AddLabel builds a real FONT text object at (x, y, z); v0's backend only
  emitted a `# LABEL` comment into the generated script. Labels join the
  rig bounds as points so annotations stay inside the framed view.
- v0's preview rig was hardcoded (and underlit/misframed — its packaged
  perspective preview rendered near-black). The rig is now COMPUTED from
  the op-stream bounds: frame whatever the recipe builds, light it to be
  seen, ortho like the prototype's good render.
- Flute boolean failures: v0 printed and carried on, so a broken column
  exited 0. Failures are collected and raised at the end — a build that
  lost its flutes must say so loudly.

Requires python >= 3.11 (tomllib); Blender >= 4.1 ships it.
"""

from __future__ import annotations

import importlib.util
import math
import os
import sys

try:
    import tomllib
except ModuleNotFoundError as exc:  # pragma: no cover
    raise SystemExit("edi_craft needs python >= 3.11 (tomllib); Blender >= 4.1 ships it.") from exc


# ---------------------------------------------------------------------------
# The material vocabulary — one table, strictly checked (mirrors the C++
# validator's table; keep the two in step).

MATERIALS = {
    "stone": ("aged_warm_limestone", (0.72, 0.68, 0.60, 1.0)),
    "limestone": ("light_limestone", (0.80, 0.78, 0.70, 1.0)),
    "marble": ("polished_marble", (0.88, 0.88, 0.90, 1.0)),
    "sandstone": ("warm_sandstone", (0.78, 0.64, 0.46, 1.0)),
    "aged_stone": ("weathered_stone", (0.55, 0.52, 0.48, 1.0)),
    "iron": ("wrought_iron", (0.16, 0.16, 0.18, 1.0)),
    "glass": ("pale_glass", (0.75, 0.85, 0.90, 1.0)),
}

ARCHITECTURAL_OPS = (
    "AddBox", "AddCylinder", "AddSphere", "AddRing", "AddMoulding", "AddPrism",
    "CutFlutes", "AddBoolean", "AddLabel",
)


# ---------------------------------------------------------------------------
# Custom craftsmen: user Python scripts in a scanned folder, each a self-
# describing step type. A script exposes MANIFEST (id, label, params),
# proof_mesh(op) -> (verts, faces) for the proof, and build(op) for bpy. The
# C++ lab reads the registry (`--list-craftsmen`) to fill the palette + render
# the inspector; the proof and the build dispatch a "Script" op to its module.

def default_craftsmen_dir() -> str:
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), "craftsmen")


def load_craftsmen(folder: str) -> dict:
    """id -> module for every .py in `folder` exposing a MANIFEST with an id."""
    registry: dict = {}
    if not folder or not os.path.isdir(folder):
        return registry
    for name in sorted(os.listdir(folder)):
        if not name.endswith(".py") or name.startswith("_"):
            continue
        path = os.path.join(folder, name)
        spec = importlib.util.spec_from_file_location(f"edi_craftsman_{name[:-3]}", path)
        if spec is None or spec.loader is None:
            continue
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        manifest = getattr(module, "MANIFEST", None)
        if isinstance(manifest, dict) and manifest.get("id"):
            registry[manifest["id"]] = module
    return registry


def craftsmen_manifest_toml(folder: str) -> str:
    """The registry as flat TOML — what the C++ lab consumes to offer custom
    steps in the palette and render their params in the inspector."""
    registry = load_craftsmen(folder)
    lines: list = []
    for i, (cid, module) in enumerate(sorted(registry.items())):
        manifest = module.MANIFEST
        lines.append(f'craftsman.{i}.id = "{cid}"')
        lines.append(f'craftsman.{i}.label = "{manifest.get("label", cid)}"')
        for j, param in enumerate(manifest.get("params", [])):
            lines.append(f'craftsman.{i}.param.{j}.key = "{param["key"]}"')
            lines.append(f'craftsman.{i}.param.{j}.label = "{param.get("label", param["key"])}"')
            lines.append(f'craftsman.{i}.param.{j}.type = "{param.get("type", "number")}"')
            lines.append(f'craftsman.{i}.param.{j}.default = "{param.get("default", "")}"')
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Strict op-stream parsing. The C++ store already audited the file once;
# this reader re-checks because a TOML edited by hand between export and
# build deserves the same discipline (and the dry run may be the first
# reader the file ever meets).

def _raw(op_key: str, name: str, value) -> str:
    # The edi TOML dialect (TomlReader.cpp): every value is a QUOTED STRING.
    # Enforce it here too, so the two strict readers accept and reject the
    # same files — a bare `width = 1.5` that tomllib happily parses would be
    # rejected on re-import into edi.
    if not isinstance(value, str):
        raise ValueError(f"{op_key}.{name}: expected quoted string value, got {value!r}")
    return value


def _number(op_key: str, fields: dict, consumed: set, name: str, default=None) -> float:
    if name not in fields:
        if default is None:
            raise ValueError(f"{op_key}.{name}: missing required key")
        return float(default)
    consumed.add(name)
    value = float(_raw(op_key, name, fields[name]))
    if not math.isfinite(value):
        raise ValueError(f"{op_key}.{name}: not a finite number")
    return value


def _optional_number(op_key: str, fields: dict, consumed: set, name: str):
    if name not in fields:
        return None
    # Same finiteness gate as required numbers — taper/start_z/end_z must
    # not smuggle nan past the reader.
    return _number(op_key, fields, consumed, name, None)


def _integer(op_key: str, fields: dict, consumed: set, name: str, default=None) -> int:
    if name not in fields:
        if default is None:
            raise ValueError(f"{op_key}.{name}: missing required key")
        return int(default)
    consumed.add(name)
    return int(_raw(op_key, name, fields[name]))


def _text(op_key: str, fields: dict, consumed: set, name: str, default=None) -> str:
    if name not in fields:
        if default is None:
            raise ValueError(f"{op_key}.{name}: missing required key")
        return str(default)
    consumed.add(name)
    return _raw(op_key, name, fields[name])


def _choice(op_key: str, fields: dict, consumed: set, name: str, default: str, allowed: tuple) -> str:
    value = _text(op_key, fields, consumed, name, default)
    if value not in allowed:
        raise ValueError(f"{op_key}.{name}: must be one of {', '.join(allowed)}, got {value!r}")
    return value


def _flag(op_key: str, fields: dict, consumed: set, name: str, default: bool) -> bool:
    raw = fields.get(name)
    if raw is None:
        return default
    consumed.add(name)
    raw = _raw(op_key, name, raw)
    if raw == "true":
        return True
    if raw == "false":
        return False
    raise ValueError(f"{op_key}.{name}: expected true or false, got {raw!r}")


def _material(op_key: str, fields: dict, consumed: set) -> str:
    material = _text(op_key, fields, consumed, "material", "stone")
    if material not in MATERIALS:
        raise ValueError(f"{op_key}.material: unknown material {material!r}")
    return material


def _point_run(op_key: str, fields: dict, run: str) -> list:
    """Read a contiguous <run>.i.{x,y} point run until the first missing index,
    by-name auditing each sub-key and refusing gaps — the same shape AddMoulding's
    profile.i run uses. Shared by AddPrism's `footprint` and its optional BL-08
    `path` (an absent run reads as an empty list). The caller marks `run` consumed."""
    out = []
    points = fields.get(run, {})
    index = 0
    while str(index) in points:
        point = points[str(index)]
        point_key = f"{op_key}.{run}.{index}"
        point_consumed: set = set()
        out.append({
            "x": _number(point_key, point, point_consumed, "x"),
            "y": _number(point_key, point, point_consumed, "y"),
        })
        extra = sorted(set(point) - point_consumed)
        if extra:
            raise ValueError(f"{point_key}.{extra[0]}: unknown recipe key")
        index += 1
    leftover = sorted(set(points) - {str(i) for i in range(index)})
    if leftover:
        raise ValueError(f"{op_key}.{run}.{leftover[0]}: gapped or unknown {run} index")
    return out


def parse_ops(path: str) -> list[dict]:
    """Parse a compiled op-stream TOML into typed op dicts, strictly.

    Mirrors the C++ store's tracking-by-consumption audit: every key the
    reader uses is recorded, and anything left over — a typo'd field, a
    gapped op index, a stray table — is rejected BY NAME. This is the last
    reader before geometry exists; silence here would be guesswork.
    """
    with open(path, "rb") as f:
        data = tomllib.load(f)

    for top_key in data:
        if top_key not in ("op", "recipe"):
            raise ValueError(f"{top_key}: unknown recipe key")
    for recipe_key in data.get("recipe", {}):
        if recipe_key not in ("id", "name"):
            raise ValueError(f"recipe.{recipe_key}: unknown recipe key")

    table = data.get("op", {})
    ops: list[dict] = []
    cylinder_axes: dict[str, str] = {}
    index = 0
    while str(index) in table:
        fields = table[str(index)]
        op_key = f"op.{index}"
        consumed: set = set()
        op_type = _text(op_key, fields, consumed, "type")
        if op_type == "AddProfileMoulding":
            name = fields.get("name", "?")
            raise ValueError(f"{op_key}: AddProfileMoulding must be compiled before building: {name}")
        if op_type == "AddRevolvedProfile":
            # The lathe reference (R1-B04): only RESOLVED streams reach the
            # craftsmen — a profile reference here means the resolve pass
            # never ran against the drawing. Same named-refusal shape as the
            # uncompiled moulding above; the C++ passes refuse it too.
            name = fields.get("name", "?")
            raise ValueError(f"{op_key}: AddRevolvedProfile must be resolved before building: {name}")
        if op_type == "AddExtrudedProfile":
            # The extrude reference (BL-01): like the lathe, only the LOWERED
            # form (AddPrism) reaches a build — a raw extrude here means resolve
            # never ran. Refused by name on BOTH sides (C++ compile/resolve and
            # here), so the seam cannot leak an unbuildable reference.
            name = fields.get("name", "?")
            raise ValueError(f"{op_key}: AddExtrudedProfile must be resolved before building: {name}")
        if op_type == "AddSweepProfile":
            # The Follow-Me sweep reference (BL-08): like the extrude, only the
            # LOWERED form (a path-bearing AddPrism) reaches a build. Refused by
            # name on BOTH sides (BL-09 fold-in — the C++ compile/resolve already
            # refuse it), so the seam cannot leak an unbuildable reference.
            name = fields.get("name", "?")
            raise ValueError(f"{op_key}: AddSweepProfile must be resolved before building: {name}")
        if op_type == "Script":
            # A custom-craftsman step: a script id, a position, and a free param
            # bag. parse_ops does not know the craftsman's schema (it lives in
            # the script's MANIFEST), so it reads the params generically and the
            # craftsman coerces them — no unknown-key audit for these fields.
            script_id = _text(op_key, fields, consumed, "script")
            name = _text(op_key, fields, consumed, "name", script_id)
            x = _number(op_key, fields, consumed, "x", 0.0)
            y = _number(op_key, fields, consumed, "y", 0.0)
            z = _number(op_key, fields, consumed, "z", 0.0)
            params = {key: value for key, value in fields.items() if key not in consumed}
            ops.append({"type": "Script", "script": script_id, "name": name,
                        "x": x, "y": y, "z": z, "params": params})
            index += 1
            continue
        if op_type not in ARCHITECTURAL_OPS:
            raise ValueError(f"{op_key}.type: unknown op type {op_type!r}")

        op: dict = {"type": op_type}
        if op_type == "AddBox":
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                width=_number(op_key, fields, consumed, "width"),
                depth=_number(op_key, fields, consumed, "depth"),
                height=_number(op_key, fields, consumed, "height"),
                z=_number(op_key, fields, consumed, "z"),
                x=_number(op_key, fields, consumed, "x", 0.0),
                y=_number(op_key, fields, consumed, "y", 0.0),
                material=_material(op_key, fields, consumed),
                z_mode=_choice(op_key, fields, consumed, "z_mode", "center", ("center", "base")),
            )
        elif op_type == "AddCylinder":
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                radius=_number(op_key, fields, consumed, "radius"),
                height=_number(op_key, fields, consumed, "height"),
                z=_number(op_key, fields, consumed, "z"),
                x=_number(op_key, fields, consumed, "x", 0.0),
                y=_number(op_key, fields, consumed, "y", 0.0),
                vertices=_integer(op_key, fields, consumed, "vertices", 96),
                material=_material(op_key, fields, consumed),
                taper_top_radius=_optional_number(op_key, fields, consumed, "taper_top_radius"),
                entasis=_flag(op_key, fields, consumed, "entasis", False),
                entasis_ratio=_number(op_key, fields, consumed, "entasis_ratio", 0.045),
                axis=_choice(op_key, fields, consumed, "axis", "z", ("x", "y", "z")),
                z_mode=_choice(op_key, fields, consumed, "z_mode", "center", ("center", "base")),
            )
            cylinder_axes[op["name"]] = op["axis"]
        elif op_type == "AddSphere":
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                radius=_number(op_key, fields, consumed, "radius"),
                z=_number(op_key, fields, consumed, "z"),
                x=_number(op_key, fields, consumed, "x", 0.0),
                y=_number(op_key, fields, consumed, "y", 0.0),
                vertices=_integer(op_key, fields, consumed, "vertices", 24),
                material=_material(op_key, fields, consumed),
            )
        elif op_type == "AddRing":
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                radius=_number(op_key, fields, consumed, "radius"),
                tube_height=_number(op_key, fields, consumed, "tube_height"),
                z=_number(op_key, fields, consumed, "z"),
                overhang=_number(op_key, fields, consumed, "overhang", 0.0),
                x=_number(op_key, fields, consumed, "x", 0.0),
                y=_number(op_key, fields, consumed, "y", 0.0),
                vertices=_integer(op_key, fields, consumed, "vertices", 96),
                material=_material(op_key, fields, consumed),
            )
        elif op_type == "AddMoulding":
            profile = []
            points = fields.get("profile", {})
            consumed.add("profile")
            point_index = 0
            while str(point_index) in points:
                point = points[str(point_index)]
                point_key = f"{op_key}.profile.{point_index}"
                point_consumed: set = set()
                profile.append({
                    "term": _text(point_key, point, point_consumed, "term"),
                    "z": _number(point_key, point, point_consumed, "z"),
                    "radius": _number(point_key, point, point_consumed, "radius"),
                })
                extra_point = sorted(set(point) - point_consumed)
                if extra_point:
                    raise ValueError(f"{point_key}.{extra_point[0]}: unknown recipe key")
                point_index += 1
            leftover_points = sorted(set(points) - {str(i) for i in range(point_index)})
            if leftover_points:
                raise ValueError(f"{op_key}.profile.{leftover_points[0]}: gapped or unknown profile index")
            if len(profile) < 2:
                raise ValueError(f"{op_key}: moulding needs at least two profile points")
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                base_z=_number(op_key, fields, consumed, "base_z"),
                profile=profile,
                x=_number(op_key, fields, consumed, "x", 0.0),
                y=_number(op_key, fields, consumed, "y", 0.0),
                vertices=_integer(op_key, fields, consumed, "vertices", 96),
                material=_material(op_key, fields, consumed),
                # BL-06: partial-angle revolve, 360 = full (today's default).
                # AddRevolvedProfile is refused-before-build on this side, so only
                # the lowered AddMoulding carries the swept arc into the mesh.
                sweep_degrees=_number(op_key, fields, consumed, "sweep_degrees", 360.0),
                # BL-07: screw/helix — rise per full turn (0 = no helix) and turn
                # count (default 1). Same reasoning: only the lowered moulding reads.
                screw_rise=_number(op_key, fields, consumed, "screw_rise", 0.0),
                screw_turns=_number(op_key, fields, consumed, "screw_turns", 1.0),
            )
        elif op_type == "AddPrism":
            # The lowered prism carrier (BL-03/04): a closed planar footprint
            # read as a contiguous footprint.i.{x,y} run, key-for-key with the C++
            # OpWriter. BL-08 adds an OPTIONAL path.i.{x,y} run (the sweep curve);
            # an absent path reads as empty = the straight height-extrude.
            footprint = _point_run(op_key, fields, "footprint")
            consumed.add("footprint")
            path = _point_run(op_key, fields, "path")
            consumed.add("path")
            if len(footprint) < 3:
                raise ValueError(f"{op_key}: prism needs at least three footprint points")
            if path and len(path) < 2:
                raise ValueError(f"{op_key}: a sweep path needs at least two points")
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                height=_number(op_key, fields, consumed, "height"),
                base_z=_number(op_key, fields, consumed, "base_z"),
                footprint=footprint,
                path=path,
                x=_number(op_key, fields, consumed, "x", 0.0),
                y=_number(op_key, fields, consumed, "y", 0.0),
                material=_material(op_key, fields, consumed),
                # BL-09: taper scale at the path end (1.0 = none). Straight
                # extrude (empty path) ignores it.
                taper_end=_number(op_key, fields, consumed, "taper_end", 1.0),
                # P4: non-linear taper exponent. t^taper_curve remaps the
                # linear fraction before the lerp. 1.0 = linear (identity).
                taper_curve=_number(op_key, fields, consumed, "taper_curve", 1.0),
                # P4b: per-axis Y end scale. 0.0 = follow taper_end (uniform,
                # default). When > 0 the Y axis narrows to this scale independently.
                taper_end_y=_number(op_key, fields, consumed, "taper_end_y", 0.0),
                # BL-10: inset shrinks the footprint inward; normal_offset
                # inflates the shell along its normals. Both 0 = no change.
                inset=_number(op_key, fields, consumed, "inset", 0.0),
                normal_offset=_number(op_key, fields, consumed, "normal_offset", 0.0),
            )
        elif op_type == "CutFlutes":
            # Explicit cutter geometry (R1-B04b): optional, present-together,
            # XOR width_ratio. Parity with the C++ store reader — same wordings,
            # same op.N: prefix (RecipeOpsStore.cpp).
            cutter_radius = _optional_number(op_key, fields, consumed, "cutter_radius")
            at_radius = _optional_number(op_key, fields, consumed, "at_radius")
            if (cutter_radius is None) != (at_radius is None):
                raise ValueError(
                    f"{op_key}: a cutter needs both .cutter_radius and .at_radius")
            if cutter_radius is not None and "width_ratio" in fields:
                raise ValueError(
                    f"{op_key}: has both an explicit cutter (.cutter_radius/.at_radius) and a .width_ratio")
            op.update(
                target=_text(op_key, fields, consumed, "target"),
                count=_integer(op_key, fields, consumed, "count"),
                depth=_number(op_key, fields, consumed, "depth"),
                width_ratio=_number(op_key, fields, consumed, "width_ratio", 0.28),
                cutter_radius=cutter_radius,
                at_radius=at_radius,
                start_z=_optional_number(op_key, fields, consumed, "start_z"),
                end_z=_optional_number(op_key, fields, consumed, "end_z"),
            )
            target_axis = cylinder_axes.get(op["target"], "z")
            if target_axis != "z":
                # Flutes are VERTICAL grooves; the cutter ring stands along Z.
                raise ValueError(
                    f"{op_key}: CutFlutes cuts vertical grooves; target "
                    f"{op['target']!r} is a {target_axis}-axis cylinder")
        elif op_type == "AddBoolean":
            # BL-11: the general composer. a/b name earlier ops; kind is one of
            # union/subtract/intersect (key-for-key with the C++ store reader).
            kind = _text(op_key, fields, consumed, "kind", "union")
            if kind not in ("union", "subtract", "intersect"):
                raise ValueError(
                    f"{op_key}.kind: kind must be union, subtract, or intersect, got {kind!r}")
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                a=_text(op_key, fields, consumed, "a"),
                b=_text(op_key, fields, consumed, "b"),
                kind=kind,
            )
        elif op_type == "AddLabel":
            op.update(
                name=_text(op_key, fields, consumed, "name"),
                text=_text(op_key, fields, consumed, "text"),
                x=_number(op_key, fields, consumed, "x"),
                y=_number(op_key, fields, consumed, "y"),
                z=_number(op_key, fields, consumed, "z"),
            )
        extra = sorted(set(fields) - consumed)
        if extra:
            raise ValueError(f"{op_key}.{extra[0]}: unknown recipe key")
        ops.append(op)
        index += 1

    leftover = sorted(set(table) - {str(i) for i in range(index)})
    if leftover:
        raise ValueError(f"op.{leftover[0]}: gapped or unknown op index")
    return ops


# ---------------------------------------------------------------------------
# Shared geometry helpers (no bpy — usable by the dry run).

def _center_z(op: dict) -> float:
    if op.get("z_mode") == "base":
        return op["z"] + op["height"] / 2.0
    return op["z"]


def bounds_of(ops: list[dict]) -> tuple[float, float, float, float, float, float]:
    """Axis-aligned bounds of the whole build — the preview rig's input."""
    inf = float("inf")
    min_x = min_y = min_z = inf
    max_x = max_y = max_z = -inf

    def include(x0, x1, y0, y1, z0, z1):
        nonlocal min_x, max_x, min_y, max_y, min_z, max_z
        min_x = min(min_x, x0)
        max_x = max(max_x, x1)
        min_y = min(min_y, y0)
        max_y = max(max_y, y1)
        min_z = min(min_z, z0)
        max_z = max(max_z, z1)

    for op in ops:
        if op["type"] == "AddBox":
            z = _center_z(op)
            include(op["x"] - op["width"] / 2, op["x"] + op["width"] / 2,
                    op["y"] - op["depth"] / 2, op["y"] + op["depth"] / 2,
                    z - op["height"] / 2, z + op["height"] / 2)
        elif op["type"] == "AddCylinder":
            z = _center_z(op) if op["axis"] == "z" else op["z"]
            r = op["radius"]
            h = op["height"]
            if op["axis"] == "x":
                include(op["x"] - h / 2, op["x"] + h / 2, op["y"] - r, op["y"] + r, z - r, z + r)
            elif op["axis"] == "y":
                include(op["x"] - r, op["x"] + r, op["y"] - h / 2, op["y"] + h / 2, z - r, z + r)
            else:
                include(op["x"] - r, op["x"] + r, op["y"] - r, op["y"] + r, z - h / 2, z + h / 2)
        elif op["type"] == "AddSphere":
            r = op["radius"]
            include(op["x"] - r, op["x"] + r, op["y"] - r, op["y"] + r, op["z"] - r, op["z"] + r)
        elif op["type"] == "AddRing":
            r = op["radius"] + op["overhang"]
            include(op["x"] - r, op["x"] + r, op["y"] - r, op["y"] + r,
                    op["z"] - op["tube_height"] / 2, op["z"] + op["tube_height"] / 2)
        elif op["type"] == "AddMoulding":
            if op.get("screw_rise", 0.0) != 0.0:
                # P7: a HELIX moulding (screw_rise != 0) rises by
                # screw_rise * screw_turns above the profile's own z range.
                # The formula-only path misses this lift entirely, so a tall
                # helix extends far past the preview box.  Use the actual mesh
                # verts instead — the same approach the swept-prism arm already
                # uses.  Framing from `_moulding_world` verts is exact by
                # construction and can never drift from the rendered mesh.
                mverts, _ = _moulding_world(op)
                mxs = [v[0] for v in mverts]
                mys = [v[1] for v in mverts]
                mzs = [v[2] for v in mverts]
                include(min(mxs), max(mxs), min(mys), max(mys), min(mzs), max(mzs))
            else:
                # Straight moulding (no helix): the formula is exact and cheap.
                # Left UNCHANGED from the original so the doric dry-run framing
                # is byte-identical (and the test suite doesn't need updating).
                r = max(point["radius"] for point in op["profile"])
                z0 = op["base_z"] + min(point["z"] for point in op["profile"])
                z1 = op["base_z"] + max(point["z"] for point in op["profile"])
                include(op["x"] - r, op["x"] + r, op["y"] - r, op["y"] + r, z0, z1)
        elif op["type"] == "AddPrism":
            if op.get("path"):
                # A swept solid: frame its actual verts (the path bends it out of
                # a simple footprint box), computed from the same mesh builder.
                verts, _ = _prism_world(op)
                xs = [v[0] for v in verts]
                ys = [v[1] for v in verts]
                zs = [v[2] for v in verts]
                include(min(xs), max(xs), min(ys), max(ys), min(zs), max(zs))
            else:
                xs = [point["x"] for point in op["footprint"]]
                ys = [point["y"] for point in op["footprint"]]
                # base_z .. base_z+height, ordered min/max so a NEGATIVE height
                # (the prism extruded downward — push/pull, BL-05) frames correctly.
                z0 = min(op["base_z"], op["base_z"] + op["height"])
                z1 = max(op["base_z"], op["base_z"] + op["height"])
                include(op["x"] + min(xs), op["x"] + max(xs),
                        op["y"] + min(ys), op["y"] + max(ys), z0, z1)
        elif op["type"] == "AddLabel":
            # A point, not a box: the op carries no text extent, but an
            # annotation must stay inside the framed view.
            include(op["x"], op["x"], op["y"], op["y"], op["z"], op["z"])
    if min_x == inf:
        return (-1, 1, -1, 1, 0, 1)
    return (min_x, max_x, min_y, max_y, min_z, max_z)


def _fmt(value: float) -> str:
    return format(value, "g")


# ---------------------------------------------------------------------------
# Mesh proof (no bpy): a deterministic OBJ of the resolved+compiled stream, so
# organic work has a proof tier beyond the ASCII silhouettes. Primitives are
# replicated ANALYTICALLY — the proof mesh need not match bpy's tessellation
# bit-for-bit; determinism and honest dimensions are the contract. The
# taper/entasis shaft and the moulding lathe SHARE their loft math with build()
# (the two functions below), so the proof and the bpy build cannot drift.

def cylinder_loft(op: dict) -> tuple[list, list]:
    """Local (verts, faces) for a tapered/entasis cylinder — v0's ring loft,
    shared by build()'s shaft branch and the OBJ proof."""
    rings = 24
    vertices = max(4, op["vertices"])
    radius = op["radius"]
    top_radius = op["taper_top_radius"] if op["taper_top_radius"] is not None else radius
    height = op["height"]
    verts: list = []
    faces: list = []
    for ring in range(rings + 1):
        t = ring / rings
        z_local = -height * 0.5 + height * t
        linear_radius = radius + (top_radius - radius) * t
        bulge = math.sin(math.pi * t) * radius * op["entasis_ratio"] if op["entasis"] else 0.0
        rr = max(0.001, linear_radius + bulge)
        for index in range(vertices):
            angle = math.tau * index / vertices
            verts.append((math.cos(angle) * rr, math.sin(angle) * rr, z_local))
    for ring in range(rings):
        current = ring * vertices
        nxt_ring = (ring + 1) * vertices
        for index in range(vertices):
            nxt = (index + 1) % vertices
            faces.append([current + index, current + nxt, nxt_ring + nxt, nxt_ring + index])
    faces.append(list(range(vertices - 1, -1, -1)))
    top_start = rings * vertices
    faces.append(list(range(top_start, top_start + vertices)))
    return verts, faces


def moulding_rings(op: dict) -> tuple[list, list]:
    """Local (verts, faces) for a moulding — the profile spun as stacked rings,
    shared by build()'s add_moulding and the OBJ proof.

    BL-06: sweep_degrees controls how much arc the rings span. At the default
    360 this is the FULL revolve — byte-identical to before (vertices points at
    tau*i/vertices, the ring wraps via (i+1)%vertices, no radial seam). Below
    360 the rings span only that arc (vertices points over [0, sweep], no wrap)
    and the two RADIAL open ends — the angle-0 and angle-sweep cross-sections —
    are capped so a partial revolve is a closed solid, not an open shell.

    BL-07: screw_rise lifts the sweep into a helix (thread / volute). The lift
    is applied HERE, at sweep time, to each angular sample's z — it is NEVER
    written back into the profile points, so the moulding_profile_not_monotonic
    validator (which reads the profile cross-section) never sees it and a helix
    cannot falsely trip it. rise 0 (default) skips this entirely and runs the
    BL-06 path verbatim — the byte-preserving guarantee."""
    vertices = max(4, op["vertices"])
    sweep = op.get("sweep_degrees", 360.0)
    screw_rise = op.get("screw_rise", 0.0)
    screw_turns = op.get("screw_turns", 1.0)
    nrings = len(op["profile"])
    verts: list = []
    faces: list = []

    if screw_rise != 0.0:
        # Helix mode (v1 rule): a thread of screw_turns FULL turns. sweep_degrees
        # does NOT also clip a helix in v1 (a partial-AND-helical sweep is a
        # future combination) — FLAGGED. The profile cross-section is swept along
        # a helical path: each angular sample lifts the WHOLE section by
        # screw_rise per full turn, so the moulding's silhouette spirals upward.
        steps = max(4, int(round(vertices * screw_turns)))  # angular segments over the whole helix
        total = math.tau * screw_turns
        nprof = nrings

        # Arc verts: (steps+1) rings × nprof profile points each.
        # Vert layout: j * nprof + i  (j=angular step, i=profile point index).
        for j in range(steps + 1):
            angle = total * j / steps
            lift = screw_rise * (angle / math.tau)  # rise per FULL turn (tau)
            for point in op["profile"]:
                rr = max(0.001, point["radius"])
                verts.append((math.cos(angle) * rr, math.sin(angle) * rr, point["z"] + lift))

        # Axis spine: one center vertex per step j, sitting on the rotation axis
        # at the helix's progressive z-lift.  These verts give the inner and
        # outer caps a shared spine to fan against — the same role axis_base
        # plays in the partial-revolve branch.  A 2-manifold solid requires that
        # EVERY undirected edge bounds exactly 2 faces; without this spine the
        # inner and outer helical tracks (i=0 and i=nprof-1) are open boundaries
        # each shared by only 1 face.
        axis_base = (steps + 1) * nprof
        for j in range(steps + 1):
            angle = total * j / steps
            lift = screw_rise * (angle / math.tau)
            verts.append((0.0, 0.0, lift))

        # Swept skin: quads spanning profile (i) × helix-step (j).
        for j in range(steps):
            cur = j * nprof
            nxt = (j + 1) * nprof
            for i in range(nprof - 1):
                faces.append([cur + i, cur + i + 1, nxt + i + 1, nxt + i])

        # Inner surface: seal the inner helical track (i=0) by bridging it to
        # the axis spine with one quad per step.  Adjacent quads share their
        # "right" edge with the next quad's "left" edge, so all intermediate
        # axis-to-arc edges are counted twice; only the j=0 and j=steps
        # endpoint edges need the caps below to reach count 2.
        for j in range(steps):
            faces.append([j * nprof, (j + 1) * nprof,
                          axis_base + j + 1, axis_base + j])

        # Outer surface: same logic for the outer helical track (i=nprof-1).
        # Winding is REVERSED from the inner quad so the two quads traverse
        # the shared axis-spine edge (axis_base+j, axis_base+j+1) in OPPOSITE
        # directions — the inner quad goes j+1→j, the outer goes j→j+1.
        # A consistently oriented closed surface requires every directed edge
        # (a→b) to appear in exactly ONE face; its reverse (b→a) in exactly
        # one other.  With matching windings both faces carry the same directed
        # spine edge and neither has its reverse, breaking orientation.
        for j in range(steps):
            faces.append([axis_base + j, axis_base + j + 1,
                          (j + 1) * nprof + nprof - 1, j * nprof + nprof - 1])

        # Start cap (j=0): close the entry face of the helix.  Fan the start
        # profile cross-section (verts 0..nprof-1) and the start axis center
        # (axis_base) into one polygon — profile traversed outer→inner so the
        # face normal points away from the solid (left-hand rule, looking toward
        # angle=0 from outside).
        faces.append(list(range(nprof - 1, -1, -1)) + [axis_base])

        # End cap (j=steps): close the exit face.  Profile traversed inner→outer
        # (opposite winding from start cap so the normal again faces outward).
        faces.append(list(range(steps * nprof, (steps + 1) * nprof)) + [axis_base + steps])

        return verts, faces

    if sweep >= 360.0:
        # Full revolve — UNCHANGED from before (the doric golden's byte path).
        for point in op["profile"]:
            rr = max(0.001, point["radius"])
            for index in range(vertices):
                angle = math.tau * index / vertices
                verts.append((math.cos(angle) * rr, math.sin(angle) * rr, point["z"]))
        for ring in range(nrings - 1):
            current = ring * vertices
            nxt_ring = (ring + 1) * vertices
            for index in range(vertices):
                nxt = (index + 1) % vertices
                faces.append([current + index, current + nxt, nxt_ring + nxt, nxt_ring + index])
        faces.append(list(range(vertices - 1, -1, -1)))
        top_start = (nrings - 1) * vertices
        faces.append(list(range(top_start, top_start + vertices)))
        return verts, faces

    # Partial revolve: vertices points span [0, sweep] inclusive, so there are
    # vertices-1 segments per ring and NO wrap (the ring is an open arc). A
    # partial sweep is a "cake slice" solid, so it also needs the ROTATION AXIS
    # spine — one center vertex (radius 0) per ring — to close the end caps and
    # the radial faces. (A full revolve wraps shut and never needs the axis.)
    arc = math.radians(sweep)
    for point in op["profile"]:
        rr = max(0.001, point["radius"])
        for index in range(vertices):
            angle = arc * index / (vertices - 1)
            verts.append((math.cos(angle) * rr, math.sin(angle) * rr, point["z"]))
    axis_base = nrings * vertices            # the axis verts start after the arc verts
    for point in op["profile"]:
        verts.append((0.0, 0.0, point["z"])) # center of this ring, on the spin axis
    for ring in range(nrings - 1):
        current = ring * vertices
        nxt_ring = (ring + 1) * vertices
        for index in range(vertices - 1):    # no wrap — the arc is open
            faces.append([current + index, current + index + 1,
                          nxt_ring + index + 1, nxt_ring + index])
    # The bottom and top caps are SECTOR fans from the axis center to the arc.
    # Winding direction: the swept skin's bottom row traverses arc verts in the
    # POSITIVE-angle direction (index → index+1).  A consistent orientation
    # requires the bottom sector fan to traverse those same edges in the OPPOSITE
    # direction, so the fan is arc-first in REVERSE angle order, closing to the
    # axis — [vertices-1, ..., 0, axis_base].  Same logic flips the top fan to
    # arc-first in FORWARD angle order — [top, top+1, ..., axis_base+nrings-1].
    # (The "axis_base first" original order put the fan's traversal in the same
    # direction as the swept skin, so both faces carried the same directed edge.)
    faces.append(list(range(vertices - 1, -1, -1)) + [axis_base])             # bottom sector
    top = (nrings - 1) * vertices
    faces.append(list(range(top, top + vertices)) + [axis_base + (nrings - 1)])  # top sector
    # The two radial end caps — the profile cross-section at angle 0 and at
    # angle sweep: up the profile, back down the axis spine (a closed 2N loop).
    faces.append([ring * vertices for ring in range(nrings)]
                 + [axis_base + ring for ring in range(nrings - 1, -1, -1)])         # angle 0
    faces.append([ring * vertices + (vertices - 1) for ring in range(nrings - 1, -1, -1)]
                 + [axis_base + ring for ring in range(nrings)])                     # angle sweep
    return verts, faces


def _axis_apply(v: tuple, axis: str) -> tuple:
    # Mirror build()'s _apply_axis: a shape built along local Z, rotated +pi/2
    # onto the named axis (so the OBJ verts bake what bpy applies as a rotation).
    x, y, z = v
    if axis == "x":
        return (z, y, -x)   # rotate about Y
    if axis == "y":
        return (x, -z, y)   # rotate about X
    return (x, y, z)


def _ring(radius: float, z: float, count: int) -> list:
    return [(math.cos(math.tau * i / count) * radius, math.sin(math.tau * i / count) * radius, z)
            for i in range(count)]


def _plain_cylinder_local(radius: float, height: float, count: int) -> tuple[list, list]:
    # Two count-gon rings (bottom, top) + side quads + two caps: the analytic
    # stand-in for bpy's primitive cylinder (whose pole/cap layout is its own).
    verts = _ring(radius, -height * 0.5, count) + _ring(radius, height * 0.5, count)
    faces = []
    for i in range(count):
        ni = (i + 1) % count
        faces.append([i, ni, count + ni, count + i])
    faces.append(list(range(count - 1, -1, -1)))
    faces.append(list(range(count, 2 * count)))
    return verts, faces


def box_mesh(op: dict) -> tuple[list, list]:
    hw, hd, hh = op["width"] / 2, op["depth"] / 2, op["height"] / 2
    cz = _center_z(op)
    x, y = op["x"], op["y"]
    verts = [(x - hw, y - hd, cz - hh), (x + hw, y - hd, cz - hh),
             (x + hw, y + hd, cz - hh), (x - hw, y + hd, cz - hh),
             (x - hw, y - hd, cz + hh), (x + hw, y - hd, cz + hh),
             (x + hw, y + hd, cz + hh), (x - hw, y + hd, cz + hh)]
    faces = [[0, 1, 2, 3], [4, 7, 6, 5], [0, 4, 5, 1],
             [1, 5, 6, 2], [2, 6, 7, 3], [3, 7, 4, 0]]
    return verts, faces


def sphere_mesh(op: dict) -> tuple[list, list]:
    # A lat/long sphere (poles collapse to a point — divergent from bpy's UV
    # sphere, fine for a shape proof). segments/rings from v0's add_sphere.
    segments = max(8, op["vertices"])
    rings = max(4, op["vertices"] // 2)
    r = op["radius"]
    x, y, z0 = op["x"], op["y"], op["z"]
    verts = []
    for ring in range(rings + 1):
        phi = math.pi * ring / rings
        z = math.cos(phi) * r
        rr = math.sin(phi) * r
        for seg in range(segments):
            theta = math.tau * seg / segments
            verts.append((math.cos(theta) * rr + x, math.sin(theta) * rr + y, z + z0))
    faces = []
    for ring in range(rings):
        for seg in range(segments):
            ns = (seg + 1) % segments
            faces.append([ring * segments + seg, ring * segments + ns,
                          (ring + 1) * segments + ns, (ring + 1) * segments + seg])
    return verts, faces


def _cylinder_world(op: dict) -> tuple[list, list]:
    if op["taper_top_radius"] is not None or op["entasis"]:
        local, faces = cylinder_loft(op)
    else:
        local, faces = _plain_cylinder_local(op["radius"], op["height"], max(4, op["vertices"]))
    axis = op["axis"]
    loc_z = _center_z(op) if axis == "z" else op["z"]
    verts = []
    for v in local:
        rx, ry, rz = _axis_apply(v, axis)
        verts.append((rx + op["x"], ry + op["y"], rz + loc_z))
    return verts, faces


def _moulding_world(op: dict) -> tuple[list, list]:
    local, faces = moulding_rings(op)
    return [(v[0] + op["x"], v[1] + op["y"], v[2] + op["base_z"]) for v in local], faces


# P6: module-level accumulator for inset-rejection warnings.  When
# _inset_polygon returns None and the caller falls back to the original
# footprint, it appends a "# WARNING: ..." string here.  obj_lines reads
# and clears this list and prepends the warnings as OBJ comment lines so
# the proof file honestly records that the inset was rejected.
_inset_fallback_log: list = []


def _inset_polygon(pts: list, inset: float):
    """P6: shrink a closed footprint loop INWARD by `inset`.
    Returns the offset polygon on success, or None if the result would be
    self-intersecting or inverted (caller falls back to the un-inset polygon).
    inset=0 returns pts unchanged (identity — byte-identical to v1 behavior).

    v1 was a per-vertex bisector-amplification offset: 1/cos_half → ∞ at
    reflex corners (interior angle > π), causing vertex blowup and self-
    intersection.  v2 replaces it with EDGE-OFFSET-THEN-INTERSECT (Phase 1 of
    Clipper2's offset engine):

    1. For each edge, build the offset LINE (the edge translated inward by
       `inset`).  Direction and anchor are all that is needed — the full line,
       not a capped segment.
    2. Intersect adjacent offset lines at each vertex via a 2×2 linear solve.
       No 1/cos_half term — numerically stable at reflex corners.
    3. Post-hoc area-flip check: if the signed shoelace area changed sign or
       collapsed, the inset was too deep → return None.
    4. Post-hoc edge-crossing check (O(n²) non-adjacent segment pairs): if any
       pair properly intersects, the offset polygon is self-intersecting →
       return None.

    The C++ prism_inset_too_large + prism_inset_reflex_pinch guards (P6) are
    the PRE-CHECK layer; _inset_polygon is the DEFENSE-IN-DEPTH layer that
    catches marginal cases that pass the C++ guards but produce bad geometry."""
    n = len(pts)
    if inset == 0.0 or n < 3:
        return pts  # identity: byte-identical to v1 (0-inset early-out)
    # Signed shoelace area gives the winding. Identical to v1 — auditable.
    area = 0.0
    for i in range(n):
        x0, y0 = pts[i]
        x1, y1 = pts[(i + 1) % n]
        area += x0 * y1 - x1 * y0
    if abs(area) < 1e-9:
        return None  # degenerate polygon — nothing to inset
    ccw = area > 0.0

    # Step 1: build per-edge offset lines.  For edge i → i+1, the inward unit
    # normal is (-dy, dx) for CCW winding (interior is to the LEFT of the
    # directed edge).  The offset line passes through (A + inset·n) with the
    # same direction as the original edge.  Store as (Px, Py, dx, dy).
    offset_lines = []
    for i in range(n):
        ax, ay = pts[i]
        bx, by = pts[(i + 1) % n]
        dx, dy = bx - ax, by - ay
        length = math.hypot(dx, dy)
        if length < 1e-12:
            return None  # zero-length edge → degenerate footprint
        dx /= length
        dy /= length
        if ccw:
            nx, ny = -dy, dx   # inward normal: 90° CCW of direction
        else:
            nx, ny = dy, -dx   # inward normal: 90° CW of direction
        offset_lines.append((ax + inset * nx, ay + inset * ny, dx, dy))

    # Step 2: for each vertex i, intersect offset_line[i-1] and offset_line[i].
    # The 2×2 system: P_prev + t·d_prev = P_curr + s·d_curr.
    # Eliminate s using the cross-product rule (Cramer):
    #   denom = d_prev × d_curr  (z-component: pdx*cdy - pdy*cdx)
    #   t = (P_curr - P_prev) × d_curr / denom
    # Parallel adjacent edges (denom ≈ 0) → the vertex is degenerate; use the
    # midpoint of the two anchor points as a graceful fallback.
    out = []
    for i in range(n):
        px, py, pdx, pdy = offset_lines[(i - 1) % n]
        cx, cy, cdx, cdy = offset_lines[i]
        denom = pdx * cdy - pdy * cdx  # d_prev × d_curr
        if abs(denom) < 1e-9:
            # Parallel/collinear: intermediate vertex is degenerate; collapse
            # it to the midpoint of the two line anchors.
            out.append(((px + cx) * 0.5, (py + cy) * 0.5))
        else:
            t = ((cx - px) * cdy - (cy - py) * cdx) / denom
            out.append((px + t * pdx, py + t * pdy))

    # Step 3: area-flip check (shoelace formula on the offset polygon).
    # A sign flip means the offset polygon wound the other way → inverted.
    # |area| < ε means it collapsed to a point or line.
    new_area = 0.0
    for i in range(n):
        x0, y0 = out[i]
        x1, y1 = out[(i + 1) % n]
        new_area += x0 * y1 - x1 * y0
    if abs(new_area) < 1e-9 or (new_area > 0) != ccw:
        return None  # inverted or collapsed

    # Step 4: edge-crossing check on the offset polygon.
    # Test all non-adjacent segment pairs (i,j) with j ≥ i+2 and NOT the
    # wrap-around pair (0, n-1).  A PROPER intersection (both params strictly
    # in (0,1)) means the polygon self-intersects at this inset depth.
    def _seg_cross(p1, p2, p3, p4):
        """True if segments p1→p2 and p3→p4 properly intersect (not at endpoints)."""
        d1x, d1y = p2[0] - p1[0], p2[1] - p1[1]
        d2x, d2y = p4[0] - p3[0], p4[1] - p3[1]
        denom2 = d1x * d2y - d1y * d2x
        if abs(denom2) < 1e-12:
            return False  # parallel / collinear
        rx, ry = p3[0] - p1[0], p3[1] - p1[1]
        t2 = (rx * d2y - ry * d2x) / denom2
        u2 = (rx * d1y - ry * d1x) / denom2
        eps = 1e-9
        return (eps < t2 < 1.0 - eps) and (eps < u2 < 1.0 - eps)

    for i in range(n):
        for j in range(i + 2, n):
            if i == 0 and j == n - 1:
                continue  # adjacent wrap-around — share a vertex, not a crossing
            p1, p2 = out[i], out[(i + 1) % n]
            p3, p4 = out[j], out[(j + 1) % n]
            if _seg_cross(p1, p2, p3, p4):
                return None  # self-intersecting offset polygon

    return out


def _offset_along_normals(verts: list, faces: list, dist: float) -> list:
    """BL-10: inflate (or, for negative dist, shrink) a shell by pushing each
    vertex along its normal — the per-vertex normal is the sum of incident face
    normals (Newell's method). dist 0 = identity."""
    if dist == 0.0:
        return verts
    normals = [[0.0, 0.0, 0.0] for _ in verts]
    for face in faces:
        nx = ny = nz = 0.0
        k = len(face)
        for i in range(k):
            ax, ay, az = verts[face[i]]
            bx, by, bz = verts[face[(i + 1) % k]]
            nx += (ay - by) * (az + bz)
            ny += (az - bz) * (ax + bx)
            nz += (ax - bx) * (ay + by)
        for idx in face:
            normals[idx][0] += nx
            normals[idx][1] += ny
            normals[idx][2] += nz
    out = []
    for (vx, vy, vz), (nx, ny, nz) in zip(verts, normals):
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        if length < 1e-12:
            out.append((vx, vy, vz))
        else:
            out.append((vx + nx / length * dist, vy + ny / length * dist, vz + nz / length * dist))
    return out


def _prism_world(op: dict) -> tuple[list, list]:
    """The lowered prism mesh, built PURELY (no bpy): the closed footprint
    polygon swept from a bottom cap at z=base_z to a top cap at z=base_z+height,
    placed at the op's (x, y) offset. Returns (verts, faces) like _moulding_world.

    The footprint may repeat its first vertex as its last (a closed drafted
    polyline does); that duplicate is dropped so the side wall has no zero-width
    quad. A NEGATIVE height puts the top BELOW the base (the push/pull-cut
    direction, BL-05) — the mesh stays valid, just inverted in z; never abs()'d,
    so the OBJ's z-extent honestly reports the signed height.

    BL-08: when op carries a PATH, the footprint is lofted along it instead
    (see _swept_prism_world). An EMPTY path is the straight extrude below,
    byte-identical to the BL-04 golden — the path branch never touches it."""
    if op.get("path"):
        return _swept_prism_world(op)
    pts = [(point["x"], point["y"]) for point in op["footprint"]]
    if len(pts) >= 2 and pts[0] == pts[-1]:
        pts = pts[:-1]
    # P6: _inset_polygon returns None when the result would be self-intersecting
    # or inverted (the post-hoc checks in v2 caught it).  Fall back to the
    # original footprint so the proof emits a valid mesh; log a warning comment
    # into the OBJ so the proof file honestly records the rejection.
    _inset_raw = _inset_polygon(pts, op.get("inset", 0.0))
    if _inset_raw is None:
        _inset_fallback_log.append(
            f"# WARNING: {op.get('name', '?')} inset rejected — self-intersection or inversion")
    else:
        pts = _inset_raw
    n = len(pts)
    x, y = op["x"], op["y"]
    z0 = op["base_z"]
    z1 = op["base_z"] + op["height"]
    verts = [(px + x, py + y, z0) for px, py in pts]          # 0..n-1  bottom
    verts += [(px + x, py + y, z1) for px, py in pts]         # n..2n-1 top
    faces = []
    faces.append([i for i in range(n)])                       # bottom cap
    faces.append([2 * n - 1 - i for i in range(n)])           # top cap (reversed)
    for i in range(n):                                        # side quads
        j = (i + 1) % n
        faces.append([i, j, n + j, n + i])
    return _offset_along_normals(verts, faces, op.get("normal_offset", 0.0)), faces


def _swept_prism_world(op: dict) -> tuple[list, list]:
    """BL-08 Follow-Me sweep: loft the footprint along the path into a closed
    swept solid (a moulding run / corridor). PURE, no bpy.

    v1 orientation rule: the footprint cross-section stands UP perpendicular to
    the path — its local x maps to the path's in-plane normal, its local y maps
    to world z (above base_z). At each path point a copy of the loop is placed,
    oriented by the MITER tangent — the bisector of the adjacent segments (P5),
    with the in-plane normal scaled by miter_scale = 2/|t_in+t_out| = 1/cos(α/2)
    so the cross-section keeps its width through the corner; endpoints use the
    single-segment tangent (scale 1.0). Consecutive loops are joined by side
    quads and the first/last loops are capped, so the result is watertight.

    BL-09 taper: each loop is scaled by lerp(1.0, taper_end, fraction-of-path-
    LENGTH) before placement — first loop at 1.0, last at taper_end (a shaft /
    spire). The scale is about the footprint's CENTROID (the mean of its points),
    not the origin or a bbox corner, so a symmetric profile narrows symmetrically
    about its own center rather than drifting toward the axis. taper_end == 1.0
    is the IDENTITY path: scaling is skipped entirely (centroid +/- can perturb
    floats), so a no-taper sweep is byte-identical to the BL-08 golden."""
    fp = [(point["x"], point["y"]) for point in op["footprint"]]
    if len(fp) >= 2 and fp[0] == fp[-1]:
        fp = fp[:-1]
    # P6: same None-fallback pattern as _prism_world — reject and warn rather
    # than emit a self-intersecting sweep cross-section.
    _fp_inset = _inset_polygon(fp, op.get("inset", 0.0))
    if _fp_inset is None:
        _inset_fallback_log.append(
            f"# WARNING: {op.get('name', '?')} inset rejected — self-intersection or inversion")
    else:
        fp = _fp_inset
    # Drop consecutive-duplicate path points so a segment always has a tangent.
    raw = [(p["x"], p["y"]) for p in op["path"]]
    path = [raw[0]]
    for p in raw[1:]:
        if p != path[-1]:
            path.append(p)
    n = len(fp)
    m = len(path)
    ox, oy, base_z = op["x"], op["y"], op["base_z"]

    taper = op.get("taper_end", 1.0)
    taper_end_y = op.get("taper_end_y", 0.0)
    # P4b early-out: skip scaling entirely when there is NEITHER X-taper NOR
    # Y-taper. X has no taper when taper_end==1.0. Y has no taper when
    # taper_end_y<=0 (sentinel = follow X, and X is 1.0) OR taper_end_y==1.0
    # (explicit 1.0 = identity). Both conditions must hold to skip; if only one
    # axis has a real taper, we must enter the per-axis scale path.
    no_x_taper = (taper == 1.0)
    no_y_taper = (taper_end_y <= 0 or taper_end_y == 1.0)
    tapering = not (no_x_taper and no_y_taper)
    # P4: taper_curve is the power applied to the linear fraction t before
    # the lerp: scale = lerp(1, end, t^taper_curve). The SAME curved fraction
    # is used for BOTH axes — taper_curve shapes the overall taper profile.
    taper_curve = op.get("taper_curve", 1.0)
    # P4b: resolve the sentinel — when taper_end_y <= 0 ("follow"), the Y
    # end scale matches X (uniform taper, existing behavior).
    ey = taper_end_y if taper_end_y > 0 else taper
    cx = sum(fx for fx, _ in fp) / n if tapering else 0.0  # footprint centroid
    cy = sum(fy for _, fy in fp) / n if tapering else 0.0
    # Cumulative path length per point, so the taper fraction tracks distance
    # travelled (not raw point index) — uneven path spacing tapers evenly.
    cumlen = [0.0]
    for k in range(1, m):
        cumlen.append(cumlen[-1] + math.hypot(path[k][0] - path[k - 1][0],
                                              path[k][1] - path[k - 1][1]))
    total = cumlen[-1] or 1.0

    # P5: pre-compute UNIT SEGMENT TANGENTS for all m−1 segments so each can
    # be looked up by index.  segs[k] = normalised direction of path[k]→path[k+1].
    # The duplicate-point filter above guarantees |seg| > 0 for all k.
    segs = []
    for k in range(m - 1):
        dx = path[k + 1][0] - path[k][0]
        dy = path[k + 1][1] - path[k][1]
        L = math.hypot(dx, dy) or 1.0
        segs.append((dx / L, dy / L))

    loops = []
    for k in range(m):
        px, py = path[k]
        # P5 BISECTOR MITER FRAME.
        #
        # BL-08 v1 used the outgoing (or incoming at the last point) segment
        # tangent — fine for straight paths but leaves a discontinuous jump at
        # corners: the quads on the "turn" side of the tube overlap while the
        # quads on the "straight" side leave a gap.
        #
        # v2: at an INTERIOR path point (0 < k < m-1), use the normalised sum
        # of the incoming and outgoing unit tangents — the bisector direction.
        # The cross-section is then placed in the MITER PLANE (perpendicular to
        # the bisector) so it meets both adjacent tubes flush.
        #
        # WHY bisector: t_in + t_out bisects the turn angle α between the two
        # segments.  The miter plane is perpendicular to the bisector in XY.
        # Placing the loop in that plane means the front and back faces of the
        # cross-section lie exactly on the adjacent segment-perpendicular planes
        # (no gap, no overlap).
        #
        # WHY miter_scale = 2/|t_in+t_out|: the miter plane is tilted α/2 from
        # each adjacent perpendicular.  A profile width W in the miter plane has
        # an apparent width of W·cos(α/2) = W·|b|/2 in the perpendicular view.
        # To preserve the intended tube width, pre-scale the in-plane fx component
        # by the inverse: 2/|b|.  Key identity: cos(α/2) = |t_in+t_out|/2,
        # so no trig is required.
        #
        # BYTE-IDENTICAL for straight paths: collinear points have t_in = t_out,
        # so b = 2·t_in, |b| = 2, miter_scale = 1.0 — identical to v1.
        #
        # ENDPOINTS: only one adjacent segment → single-segment perpendicular
        # frame, miter_scale = 1.0, unchanged from v1.
        if k == 0:
            tx, ty = segs[0]
            miter_scale = 1.0
        elif k == m - 1:
            tx, ty = segs[-1]
            miter_scale = 1.0
        else:
            tin_x, tin_y = segs[k - 1]
            tout_x, tout_y = segs[k]
            bx, by = tin_x + tout_x, tin_y + tout_y
            bl = math.hypot(bx, by)
            if bl < 1e-6:       # near-hairpin; validate should have refused this
                tx, ty = tout_x, tout_y
                miter_scale = 1.0
            else:
                tx, ty = bx / bl, by / bl
                # Python safety clamp: validate is the real guard (it refuses
                # bl < 0.5, i.e. miter_scale > 4); the clamp prevents infinite
                # coordinates if _swept_prism_world is called without validate.
                miter_scale = min(2.0 / bl, 10.0)
        # In-plane normal, pre-scaled by miter_scale.
        # At interior corners this widens the cross-section to compensate for
        # the oblique cut; at endpoints and straight segments it equals nx/ny.
        nx_eff = (-ty) * miter_scale
        ny_eff =   tx  * miter_scale
        t = (cumlen[k] / total) ** taper_curve  # P4: curved fraction (1.0 = linear)
        sx = 1.0 + (taper - 1.0) * t   # P4b: per-axis X scale
        sy = 1.0 + (ey - 1.0) * t      # P4b: per-axis Y scale
        for fx, fy in fp:
            if tapering:  # scale the cross-section about its centroid, per axis
                fx, fy = cx + sx * (fx - cx), cy + sy * (fy - cy)
            loops.append((ox + px + fx * nx_eff, oy + py + fx * ny_eff, base_z + fy))

    verts = list(loops)
    faces = []
    for k in range(m - 1):  # side quads between consecutive cross-sections
        a, b = k * n, (k + 1) * n
        for i in range(n):
            j = (i + 1) % n
            faces.append([a + i, a + j, b + j, b + i])
    faces.append([i for i in range(n)])                       # start cap
    last = (m - 1) * n
    faces.append([last + (n - 1 - i) for i in range(n)])      # end cap (reversed)
    return _offset_along_normals(verts, faces, op.get("normal_offset", 0.0)), faces


def _ring_world(op: dict) -> tuple[list, list]:
    fake = dict(op, radius=op["radius"] + op["overhang"], height=op["tube_height"],
                taper_top_radius=None, entasis=False, axis="z", z_mode="center",
                entasis_ratio=0.045)
    return _cylinder_world(fake)


def _flute_target_extent(ops: list, name: str) -> tuple:
    # The z-centre, z-height, and xy-radius of a flute target (a z cylinder).
    for op in ops:
        if op.get("name") == name and op["type"] == "AddCylinder":
            return _center_z(op), op["height"], op["radius"]
    return 0.0, 1.0, 1.0


def cutflute_objects(ops: list, op: dict) -> list:
    # CutFlutes emits its CUTTERS as named objects (A's offset arithmetic).
    # Boolean cutting is execution's job; a proof shows where the tool bites.
    z_c, z_h, target_r = _flute_target_extent(ops, op["target"])
    count = max(1, op["count"])
    if op["cutter_radius"] is not None and op["at_radius"] is not None:
        cutter_radius = op["cutter_radius"]
        cutter_offset = op["at_radius"] + cutter_radius - op["depth"]
    else:
        cutter_radius = max(0.02, target_r * op["width_ratio"] * 0.5)
        cutter_offset = max(0.001, target_r + cutter_radius - op["depth"])
    z0 = z_c - z_h * 0.5 if op["start_z"] is None else op["start_z"]
    z1 = z_c + z_h * 0.5 if op["end_z"] is None else op["end_z"]
    cutter_depth = max(0.001, z1 - z0)
    z_mid = (z0 + z1) * 0.5
    objects = []
    for index in range(count):
        angle = math.tau * index / count
        cx = math.cos(angle) * cutter_offset
        cy = math.sin(angle) * cutter_offset
        local, faces = _plain_cylinder_local(cutter_radius, cutter_depth, 18)
        verts = [(v[0] + cx, v[1] + cy, v[2] + z_mid) for v in local]
        objects.append((f"{op['target']}.flute_cutter_{index:02d}", verts, faces))
    return objects


def _mesh_for(op: dict, craftsmen: dict | None) -> tuple | None:
    """(verts, faces) for one buildable op, or None for the ops that carry no
    single mesh (AddLabel text; CutFlutes/AddBoolean, which compose others).
    Used by AddBoolean's proof to fetch its operands' meshes."""
    kind = op["type"]
    if kind == "AddBox":
        return box_mesh(op)
    if kind == "AddCylinder":
        return _cylinder_world(op)
    if kind == "AddSphere":
        return sphere_mesh(op)
    if kind == "AddRing":
        return _ring_world(op)
    if kind == "AddMoulding":
        return _moulding_world(op)
    if kind == "AddPrism":
        return _prism_world(op)
    if kind == "Script" and craftsmen is not None:
        module = craftsmen.get(op["script"])
        if module is not None and hasattr(module, "proof_mesh"):
            return module.proof_mesh(op)
    return None


def boolean_objects(ops: list, op: dict, craftsmen: dict | None = None) -> list:
    # AddBoolean's proof shows its two OPERANDS (looked up by name), each tagged
    # with the boolean intent — it does NOT compute the CSG. The real union/
    # subtract/intersect is execution-only, exactly as CutFlutes emits its
    # cutters rather than the cut result (a proof shows the inputs to the cut).
    out = []
    for role in ("a", "b"):
        name = op[role]
        operand = next((o for o in ops if o.get("name") == name), None)
        mesh = _mesh_for(operand, craftsmen) if operand is not None else None
        if mesh is not None:
            out.append((f"{op['name']}.{op['kind']}.{role}", mesh[0], mesh[1]))
    return out


def obj_objects(ops: list, craftsmen: dict | None = None) -> list:
    """(name, verts, faces) per buildable op — the OBJ proof's object list, in
    op order. AddLabel carries no mesh (it is text), so it is skipped; a Script
    op asks its craftsman for the proof mesh (an unknown craftsman is skipped)."""
    if craftsmen is None:
        craftsmen = load_craftsmen(default_craftsmen_dir())
    # The set of op-NAMES consumed as an operand by ANY AddBoolean. In the real
    # bpy build a boolean CONSUMES its operands (they are not standalone after);
    # the proof mirrors that — a consumed op appears ONCE, tagged under the
    # boolean, not also standalone. v1 rule: BOTH a and b are inputs the boolean
    # consumes, so both are suppressed standalone. Only standalone-mesh ops are
    # skipped; a composite op (CutFlutes/AddBoolean) is never suppressed (so a
    # chained boolean whose own result feeds another still emits its tagged ops).
    consumed = set()
    for op in ops:
        if op["type"] == "AddBoolean":
            consumed.add(op["a"])
            consumed.add(op["b"])
    standalone_kinds = {"AddBox", "AddCylinder", "AddSphere", "AddRing",
                        "AddMoulding", "AddPrism", "Script"}
    out = []
    for op in ops:
        kind = op["type"]
        if kind in standalone_kinds and op.get("name") in consumed:
            continue  # consumed by a boolean -> appears only tagged under it
        if kind == "AddBox":
            out.append((op["name"], *box_mesh(op)))
        elif kind == "AddCylinder":
            out.append((op["name"], *_cylinder_world(op)))
        elif kind == "AddSphere":
            out.append((op["name"], *sphere_mesh(op)))
        elif kind == "AddRing":
            out.append((op["name"], *_ring_world(op)))
        elif kind == "AddMoulding":
            out.append((op["name"], *_moulding_world(op)))
        elif kind == "AddPrism":
            out.append((op["name"], *_prism_world(op)))
        elif kind == "CutFlutes":
            out.extend(cutflute_objects(ops, op))
        elif kind == "AddBoolean":
            out.extend(boolean_objects(ops, op, craftsmen))
        elif kind == "Script":
            module = craftsmen.get(op["script"])
            if module is not None and hasattr(module, "proof_mesh"):
                verts, faces = module.proof_mesh(op)
                out.append((op["name"], verts, faces))
    return out


def obj_lines(ops: list, craftsmen: dict | None = None) -> list:
    """The OBJ text: `o <name>` groups, `v`/`f` only (no normals/uv — the proof
    is shape, not shading), 1-indexed across the file, op order, fixed `g` floats.

    P6: if any prism's inset was rejected and fell back to the original footprint,
    the warning comment lines accumulated in _inset_fallback_log are prepended to
    the output.  The `# WARNING: ...` lines are valid OBJ comments (OBJ ignores
    lines starting with '#'); they appear before the first 'o' group so they are
    visible at the top of the file without parsing the geometry."""
    global _inset_fallback_log
    _inset_fallback_log.clear()
    objs = obj_objects(ops, craftsmen)  # may populate _inset_fallback_log
    # Consume warnings collected during mesh generation (clear after snapshot).
    lines: list = list(_inset_fallback_log)
    _inset_fallback_log.clear()
    base = 1
    for name, verts, faces in objs:
        lines.append(f"o {name}")
        for vx, vy, vz in verts:
            lines.append(f"v {_fmt(vx)} {_fmt(vy)} {_fmt(vz)}")
        for face in faces:
            lines.append("f " + " ".join(str(base + i) for i in face))
        base += len(verts)
    return lines


def plan_lines(ops: list[dict]) -> list[str]:
    """The dry run: one deterministic line per op, plus the computed rig."""
    lines = [f"# edi_craft dry run — {len(ops)} ops"]
    for op in ops:
        if op["type"] == "AddBox":
            lines.append(
                f"AddBox {op['name']} {_fmt(op['width'])}x{_fmt(op['depth'])}x{_fmt(op['height'])}"
                f" center_z={_fmt(_center_z(op))} material={op['material']}")
        elif op["type"] == "AddCylinder":
            top = op["taper_top_radius"]
            taper = f" top_r={_fmt(top)}" if top is not None else ""
            entasis = f" entasis={_fmt(op['entasis_ratio'])}" if op["entasis"] else ""
            lines.append(
                f"AddCylinder {op['name']} r={_fmt(op['radius'])}{taper} h={_fmt(op['height'])}"
                f" center_z={_fmt(_center_z(op) if op['axis'] == 'z' else op['z'])}"
                f" axis={op['axis']}{entasis} vertices={op['vertices']} material={op['material']}")
        elif op["type"] == "AddSphere":
            lines.append(
                f"AddSphere {op['name']} r={_fmt(op['radius'])} z={_fmt(op['z'])} material={op['material']}")
        elif op["type"] == "AddRing":
            lines.append(
                f"AddRing {op['name']} r={_fmt(op['radius'])} tube_h={_fmt(op['tube_height'])}"
                f" z={_fmt(op['z'])} material={op['material']}")
        elif op["type"] == "AddMoulding":
            max_r = max(point["radius"] for point in op["profile"])
            lines.append(
                f"AddMoulding {op['name']} base_z={_fmt(op['base_z'])} points={len(op['profile'])}"
                f" max_r={_fmt(max_r)} material={op['material']}")
        elif op["type"] == "AddPrism":
            # A path (BL-08) makes it a swept solid; absent, it is the straight
            # height-extrude. The plan line stays byte-identical when no path.
            path = op.get("path") or []
            sweep = f" path={len(path)}" if path else ""
            lines.append(
                f"AddPrism {op['name']} base_z={_fmt(op['base_z'])} height={_fmt(op['height'])}"
                f" points={len(op['footprint'])}{sweep} material={op['material']}")
        elif op["type"] == "CutFlutes":
            z_range = ""
            if op["start_z"] is not None and op["end_z"] is not None:
                z_range = f" z={_fmt(op['start_z'])}..{_fmt(op['end_z'])}"
            # Explicit cutter shows its own geometry; otherwise the width_ratio
            # derivation (R1-B04b). Absent the pair this line is byte-identical
            # to before — the committed doric golden uses width_ratio.
            if op["cutter_radius"] is not None and op["at_radius"] is not None:
                cutter = f"cutter_r={_fmt(op['cutter_radius'])} at_r={_fmt(op['at_radius'])}"
            else:
                cutter = f"width_ratio={_fmt(op['width_ratio'])}"
            lines.append(
                f"CutFlutes {op['target']} count={op['count']} depth={_fmt(op['depth'])}"
                f" {cutter}{z_range}")
        elif op["type"] == "AddBoolean":
            lines.append(
                f"AddBoolean {op['name']} {op['kind']} {op['a']} {op['b']}")
        elif op["type"] == "AddLabel":
            lines.append(f"AddLabel {op['name']} \"{op['text']}\" at ({_fmt(op['x'])}, {_fmt(op['y'])}, {_fmt(op['z'])})")

    x0, x1, y0, y1, z0, z1 = bounds_of(ops)
    span = max(x1 - x0, y1 - y0, z1 - z0)
    center_z = (z0 + z1) / 2.0
    lines.append(
        f"preview rig: ortho_scale={_fmt(span * 1.2)}"
        f" camera=({_fmt((x0 + x1) / 2)}, {_fmt(y0 - span * 1.6)}, {_fmt(center_z)})"
        f" target_z={_fmt(center_z)}")
    return lines


# ---------------------------------------------------------------------------
# The craftsmen (bpy). Imported lazily so the dry run never needs Blender.

def build(ops: list[dict]) -> None:  # pragma: no cover — exercised in Blender
    import bpy

    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    materials = {}
    for key, (name, color) in MATERIALS.items():
        material = bpy.data.materials.new(name)
        material.diffuse_color = color
        # v0's node wiring verbatim: renders shade with Roughness 0.68, not
        # the engine default (the if-guard tolerates a renamed node, same
        # silent-skip v0 chose).
        material.use_nodes = True
        bsdf = material.node_tree.nodes.get("Principled BSDF")
        if bsdf:
            bsdf.inputs["Base Color"].default_value = color
            bsdf.inputs["Roughness"].default_value = 0.68
        materials[key] = material

    failures: list[str] = []

    def assign_material(obj, key):
        obj.data.materials.append(materials[key])  # strict: key was validated

    def polish(obj, amount, segments):
        # v0's finish pass: a soft bevel plus weighted normals.
        modifier = obj.modifiers.new("soft_bevel", "BEVEL")
        modifier.width = amount
        modifier.segments = segments
        modifier.affect = "EDGES"
        obj.modifiers.new("weighted_normals", "WEIGHTED_NORMAL")

    def _apply_axis(obj, axis):
        # v0's rotate_axis: lay the part along X or Y; Z stands as built.
        if axis == "x":
            obj.rotation_euler = (0.0, math.pi / 2, 0.0)
        elif axis == "y":
            obj.rotation_euler = (math.pi / 2, 0.0, 0.0)

    def add_box(op):
        z = _center_z(op)
        bpy.ops.mesh.primitive_cube_add(size=1, location=(op["x"], op["y"], z), align="WORLD")
        obj = bpy.context.object
        obj.name = op["name"]
        obj.dimensions = (op["width"], op["depth"], op["height"])
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        assign_material(obj, op["material"])
        polish(obj, 0.04, 1)
        return obj

    def add_cylinder(op):
        top = op["taper_top_radius"]
        if top is None and not op["entasis"]:
            z = _center_z(op) if op["axis"] == "z" else op["z"]
            bpy.ops.mesh.primitive_cylinder_add(
                vertices=op["vertices"], radius=op["radius"], depth=op["height"],
                location=(op["x"], op["y"], z), align="WORLD")
            obj = bpy.context.object
            obj.name = op["name"]
            _apply_axis(obj, op["axis"])  # v0's rotate_axis, both branches
            assign_material(obj, op["material"])
            polish(obj, 0.035, 1)
            return obj
        # Tapered/entasis shaft: v0's ring loft, now shared with the OBJ proof
        # (cylinder_loft is the same math this branch had inline).
        verts, faces = cylinder_loft(op)
        mesh = bpy.data.meshes.new(op["name"] + "_mesh")
        mesh.from_pydata(verts, [], faces)
        mesh.update(calc_edges=True)
        obj = bpy.data.objects.new(op["name"], mesh)
        bpy.context.collection.objects.link(obj)
        # v0's guard exactly: base-z lifting applies only when standing on Z.
        obj.location = (op["x"], op["y"], _center_z(op) if op["axis"] == "z" else op["z"])
        _apply_axis(obj, op["axis"])
        bpy.context.view_layer.objects.active = obj
        assign_material(obj, op["material"])
        polish(obj, 0.035, 1)
        return obj

    def add_sphere(op):
        bpy.ops.mesh.primitive_uv_sphere_add(
            segments=max(8, op["vertices"]), ring_count=max(4, op["vertices"] // 2),
            radius=op["radius"], location=(op["x"], op["y"], op["z"]), align="WORLD")
        obj = bpy.context.object
        obj.name = op["name"]
        assign_material(obj, op["material"])
        polish(obj, 0.01, 1)
        return obj

    def add_ring(op):
        # v0's add_ring is a cylinder alias built at radius + overhang —
        # matching v0's emitter and this file's own bounds_of. The true
        # remaining divergence: no torus is built, so overhang widens the
        # alias rather than overhanging a tube.
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=op["vertices"], radius=op["radius"] + op["overhang"], depth=op["tube_height"],
            location=(op["x"], op["y"], op["z"]), align="WORLD")
        obj = bpy.context.object
        obj.name = op["name"]
        assign_material(obj, op["material"])
        polish(obj, 0.035, 1)
        return obj

    def add_moulding(op):
        # The mason's profile spun as stacked rings, capped both ends — shared
        # with the OBJ proof (moulding_rings is this branch's former inline math).
        verts, faces = moulding_rings(op)
        mesh = bpy.data.meshes.new(op["name"] + "_mesh")
        mesh.from_pydata(verts, [], faces)
        mesh.update(calc_edges=True)
        obj = bpy.data.objects.new(op["name"], mesh)
        bpy.context.collection.objects.link(obj)
        obj.location = (op["x"], op["y"], op["base_z"])
        bpy.context.view_layer.objects.active = obj
        assign_material(obj, op["material"])
        polish(obj, 0.03, 2)
        return obj

    def add_prism(op):
        # The lowered prism: built from the SAME pure _prism_world mesh the OBJ
        # proof uses, so the headless proof and the Blender build cannot drift.
        # _prism_world already places verts in world space (x/y offset, base_z),
        # so the object sits at the origin.
        verts, faces = _prism_world(op)
        mesh = bpy.data.meshes.new(op["name"] + "_mesh")
        mesh.from_pydata(verts, [], faces)
        mesh.update(calc_edges=True)
        obj = bpy.data.objects.new(op["name"], mesh)
        bpy.context.collection.objects.link(obj)
        bpy.context.view_layer.objects.active = obj
        assign_material(obj, op["material"])
        polish(obj, 0.02, 1)
        return obj

    def cut_flutes(op):
        target = bpy.data.objects.get(op["target"])
        if target is None:
            failures.append(f"CutFlutes: target not found: {op['target']}")
            return
        count = max(1, op["count"])
        if op["cutter_radius"] is not None and op["at_radius"] is not None:
            # Explicit cutter geometry (R1-B04b): pipeline A's radial_groove
            # placement, mirrored verbatim from pipeline A's radial_groove
            # emitter (RecipeEmit.cpp, retired R1-B06). This branch runs only
            # under bpy: the smoke pins the INPUTS (the plan line and the
            # sample's explicit cutter values); the formula itself is pinned
            # by this comment and the R2 headless-guard candidate — the
            # cutter ring's radius is used as given, and its centre rides at
            # at_radius + cutter_radius - depth so it overlaps the surface by
            # exactly `depth`. No bbox/width_ratio derivation and no 0.001
            # floor: A's emitter trusts the drafted numbers (the floor in the
            # derived branch guards the DERIVED radius, which can collapse).
            cutter_radius = op["cutter_radius"]
            cutter_offset = op["at_radius"] + cutter_radius - op["depth"]
        else:
            radius = max(target.dimensions.x, target.dimensions.y) * 0.5
            cutter_radius = max(0.02, radius * op["width_ratio"] * 0.5)
            cutter_offset = max(0.001, radius + cutter_radius - op["depth"])
        z0 = target.location.z - target.dimensions.z * 0.5 if op["start_z"] is None else op["start_z"]
        z1 = target.location.z + target.dimensions.z * 0.5 if op["end_z"] is None else op["end_z"]
        cutter_depth = max(0.001, z1 - z0)
        z_mid = (z0 + z1) * 0.5
        # v0-verbatim: the cutter ring is centred at world (0, 0) — it
        # ignores target.location.x/y, like the prototype it ports.
        for index in range(count):
            angle = math.tau * index / count
            location = (math.cos(angle) * cutter_offset, math.sin(angle) * cutter_offset, z_mid)
            bpy.ops.mesh.primitive_cylinder_add(
                vertices=18, radius=cutter_radius, depth=cutter_depth,
                location=location, align="WORLD")
            cutter = bpy.context.object
            cutter.name = f"{op['target']}.flute_cutter_{index:02d}"
            modifier = target.modifiers.new(name=f"flute_cut_{index:02d}", type="BOOLEAN")
            modifier.operation = "DIFFERENCE"
            if hasattr(modifier, "solver"):
                modifier.solver = "EXACT"
            modifier.object = cutter
            bpy.ops.object.select_all(action="DESELECT")
            target.select_set(True)
            bpy.context.view_layer.objects.active = target
            try:
                bpy.ops.object.modifier_move_to_index(modifier=modifier.name, index=0)
            except Exception as exc:  # older Blender: cosmetic, not fatal
                print(f"edi_craft: could not reorder {modifier.name}: {exc}")
            try:
                bpy.ops.object.modifier_apply(modifier=modifier.name)
            except Exception as exc:
                # v0 printed and carried on; collected here and raised at
                # the end so a fluteless column cannot exit 0.
                failures.append(f"CutFlutes: failed to apply {modifier.name}: {exc}")
                target.modifiers.remove(modifier)
            bpy.data.objects.remove(cutter, do_unlink=True)

    def add_label(op):
        # Divergence from v0 (see docstring): v0 emitted only a comment;
        # here the label is a real FONT object, sized relative to the build.
        curve = bpy.data.curves.new(op["name"], type="FONT")
        curve.body = op["text"]
        obj = bpy.data.objects.new(op["name"], curve)
        obj.location = (op["x"], op["y"], op["z"])
        bpy.context.collection.objects.link(obj)
        return obj

    def add_boolean(op):
        # BL-11: the only tier that truly cuts. Apply a boolean modifier to
        # operand a, referencing operand b, then apply it. (The OBJ proof showed
        # the operands tagged; here the CSG actually happens.)
        a = bpy.data.objects.get(op["a"])
        b = bpy.data.objects.get(op["b"])
        if a is None or b is None:
            failures.append(f"AddBoolean: operand not found: {op['a']!r} / {op['b']!r}")
            return
        operation = {"union": "UNION", "subtract": "DIFFERENCE",
                     "intersect": "INTERSECT"}[op["kind"]]
        modifier = a.modifiers.new(name=op["name"], type="BOOLEAN")
        modifier.operation = operation
        if hasattr(modifier, "solver"):
            modifier.solver = "EXACT"
        modifier.object = b
        bpy.ops.object.select_all(action="DESELECT")
        a.select_set(True)
        bpy.context.view_layer.objects.active = a
        try:
            bpy.ops.object.modifier_apply(modifier=modifier.name)
        except Exception as exc:  # collected so a failed cut cannot exit 0
            failures.append(f"AddBoolean: failed to apply {op['name']}: {exc}")
            a.modifiers.remove(modifier)
        else:
            bpy.data.objects.remove(b, do_unlink=True)  # b consumed into the result
        return a

    builders = {
        "AddBox": add_box,
        "AddCylinder": add_cylinder,
        "AddSphere": add_sphere,
        "AddRing": add_ring,
        "AddMoulding": add_moulding,
        "AddPrism": add_prism,
        "CutFlutes": cut_flutes,
        "AddBoolean": add_boolean,
        "AddLabel": add_label,
    }
    custom = load_craftsmen(default_craftsmen_dir())
    for op in ops:
        if op["type"] == "Script":
            module = custom.get(op["script"])
            if module is None or not hasattr(module, "build"):
                failures.append(f"Script: unknown craftsman {op['script']!r}")
            else:
                module.build(op)
            continue
        builders[op["type"]](op)

    # The preview rig, computed from what was actually built — v0 hardcoded
    # a rig that neither framed nor lit its own column.
    x0, x1, y0, y1, z0, z1 = bounds_of(ops)
    span = max(x1 - x0, y1 - y0, z1 - z0)
    center = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0)

    bpy.ops.object.light_add(type="SUN", location=(center[0] - span, center[1] - span, z1 + span))
    light = bpy.context.object
    light.name = "preview_sun"
    light.data.energy = 3.0
    light.rotation_euler = (math.radians(50), 0.0, math.radians(-35))

    bpy.ops.object.camera_add(
        location=(center[0], y0 - span * 1.6, center[2]),
        rotation=(math.pi / 2, 0.0, 0.0))
    camera = bpy.context.object
    camera.name = "preview_camera"
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = span * 1.2
    camera.data.clip_start = max(0.001, span * 0.01)  # tiny builds must not clip
    camera.data.clip_end = max(100.0, span * 10.0)
    bpy.context.scene.camera = camera

    if failures:
        raise RuntimeError("edi_craft build finished with failures:\n  " + "\n  ".join(failures))
    print(f"edi_craft: built {len(ops)} ops.")


def main(argv: list[str]) -> int:
    # Inside Blender, script args follow "--"; standalone they are argv[1:].
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = argv[1:]
    # --list-craftsmen[=<folder>]: print the custom-craftsman registry as TOML
    # (the lab's palette + inspector read it) and exit. Needs no ops file.
    for arg in argv:
        if arg == "--list-craftsmen" or arg.startswith("--list-craftsmen="):
            folder = arg.split("=", 1)[1] if "=" in arg else default_craftsmen_dir()
            print(craftsmen_manifest_toml(folder))
            return 0
    dry_run = "--dry-run" in argv
    obj_out = next((arg.split("=", 1)[1] for arg in argv if arg.startswith("--obj-out=")), None)
    paths = [arg for arg in argv if not arg.startswith("--")]
    if len(paths) != 1:
        print("usage: edi_craft.py [--dry-run] [--obj-out=<path>] <ops_compiled.toml>", file=sys.stderr)
        return 2
    ops = parse_ops(paths[0])
    if obj_out is not None:
        # The mesh proof: a deterministic OBJ, no Blender — write it and exit.
        with open(obj_out, "w") as handle:
            handle.write("\n".join(obj_lines(ops)) + "\n")
        return 0
    if dry_run:
        print("\n".join(plan_lines(ops)))
        return 0
    build(ops)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
