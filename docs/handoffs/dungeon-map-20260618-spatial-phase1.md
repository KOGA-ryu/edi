# Campaign handoff — Spatial-model inversion (PHASE 1 ✅ COMPLETE · PHASE 2 in progress)

## ⏸ POWER-DOWN CHECKPOINT (2026-06-18, end of session) — RESUME RECIPE
**Phase 1 COMPLETE + merged (master `dec48db`).** Phase 2 (full breadth, I LEAD) underway:
- **Wire extension (P2-A), conditional + canary-guarded (see `docs/dungeon-map-wire-schema.md`
  — I own column order):** A1 `nodes[]` ✓ merged (426b379) · A2 `level` ✓ merged (5fa7583)
  · **A3** nodes `radius` + rooms `derivation`/`bounded_by` + `DraftingMapRoom.boundedBy`
  ✓ **COMMITTED 288571d** (hub merging). Clean tree, nothing uncommitted.
- **P2-B span generator (brief `076`) — ARMED.** Fires on resume the moment BOTH: (a)
  drafting's `deriveSpanFootprint` (`DraftingSpan.h`, tip `3b62884`, reviewer-CLEARED
  high-confidence) is on LOCAL master (hub merging now); (b) A3 has landed (done). It adds
  `MapNodeSpec`/`MapSpanSpec` to MapSpec + teaches `createMapFromSpec` to mint nodes +
  derive span-rooms (AABB-first). Signature in 076.
- **FIRST INVERTED RENDER is LIVE** — blender-lab P2-C built to my FINAL wire contract
  (`074`), golden-tested the sample, rendered `/tmp/m0/inverted.png` @`07afe8f` (small
  nodes + big span-rooms + wall-optional + Z-stacked floors). They await the P2-B
  **generated** TOON to lock against.
- **RESUME STEP 1:** confirm `DraftingSpan.h` on master + fire `076` (P2-B) to the idle
  builder (exact path; capture-pane confirm). **STEP 2:** when P2-B lands, produce a
  generated inverted TOON (P2-B's test builds one; run/extract it to a file) → bus
  blender-lab to lock the render ⇒ closes generator→inverted-TOON→realizer→render.
  **STEP 3:** continue the wire — A4 `walls`/`kind`/`ceiling`/`floor`, A5 `feet_per_band`
  + `levels[]`; then `.map.toml` grammar, vertical features, dual-graph.
- **CADENCE (hub workflow):** I fire builder slices (verify the `.md` EXISTS first; only
  send when the builder is IDLE; capture-pane confirm). **The HUB relays each commit to
  edi-ui — I do NOT hand to edi-ui.** Coordinate span-footprint w/ drafting, realizer-read
  w/ blender-lab (both done for this round). Run the queue autonomously, no idle between
  slices.
- Open coordination files: `074` (wire contract FINAL → blender-lab), `076` (P2-B armed),
  `073` (span request → drafting, answered by 3b62884).

---

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

## SIMPLIFIED CADENCE (hub workflow fix, 2026-06-18) — what the planner does per slice
The HUB now detects each builder commit + drives the edi-ui integration/merge. The planner
does ONLY: **(1)** write the next builder brief as a REAL numbered `.md`; **(2)** C-u the
builder input + fire its EXACT path (only when the builder is IDLE); **(3)** builder commits
IN-TURN; **(4)** fire the drafting-core review brief. **HARD RULE: never fire a brief
pointer unless that `.md` FILE EXISTS.** No more planner green-gate / edi-ui merge brief
(the hub owns integration). Capture-pane to confirm each builder send submitted.

## STATUS (master `cc95528`)
- **Slice 1 GOLDEN canary** — ✅ MERGED. **Slice 2 syncGraphForMovedObject** — ✅ MERGED.
  **3a `level`** — ✅ MERGED. **3b `RoomDerivation`** — ✅ MERGED. **3c `DraftingNode`** —
  ✅ MERGED (all → master cc95528). drafting-core cleared 2/3a/3b/3c (all clean; node
  id-recovery fence + the field/enum templates blessed).
- **3d `OverlapPolicy` + pure `footprintsOverlap`** — ✅ COMMITTED `581e249`; hub merging;
  drafting-core review fired (066).
- **3e `RoomKind` + per-edge wall presence on RoomSpec** — brief 065, FIRING to builder. [IN FLIGHT]
- **3f carry `level` through plug & connection** — NEXT.
- **Determinism harness** (sorted containers / seed / byte-stable golden) — Phase-1 EXIT gate.

Every slice: additive + DEFAULTS preserve behavior + OFF the TOON wire ⇒ the CANARY stays
byte-identical (the hard gate). Persisted structs get an additive MessagePack round-trip
(missing⇒default, NO version bump); RoomSpec (3e) is transient (not persisted) so no MsgPack.

## Hot files (the hub serializes integration; drafting-core reviews the src/drafting-touching)
`DraftingMapTypes.h`, `DraftingRoom.h`, `DraftingSerialize.cpp`, `MapToonExport.cpp`,
`createMapFromSpec`.

## Planner-ahead protocol
Keep 2–3 slices specced ahead. Every slice carries its acceptance (the canary stays
byte-identical + a positive new-field round-trip test). Report milestones via bus-hub.

## M0 PREDECESSOR — COMPLETE
M0 crypt + scale-knob shipped: `edi --generate-crypt <out> --scale <S>` → scaled TOON +
`scale:` meta → realizer → OptiX/5090 render. CLI live (edi-ui 048c), 047 audit clean
(reviewer 050). See `dungeon-map-20260618-m0-crypt.md`.
