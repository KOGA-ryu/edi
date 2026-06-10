---
description: Long-horizon code-health program — work the phase backlog, one verified slice at a time, until it is empty
---

# Goal: code-health program

Work the phase backlog below in order, one verified slice at a time. On every
invocation: derive state from the repo, find the first incomplete phase, work it
until its Definition of Done holds, then move to the next. Do not pause between
slices or phases to ask permission. Pairs with `/loop /goal` for multi-session runs.

## Hard rules

- **No JSON** in project source (`.claude/` exempt). **No `.js`/`.qml`** — scans stay at zero.
- **Data-oriented design**: variation as data (enums, kinds, plan structs, member
  pointers, spec aggregates) or plan callables; pure logic in `src/drafting/`-style
  free functions over plain structs. No subclassing for behavior.
- **Behavior-preserving** unless the slice is explicitly a fix or a feature phase.
- Tests must be able to fail: when adding a test target, mutation-check it once
  (sabotage the code under test, confirm the test aborts, restore).

## State derivation (no state files)

- `git log --oneline` — done slices are `claude:` commits (`codex:` before 2026-06-10).
- `wc -l` on phase targets; `ctest --test-dir build -N` for the test inventory.
- A fresh scan of the phase's target files for remaining veins.

## Phase backlog

### Phase 1 — Projection dedup: `src/core/DrawingDocumentProjection.cpp` (~646 lines)
Scan for repeated QVariantMap-building atoms and functions differing only by
parameterizable data; extract per the established helper patterns.
**DoD:** no two functions differ only by a constant/enum/callable; repeated
map-building atoms behind shared helpers; suite green.

### Phase 2 — Canvas dedup: `src/widgets/DrawingCanvasWidget.cpp` + `DrawingCanvas*.cpp`
Same treatment. The `drawing_canvas_projected_*` tests already cover projections —
extend them when pure logic moves into testable free functions.
**DoD:** same dedup criterion across the canvas file family; suite green.

### Phase 3 — Core sweep: `DrawingCore.cpp`, `DrawingModelBuilder.cpp`, `DrawingSvgExport.cpp`, `src/io/*Store.cpp`
Same treatment, largest file first. Newly extracted pure logic gets a focused test
target registered in CMake.
**DoD:** same dedup criterion per file; new pure logic tested; suite green.

### Phase 4 — Canvas interaction tests
Offscreen test target driving `DrawingCanvasWidget` mouse paths (click-create,
drag-move, marquee select, handle edit) and asserting controller projections,
modeled on `edi_shell_window_tests`.
**DoD:** target registered and green; mutation-checked once; covers at minimum
create-by-click, selection, and drag-move.

### Phase 5 — Shell test extension
Extend `edi_shell_window_tests` to the wiring it does not yet cover: guide visuals
(label/color/dash/show-label), dimension kind combo, calibration row, nudge/align/
offset/mirror/repeat buttons, geometry editor spins (normalized and physical).
**DoD:** each listed control exercised with a controller-state assertion.

### Phase 6 — Review cycle
Run the find→verify review protocol (multiple independent finder angles, one
verifier per surviving candidate, REFUTED only with constructible evidence) over
all commits since the previous review marker (`claude: add shell window wiring tests`
or the most recent Phase 6 commit). Apply CONFIRMED/PLAUSIBLE findings as slices;
record skipped findings in the phase report.
**DoD:** every surviving finding either applied or explicitly skipped with reason.

### Phase 7 — Replenish or terminate
Update `CLAUDE.md` to match reality. Rewrite this backlog: delete finished phases,
append newly discovered work — each new phase MUST have an objective DoD and come
from evidence (a scan, a review finding, a failing invariant), never invented to
keep busy. If no qualifying work exists, write the final report and stop.

## Per-slice protocol

1. `git status --short` clean, else stop and report.
2. Smallest, lowest-risk slice of the current phase; one vein per commit.
3. Verify before commit: `cmake --build build` clean; `ctest --test-dir build
   --output-on-failure` fully green; no `.js`/`.qml` anywhere, no `.json` outside
   `.claude/`, no QtQml/QtQuick refs; read the diff for behavior preservation.
4. Commit `claude: <imperative summary>` + short body + the Co-Authored-By trailer.
   If `.git/index.lock`/`HEAD.lock` blocks, confirm zero-byte stale (only
   `fsmonitor--daemon` running) before removing.
5. Repeat. At phase DoD, state the phase result in one paragraph, continue.

## Stop conditions

- Test failure that escapes the current slice → revert the slice, report.
- A slice needs a behavior change or API decision not covered by the phase
  definition → skip it, note it, continue with the next slice.
- Phase 7 finds no qualifying work → final report (line counts, test count,
  phases completed, skipped items) and stop.
