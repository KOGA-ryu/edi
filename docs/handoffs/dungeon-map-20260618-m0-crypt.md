# Campaign handoff — M0 crypt slice (generator + socket contract)

**Department:** edi-dungeon-map · **Branch:** dept/dungeon-map (rebased on local master)
**Milestone:** `~/dept-bus/M0-CRYPT-SLICE.md` — prove generator → realizer → 5090 render.
**This dept owns:** the GENERATOR (hardcoded crypt MapSpec → createMapFromSpec → live
doc + 2 block instances → Seam-B/C TOON) AND the **M0 SOCKET CONTRACT**.

## State of the world (grounding, verified 2026-06-18)
- Tool-first program COMPLETE; map authoring + TOON export all shipped & green.
- Infra ALREADY present (no new persistence needed):
  - `DraftingBlock.assetRef` + `BlockPlacementMetadata{assetRef,instanceId,rotationDeg,scale}`.
  - `exportMapToToon(const DraftingDocument&, …)` emits `blocks[B]{room,asset,origin,scale,rotation}`
    (Seam C, src/io/MapToonExport.cpp).
  - `MapSpec` (DraftingRoom.h): rooms (NamedRoomSpec{name,RoomSpec}) + MapConnectionSpec.
  - `DrawingDocumentController::createMapFromSpec(spec, canvasPerAuthoredUnit=1.0)`.
  - `placeBlockInstance` / `defineBlockFromSelection` (FLATTEN placement, translate-only).
  - CLI `--export-map <toon>` reads `.edidraw` (doc, carries blocks) or `.map.toml`.
  - `kCorridorWidth = 0.045` canvas units = doorway width in createMapFromSpec.
- So M0's NEW work = (1) the standalone generator that hardcodes the crypt + places the
  2 props + drives the TOON export; (2) settling the socket contract.

## Gate order (kickoff mandate: CONTRACT FIRST)
1. **Reviewer gate — socket contract. ✅ SETTLED → FROZEN (v1).** Reviewer reply 039
   = "settled NO (not yet)" with 3 required doc edits (all folded) + reconciliation
   against the ALREADY-MERGED realizer (`tools/blender/edi_realize.py`, master ac3bf96)
   and the live exporter. KEY FACTS the freeze captured:
   - plugs wire = **6 cols** `{room,name,edge,type,connected,flags}`; edge is the BARE
     letter **N/E/S/W** (not words); `flags` ·-joined neutral tags (the live exporter).
   - **Plug position rule (Blocker B):** position is NOT on the wire — the realizer
     derives it as the room **edge MIDPOINT**; corridor straight if midpoints colinear
     else one L (realizer handles both). Generator authors plugs at edge midpoints.
   - **STAIR is a block asset_ref** `crypt.stair` (no elevation on the wire), not
     graph-expanded — reconciled §2/§4.
   - Greybox constants are REALIZER-AUTHORITATIVE (WALL_H=12, CORRIDOR_W=5, DOOR_W=4);
     the false `kCorridorWidth` equivalence DELETED (§6). edi binds only §0+§1.
2. **Fold verdict → freeze contract**, then bus-hub it to blender-lab + the hub so
   the realizer can build in parallel.
3. **Builder batch — the generator:**
   - **G1** (brief 041, STAGED) — structure path: `buildCryptMapSpec()` +
     end-to-end controller-driven test. Fires on "contract FROZEN" doorbell.
   - **G2** (brief 042, STAGED) — props: add the 2 `MapBlockSpec` crypt props. Fires
     after G1 lands AND drafting's `MapSpec::blocks` field is live on local master.
4. **Closeout** + bus-hub when the generator emits a valid crypt TOON green.

## Cross-dept: MapSpec block field — SETTLED (drafting reply 042)
- `MapBlockSpec{assetRef, position(ABSOLUTE authored feet, centre), rotationDeg,
  scale, name}` + `MapSpec::blocks` (additive, default-empty). NOT reusing
  DraftingBlock (definition-less declaration, deliberate twin).
- createMapFromSpec realizes inline: Point marker + `BlockPlacementMetadata`,
  instanceId off the one serial ("blockinst"). `room` column DERIVED by containment.
- Tags vector LEFT OUT — matches contract §8 (asset_ref carries identity; no tags
  column). Flag to drafting only if the realizer needs neutral socket tags.
- Drafting building NOW; green-this-session; will bus the green master tip with the
  field live → then G2 fires.
- The crypt props (G2): `crypt.sarcophagus` @ (45,10), `crypt.brazier` @ (40,5) —
  both inside the crypt footprint x∈[35,55] y∈[0,20] → room "crypt".

## Decisions made by the planner (to ratify)
- Generator = standalone in-repo C++ (fork ratified by kickoff). Builds a MapSpec in
  authored FEET, docks to createMapFromSpec, places 2 blocks, exports the document TOON.
- **Structure expanded by the realizer; props ride as block asset_refs.** The 10
  piece types are the realizer's vocabulary, NOT TOON columns.
- **No tags column added for M0** — asset_ref + plug/conn type carry the neutral tags
  (additive future extension if the realizer needs it).

## Resume recipe
- Read this doc + `docs/dungeon-map-m0-socket-contract.md` + `~/dept-bus/M0-CRYPT-SLICE.md`.
- Branch dept/dungeon-map, rebase on LOCAL master, build dir present, green at last check.
- Next action depends on reviewer reply 039.
