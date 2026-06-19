# Campaign handoff — Spatial-model inversion, PHASE 1 (foundational data-spine)

**Department:** edi-dungeon-map = **LEAD** (sequences + builds the data-spine on
dept/dungeon-map). **drafting-core** = REVIEW/CONSULT on drafting-core-touching slices
(does NOT parallel-build the hot files). **edi-ui** = SOLE INTEGRATION owner (every
hot-file change → master through edi-ui; campaign `ui-20260618-spatial-phase1`).
Source of truth: `~/dept-bus/SPATIAL-MODEL-BACKLOG.md` (DECISIONS — 15 ratified forks —
+ EXECUTION, ratified 2026-06-18 "make it so").

## The model (framing)
INVERT: the placed thing becomes a small connector NODE; the span BETWEEN nodes becomes
the big room. Plus: discrete `level` floors, wall-optional rooms, area-occupying vertical
features, overlap-as-resolvable-event. edi stays drafting-only (neutral geometry+tags);
the wire extends ADDITIVELY; every dimension is DATA.

## Ratified decisions that bind Phase 1 (from DECISIONS)
1. Inversion **COEXIST** — `RoomDerivation { Placed | SpanDerived }` enum; both models share one doc.
7. **`syncGraphForMovedObject`: BUILD NOW** (own slice) — 3 items depend + fixes deriveEdge drift.
8. Elevation = **discrete int `level` band**; edi owns NO Z; DATA `feet_per_band`.
5. TOON growth = **HEADER-AS-TRUTH** (index BY NAME; one owner of canonical column order; no version line).
10. Vertical features = **HYBRID** (links ride CONNECTIONS; risers = new `DraftingVerticalFeature`).
11. Connector⇄plug = **SUBSUME** (node IS the doorway; plugs re-anchor — needs syncGraphForMovedObject).
12. Overlap default **PICK-ONE** + per-room priority/keepAlways; 3. pure resolver in src/drafting, GENERATOR is default caller.
13. Wall-less = **RoomKind/enclosure enum driving DRAWING only** (presentation, like WallType).
+ additive MessagePack (NO version bump), determinism harness is a Phase-1 gate.

## THE CANARY (edi-ui baseline, master @0d5ca60)
`tests/data/dungeon.map.toml` → TOON **sha256 6c632293…229e43…2b0e3, 1685 bytes,
rooms[12]·plugs[26]·connections[12]**. MUST stay byte-identical after every additive slice.
Drift ⇒ a default is non-neutral ⇒ HALT + flag hub (do NOT re-bless).

## ORDER (each: green on dept → edi-ui merges → edi-gate green)
1. **Crypt/dungeon regression-lock GOLDEN** (the canary; pins TOON byte-identical + the
   ~74 object count) — brief 051, FIRED. [IN FLIGHT]
2. **`syncGraphForMovedObject`** — sibling to `pruneGraphForRemovedObject`
   (`src/drafting/DraftingGraphOps.{h,cpp}`; TODO at `DraftingMapTypes.h:108-109`).
   Recompute plug anchors for moved anchor objects; retires the deriveEdge drift. [NEXT]
3. **Additive struct slices** (each additive + MsgPack round-trip + green; gate the
   drafting-core-touching ones through drafting-core review):
   - `int level = 0` on `DraftingMapRoom` (+ carry through plug/connection records).
   - `RoomDerivation` enum (COEXIST; Placed default) on the room record.
   - `DraftingNode` connector vector (minted id, anchor, footprint/radius DATA, type/name)
     + `highestDocumentIdSerial` checklist + prune/sync coverage.
   - `OverlapPolicy` enum + pure `footprintsOverlap` primitive (src/drafting; drafting-core review).
   - `RoomKind` enum + per-edge wall presence on `RoomSpec` (drafting-core review;
     `planDraftingRoom` honors it later in Phase 2).
   - carry `level` through plug & connection.
   - determinism harness (sorted containers / seed / byte-stable golden) — Phase-1 gate.

## Integration contract (edi-ui SOLE integrator)
Hot files: `DraftingMapTypes.h`, `DraftingRoom.h`, `DraftingSerialize.cpp`,
`MapToonExport.cpp`, `createMapFromSpec`. I build each slice green on dept/dungeon-map,
then bus edi-ui to merge (one at a time — no parallel hot-file builds). drafting-core
reviews the core-touching slices BEFORE I hand them to edi-ui.

## Planner-ahead protocol
Keep 2–3 slices specced ahead. Every slice carries its acceptance (the canary stays
byte-identical + a positive new-field round-trip test). Report milestones via bus-hub.

## M0 PREDECESSOR — COMPLETE
M0 crypt + scale-knob shipped: `edi --generate-crypt <out> --scale <S>` → scaled TOON +
`scale:` meta → realizer → OptiX/5090 render. CLI live (edi-ui 048c), 047 audit clean
(reviewer 050). See `dungeon-map-20260618-m0-crypt.md`.
