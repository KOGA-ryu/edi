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

### Builder batch — RUN 2026-06-16 (builder, dept/drafting)
All three slices implemented and committed on `dept/drafting`. Green gate run after each slice.

- **Prereq fix (own commit `3f2c068`)** — `tests/{drafting_room,drafting_ascii_map,drafting_corridor}_tests.cpp` were missing `#include <memory>` (a toolchain change dropped the transitive include), so `std::make_shared` failed to resolve and the WHOLE build broke before any test ran. Pre-existing on `dept/drafting`, unrelated to fill, but it blocked the gate for all drafting work — fixed first, in its own commit, to keep the fill slices clean.
- **Slice 1 (`a225491`)** — `DraftingPlotFill {objectId, points (closed ring), color, opacity}` added to `DraftingPlotPlan.h` + `fills` vector on `DraftingPlotPlan` and `DraftingPlotJob`. `closedFillRing` (an if-constexpr visit mirroring the painter's fillable set: rectangle/circle/ellipse/polygon) + `appendPlotFill` (gate: `opacity>0 && isValidStrokeColor`) collect into the plan; `applyCalibrationScale` scales fill points too; `buildDraftingPlotJob` copies `fills`. HPGL/G-code untouched (they read only `strokeSegments`) — their tests stay green. **Resolved the tessellation flag:** circle fill uses **32** segments to MATCH its stroke outline (not sampleEllipse's 64) so fill never peeks past the stroke; ellipse reuses `sampleEllipse`. Test: new fill-collection block in `drafting_plot_plan_tests.cpp`.
- **Slice 2 (`2d48501`)** — `svgFromPlotJob` emits one closed `<path fill="#rrggbb" … Z/>` per `DraftingPlotFill`, BEFORE the stroke-group loop (fill-under-stroke z-order), `fill-opacity` only when ≠1 (gated on formatted text like stroke-opacity). Golden bytes unchanged (fill-less job → no fill path → byte-identical); additive assertions added to `drafting_svg_out_tests.cpp`.
- **Slice 3 (`170bccf`)** — corrected `docs/drafting-gaps.md §0`: fill is rendered end-to-end (canvas + SVG); remaining gaps narrowed to the authoring legs (controller setter + inspector UI) + solid-only. Docs-only.

**Green gate:** `cmake --build build` clean; `ctest` **95/95 pass** with ONE test excluded — see blocker below. Scan clean (no `.js`/`.qml`, no `.json` outside `.claude/`, no QtQml/QtQuick).

## Open questions / blockers
- **PRE-EXISTING, OUT-OF-CHARTER (not resolved):** `edi_shell_window_tests` fails on a **golden PNG pixel-budget mismatch** (6508 differing pixels vs budget 4180 against `tests/golden/default_shell_1100x760.png`). This is a UI-chrome rendering golden owned by **edi-ui**, and the drift is environmental (font/rasterization), NOT caused by this campaign — the fill work touches zero rendered chrome (default doc has no filled objects; canvas fill predates this work). I did NOT regenerate the golden: that is edi-ui's artifact and a machine-specific regen would be the wrong fix from this seat. Flagging for the HUB to route to edi-ui. All other 95 tests pass; my fill slices are fully green.
- Boundary otherwise settled and respected: per-object, solid-only, additive output, no change to the plotter segment vocabulary.

## Next
- Run the builder batch in the `/Users/kogaryu/edi-drafting` worktree (rebase master → implement slices 1–3 → green gate → commit to `dept/drafting`), then write the closeout freezing the side-channel boundary.
