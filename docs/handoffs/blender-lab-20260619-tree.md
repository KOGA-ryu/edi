# Campaign handoff — blender-lab-20260619-tree (CLOSEOUT)

**Order:** build a TREE — the zoo's first ORGANIC, real-content asset — researched,
built, **critiqued and refined** up a detail ladder via a priority rubric, taken
through the proven realize loop to a **forest payoff** (build-once-place-many with
seed variation).

**Branch:** `dept/blender-lab` · **Baseline:** master `2751fd6`. Green gate
**116/116** + scan throughout. NO push (hub owns the origin bridge).

## ✅ CAMPAIGN COMPLETE
A procedural, **parametric + seeded** TREE craftsman that rides the proven loop:
forge → bake `.blend` → curate `AssetRecord` → TOON manifest → **INSTANCE** → render.
The forest payoff render shows **20 instances from 3 build-once tree datablocks** with
seed/param variety, lit, on OptiX/RTX-5090.

## Technique (research: `docs/architecture/tree-asset.md`)
**Recursive parametric self-similar branching, seeded** (Weber-Penn lineage) — the
ONLY candidate fitting the craftsman's dual-tier contract: the WHOLE mesh is a
closed-form pure-Python `(verts, faces)` function of named params + `seed`, so
`proof_mesh` (the OBJ/proof tier) and `build` (bpy `from_pydata`) run the SAME
generator. L-systems ruled out (productions are strings, unrepresentable in the
number/integer/material param limit); space-colonization out (not dial-able, cost,
fiddly determinism); sapling addon not available headless; geo-nodes can't do a pure
proof. Pipe model / da Vinci rule for radius thinning; golden-angle phyllotaxis.

## The detail ladder + critique loop (each level: build → render → adversarial critique → refine until the gate passes → advance)
- **L0 FORM** (`e419bb4`) — tapered trunk + crown blob. Gate SILHOUETTE A1+A4. PASS.
- **L1 ARMATURE** (`a091bd4`) — recursive branching skeleton. Gate BRANCHING C1+C4. PASS.
  Critique caught an undisclosed height overshoot (2.57×) → L2.
- **L2 STRUCTURE** (`0cf49ec`) — pipe-model thinning + taper + curve + distributed
  primaries. Gate C2+C3+B2+B3+budget. **2 refine rounds:** the uniform "safety fit"
  was crushing the trunk 56% + the smoke asserted the pre-fit ratio (dishonest gate)
  → position-only fit + final-mesh B2 assert; reviewer REFUTED "can't fix crown width"
  with a param sweep → `lengthRatio` 0.72→0.56 (collateral-free). PASS.
- **L3 CANOPY** (`8b15bd2`) — clumped foliage at outer tips. Gate D1+D2+D4. **1 refine:**
  first pass read as sparse dots (fill 0.045) — D2 had only an upper bound → added a
  LOWER bound (0.15≤fill<0.6) + denser canopy (fill 0.27). PASS.
- **L4 BARK/BASE** (`620bfc9`) — root flare/buttress + healthy crown ratio + bark/leaf
  material split (provably-exact face partition). Gate PROPORTION B1+B4. PASS.
- **L5a VARIATION** (`c66f35c`) — seeded jitter with strict determinism (ONE RNG,
  fixed DFS order, draw-before-clip). Gate E1+E2+E3: same-seed byte-identical, seeds
  differ, no mirror. PASS (reviewer RAN the determinism checks).
- **L5b FOREST PAYOFF** (`cc0dd58`) — 3 seeded variants (full/sparse-tall/full-round)
  curated as `asset_0007/8/9`, a 20-tree forest map, realize-instance render. Build-once
  confirmed (3 datablocks, 20 instances). Reviewer caught a green-gate miss (stale
  manifest-roster pin in `edi_realize_smoke` after the manifest grew 6→9) → FIXED
  (subset check + tree-row textures decode assertion). PASS, 116/116.

## Artifacts
- Craftsman: `tools/blender/craftsmen/tree.py` (24-param MANIFEST; pure `proof_mesh`;
  bpy `build` = proof_mesh + from_pydata + 2-slot bark/leaf material assignment).
- Preview harness: `tools/blender/preview_tree.py` (multi-angle OptiX renders, `--seed=`).
- Recipes/bakes: `samples/zoo/recipes/tree_{a,b,c}_ops_compiled.toml`,
  `samples/zoo/meshes/tree_{a,b,c}.blend`. Per-level renders `samples/tree/tree_L*`.
- Zoo: `testerAssetCatalog()` now `assets[9]` (6 shapes + 3 trees); fixtures re-minted.
- **Payoff render:** `samples/zoo/forest_demo.{png,toon,render.log}` — 20 instances, OptiX, 3.5s.

## Boundary / discipline held
New craftsman is purely ADDITIVE (a generic `ScriptOp`; no C++ edit, no core touches
the forge). No JSON; TOON manifest; greybox fallback intact (crypt `instances=0`
unchanged). Dual-tier parity + same-seed determinism are the load-bearing invariants.

## Follow-ups (non-blocking, logged for backlog)
1. **Foliage GREEN is realizer-appearance**, not baked: the forest's brown bark / green
   canopy come from `edi_realize`'s `GREY_MATERIALS` (`bark_brown`/`leaf_green`)
   overriding the baked slots per the AssetRecord `textureRefs`. The baked `.blend`
   slots are `aged_stone`/`sandstone` (existing keys — additive). Real baked foliage
   materials await the **art-forge pillar**.
2. **Bake-smoke gap:** `build` is `# pragma: no cover`, so no automated check that a
   baked `.blend` has 1 mesh + 2 slots + base z=0. L5b's first real bake surfaced a
   latent `import edi_craft` bug in `tree.build` (fixed) — a bpy-gated bake smoke would
   have caught it. Backlog a bake smoke for craftsman `build` paths.
3. **Forest framing:** the realizer's shared auto-camera frames a large flat map nearly
   top-down. Acceptable for the payoff; a configurable lower hero angle is backlog (the
   camera is shared infra — changing it risks the crypt-regression invariant).

## Commit chain (dept/blender-lab, NOT pushed)
research `8313f14` · L0 `e419bb4` · L1 `a091bd4` · L2 `0cf49ec` · L3 `8b15bd2` ·
L4 `620bfc9` · L5a `c66f35c` · L5b `cc0dd58`.
