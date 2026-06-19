"""PREVIEW-RENDER HARNESS — the reusable multi-angle renderer for the tree
campaign (and any craftsman recipe). Renders an asset from several angles so the
reviewer can score the SILHOUETTE rubric (A1-A4) and canopy/asymmetry dimensions
from real images, not just the offline OBJ math.

Usage (inside Blender):
    blender --background --python tools/blender/preview_tree.py -- \\
        <recipe.toml> --out=<prefix> [--samples=N] [--angles=front,threequarter,top]

It is reused at EVERY ladder level (L0..L5): the harness is generic — it parses
the recipe with edi_craft.parse_ops, builds each op via its craftsman's build()
(the SAME bpy twin the asset bake uses), then frames a camera to the mesh bbox
per requested angle and renders <prefix>_<angle>.png.

Rendering policy mirrors edi_realize: Cycles + OptiX GPU, and a HARD REFUSAL of
the silent CPU fallback (edi_realize.setup_optix raises if no GPU is found). A low
--samples default (24) keeps iteration fast; the milestone render uses 64.

This is a bpy script — it is NOT covered by ctest (no Blender there). The
Blender-free half (the tree generator) is covered in tests/edi_craft_smoke.py.
"""

import math
import os
import sys

# Make edi_craft / edi_realize importable when run via `blender --python`.
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import edi_craft       # noqa: E402
import edi_realize     # noqa: E402


# Each angle is a unit direction the camera looks FROM (toward the bbox centre),
# expressed as (azimuth_deg around +z, elevation_deg above the horizon). Front is
# a level elevation looking along -y; threequarter orbits 45° and lifts; top looks
# straight down. The camera distance is derived from the bbox so the whole asset
# frames regardless of size.
ANGLE_DIRS = {
    # name:        (azimuth_deg, elevation_deg)
    "front":        (0.0, 8.0),     # near-level elevation — the silhouette test
    "threequarter": (45.0, 25.0),   # orbit + lift — reads branch depth (L1+)
    "top":          (0.0, 89.0),    # plan view — canopy gaps (L3+)
}


def _arg(argv, prefix, default=None):
    return next((a.split("=", 1)[1] for a in argv if a.startswith(prefix)), default)


def _bbox(bpy):
    """World-space bbox of every MESH object in the scene (the built recipe)."""
    xs, ys, zs = [], [], []
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue
        for corner in obj.bound_box:
            wx, wy, wz = obj.matrix_world @ _vec(bpy, corner)
            xs.append(wx); ys.append(wy); zs.append(wz)
    if not xs:
        raise RuntimeError("no MESH objects in scene — nothing to frame")
    return (min(xs), max(xs), min(ys), max(ys), min(zs), max(zs))


def _vec(bpy, triple):
    from mathutils import Vector
    return Vector((triple[0], triple[1], triple[2]))


def _place_camera(bpy, name, centre, span, azimuth_deg, elevation_deg):
    """Frame an ORTHO camera at (azimuth, elevation) looking at `centre`. ORTHO
    keeps the silhouette honest (no perspective foreshortening of the trunk)."""
    from mathutils import Vector
    az = math.radians(azimuth_deg)
    el = math.radians(elevation_deg)
    # Direction FROM the centre TO the camera (unit), then push out by ~2*span.
    dir_x = math.cos(el) * math.sin(az)
    dir_y = -math.cos(el) * math.cos(az)
    dir_z = math.sin(el)
    dist = span * 2.0
    loc = Vector((centre[0] + dir_x * dist,
                  centre[1] + dir_y * dist,
                  centre[2] + dir_z * dist))

    cam_data = bpy.data.cameras.new(name)
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = span * 1.25  # a little margin around the asset
    cam_data.clip_start = max(0.001, span * 0.001)
    cam_data.clip_end = max(100.0, dist * 4.0)
    cam = bpy.data.objects.new(name, cam_data)
    bpy.context.collection.objects.link(cam)
    cam.location = loc
    # Point the camera at the centre: -Z forward, +Y up (Blender camera convention).
    direction = Vector(centre) - loc
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return cam


def main(argv):
    import bpy

    recipe = next((a for a in argv if not a.startswith("--")), None)
    out_prefix = _arg(argv, "--out=")
    if not recipe or not out_prefix:
        sys.stderr.write(
            "usage: blender --background --python preview_tree.py -- "
            "<recipe.toml> --out=<prefix> [--samples=N] [--angles=front,threequarter,top]\n")
        return 2
    samples = int(_arg(argv, "--samples=", "24"))
    angles_arg = _arg(argv, "--angles=", "front,threequarter,top")
    angles = [a.strip() for a in angles_arg.split(",") if a.strip()]
    for a in angles:
        if a not in ANGLE_DIRS:
            sys.stderr.write(f"unknown angle {a!r}; known: {', '.join(ANGLE_DIRS)}\n")
            return 2

    # Fresh scene: drop the default cube/camera/light so they don't pollute the
    # bbox or the silhouette.
    bpy.ops.wm.read_factory_settings(use_empty=True)

    # Build the recipe through each op's craftsman build() (the bpy twin). We call
    # the craftsmen directly rather than edi_craft.build so the scene carries ONLY
    # the asset meshes — no preview_sun/preview_camera rig to confuse the bbox.
    ops = edi_craft.parse_ops(recipe)
    custom = edi_craft.load_craftsmen(edi_craft.default_craftsmen_dir())
    builders = None  # built-in arch ops are not needed for the craftsman previews
    built = 0
    for op in ops:
        if op["type"] == "Script":
            module = custom.get(op["script"])
            if module is None or not hasattr(module, "build"):
                sys.stderr.write(f"unknown craftsman {op['script']!r}\n")
                return 3
            module.build(op)
            built += 1
        else:
            # Built-in architectural ops: defer to edi_craft's builder table by
            # building the WHOLE stream once (rare for the tree campaign, but keeps
            # the harness general). Only reached for non-Script recipes.
            if builders is None:
                edi_craft.build(ops)
                built = len(ops)
            break
    if built == 0:
        sys.stderr.write("recipe built no geometry\n")
        return 3

    # A ground plane at z=0 so the trunk base reads as planted, plus a neutral
    # mid-grey world so the silhouette is legible from every angle.
    x0, x1, y0, y1, z0, z1 = _bbox(bpy)
    span = max(x1 - x0, y1 - y0, z1 - z0, 1e-3)
    centre = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0)

    bpy.ops.mesh.primitive_plane_add(size=span * 6.0, location=(centre[0], centre[1], 0.0))

    world = bpy.data.worlds.new("preview_world")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg is not None:
        bg.inputs[0].default_value = (0.5, 0.5, 0.5, 1.0)  # neutral grey backdrop
        bg.inputs[1].default_value = 0.6
    bpy.context.scene.world = world

    # A sun for form (matches edi_craft's preview rig direction).
    bpy.ops.object.light_add(
        type="SUN", location=(centre[0] - span, centre[1] - span, z1 + span))
    sun = bpy.context.object
    sun.data.energy = 4.0
    sun.rotation_euler = (math.radians(50), 0.0, math.radians(-35))

    # Cycles + OptiX GPU; refuse silent CPU fallback (setup_optix raises).
    chosen, devreport = edi_realize.setup_optix(bpy, samples, edi_realize.DEFAULT_DIMS)
    gpu_enabled = [n for (n, t, e) in devreport if e and t in ("OPTIX", "CUDA")]
    print("=" * 64)
    print("preview_tree — multi-angle craftsman render")
    print(f"recipe: {recipe}   built {built} op(s)")
    print(f"render engine: CYCLES   compute_device_type: {chosen}   cycles.device: GPU")
    print(f"samples: {samples}")
    for (dn, dt, en) in devreport:
        print(f"  [{'X' if en else ' '}] {dt:6s} {dn}")
    if not gpu_enabled:
        sys.stderr.write("FATAL: no GPU device enabled — would fall back to CPU\n")
        return 3
    print(f"GPU CONFIRMED: rendering on {chosen} device(s): {gpu_enabled}")
    print("=" * 64)

    scene = bpy.context.scene
    scene.render.image_settings.file_format = "PNG"
    out_dir = os.path.dirname(os.path.abspath(out_prefix))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    outputs = []
    for name in angles:
        az, el = ANGLE_DIRS[name]
        cam = _place_camera(bpy, f"preview_cam_{name}", centre, span, az, el)
        scene.camera = cam
        out_png = f"{out_prefix}_{name}.png"
        scene.render.filepath = out_png
        bpy.ops.render.render(write_still=True)
        outputs.append(out_png)
        print(f"rendered {name}: {out_png}")

    print("=" * 64)
    print(f"device confirmed: {chosen}  GPU: {gpu_enabled}")
    for o in outputs:
        print(f"  out: {o}")
    print("=" * 64)
    return 0


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    sys.exit(main(argv))
