# Campaign handoff — blender-lab-20260619-realize

**Pillar A/E (REALIZE):** make the zoo catalog produce **instanced** renders —
"build once, place many." Replace (incrementally) the greybox-regen realizer with
one that resolves each placement's `assetRef` → catalog `meshRef` and **instances**
the built mesh at the placement transform + applies `textureRefs`.

**Branch:** `dept/blender-lab` · **Baseline:** merged LOCAL master `c842554`
(`edi_zoo_core` + `edi_asset_link` + `AssetZooStore` present); post-merge green
gate **114/114** + scan clean. NO push (hub owns the origin bridge).

## Inputs that just landed (master c842554)
- `edi_zoo_core` (`src/zoo/`): `AssetRecord{id,name,category,meshRef,proxyRef,
  curated,textureRefs,sockets}` + `AssetZoo` + pure ops (`mintAssetId`, `addAsset`,
  `curateAsset`, `findAsset`, `addSocket`…) + MessagePack codec.
- `AssetZooStore` (`src/io/`): `saveAssetZoo` / `loadAssetZoo` / `defaultAssetZooPath`.
- `edi_asset_link` (`src/assetlink/`): `resolveAssetRef`, `collectDocumentAssetRefs`,
  `validateDocumentAssetRefs` — the C++ bridge from a document's assetRefs to records.

## Current realizer (what we're changing) — `tools/blender/edi_realize.py`
Reads a Seam-B/C **TOON map** (rooms/plugs/connections/nodes/blocks), and for every
piece **regenerates** a greybox primitive from a realizer-owned `GreyboxDims` table.
Blocks carry `asset = "<theme>.<piece>"` (e.g. `crypt.sarcophagus`) — a piece-type
label, **not** a zoo `AssetId`. Two tiers: PURE (`parse_toon`+`plan_greybox`+OBJ
proof, smoke-tested) and BLENDER (`build_scene` builds primitives, Cycles OptiX).

## The architecture gap this campaign closes
The realizer is **Python/TOON**; the zoo is **C++/MessagePack**. For instancing, the
realizer needs, per placement `assetRef`: the `meshRef` (what to link), `textureRefs`,
and `category`. Two bridges, both at a HIGHER layer (boundary law — neither core
depends on the other):
1. **Catalog → realizer:** a neutral **TOON zoo-manifest** export (mirrors
   `MapToonExport`), so Python stays TOON-only. (Reading MessagePack in Python is the
   rejected alternative.)
2. **Built mesh → meshRef:** the forge (`edi_craft.py`) must **bake the mesh ONCE**
   to a linkable artifact file = `meshRef`. Today edi_craft only builds in-memory +
   an OBJ proof; it has no "save artifact" step.

The Blender **instancing technique** (shared datablock so N placements don't
duplicate the mesh — the literal "build once") and the **artifact format**
(.blend library link vs OBJ import) are genuine bpy unknowns → research gate first.

## Sequencing: RESEARCH → R1 → R2
R1 first (the brief's recommendation) so realize has real data to instance — but a
single **research pass precedes R1**, because R1's `meshRef` artifact format is
decided by what R2's realizer can instance. One research pass de-risks both.

### S0 — RESEARCH (researcher gate, no code)
Resolve: (a) bpy instancing technique that SHARES one mesh datablock across N
placements (collection-instance vs linked-duplicate vs `data.libraries.load` append/
link) + cost; (b) the `meshRef` artifact format the forge bakes and the realizer
links (.blend vs OBJ — instancing vs copy); (c) confirm the **TOON zoo-manifest**
bridge shape (assetRef→meshRef,category,textureRefs,sockets) and that it respects the
boundary. Output: a design doc under `docs/`. **Dispatched first.**

### S0 OUTCOME (research landed) — `docs/architecture/realize-instancing.md`
- **Q1 instancing:** share ONE mesh datablock — `bpy.data.objects.new(name, mesh)`
  per placement (linked-duplicate at the data level); per-instance transform on
  `obj.location/rotation_euler/obj.scale` — **never** `transform_apply(scale=True)`
  on shared data (corrupts every instance). Per-instance material via a slot with
  `link='OBJECT'` (default `'DATA'` shares on the mesh).
- **Q2 meshRef format:** per-asset **`.blend`**, baked with
  `bpy.data.libraries.write(path, {meshes,materials}, fake_user=True)`; realizer pulls
  it via `bpy.data.libraries.load(path, link=…)`. OBJ/glTF import = copy → rejected
  (defeats build-once); OBJ stays the proof tier.
- **Q3 bridge:** TOON `assets[N]{id,name,category,meshRef,proxyRef,textures,sockets}:`
  section, **curated-only**, `textures` `·`-joined, `sockets` `·`-joined `name@x,y`
  (whole cell quoted — carries a comma). Add one `elif section=="assets":` arm to
  `parse_toon`. Pure-string over zoo structs → hosts in `edi_zoo_core`, boundary holds.

### R1 — forge → zoo handoff (HUB tester-shape set: `~/dept-bus/hub/briefs/tester-shapes-spec.md`)
Six tester shapes span shape-space; all dims are **named params, no magic literals**.
Every route uses an EXISTING recipe op/craftsman (verified) — R1 is recipe-TOML
authoring + bake + curate, **no new C++ recipe op**:
| # | id | category | route (exists) |
|---|---|---|---|
| 1 | square | floor/box-column | `AddPrism` square footprint |
| 2 | circle | column | `AddCylinder` |
| 3 | hexagon | column/tile | `AddPrism` hex footprint |
| 4 | **star6** | ornament/finial | `nfold_star` ScriptOp (EXISTS) ← depth-first |
| 5 | corner_l | wall/junction | `AddPrism` L-footprint (`samples/extruded_figure` is the template) |
| 6 | ring | archway/tube | `AddBoolean` outer−inner cyl (`samples/boolean_op` is the template) |

- **R1a (C++):** zoo→TOON **manifest exporter** `exportZooToToon(const AssetZoo&)` in
  `edi_zoo_core` (pure-string, curated-only, the Q3 layout) + a focused test. Independent
  of R1b.
- **R1b (Python):** `edi_craft.py --asset-out=<path.blend>` bake step — run `build(ops)`
  ONCE then `libraries.write(...,fake_user=True)`; keep the OBJ proof. + smoke. Independent
  of R1a.
- **R1c (DEPTH-FIRST, one shape = `star6`):** forge star6 → bake `.blend` (R1b) → mint a
  **curated** `AssetRecord`(meshRef→.blend, category=`ornament`, proxyRef, optional
  socket) → persist a sample zoo fixture under `samples/` → export the manifest (R1a).
  Proves the WHOLE forge→zoo→manifest handoff on ONE shape.
- **R1d (BREADTH, the other five):** square, circle, hexagon, corner_l, ring — each the
  identical path (author recipe TOML → `--asset-out` bake → mint curated record →
  manifest). May split if corner_l/ring authoring needs its own slice. Result: a
  6-asset curated zoo fixture feeding R2.

### R2 — realize-by-instancing
- **R2a (Python plan):** `edi_realize.py` loads the zoo manifest (`--zoo=…`); a
  placement whose `asset` resolves to a curated manifest entry plans an **INSTANCE**
  piece (meshRef + transform + textureRefs) instead of a greybox primitive. **Greybox
  stays the FALLBACK** for unresolved refs (the M0 crypt keeps rendering). OBJ proof +
  smoke asserts resolution.
- **R2b (Python bpy):** `build_scene` links/instances the `meshRef` artifact (shared
  datablock per S0 — build once) at each transform + applies `textureRefs` as
  materials. **Render proof on the 5090** (the M0 path).

Honest scope: R2 lands the instancing PATH and instances the ONE depth-first element
for real; greybox remains the fallback for the other 9 piece types (baking all 10 is
content work, not this campaign).

## Boundary (the law)
- Cores isolated: NO `edi_zoo_core`/`edi_drafting_core` dependency on recipe/blender.
  Bridges (manifest export, realize) link at a higher layer — the `edi_asset_link`
  precedent. Three-tier: edi records/instances; the engine moves; the realizer
  instances + renders, it does not invent rules.
- Data-oriented; **no JSON**; additive MessagePack; **no hardcoded dimensions** (every
  dimension is named DATA — new CLAUDE.md rule); TOON for the Python handoff.

## Gate per slice
reviewer boundary → builder → green gate (`cmake --build build` + `ctest` +
scan + `edi_craft`/realize smoke; **render proof** for any realizer-OUTPUT change) →
closeout. One slice per commit, `claude:` + teaching body. Report each gate + SHA via
`bus-hub blender-lab`.

## Cross-slice contract (settled — R1c sets it, R2a/R2b consume it)
- **meshRef** = a catalog-relative POSIX key, e.g. `meshes/star6.blend`. The realizer
  resolves it against an `--asset-dir` (default = the manifest file's directory).
- **Datablock load:** the realizer links ALL meshes from the `.blend` and instances
  them as the placement's asset (robust to multi-mesh assets; star6 = one mesh named
  `girih.star_mesh`). Share ONE datablock across placements via
  `bpy.data.objects.new(name, mesh)` (proven live).
- **Fixtures live under `samples/zoo/`:** `meshes/<shape>.blend` (baked artifacts),
  `tester_zoo.editzoo` (MessagePack catalog via AssetZooStore), `tester_zoo.manifest.toon`
  (exportZooToToon output the realizer reads).

## Status log
- 2026-06-19: synced to master `c842554`; baseline green 114/114; plan posted; S0
  research done (`docs/architecture/realize-instancing.md`).
- 2026-06-19: **R1a** CLOSED — exportZooToToon + zooManifestFieldProblem (SHA `fffce80`,
  green 115/115; reviewer MUST-FIX on byte-wise U+00B7 scan fixed). Plan/research docs
  persisted `aa63b9a`.
- 2026-06-19: **R1b** CLOSED — `edi_craft --asset-out` .blend bake (SHA `cea97e0`, green
  115/115). VERIFIED LIVE on Blender 4.5.9: baked `samples/zoo/meshes/star6.blend`
  (1 mesh, 138KB); links back + 3 instances share one datablock (build-once proven).
- 2026-06-19: **R1c** CLOSED (depth-first) — curated star6 `AssetRecord` + `--mint-tester-zoo`
  headless CLI + committed fixtures `samples/zoo/{meshes/star6.blend,tester_zoo.editzoo,
  tester_zoo.manifest.toon}` (SHA `d23a6b4`, green 116/116). Reviewer ACCEPT; `.editzoo`
  round-trip verified, manifest decodes through parse_toon grammar. Manifest:
  `asset_0001,star6,ornament,meshes/star6.blend,"","",""`. NOTE for R2: catalog file is
  `.editzoo` while `AssetZooStore` default is `.edizoo` — realizer reads the `.toon` manifest,
  so unaffected; standardize later if it confuses.
- 2026-06-19: **R2a** CLOSED (reviewer ACCEPT) — realizer pure tier parses the manifest
  (`assets` arm), resolves placements → INSTANCE pieces, greybox FALLBACK preserved; `--manifest`
  /`--asset-dir` CLI; `samples/zoo/star6_demo.toon` (4 placements); OBJ proof markers; smoke
  proves the cross-language round-trip (the owed proof). SHA `c7d6f19`, green 116/116.
- 2026-06-19: **R2b** CLOSED (reviewer SEND-BACK→fixed) — bpy effector: link mesh ONCE +
  `objects.new(name,mesh)` shared datablock per placement, `obj.scale` (never transform_apply),
  per-instance material via `link='OBJECT'`. Reviewer MUST-FIX: textureless-material trigger
  read live `mesh.materials` (poisoned by instance 0's slot) → only 1/4 got the override; FIXED
  by capturing the baked-material flag at LINK time → headless-verified 4/4 bronze, shared geom
  untouched (32→32 verts). SHA `6314fb4` (amended). **FIRST INSTANCED RENDER**: OptiX/RTX-5090,
  `instances=4`, 3.4s → `samples/zoo/star6_demo.{png,render.log}`.
- ✅ **DEPTH-FIRST MILESTONE COMPLETE** — one element (star6) went the whole loop:
  forge → bake (.blend) → mint (curated AssetRecord) → manifest (TOON) → instance (shared
  datablock) → render (GPU). The machine is proven on one shape.
- NEXT: **R1d BREADTH** — the other five shapes (square/circle/hexagon/corner_l/ring) via the
  IDENTICAL path (recipe TOML → `--asset-out` bake → catalog record → re-mint fixtures), then a
  multi-shape instanced render (the visible payoff; give the demo map a light so it reads).
