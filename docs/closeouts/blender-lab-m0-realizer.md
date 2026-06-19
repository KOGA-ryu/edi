# Closeout — M0 REALIZER boundary (Seam A's half of the crypt hardware gate)

Campaign: `blender-lab-20260618-m0-realizer`. Frozen 2026-06-18. This freezes
WHERE the realizer lives and WHAT contract it holds, so future M-milestone work
does not re-litigate it.

## What shipped
`tools/blender/edi_realize.py` — a standalone bpy REALIZER. Reads a Seam-B TOON
map, resolves socket + asset_ref to a GREYBOX primitive per piece type, snaps on
the 5 ft grid, sets the brazier block as a light, renders Cycles OptiX. Plus
`samples/crypt_m0/crypt.toon` (fixture), `tests/edi_realize_smoke.py` (ctest,
pure tier), and the edi_craft bpy seam re-verified on Blender 4.5.9.

## Gate result — PASS (all four criteria)
1. Valid 1920×1080 PNG of the assembled crypt (2 rooms + L-corridor + 10
   structural piece types + sarcophagus + brazier + brazier light) —
   `samples/crypt_m0/crypt.png`.
2. Rendered on the RTX 5090 via Cycles OptiX, GPU CONFIRMED in the log (no CPU
   fallback — `setup_optix` raises rather than fall back) —
   `samples/crypt_m0/render.log`.
3. 3.4 s (< 120 s), ~1.5 GB peak (< 32 GB).
4. One command: `blender --background --python tools/blender/edi_realize.py --
   <map.toon> --render=<png>`.

## Frozen boundaries (do not re-litigate)
- **Two-tier file** (pure parse/plan + OBJ proof; bpy build/render behind
  `import bpy`), mirroring `tools/blender/edi_craft.py`. The pure tier is where
  CI + the reviewer assert correctness without a GPU. Keep it that way.
- **The realizer reads the MAP TOON, not the recipe TOON.** `parse_toon` is a
  distinct reader from `edi_craft.parse_ops`. Its authority is the C++ writer
  `src/io/MapToonExport.cpp`; column lookup is BY NAME (`cols.index`), so adding
  a trailing column to the writer cannot misread older maps by position.
- **Socket table = the 10 structural piece types + the props**, named
  `<theme>.<piece>`. A real artist mesh swaps in BY asset_ref without touching
  the plan. The greybox primitives are placeholders only.
- **OptiX setup refuses the silent CPU fallback** and logs every device — the
  "GPU CONFIRMED" line is evidence, by design.

## Known greybox limits (acknowledged, NOT bugs — future milestones)
- `_route_corridor` is a naive single-bend L between edge-midpoint anchors; it
  assumes mutually-facing plugs and can overlap room interiors otherwise.
  Obstacle-aware corridor ROUTING is dungeon-map's Seam B/C domain — the realizer
  greyboxes whatever route it is handed.
- The "10 piece types" claim is fixture-specific: the socket table COVERS 10; the
  crypt fixture is purpose-built to YIELD 10; stock maps yield 8 (`corridor_l` +
  `stair` are layout-specific). Pinned in the smoke test.
- The ceiling is instantiated (seam resolves it) but `hide_render=True` for the
  dollhouse cutaway shot. A faint world ambient is fill; the brazier is the sole
  light OBJECT.

## Contract sync
Built against the SPEC socket contract — dungeon-map had not yet published a
reviewer-gate contract. When dungeon-map emits the real crypt TOON, drop it in
place of `samples/crypt_m0/crypt.toon`; the parser already tolerates the live
writer's exact byte-shape (quoting, `·`-joined flags U+00B7, optional flags
column + blocks section).

## Integration state
dept/blender-lab rebased onto master (@03b8cc1), edi-gate GREEN (build + 105/105
ctest + scan). Master FF-merge is the edi-ui integration hub's lane
(`ui-20260618-m0-integration`); the LEDGER row signals the green tip is READY.
</content>
