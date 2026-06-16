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

### Reviewer gate — 2026-06-16 — edi-dungeon-map-reviewer (OPEN)
- Brief: `~/dept-bus/edi-dungeon-map/briefs/002-reviewer-cartography-survey.md`
- (awaiting findings)

## Open questions / blockers
- The `src/drafting` ownership boundary with edi-drafting — to be FLAGGED to the
  hub for arbitration once the reviewer enumerates it.

## Next
- Fold reviewer findings into `docs/architecture/edi-dungeon-map.md`.
- Decide behavior-preserving refactor slices; brief the builder.
