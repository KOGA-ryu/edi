# Campaign handoff — blender-lab-20260619-artforge (HELD by hub order)

**Status:** ⏸ HELD at **C1** by hub order (2026-06-19) — top priority moved to the
game-asset library (hub runs that separately). Do NOT proceed to C2+ until the hub
resumes. Branch `dept/blender-lab`, NOT pushed (hub owns origin).

## What the art forge is
Pillar C: turn edi's sacred-geometry/girih patterns into real, tileable, SEAMLESS
textures applied as real Blender materials on zoo assets. Research + full ladder
spec in `docs/architecture/art-forge.md`.

## Decisions locked (research + reviewer-boundary, both GREEN)
- **Rasterizer fork = (b2):** a PURE PIL rasterization of a closed-form pure-Python
  `tile_polys` generator (byte-deterministic, GPU-free). Cycles ortho-render rejected
  (can't be byte-identical for the REPRODUCIBLE/SEAMLESS gates); C++ SVG→raster
  rejected (the C++ drafting core has no tiling math — would be a new kit + a Qt/vendored
  rasterizer; NO drafting sub-campaign flagged). Blender (bpy) is still used for C2+
  (material on a plane, asset renders) — only the flat-texture bake is pure-PIL.
- **Motif:** 8-pointed star-and-cross, wallpaper group p4m, SQUARE translational cell.
- **Generator:** `tools/blender/forge/girih_tile.py` — craftsman-SHAPED (MANIFEST + pure
  `tile_polys` + PIL `bake_png`), reuses `nfold_star._resolve_k` (one owner), NOT scanned
  into the `--list-craftsmen` 3D palette (verified `load_craftsmen` is craftsmen/-only).
- **textureRef bridge (additive, for C2/C3):** a textureRef MAY be a catalog-relative
  image path (`textures/x.png`, resolved vs `--asset-dir` like meshRef) → `_instance_material`
  loads it as an Image-Texture(REPEAT); a bare name still falls back to `GREY_MATERIALS`
  flat color (greybox + existing assets byte-identical). Round-trip through `·`-join
  verified. NOT YET BUILT (C2/C3).

## Progress (commits on dept/blender-lab, NOT pushed)
- research+boundary doc `43b01e5` → amended decisions in `art-forge.md`.
- **C0** (RASTERIZE) `38d3b0f` — `girih_tile` generator + pure-PIL bake → `samples/textures/girih_floor.png`
  (gold 8/3 octagram on indigo ground, azure corner crosses, p4 symmetry). Gate
  FIDELITY-lite + crispness + reproducible PASS (1 refine: first bake mis-placed stars at
  corners → fixed to centered star per the doc). Green 117/117. **← hub's requested hold point.**
- **C1** (SEAMLESS) `cabd0ca` — edge-match gate (ε=0, half-open `[0,cell)` pixel convention)
  + 3×3 proof `samples/textures/girih_floor_3x3.png` (no seam; corner quarter-crosses
  complete into full crosses at 4-tile nodes). Green 117/117. **← branch HEAD; landed green
  in-flight just as the hold arrived.**

**Merge boundary is the hub's call:** FF to C0 (`38d3b0f`) for the exact requested hold,
or include C1 (`cabd0ca`) — both are clean, green, self-contained stopping points.

## Remaining ladder (PENDING — resume here)
- **C2** MATERIAL — PNG → Principled Base Color via Image Texture (REPEAT) on a test plane;
  extend `_instance_material` with the image-path branch + flat-color fallback. Gate PIPELINE-FIT.
- **C3** ON-AN-ASSET (payoff-1) — textureRef image path on a real asset (tree bark/leaf or a
  wall); slot-override reuse. Gate READS-AT-SCALE.
- **C4** DEPTH — normal/roughness map (line-proximity height → Bump); more motifs. Gate CRISPNESS/depth.
- **C5** PAYOFF — the textured WALL (or tree with real bark+leaf), rendered. Finishes the one-wall slice.

Rubric (priority): SEAMLESS > FIDELITY > CRISPNESS > READS-AT-SCALE > REPRODUCIBLE >
PIPELINE-FIT — mostly OFFLINE-computable via PIL (see `art-forge.md` §6, §9 pinned thresholds).
