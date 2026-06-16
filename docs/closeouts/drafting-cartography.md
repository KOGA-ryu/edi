# Closeout — drafting core cartography (map + exhaustiveness invariants)

> Freezes a boundary so future work does not re-litigate it. When a campaign has
> established WHERE something lives and WHAT is allowed, record it here and link it
> from the ledger.

- **Boundary**: the drafting core's architecture map + the exhaustiveness/dedup
  invariants established while documenting it
- **Department**: edi-drafting
- **Campaign**: drafting-20260616-cartography
- **Date**: 2026-06-16

## The decision
The drafting core (`src/drafting`, `src/core`) was mapped, documented, and
behavior-preservingly refactored BEFORE feature work. Three durable outcomes are now
frozen:

1. **`docs/architecture/edi-drafting.md` is THE map of the drafting core** — the two
   layers, the core types, the 14 `DraftingGeometry` arms + all visit sites, the 33
   `DraftingCommand` arms + the single dispatch, the controller spine + call-graph,
   the seams, and the refactor log. It is LIVING: keep it current as code changes.

2. **Every `std::visit` over `DraftingGeometry` and `DraftingCommand` is now
   compile-exhaustive** (terminal `static_assert(always_false_v<…>)`). `applyDraftingCommand`
   (`985e200`) and the four formerly-unguarded geometry visits — `DraftingMirror`
   `mirrorGeometry`, `DraftingQuickMeasure` `quickMeasureAt`, `DraftingPlotPlan`
   `appendPlotSegments` + `closedFillRing` (`2a1be77`) — were guarded by making each
   kind's CURRENT behavior explicit first, so a new 15th kind now FAILS THE BUILD
   instead of silently mis-behaving.

3. **The translation commands are deduped** (`818736e` + `9ab72aa`): a file-local
   `applyTranslationPlan(DraftingDocument&, const vector<DraftingTranslation>&)` runs
   the shared copy→loop-`moveObject`→commit; AlignSelection + DistributeSelection
   delegate. The `kCircleSegments` plot constant replaced the duplicated `32`
   (`cae383a`).

The `src/drafting` OWNERSHIP boundary (drafting core vs dungeon-map's map graph) was
settled separately by **HUB RULING H2** (`~/dept-bus/RULING-H2-src-drafting-boundary.md`,
recorded verbatim in architecture doc §6) — by-domain, single document, shared headers
co-edited by region. That ruling is the authority on ownership; this closeout does not
re-state it.

## Why (the reasoning that must NOT be re-argued)
- **Exhaustiveness via "make current behavior explicit, THEN guard"** — not a bare
  `always_false_v`. The unguarded visits let some kinds fall through a catch-all; a
  bare guard would have failed to compile for those kinds AND changed behavior. Making
  each kind's behavior explicit first kept the 14 known kinds byte-identical and made
  only a future kind a compile error. (Reviewer diff-audit: ACCEPT.)
- **MoveSelection is NOT folded into `applyTranslationPlan`** — on purpose. Its per-id
  `containsObject` guard must stay INTERLEAVED with the move: the first failing id in
  selection order decides the rejection MESSAGE, and `DraftingCommandResult.message` is
  observable upstream (`finishEdit`). A `[present+locked, missing]` selection must
  surface "object is locked", not a pre-scan's "selection target does not exist". The
  first dedup attempt hoisted the guard and was SENT BACK on exactly this divergence;
  do not re-attempt the hoist.
- **The Mirror mirrorable-set lives in two hand-kept lists** (`mirrorGeometry` keys on
  geometry type; `supportsMirror` keys on `DraftingShapeKind`). They agree today and
  `supportsMirror` gates before the visit, so unifying buys nothing and risks behavior.
  Do not "fix" this into a unification.

## The contract (what future work must respect)
- Adding a 15th geometry kind: you MUST add an arm to every guarded geometry visit
  (the build names the function that's missing one) AND revisit `supportsMirror`.
- Adding a `DraftingCommand` arm: you MUST handle it in `applyDraftingCommand` (the
  build enforces it).
- Keep `DraftingCommandResult.message` stable for a given rejection path — it is
  observable; treat it as behavior, not a debug string.
- `applyTranslationPlan` serves Align/Distribute; MoveSelection keeps its interleaved
  guard loop. Do not merge them.
- Keep `docs/architecture/edi-drafting.md` current when you touch the core.

## Out of scope / explicitly NOT allowed
- The `highestDocumentIdSerial` rooms-comment + anything map-id-related — that is a
  MAP region, owned by edi-dungeon-map (ruling H2). Not edited here.
- `DraftingMapTypes.h` extraction (moving the map struct/enum DEFINITIONS) — that is
  edi-dungeon-map's slice, not ours.
- `transformGeometry` (rotate/scale over the 14 kinds) — drafting-OWNED but a parked
  FEATURE (`~/dept-bus/ROADMAPS-DRAFT.md`); NOT built during cartography.
- Any feature work; this campaign was map + document + behavior-preserving refactor only.

## Pointers
- Map: `docs/architecture/edi-drafting.md`.
- Code: `src/drafting/DraftingCommands.cpp` (`applyDraftingCommand` guard,
  `applyTranslationPlan`, MoveSelection loop), `DraftingMirror.cpp`,
  `DraftingQuickMeasure.cpp`, `DraftingPlotPlan.cpp` (`kCircleSegments`).
- Tests: `tests/drafting_commands_tests.cpp` (locked-then-missing ordering),
  `drafting_mirror_tests`, `drafting_quick_measure_tests`, `drafting_plot_plan_tests`.
- Handoff: `docs/handoffs/drafting-20260616-cartography.md`.
- Related: `~/dept-bus/RULING-H2-src-drafting-boundary.md` (ownership), closeout
  `docs/closeouts/drafting-fill-side-channel.md`.
