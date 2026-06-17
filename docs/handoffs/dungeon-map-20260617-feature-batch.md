# Handoff — dungeon-map-20260617-feature-batch

> Per-campaign state + the live DM task board. Each gate appends its result; the
> board is the source of truth a fresh session recovers from.

- **Campaign**: dungeon-map-20260617-feature-batch
- **Department**: edi-dungeon-map
- **Goal**: the usable-map polish ring past the stop-line — frame-on-load,
  interior features, neutral plug flags, Seam-C edited round-trip, region fill,
  map browser content, block rotation/scale record+export. DM-01..13 NOW;
  DM-14/15 (per-instance geometry transform) WAIT for drafting's
  `transformGeometry` (DR-01) — hub signals when merged to master.

## Operating rules (HUB dispatch 2026-06-17)
- I build **OP/controller verbs + pure plans + data/parse/persist/export ONLY**.
  **edi-ui wires the chrome** from the surface spec (`~/edi/docs/ui-surface/
  dungeon-map/DM-surfaces.md`) separately. Do NOT edit the edi-ui-owned shell
  files (`EdiShellWindow*`, `app/main.cpp`, `CMakeLists.txt`,
  `DrawingCanvasWidget`, `DraftingFeaturePanels.cpp`) — that's a hub/edi-ui hand.
- Autonomous through gates: reviewer boundary ONLY where genuinely needed → builder
  → green gate (`cmake --build build && ctest -E edi_shell_window_tests` + scan +
  reference-dungeon snapshot) → next. Rebase on master at each task start.
- Commit to `dept/dungeon-map`; do NOT merge/push — the hub routes verified work
  to edi-ui. Report to hub every ~3 tasks / on a blocker / at batch-done.
- Forks RATIFIED (follow the spec's recommendations): plug-flags = open token run;
  region-fill arm = inspector `fillRegionButton` (A); DM-14 spins = Left panel;
  DM-11 golden = edi-ui co-bless when it lands.

## Master state note
Rebased onto master `f87bc1b` (edi-drafting's `<memory>` test fix — my identical
`c9d6156` was auto-skipped). My 22 cartography commits ride on top, still unmerged
(hub routes them to edi-ui). `origin/dept/dungeon-map` diverged (box vs Mac, 6
commits) — hub is settling; I do not push.

## Task board (DM-01..15)
Legend: ✅ done · ▶ in gate · ◻ queued · ⏸ waiting (DR-01) · 🟦 edi-ui-coordinated (I build only the OP/controller sliver)

| Task | What (my sliver) | Wave/dep | Status |
| --- | --- | --- | --- |
| DM-04 | plug flags: `DraftingPlug.flags` + `RoomPlugSpec` parse (comma-split) | W1 | ▶ batch-1 |
| DM-05 | persist plug flags: additive `plugValue`/`readPlug`, no bump | W2 ←04 | ▶ batch-1 |
| DM-06 | plug flags → TOON: `writePlugRow` `flags` column, both overloads | W3 ←05 | ▶ batch-1 |
| DM-02 | interior features: `RoomSpec.features` data + parse | W1 | ◻ batch-2 |
| DM-03 | features → Point markers in `createMapFromSpec` (+`feature:<type>` tag) | W2 ←02 | ◻ batch-2 |
| DM-07 | Seam C edited round-trip: store room footprint+name in `document.rooms` | W1 | ◻ batch-3 |
| DM-08 | Seam C round-trip regression test | W2 ←07 | ◻ batch-3 |
| DM-12 | block rotation/scale fields: `BlockPlacementMetadata.rotationDeg/scale` + persist | W1 | ◻ batch-4 |
| DM-13 | export reads rotation/scale (replace 1/0 placeholders) | W3 ←12 | ◻ batch-4 |
| DM-09 | region-fill boundary trace: pure `planRegionFill` (`DraftingRegionFill`) | W1 | ◻ batch-5 (reviewer boundary first) |
| DM-10 | region-fill controller verb: `PointCaptureIntent::RegionFill` → filled Polygon | W2 ←09 | ◻ batch-5 |
| DM-01 | view-auto-fit: `computeDocumentBounds()` controller getter (sliver) | 🟦 W1 | ◻ assess (mostly edi-ui) |
| DM-11 | map browser content: shared read-only `deriveEdge(plug,room)` helper (sliver) | 🟦 W1 | ◻ assess (panel is edi-ui) |
| DM-14 | place rotated/scaled block (`placeBlockInstance` extended) | ⏸ ←12,DR-01 | ⏸ waits transformGeometry |
| DM-15 | transform placed instance (`transformBlockInstance`) | ⏸ ←14,DR-01 | ⏸ waits transformGeometry |

## Gate log

### Kickoff — 2026-06-17 — edi-dungeon-map-planner
- Read dispatch + DM bucket + surface spec. Rebased on master. Dispatched
  batch-1 (plug-flags spine DM-04/05/06) to builder + a region-fill reviewer
  boundary gate (DM-09/10) to reviewer in parallel.

## Open questions / blockers
- DM-14/15 blocked on DR-01 (`transformGeometry`) — drafting builds it first; hub
  signals merge. DM-12/13 deliberately split off so they land now (identity-valued).

## Next
- Batches 2–5 queued; dispatch each as the builder frees. Assess DM-01/DM-11
  sliver vs edi-ui ownership. Report kickoff to hub.
