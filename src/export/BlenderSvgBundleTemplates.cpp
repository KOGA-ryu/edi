#include "BlenderSvgBundleTemplates.h"

namespace BlenderSvgBundleTemplates {
QString importScript() {
    return QString::fromUtf8(R"PY(# Draftsman Blender SVG bundle importer.
# Run in Blender with: blender --python import_drawing_svg.py

import bpy
import json
import re
from pathlib import Path

BUNDLE_DIR = Path(__file__).resolve().parent
MANIFEST_PATH = BUNDLE_DIR / "manifest.json"

manifest = {}
if MANIFEST_PATH.exists():
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))

import_hints = manifest.get("blender_import", {})
SVG_PATH = BUNDLE_DIR / import_hints.get("svg_file", "drawing.svg")
COLLECTION_NAME = import_hints.get("collection_name", "Draftsman SVG")
SCALE = float(import_hints.get("scale", 0.01))

if not SVG_PATH.exists():
    raise FileNotFoundError(f"Missing SVG file: {SVG_PATH}")

try:
    bpy.ops.preferences.addon_enable(module="io_curve_svg")
except Exception:
    pass

scene_collection_names = {collection.name for collection in bpy.context.scene.collection.children}
collection = bpy.data.collections.get(COLLECTION_NAME)
if collection is None:
    collection = bpy.data.collections.new(COLLECTION_NAME)
if collection.name not in scene_collection_names:
    bpy.context.scene.collection.children.link(collection)

def safe_name(value, fallback):
    raw = str(value or "").strip()
    if not raw:
        raw = fallback
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", raw).strip("_") or fallback

def ensure_child_collection(parent, name):
    collection_name = safe_name(name, "ungrouped")
    if collection_name == parent.name:
        return parent
    child = bpy.data.collections.get(collection_name)
    if child is None:
        child = bpy.data.collections.new(collection_name)
    if child.name not in {existing.name for existing in parent.children}:
        parent.children.link(child)
    return child

def ensure_material(name, entry):
    material_name = safe_name(name, "")
    if not material_name:
        return None
    material = bpy.data.materials.get(material_name)
    if material is None:
        material = bpy.data.materials.new(material_name)
    color = color_from_entry(entry)
    if color:
        material.diffuse_color = color
    return material

def hex_to_rgba(value):
    raw = str(value or "").strip().lstrip("#")
    if len(raw) == 3:
        raw = "".join(ch * 2 for ch in raw)
    if len(raw) not in (6, 8):
        return None
    try:
        r = int(raw[0:2], 16) / 255.0
        g = int(raw[2:4], 16) / 255.0
        b = int(raw[4:6], 16) / 255.0
        a = int(raw[6:8], 16) / 255.0 if len(raw) == 8 else 1.0
        return (r, g, b, a)
    except ValueError:
        return None

def color_from_entry(entry):
    style = entry.get("style", {})
    return hex_to_rgba(style.get("fill_color")) or hex_to_rgba(style.get("stroke_color"))

before_names = set(bpy.data.objects.keys())

try:
    bpy.ops.import_curve.svg(filepath=str(SVG_PATH))
except Exception as exc:
    raise RuntimeError("Blender SVG import failed. Enable SVG import support, then rerun this script.") from exc

manifest_objects = manifest.get("objects", [])
manifest_by_id = {str(entry.get("id", "")): entry for entry in manifest_objects if entry.get("id")}

def clean_object_name(name):
    result = str(name)
    if result.startswith("draftsman_"):
        result = result[len("draftsman_"):]
    if len(result) > 4 and result[-4] == "." and result[-3:].isdigit():
        result = result[:-4]
    return result

def manifest_entry_for(obj, index):
    object_id = clean_object_name(obj.name)
    if object_id in manifest_by_id:
        return manifest_by_id[object_id]
    if index < len(manifest_objects):
        return manifest_objects[index]
    return {}

def metadata_value(entry, key, default=""):
    metadata = entry.get("metadata", {})
    value = entry.get(key, metadata.get(key, default))
    return "" if value is None else value

imported = [obj for obj in bpy.data.objects if obj.name not in before_names]
for index, obj in enumerate(imported):
    entry = manifest_entry_for(obj, index)
    tags = entry.get("tags", [])
    role = str(metadata_value(entry, "role"))
    material_name = str(metadata_value(entry, "material"))
    export_group = str(metadata_value(entry, "export_group"))
    target_collection = ensure_child_collection(collection, export_group) if export_group.strip() else collection
    material = ensure_material(material_name, entry)
    for existing_collection in list(obj.users_collection):
        existing_collection.objects.unlink(obj)
    target_collection.objects.link(obj)
    obj["draftsman_id"] = str(entry.get("id", clean_object_name(obj.name)))
    obj["draftsman_type"] = str(entry.get("type", "unknown"))
    obj["draftsman_tags"] = json.dumps(tags)
    obj["draftsman_role"] = role
    obj["draftsman_material"] = material_name
    obj["draftsman_intent"] = str(metadata_value(entry, "intent"))
    obj["draftsman_export_group"] = export_group
    obj["draftsman_manifest_index"] = index
    if material and hasattr(obj.data, "materials"):
        obj.data.materials.clear()
        obj.data.materials.append(material)
    role_prefix = safe_name(role, "")
    obj.name = "draftsman_" + (role_prefix + "_" if role_prefix else "") + obj.name
    obj.location.z = 0.0
    obj.scale = (SCALE, SCALE, SCALE)

manifest_count = manifest.get("object_count", len(imported))
print(f"Imported {len(imported)} SVG object(s) into collection '{COLLECTION_NAME}' from {SVG_PATH}")
print(f"Manifest object count: {manifest_count}")
)PY");
}

QString verifyScript() {
    return QString::fromUtf8(R"PY(#!/usr/bin/env python3
"""Verify a Draftsman Blender SVG export bundle before opening Blender."""

import json
import sys
from collections import Counter
from pathlib import Path

BUNDLE_DIR = Path(__file__).resolve().parent
REQUIRED_FILES = [
    "drawing.svg",
    "manifest.json",
    "export_report.json",
    "import_drawing_svg.py",
]
SUPPORTED_TYPES = {
    "point",
    "line",
    "polyline",
    "circle",
    "arc",
    "rectangle",
    "polygon",
    "image_reference_frame",
    "ascii_crop_frame",
    "ascii_cell_region",
    "tone_probe",
    "glyph_baseline",
}

errors = []
warnings = []

def load_json(name):
    path = BUNDLE_DIR / name
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        errors.append(f"{name} is not readable JSON: {exc}")
        return {}

for filename in REQUIRED_FILES:
    if not (BUNDLE_DIR / filename).exists():
        errors.append(f"missing required file: {filename}")

manifest = load_json("manifest.json") if (BUNDLE_DIR / "manifest.json").exists() else {}
report = load_json("export_report.json") if (BUNDLE_DIR / "export_report.json").exists() else {}

objects = manifest.get("objects", [])
manifest_count = int(manifest.get("object_count", len(objects)) or 0)
report_count = int(report.get("object_count", -1) or 0)
if manifest_count != len(objects):
    errors.append(f"manifest object_count {manifest_count} does not match objects length {len(objects)}")
if report_count != manifest_count:
    errors.append(f"report object_count {report_count} does not match manifest object_count {manifest_count}")

ids = [str(obj.get("id", "")).strip() for obj in objects]
missing_ids = sum(1 for object_id in ids if not object_id)
if missing_ids:
    errors.append(f"{missing_ids} object(s) missing id")

duplicates = sorted(object_id for object_id, count in Counter(ids).items() if object_id and count > 1)
if duplicates:
    errors.append("duplicate object id(s): " + ", ".join(duplicates))

type_counts = Counter(str(obj.get("type", "unknown") or "unknown") for obj in objects)
unknown_types = sorted(object_type for object_type in type_counts if object_type not in SUPPORTED_TYPES)
if unknown_types:
    warnings.append("unknown object type(s): " + ", ".join(unknown_types))

coverage = report.get("metadata_coverage", {})

print("Draftsman Blender SVG Bundle Check")
print(f"Bundle: {BUNDLE_DIR}")
print(f"Objects: {manifest_count}")
print("")
print("Object Types:")
if type_counts:
    for object_type, count in sorted(type_counts.items()):
        print(f"- {object_type}: {count}")
else:
    print("- none")

print("")
print("Metadata Coverage:")
print(f"- role: {coverage.get('with_role', 0)}")
print(f"- material: {coverage.get('with_material', 0)}")
print(f"- export_group: {coverage.get('with_export_group', 0)}")
print(f"- tags: {coverage.get('with_tags', 0)}")
print(f"- missing metadata: {coverage.get('missing_metadata', 0)}")
print(f"- missing ids: {coverage.get('missing_ids', missing_ids)}")

if warnings:
    print("")
    print("Warnings:")
    for warning in warnings:
        print(f"- {warning}")

if errors:
    print("")
    print("Errors:")
    for error in errors:
        print(f"- {error}")
    sys.exit(1)

print("")
print("Bundle structure ok.")
)PY");
}

QString readme() {
    return QString::fromUtf8(R"TXT(Draftsman Blender SVG Bundle

Files:
- drawing.svg: exported Draftsman vector drawing.
- manifest.json: compact object metadata and Blender import hints.
- export_report.json / export_report.txt: export audit.
- import_drawing_svg.py: Blender script that imports drawing.svg as curves.
- verify_bundle.py: local bundle self-check before Blender import.

Imported Blender objects receive Draftsman custom properties:
draftsman_id, draftsman_type, draftsman_tags, draftsman_role, draftsman_material.

If object metadata is present, import_drawing_svg.py also:
- links objects into child collections using export_group.
- creates/assigns materials using material.
- prefixes object names using role.

Run:
python3 verify_bundle.py
blender --python import_drawing_svg.py

Or open Blender, load import_drawing_svg.py in the Text Editor, and run it.
)TXT");
}
} // namespace BlenderSvgBundleTemplates
