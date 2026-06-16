# Closeout — fill side-channel into SVG export

> Freezes a boundary so future work does not re-litigate it. When a campaign has
> established WHERE something lives and WHAT is allowed, record it here and link it
> from the ledger. This is the antidote to "building wrappers faster than we
> understood the boundary."

- **Boundary**: how per-object fill reaches the SVG writer (the fill side-channel)
- **Department**: edi-drafting
- **Campaign**: drafting-20260616-fill-svg
- **Date**: 2026-06-16

## The decision
Per-object **SOLID** fill is carried into export on a **separate side-channel**,
`vector<DraftingPlotFill>`, hanging on `DraftingPlotPlan`/`DraftingPlotJob` BESIDE
the stroke segments — NOT on the shared `DraftingPlotSegment` vocabulary. Each
record is `{objectId, points (a closed ring), color (#rrggbb), opacity}`.

`buildDraftingPlotPlan` collects one fill record per object that the canvas would
fill — the gate is `opacity > 0 && isValidStrokeColor`, over exactly the fillable
closed kinds the painter handles (**rectangle / circle / ellipse / polygon**).
`svgFromPlotJob` emits one closed `<path fill="#rrggbb" … Z/>` per fill record,
**before** the stroke-group loop (so fill renders UNDER the strokes), emitting
`fill-opacity` only when ≠ 1. HPGL and G-code read only `strokeSegments`, so they
ignore the fills and their golden output is unchanged. A default object (opacity 0)
produces no fill record, so the existing golden SVG bytes stay **byte-identical**.

## Why (the reasoning that must NOT be re-argued)
The SVG writer consumes `DraftingPlotJob`, a flat `DraftingPlotSegment` stream
SHARED with HPGL and G-code — pen-plotter / CNC targets that have no fill concept.
Two tempting shortcuts were rejected:
- **Add a fill field to `DraftingPlotSegment`/`DraftingPlotJob`'s segment vocabulary.**
  Rejected: it pollutes the shared plotter vocabulary with a paint concept the
  pen/CNC legs can never honor.
- **Hang `fill=` on the existing per-pen `<path>`.** Rejected: that path groups
  MANY objects under one pen, so a fill would bleed across every object sharing the
  pen. Fill must be per-object and emitted as its own closed path.

A separate side-channel keeps the plotter vocabulary clean, makes fill strictly
additive (zero impact on the pen/CNC legs and on default-document golden bytes),
and matches the canvas painter's existing per-object fill rule exactly.

**Tessellation note (settled):** circle fill uses **32** segments to MATCH its
stroke outline (not `sampleEllipse`'s 64), so the fill never peeks past the stroke;
ellipse fill reuses `sampleEllipse`.

## The contract (what future work must respect)
- Fill arrives by the `DraftingPlotFill` side-channel; NEVER add fill to
  `DraftingPlotSegment` or the stroke-segment stream.
- Fill is **per object**, emitted as one closed `<path>` per record, UNDER the
  strokes (before the stroke-group loop).
- The fillable set MUST stay in lockstep with the canvas painter
  (rectangle / circle / ellipse / polygon); changing one without the other is a bug.
- Fill gate is `opacity > 0 && isValidStrokeColor`; `fill-opacity` is emitted only
  when ≠ 1.
- Default objects (opacity 0) must keep producing byte-identical golden SVG — any
  new fill assertions are ADDITIVE.

## Out of scope / explicitly NOT allowed
- **Hatch / pattern fill** — solid only.
- **Layer-level fill** — none exists; do not invent one here.
- **Authoring legs** — the controller setter and the inspector UI that let a user
  SET a fill are NOT part of this boundary (the model + render path are; authoring
  is a later campaign, and the UI surface is edi-ui's).
- Changing the existing golden SVG bytes.

## Pointers
- Code: `src/drafting/DraftingPlotPlan.h` (`DraftingPlotFill`, `closedFillRing`,
  `appendPlotFill`, `applyCalibrationScale`, `buildDraftingPlotJob`);
  `src/drafting/DraftingSvgOut.cpp` (`svgFromPlotJob`); model at
  `DraftingTypes.h` (`FillStyle`) on `DraftingObject.fill` (`DraftingDocument.h`).
- Tests: `tests/drafting_plot_plan_tests.cpp` (fill-collection block),
  `tests/drafting_svg_out_tests.cpp` (additive fill assertions, golden preserved).
- Related: handoff `docs/handoffs/drafting-20260616-fill-svg.md`; charter
  `docs/departments/edi-drafting.md`; backlog `docs/drafting-gaps.md §0`.
