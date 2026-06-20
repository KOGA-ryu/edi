#!/usr/bin/env python3
"""ctest smoke for the REAL bpy bake path — closes the proof_mesh<->Blender drift gap.

Every craftsman ``build()`` and the realizer's ``build_scene``/``render``/``_bake_asset``
are ``# pragma: no cover`` because they need Blender, so nothing in ctest ever
proved the bpy mesh matches the pure ``proof_mesh`` the OBJ/ASCII proof renders.
This test drives a REAL headless Blender to build a couple of recipes through
``edi_craft.build()`` and asserts the bpy result == ``proof_mesh`` exactly.

GUARDED like girih_tile_smoke's PIL tier: if Blender is absent (no ``blender`` on
PATH and no ``BLENDER`` override) this prints a clear SKIP and exits 0, so ctest
stays green on a Blender-less CI box. Blender IS present on the lab box, so here
it runs the real bake.

For each recipe we bake we assert, against the bpy mesh datablock:
  - > 0 verts and > 0 faces;
  - a SANE bbox — finite, non-zero extent on every axis, base near z=0
    (origin-at-base, the asset contract);
  - the bpy vert/face count MATCHES proof_mesh's EXACTLY. A craftsman's
    ``build`` is ``proof_mesh`` + ``mesh.from_pydata`` with NO destructive
    modifier (the soft-bevel is non-destructive and is not even added to a
    Script object), so the count is exact — no tolerance. If a future build()
    bakes a modifier in, switch to dimensions and a stated tolerance.

We also exercise ``--asset-out`` so the ``_bake_asset`` (``libraries.write``)
``# pragma: no cover`` path runs and produces a non-empty .blend library.

D10 also named the CRYPT REALIZER, whose ``build_scene`` (tools/blender/
edi_realize.py) is the other half of the bpy-coverage gap and was likewise
``# pragma: no cover``. ``build_scene`` only CONSTRUCTS the bpy scene
(meshes/objects/lights, returns a bbox); the GPU-only ``setup_optix``/``render``
are separate and stay out of CI scope. So we ALSO drive the realizer's pure ->
bpy path over the crypt greybox HEADLESS (no render): ``parse_toon`` the map ->
``_resolve_dims`` -> ``plan_greybox`` (the crypt has no ``assets`` section, so
``instances=0`` and no ``--asset-dir``/.blend loading is needed) ->
``build_scene``. We assert it returns a finite, non-degenerate bbox sitting near
the floor, that the scene gained > 0 mesh objects with > 0 total verts, and —
the realizer's OWN invariant — that the bpy mesh-object count == the number of
non-light pieces in the plan (one bpy object per non-light piece). This proves
the realizer's bpy scene-construction matches its pure plan; render/setup_optix
stay GPU-only.
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile

# Keep ctest from littering __pycache__ into the source tree.
sys.dont_write_bytecode = True

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "tools", "blender"))

import edi_craft  # noqa: E402

# Recipes to bake. Each enters a DIFFERENT bpy code arm: a seeded container
# (the batch-1 game asset) and the organic tree (the recursive/foliage build).
# Both go through Script -> module.build -> proof_mesh + from_pydata.
RECIPES = [
    os.path.join(ROOT, "samples", "zoo", "recipes", "barrel_ops_compiled.toml"),
    os.path.join(ROOT, "samples", "tree", "tree_ops_compiled.toml"),
]

# The crypt greybox map the realizer half of the smoke builds through
# build_scene. It has NO assets section (instances=0), so the pure plan needs no
# --asset-dir / .blend loading — build_scene runs headless, no GPU.
CRYPT_MAP = os.path.join(ROOT, "samples", "crypt_m0", "crypt.toon")

# The probe RUNS INSIDE Blender: it imports edi_craft, builds the ops, and dumps
# each built MESH object's vert/face count + bbox as one JSON line per object
# (prefixed so we can pluck it out of Blender's chatty stdout).
PROBE = r'''
import json, os, sys
# The probe lives in a tmp dir, so it cannot find edi_craft from its own path:
# the ROOT is handed in as the first arg after "--" (cwd is also ROOT, but be
# explicit so the probe is robust to how Blender is launched).
sys.dont_write_bytecode = True
argv = sys.argv[sys.argv.index("--") + 1:]
root, recipe = argv[0], argv[1]
sys.path.insert(0, os.path.join(root, "tools", "blender"))
import bpy
import edi_craft

ops = edi_craft.parse_ops(recipe)
edi_craft.build(ops)
for obj in bpy.data.objects:
    if obj.type != "MESH":
        continue
    me = obj.data
    xs = [v.co.x for v in me.vertices]
    ys = [v.co.y for v in me.vertices]
    zs = [v.co.z for v in me.vertices]
    rec = {
        "name": obj.name,
        "verts": len(me.vertices),
        "faces": len(me.polygons),
        "bbox": [min(xs), min(ys), min(zs), max(xs), max(ys), max(zs)] if xs else None,
    }
    print("EDIBAKE " + json.dumps(rec))
'''


# The REALIZER probe: runs INSIDE Blender, drives the realizer's pure -> bpy
# scene-construction path (NO render — setup_optix/render need the GPU) over the
# crypt greybox, and dumps the bbox + the mesh-object/vert counts + the plan's
# non-light-piece count as one JSON line so the parent can assert the invariant.
REALIZE_PROBE = r'''
import json, os, sys
sys.dont_write_bytecode = True
argv = sys.argv[sys.argv.index("--") + 1:]
root, mapping = argv[0], argv[1]
sys.path.insert(0, os.path.join(root, "tools", "blender"))
import bpy
import edi_realize

with open(mapping, "r", encoding="utf-8") as fh:
    doc = edi_realize.parse_toon(fh.read())
# The crypt carries no scale meta and no assets section: S=1, instances=0.
dims = edi_realize._resolve_dims(doc, doc.scale)
pieces = edi_realize.plan_greybox(doc, dims)
# The realizer's invariant: build_scene builds ONE bpy object per NON-LIGHT
# piece (lights become light objects, counted apart). Compute the expected
# mesh-object count from the pure plan before touching bpy.
non_light = sum(1 for p in pieces if not p.is_light)
# build_scene CONSTRUCTS the scene (clears defaults, builds primitives/lights,
# returns the world bbox). It does NOT render.
bb = edi_realize.build_scene(bpy, pieces, dims)
mesh_objs = [o for o in bpy.data.objects if o.type == "MESH"]
total_verts = sum(len(o.data.vertices) for o in mesh_objs)
rec = {
    "bbox": list(bb),
    "mesh_objects": len(mesh_objs),
    "total_verts": total_verts,
    "non_light_pieces": non_light,
    "total_pieces": len(pieces),
}
print("EDIREALIZE " + json.dumps(rec))
'''


def _blender_exe():
    return os.environ.get("BLENDER") or shutil.which("blender")


def _run_probe(blender, recipe, probe_path):
    """Drive headless Blender over `recipe`, return {name: rec} from EDIBAKE lines."""
    proc = subprocess.run(
        [blender, "--background", "--factory-startup", "--python", probe_path,
         "--", ROOT, recipe],
        cwd=ROOT, capture_output=True, text=True, timeout=300,
    )
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        raise AssertionError(f"blender bake of {recipe} exited {proc.returncode}")
    out = {}
    for line in proc.stdout.splitlines():
        if line.startswith("EDIBAKE "):
            rec = json.loads(line[len("EDIBAKE "):])
            out[rec["name"]] = rec
    return out, proc.stdout


def _run_realize_probe(blender, mapping, probe_path):
    """Drive headless Blender over the realizer's build_scene (no render) for
    `mapping`; return the single EDIREALIZE record."""
    proc = subprocess.run(
        [blender, "--background", "--factory-startup", "--python", probe_path,
         "--", ROOT, mapping],
        cwd=ROOT, capture_output=True, text=True, timeout=300,
    )
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        raise AssertionError(f"blender build_scene of {mapping} exited {proc.returncode}")
    for line in proc.stdout.splitlines():
        if line.startswith("EDIREALIZE "):
            return json.loads(line[len("EDIREALIZE "):])
    sys.stderr.write(proc.stdout)
    raise AssertionError(f"realizer probe over {mapping} produced no EDIREALIZE line")


def main() -> int:
    blender = _blender_exe()
    if not blender:
        print("SKIP: blender not found (set BLENDER=/path/to/blender to run the real bake)")
        return 0

    tmp = tempfile.mkdtemp(prefix="edi_bpy_bake_")
    probe_path = os.path.join(tmp, "probe.py")
    realize_probe_path = os.path.join(tmp, "realize_probe.py")
    try:
        with open(probe_path, "w") as f:
            f.write(PROBE)
        with open(realize_probe_path, "w") as f:
            f.write(REALIZE_PROBE)

        craftsmen = edi_craft.load_craftsmen(edi_craft.default_craftsmen_dir())
        for recipe in RECIPES:
            ops = edi_craft.parse_ops(recipe)
            # proof_mesh truth (pure, no Blender): name -> (verts, faces).
            proof = {name: (v, f) for name, v, f in edi_craft.obj_objects(ops, craftsmen)}
            assert proof, f"{recipe}: no proof objects to compare against"

            baked, stdout = _run_probe(blender, recipe, probe_path)
            print(f"baked {os.path.basename(recipe)}: "
                  f"{', '.join(sorted(baked))} (objects from real bpy)")

            for name, (pv, pf) in proof.items():
                assert name in baked, \
                    f"{recipe}: bpy did not produce an object named {name!r} (got {sorted(baked)})"
                rec = baked[name]
                # > 0 verts and faces.
                assert rec["verts"] > 0 and rec["faces"] > 0, \
                    f"{name}: bpy mesh is empty (v={rec['verts']} f={rec['faces']})"
                # SANE bbox: finite, non-zero extent every axis, base near z=0.
                bb = rec["bbox"]
                assert bb is not None and all(map(_finite, bb)), \
                    f"{name}: non-finite bbox {bb}"
                x0, y0, z0, x1, y1, z1 = bb
                assert x1 > x0 and y1 > y0 and z1 > z0, \
                    f"{name}: degenerate bbox extent {bb}"
                assert abs(z0) < 1e-4, \
                    f"{name}: base must sit at z=0 (origin-at-base), got z-min {z0}"
                # EXACT count match: build = proof_mesh + from_pydata, no baked modifier.
                assert rec["verts"] == len(pv), \
                    f"{name}: bpy verts {rec['verts']} != proof_mesh {len(pv)}"
                assert rec["faces"] == len(pf), \
                    f"{name}: bpy faces {rec['faces']} != proof_mesh {len(pf)}"
                print(f"  {name}: bpy verts/faces = {rec['verts']}/{rec['faces']} "
                      f"== proof_mesh {len(pv)}/{len(pf)} (EXACT)")

        # --asset-out: run the _bake_asset (libraries.write) path so the
        # build-once .blend artifact bake is no longer wholly uncovered.
        asset_recipe = RECIPES[0]
        asset_out = os.path.join(tmp, "barrel.blend")
        proc = subprocess.run(
            [blender, "--background", "--factory-startup", "--python",
             os.path.join(ROOT, "tools", "blender", "edi_craft.py"),
             "--", asset_recipe, f"--asset-out={asset_out}"],
            cwd=ROOT, capture_output=True, text=True, timeout=300,
        )
        if proc.returncode != 0:
            sys.stderr.write(proc.stdout)
            sys.stderr.write(proc.stderr)
            raise AssertionError(f"--asset-out bake exited {proc.returncode}")
        assert os.path.exists(asset_out) and os.path.getsize(asset_out) > 0, \
            "_bake_asset wrote no .blend library"
        print(f"baked asset library {os.path.basename(asset_out)} "
              f"({os.path.getsize(asset_out)} bytes) via _bake_asset")

        # ---- REALIZER half: build_scene over the crypt greybox (NO render) ----
        # This exercises edi_realize.build_scene's bpy scene-construction path
        # headless (setup_optix/render stay GPU-only). The crypt has instances=0,
        # so no --asset-dir/.blend loading is needed.
        rec = _run_realize_probe(blender, CRYPT_MAP, realize_probe_path)
        bb = rec["bbox"]
        # A finite, non-degenerate bbox (all 6 components finite, real extent).
        assert len(bb) == 6 and all(map(_finite, bb)), \
            f"build_scene returned a non-finite bbox {bb}"
        x0, y0, z0, x1, y1, z1 = bb
        assert max(x1 - x0, y1 - y0, z1 - z0) > 0.0, \
            f"build_scene bbox is degenerate (zero extent) {bb}"
        # The greybox base (floor slab) sits near z=0 — origin-at-floor, sane.
        assert -1.0 < z0 < 1.0, \
            f"build_scene bbox base must sit near the floor, got z-min {z0}"
        # > 0 mesh objects with > 0 total verts.
        assert rec["mesh_objects"] > 0, "build_scene produced no mesh objects"
        assert rec["total_verts"] > 0, "build_scene mesh objects have no verts"
        # The realizer's OWN invariant: one bpy mesh object per NON-LIGHT piece.
        assert rec["mesh_objects"] == rec["non_light_pieces"], (
            f"build_scene built {rec['mesh_objects']} mesh objects but the plan "
            f"has {rec['non_light_pieces']} non-light pieces "
            f"(of {rec['total_pieces']} total) — the one-object-per-piece "
            f"invariant broke")
        print(f"realized {os.path.basename(CRYPT_MAP)}: build_scene -> "
              f"{rec['mesh_objects']} mesh objects, {rec['total_verts']} verts "
              f"== {rec['non_light_pieces']} non-light pieces "
              f"(of {rec['total_pieces']} plan pieces), bbox z-min {z0:.4f} "
              f"(real headless bpy, NO render)")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("bpy_bake_smoke: OK (real headless Blender bake == proof_mesh)")
    return 0


def _finite(x):
    return x == x and x not in (float("inf"), float("-inf"))


if __name__ == "__main__":
    sys.exit(main())
