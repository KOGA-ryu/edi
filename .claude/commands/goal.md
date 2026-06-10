---
description: Long-horizon code-health program — work the phase backlog, one verified slice at a time, until it is empty
---

# Goal: code-health program

Work the phase backlog below in order, one verified slice at a time. On every
invocation: derive state from the repo, find the first incomplete phase, work it
until its Definition of Done holds, then move to the next. Do not pause between
slices or phases to ask permission. Pairs with `/loop /goal` for multi-session runs.

## Status: backlog empty (2026-06-10)

All seven phases of the first program run are complete:
1. ✅ Projection dedup (handle visual spec, segment atoms) — 3 slices.
2. ✅ Canvas dedup (withAlpha, boundsToScreenRect→viewport, tool set as data,
   screenLine, drawCrosshair, DrawingCanvasValues module) — 7 slices.
3. ✅ Core sweep — completed empty: the listed files are 4–11-line stubs and the
   io stores are placeholders awaiting the persistence format decision. The
   original backlog estimate was wrong; recorded here so it is not re-queued.
4. ✅ Canvas interaction tests (`drawing_canvas_widget_tests`, mutation-checked).
5. ✅ Shell test extension (guide visuals, dimension, geometry spins, transforms,
   calibration; mutation-checked).
6. ✅ Review cycle over `8627a67..` — 5 findings, 4 applied, 1 skipped with
   reason (rotated-rectangle normalization not provably behavior-preserving).
7. ✅ Replenish audit — no qualifying evidence-based work remains in scope.

Known work that is OUT of scope for this program (needs user decisions):
- Text-editor surface + persistence format (stores are stubbed; format must not
  be JSON — a design decision, then feature work).
- Angled-construction-tool creation preview gap (behavior change; spawned as a
  user-approvable task chip).
- `drawObject`'s rotated rectangle draws the un-normalized rect; whether
  negative-extent rectangles should normalize there is a behavior question.

To restart the program, replace this Status section with a new evidence-based
phase backlog (each phase MUST have an objective DoD and come from a scan, a
review finding, or a failing invariant — never invented to keep busy).

## Hard rules

- **No JSON** in project source (`.claude/` exempt). **No `.js`/`.qml`** — scans stay at zero.
- **Data-oriented design**: variation as data (enums, kinds, plan structs, member
  pointers, spec aggregates) or plan callables; pure logic in `src/drafting/`-style
  free functions over plain structs. No subclassing for behavior.
- **Behavior-preserving** unless the slice is explicitly a fix or a feature phase.
- Tests must be able to fail: when adding a test target, mutation-check it once
  (sabotage the code under test, confirm the test aborts, restore — and force a
  hard rebuild of the target's objects around the mutate/restore, since fast
  cycles can land within make's mtime granularity and run stale binaries).

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
- Backlog empty → final report (line counts, test count, phases completed,
  skipped items) and stop.
