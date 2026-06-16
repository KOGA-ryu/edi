# Handoff — dungeon-map-20260616-cartography

> The per-campaign state. Each gate appends its result; the NEXT gate reads this
> first. Agents hand off THROUGH this file — they cannot message each other.

- **Campaign**: dungeon-map-20260616-cartography
- **Department**: edi-dungeon-map
- **Goal (one line)**: MAP, DOCUMENT, and behavior-preservingly REFACTOR the
  dungeon-map subsystem so its architecture is understood and clean before any
  features land. Output: `docs/architecture/edi-dungeon-map.md` + a set of
  settled, behavior-preserving refactor slices.
- **Boundary (the question the reviewer gate must settle)**: In `src/drafting`,
  exactly which files/types are MAP-SPECIFIC (ours) vs CORE-GEOMETRY
  (edi-drafting's)? Where are the seams, the command arms + visit sites, the
  MessagePack codec, and the refactor candidates (dead code, duplication,
  data-oriented drift, non-exhaustive visits, un-commented wiring)?

## Mandate constraints (hold even while documenting)
- Neutral geometry + tags only. NO game rules, NO generation. The tool-first
  program (Phases A–D) is COMPLETE; this campaign does NOT build features.
- plugs + declared_connections are 2 document-level vectors (a plug is a
  RELATION, not a geometry variant) — confirm still true.
- MessagePack map fields are additive-tolerant (missing key ⇒ default, NO
  version bump, like `wall_visual`) — confirm + document the codec.
- Seam B/C export is TOON to the user's OWN engine (never JSON, never UVTT).
- `transformGeometry` (rotate/scale over the 14 kinds) does NOT exist yet — it is
  edi-drafting-owned; future rotation features depend on it. Note, don't build.

## Green gate on this box
`cmake --build build && ctest --test-dir build -E edi_shell_window_tests
--output-on-failure` + the scan + reference-dungeon `--snapshot`. The
`edi_shell_window_tests` golden PNG drift is a known edi-ui environmental failure
(hub ledger H1) — exclude it; do NOT try to fix it.

## Known map-specific anchors (from planner's orientation grep)
- `src/drafting/DraftingRoom.{h,cpp}`, `DraftingCorridor.{h,cpp}`,
  `DraftingAsciiMap.{h,cpp}`, `DraftingGraphOps.{h,cpp}`, `DraftingPathfind.h`
- `src/io/MapToonExport.{h,cpp}`, `src/io/RoomSpecStore.{h,cpp}`
- Map arms threaded through shared `src/drafting` files: `DraftingTypes.h`,
  `DraftingDocument.h`, `DraftingCommands.{h,cpp}`, `DraftingSerialize.cpp`
- Controller/CLI: `src/core/DrawingDocumentController.cpp`, `DrawingCore.h`,
  `src/widgets/EdiShellWindowIo.cpp`
- Reference dungeon: `tests/data/dungeon.map.toml`

## Gate log

### Reviewer gate — 2026-06-16 — edi-dungeon-map-reviewer (CLOSED — boundary settled YES)
- Brief: `~/dept-bus/edi-dungeon-map/briefs/002-reviewer-cartography-survey.md`
- Reply: `~/dept-bus/edi-dungeon-map/replies/002-reviewer-cartography-survey.md`
- **Boundary: settled YES.** Map subsystem cleanly separable — wholly-owned files
  + a small, well-marked set of arms in shared `src/drafting` files. Full
  ownership table folded into `docs/architecture/edi-dungeon-map.md §1`.
- Neutral law CONFIRMED end-to-end (no passable/weight/direction on any persisted
  struct; A* costs are transient routing internals, never serialized/exported).
- MessagePack CONFIRMED additive-tolerant, no version bump. Cascades CLEAN.
- Findings → refactor slices (architecture doc §7): **A1** (BUG latent — runtime
  command guard should be compile-time `always_false_v`), **N1** (TOON emitter
  duplication across the 2 overloads), **B1** (stale `plug.anchor` — document the
  contract now; real sync fix is note-don't-build per mandate). N2 no-action,
  N3 low-value/deferred.
- Boundary risks (low, for hub awareness): shared `DraftingTypes.h` variant
  static_assert region, the `DraftingCommands` variant append list, and the
  `createObjectsAndSelect` richer overload.

### Builder batch — 2026-06-16 — edi-dungeon-map-builder (OPEN)
- Brief: `~/dept-bus/edi-dungeon-map/briefs/003-builder-cartography-refactors.md`
- Slices: A1 (compile-time command guard) · N1 (TOON emitter de-dup) · B1
  (document the plug.anchor staleness contract). All behavior-preserving.

### HUB RULING H2 — 2026-06-16 — src/drafting ownership boundary
- `~/dept-bus/RULING-H2-src-drafting-boundary.md`. By-domain, SINGLE document
  (no split of `DraftingDocument`/`DraftingCommand`). edi-dungeon-map owns the map
  graph whole-file set + the 7 map command arms + the map struct/enum DEFINITIONS;
  `WallGeometry` stays CORE. Shared headers co-edited by REGION not by file.
  Recorded verbatim in `docs/architecture/edi-dungeon-map.md §1`.
- **New deliverable (fold into cartography):** extract the map struct/enum defs
  into a dungeon-map-owned `DraftingMapTypes.h` (document keeps its vectors). Pure
  code motion, before any feature work.
- `transformGeometry` = drafting-owned shared primitive, a FEATURE, note-don't-
  build (recorded §8).

### Reviewer scoping gate (DraftingMapTypes.h) — 2026-06-16 — edi-dungeon-map-reviewer (CLOSED — design settled YES)
- Brief: `004-reviewer-mapttypes-extraction-design.md` ·
  Reply: `~/dept-bus/edi-dungeon-map/replies/004-reviewer-mapttypes-extraction-design.md`
- **Cycle resolved truthfully:** since C++17 `std::vector` may be instantiated
  with an INCOMPLETE element type, so `struct DraftingObject;` forward-declared in
  the map header lets `DraftingBlock` hold `std::vector<DraftingObject>` by value —
  the single-header shape (a) is viable, no `DraftingDocument.h` include cycle.
- **Two metadata structs are Point2D-free** (`WallVisualMetadata` = one `WallType`;
  `BlockPlacementMetadata` = three strings) and embed in `ObjectMetadata`; the
  doc-record structs (`DraftingPlug.anchor`, `DraftingMapRoom.origin`) need
  `Point2D` BY VALUE → the single include point sits just before `ObjectMetadata`
  (`DraftingTypes.h:~201`), where `Point2D`/`Bounds2D` are already complete.
- **PLANNER DECISION: shape (a)** — one `DraftingMapTypes.h`, included once
  mid-`DraftingTypes.h` (close/reopen namespace), forward-declaring core
  `DraftingObject`. Rationale: faithful to H2's literal single-header mandate;
  reviewer's lean; forward-decl is one standard, commented line. Hub given a
  veto-window toward fallback shape (c) (two headers) — non-blocking, since the
  extraction lands after 003 anyway.
- `.cpp` motion: NONE (name-funcs resolve through the include chain). Pure motion.

### Builder batch 003 (A1/N1/B1) — 2026-06-16 — edi-dungeon-map-builder (DONE)
- Reply: `~/dept-bus/edi-dungeon-map/replies/003-builder-cartography-refactors.md`
- Commits: prep `c9d6156` (+`<memory>`), A1 `4ca427e`, N1 `fcae7eb`, B1 `1a26ee7`.
- **Gate GREEN:** build exit 0; `ctest -E edi_shell_window_tests` → 95/95 pass;
  N1 golden UNMODIFIED; scan clean; snapshot PNG 900×760; export spot-check
  rooms[12]/plugs[26]/connections[12]. Behavior-preserving.
- A1 pre-check rigorous: 34 variant alternatives ↔ 34 handled branches (empty
  diff); guard verified to FIRE on a deleted arm, then restored (not committed).
- N1: helpers take resolved strings (the two overloads share no room/plug TYPE —
  `MapSpec` room vs `DraftingMapRoom`); byte-identical output proves the de-dup.
- **Ownership correction (planner):** the prep `<memory>` fix touched
  `tests/drafting_room_/_corridor_/_ascii_map_` — the builder tagged these
  "edi-drafting territory," but they test `planDraftingRoom`/`CorridorSpec`/
  `DraftingAsciiMap` = wholly-OURS map primitives (arch §1). So the fix is in OUR
  territory, correctly done, NO handoff needed for the test files. The builder's
  deeper note — sweep non-test DRAFTING SOURCES for the same dropped-transitive-
  include — is a speculative edi-drafting hygiene FYI (build is green), passed to
  the hub as one line, not a blocker.

### Builder extraction slice 005 (DraftingMapTypes.h, shape a) — DISPATCHED
- Brief: `~/dept-bus/edi-dungeon-map/briefs/005-builder-mapttypes-extraction.md`
- 003 committed (B1 `1a26ee7` present) → dispatched. Builds on 003; B1's comment
  travels with the moved `DraftingPlug`.

### Reviewer diff audit of 003 — DISPATCHED (parallel with 005)
- Brief: `~/dept-bus/edi-dungeon-map/briefs/006-reviewer-003-diff-audit.md`
- Read-only adversarial audit of commits `4ca427e`/`fcae7eb`/`1a26ee7` (+ prep
  `c9d6156`). Independent of 005 (immutable commits) → runs in parallel.

## Open questions / blockers
- None blocking. Boundary SETTLED by HUB H2. The `DraftingMapTypes.h` include
  shape is being designed (gate 004); a flagged hub question may surface if H2's
  "single header" conflicts with the `DraftingBlock`↔`DraftingObject` cycle.

## Next
- Builder lands A1/N1/B1 (003) → reviewer settles extraction design (004) →
  builder executes extraction (005, AFTER 003 so B1's comment rides along) →
  diff audit → closeout.
