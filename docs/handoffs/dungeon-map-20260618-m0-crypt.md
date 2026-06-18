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

## USER DIRECTIVE (2026-06-18, hub brief 043): DOUBLE the crypt
- "Double the rooms and hallways." Split across depts:
  - ROOMS (wire/generator, mine): entrance 30×30 @ (0,10), crypt 50×50 @ (70,0),
    STRAIGHT corridor (colinear plug midpoints at world y=25). Doubled TOON authored:
    `~/dept-bus/dungeon-map/crypt_doubled.toon`. Props repositioned (scale stays 1):
    sarcophagus (95,25), brazier (82,12), stair (76,8).
  - HALLWAY WIDTH (realizer constant, blender-lab): CORRIDOR_W 5→10, DOOR_W →8 in
    edi_realize.py. KEY: the frozen contract made corridor/door width REALIZER-
    AUTHORITATIVE (§5) — NOT on the wire — so this is NOT a contract-lock invariant;
    the lock stays on track. (Corrected the hub brief's pinned-DOOR_W assumption.)
- Piece-types: STRAIGHT ⇒ 9/10 (no corridor_l); all-10 needs an L-route (user's call;
  merged fixture keeps the all-10 demo). Flagged to hub.
- Dispatched: doubled TOON → blender-lab to render on 5090 (auto-opens on desktop);
  G1/G2 briefs re-retargeted to the doubled layout; builder rebuilding.
- Contract commits: DOOR_W/double fold; bus-hub'd the hub.
- ✅ DELIVERED (blender-lab reply 041): `/tmp/m0/crypt_doubled.png` (OptiX/5090, 3.4s,
  1080p). Hallway widened in the realizer @d1f2ca1 (CORRIDOR_W→10, DOOR_W→8) + a
  scale-robust fix: the doorway WALL OPENING now tracks CORRIDOR_W (two 5 ft segments
  open for a 10 ft corridor) so the corridor enters full-width both ends. 9/10 types.
  Milestone bus-hub'd. Contract lock confirmed on track (DOOR_W was realizer-side).

## Cross-dept: realizer confirmation — ALL 3 ASKS CONFIRMED (blender-lab reply 040)
- Realizer adopts contract v1 as-is (no renegotiation). 2D→3D map-x→X/map-y→Y/+Z-up
  matches; `crypt.stair` is the realizer's stair-block path; the §3 slice RENDERED on
  the 5090 (OptiX, 3.3 s, 1080p). Realizer green on dept/blender-lab @821e87a.
- HEADS-UP folded: a STRAIGHT slice yields only 8/10 piece types (no corridor_l, no
  stair). The M0 GATE wants all 10 from the one-command generator→PNG. So **§3
  RETARGETED**: the generator now REPRODUCES the proven fixture
  `samples/crypt_m0/crypt.toon` (L-corridor + crypt.stair) → all 10 → drop-in for the
  fixture. Contract §3 + G1/G2 briefs updated; G1 builder sent a REVISED-geometry
  doorbell (commit 5b6f672).
- Fixture golden (the generator's target):
  rooms: entrance "0,0" "15,15" / crypt "35,25" "20,20" (stone);
  plugs: entrance,to_crypt,E,door,true,crypt | crypt,to_entrance,W,door,true,crypt;
  connections: entrance.to_crypt ↔ crypt.to_entrance, corridor;
  blocks: crypt.sarcophagus "45,35" | crypt.brazier "40,40" | crypt.stair "36,35".

## Cross-dept: MapSpec block field — SETTLED (drafting reply 042/043)
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

## STANDING RULE (2026-06-18, USER) — every dimension is DATA
Source: `~/dept-bus/SCALE-POLICY.md` (HARD RULE / invariant 0) + PROTOCOL final section.
NO magic dimension literals in logic; every dimension is a named field in a spec/
config/constants-table, derived or parameterizable. Exempt: epsilons/tolerances, unset
0.0. REVIEWER-ENFORCED (no mechanical scan). Dungeon-map application:
- Generator crypt LAYOUT dims → file-scope NAMED DATA TABLES (kCryptRooms/Plugs/Conns/
  Blocks); buildCryptMapSpec only translates tables→MapSpec, no inline dimension literal.
- Plug `at` DERIVED as edge midpoint from room dims (scale-robust, no second literal).
- IN FLIGHT: builder sweeping the magic-literal G1 (d34fb11) to the table form,
  behavior-preserving. Then REVIEWER audits the diff for magic dims + sweeps other
  dungeon-map code opportunistically.
- NOT mine: kCorridorWidth=0.045 retirement = drafting's slice (src/core), per
  SCALE-POLICY per-dept split. Dungeon-map emits FEET (the canonical source of truth).

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
