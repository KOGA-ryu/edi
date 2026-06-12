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

import math
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
    "AddBox", "AddCylinder", "AddSphere", "AddRing", "AddMoulding", "CutFlutes", "AddLabel",
)


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
            r = max(point["radius"] for point in op["profile"])
            z0 = op["base_z"] + min(point["z"] for point in op["profile"])
            z1 = op["base_z"] + max(point["z"] for point in op["profile"])
            include(op["x"] - r, op["x"] + r, op["y"] - r, op["y"] + r, z0, z1)
        elif op["type"] == "AddLabel":
            # A point, not a box: the op carries no text extent, but an
            # annotation must stay inside the framed view.
            include(op["x"], op["x"], op["y"], op["y"], op["z"], op["z"])
    if min_x == inf:
        return (-1, 1, -1, 1, 0, 1)
    return (min_x, max_x, min_y, max_y, min_z, max_z)


def _fmt(value: float) -> str:
    return format(value, "g")


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
        # Tapered/entasis shaft: v0's ring loft, verbatim math.
        rings = 24
        vertices = max(4, op["vertices"])
        radius = op["radius"]
        top_radius = top if top is not None else radius
        height = op["height"]
        verts = []
        faces = []
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
        # The mason's profile spun as stacked rings, capped both ends.
        vertices = max(4, op["vertices"])
        verts = []
        faces = []
        for point in op["profile"]:
            rr = max(0.001, point["radius"])
            for index in range(vertices):
                angle = math.tau * index / vertices
                verts.append((math.cos(angle) * rr, math.sin(angle) * rr, point["z"]))
        for ring in range(len(op["profile"]) - 1):
            current = ring * vertices
            nxt_ring = (ring + 1) * vertices
            for index in range(vertices):
                nxt = (index + 1) % vertices
                faces.append([current + index, current + nxt, nxt_ring + nxt, nxt_ring + index])
        faces.append(list(range(vertices - 1, -1, -1)))
        top_start = (len(op["profile"]) - 1) * vertices
        faces.append(list(range(top_start, top_start + vertices)))
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

    craftsmen = {
        "AddBox": add_box,
        "AddCylinder": add_cylinder,
        "AddSphere": add_sphere,
        "AddRing": add_ring,
        "AddMoulding": add_moulding,
        "CutFlutes": cut_flutes,
        "AddLabel": add_label,
    }
    for op in ops:
        craftsmen[op["type"]](op)

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
    dry_run = "--dry-run" in argv
    paths = [arg for arg in argv if not arg.startswith("--")]
    if len(paths) != 1:
        print("usage: edi_craft.py [--dry-run] <ops_compiled.toml>", file=sys.stderr)
        return 2
    ops = parse_ops(paths[0])
    if dry_run:
        print("\n".join(plan_lines(ops)))
        return 0
    build(ops)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
