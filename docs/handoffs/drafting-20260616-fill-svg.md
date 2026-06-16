# Handoff — drafting-20260616-fill-svg

- **Campaign**: drafting-20260616-fill-svg
- **Department**: edi-drafting
- **Goal (one line)**: wire the dormant `fill` data model into SVG export (today the SVG writer emits `fill='none'`).
- **Boundary (the question the reviewer gate must settle)**: where does fill live, what fill kinds exist, and exactly how should a filled shape be emitted in SVG — without inventing a new model or duplicating paint logic?

## Gate log

### Research gate — SKIPPED
- The missing input is an OWNERSHIP/repo question (where does fill live, what's allowed), not external/reference knowledge — so per the gate discipline this opens at the reviewer gate, not the research gate.

### Reviewer gate — 2026-06-16 — general-purpose (briefed as edi-drafting reviewer)
- **PREMISE CORRECTED.** Fill is NOT dormant — `docs/drafting-gaps.md` is stale. Fill is authored, validated, persisted, and ALREADY RENDERED ON THE CANVAS (`DrawingCanvasObjectPainter.cpp:464-480`). Only the SVG leg is missing.
- **Ownership:** model = `FillStyle {opacity, color}` (`DraftingTypes.h:139`) on `DraftingObject.fill` (`DraftingDocument.h:17`); the `fill="none"` literal is in `svgFromPlotJob` (`DraftingSvgOut.cpp:79`); SVG entry = `exportSvgDocument` (`DrawingDocumentController.cpp:635`).
- **THE BOUNDARY TRAP (why this gate mattered):** the SVG writer consumes `DraftingPlotJob` — a flat `DraftingPlotSegment` stream SHARED with HPGL + G-code (pen plotters, no fill concept). Do NOT add a fill field to the segment/job vocabulary (pollutes HPGL/G-code), and do NOT hang `fill=` on the existing per-pen `<path>` (it groups many objects → fill bleeds across them). Fill must arrive by a SEPARATE channel and be a separate closed `<path>`/`<polygon>` per object.
- **Reuse (don't re-implement):** closed-ring samplers `sampleEllipse`/`sampleArc`/`sampleSpline` (`DraftingGeometry.h:48-60`); `draftingStrokeColorIsValid` (`DraftingPlotPlan.h:123`); the canvas fill rule `opacity>0 && valid colour` (`DrawingCanvasObjectPainter.cpp:475`).
- **Allowed:** per-object SOLID fill only; emit under the strokes; only the closed kinds the painter fills (rectangle/circle/ellipse/polygon); emit `fill-opacity` only when ≠1; default objects (opacity 0) stay BYTE-IDENTICAL.
- **Not allowed:** fill on `DraftingPlotSegment`; layer-level fill (none exists); hatch/pattern (out of scope — solid only); changing the existing golden SVG bytes (additive assertions only).
- **Verdict: BOUNDARY SETTLED = YES.** The one fork (how fill reaches the writer) is resolved: a fill side-channel on `DraftingPlotJob`, NOT a document param.

### Builder batch — SCOPED (ready; not yet run)
1. **Slice 1 — plumbing.** `buildDraftingPlotPlan` collects `vector<DraftingPlotFill>{objectId, closed-ring pts, #rrggbb, opacity}` for closed/fillable/opacity>0 objects, carried on `DraftingPlotJob` beside the stroke segments. SVG reads it; HPGL/G-code ignore it (verify their tests stay green).
2. **Slice 2 — emit.** In `svgFromPlotJob`, before the stroke loop, emit one closed `<path fill="#rrggbb"` (+ `fill-opacity` when ≠1) per fill record, reusing the ring samplers + the file's number formatting. New assertions in `tests/drafting_svg_out_tests.cpp`; preserve existing golden bytes.
3. **Slice 3 — cleanup.** Correct `docs/drafting-gaps.md §0` (fill IS rendered now).
   - Hidden-complexity flags carried for the builder: solid-only (hatch out), fill-opacity is real, closed-set must match the painter exactly, z-order fill-under-stroke, and the circle-tessellation mismatch (32 in plot plan vs 64 in `sampleEllipse` — pick one).

## Open questions / blockers
- None blocking — boundary settled. (Builder must respect: per-object, solid-only, additive output, no change to the plotter segment vocabulary.)

## Next
- Run the builder batch in the `/Users/kogaryu/edi-drafting` worktree (rebase master → implement slices 1–3 → green gate → commit to `dept/drafting`), then write the closeout freezing the side-channel boundary.
