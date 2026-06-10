---
description: Long-horizon program — work the phase backlog, one verified slice at a time, until it is empty
---

# Goal: rebuild program

Work the phase backlog below in order, one verified slice at a time. On every
invocation: derive state from the repo, find the first incomplete phase, work it
until its Definition of Done holds, then move to the next. Do not pause between
slices or phases to ask permission. Pairs with `/loop /goal` for multi-session runs.

## The backlog

The detailed implementation spec for every phase is `docs/rebuild_roadmap.md` —
READ IT FIRST (it is written to be executed cold, including designs, file
locations, test plans, and per-phase DoD). Behavioral reference for the
features being restored: `docs/legacy_inventory.md`.

- **R1 — MessagePack value codec + drawing save/open** (roadmap §R1)
- **R2 — Undo/redo** (roadmap §R2)
- **R3 — Zoom, pan, and the keyboard map** (roadmap §R3)
- **R4 — Arc and regular polygon tools** (roadmap §R4)
- **R5 — Plot output: SVG export + HPGL emitter** (roadmap §R5)
- **R6 — TOML settings persistence** (roadmap §R6)
- **R7 — Review cycle**: run the find→verify review protocol (multiple
  independent finder angles, one verifier per surviving candidate, REFUTED only
  with constructible evidence) over all commits since the marker
  `claude: distill the legacy version into a rebuild spec`. Apply
  CONFIRMED/PLAUSIBLE findings as slices; record skips with reasons.
- **R8 — Replenish or terminate**: update CLAUDE.md and the roadmap to match
  reality; rewrite this backlog from evidence (a scan, a review finding, a
  failing invariant, or the remaining items in `docs/legacy_inventory.md` —
  e.g. rulers, tool variants, object metadata, splines, hatching, SVG import);
  each new phase MUST have an objective DoD. If nothing qualifies, final
  report and stop.

OUT OF SCOPE for autonomous work (user decisions or user's own projects):
- The **text editor** surface — the user's personal learning project. Never
  build it autonomously; mechanical cleanup touching `src/text/` is fine.
- Multi-workspace shell / activity rail / theme UI — needs user design input.

## Hard rules

- **No JSON** in project source (`.claude/` exempt). **No `.js`/`.qml`** — scans stay at zero.
- **Data-oriented design**: variation as data (enums, kinds, plan structs, member
  pointers, spec aggregates) or plan callables; pure logic in `src/drafting/`-style
  free functions over plain structs. No subclassing for behavior.
- These phases are features: behavior-additive is expected, but never change
  existing behavior silently — call it out in the commit body when it happens.
- Tests must be able to fail: when adding a test target, mutation-check it once
  (sabotage the code under test, confirm the test aborts, restore — and force a
  hard rebuild of the target's objects around the mutate/restore, since fast
  cycles can land within make's mtime granularity and run stale binaries).
- Confirm repo identity before working: `git rev-parse --show-toplevel` must be
  `/Users/kogaryu/edi`. Never touch
  `/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE`.

## Per-slice protocol

1. `git status --short` clean, else stop and report.
2. Smallest, lowest-risk slice of the current phase; within a phase land the
   pure core (+ tests) before the controller before the shell.
3. Verify before commit: `cmake --build build` clean; `ctest --test-dir build
   --output-on-failure` fully green; no `.js`/`.qml` anywhere, no `.json` outside
   `.claude/`, no QtQml/QtQuick refs; read the diff.
4. Commit `claude: <imperative summary>` + short body + the Co-Authored-By trailer.
   If `.git/index.lock`/`HEAD.lock` blocks, confirm zero-byte stale (only
   `fsmonitor--daemon` running) before removing.
5. Repeat. At phase DoD, state the phase result in one paragraph, continue.

## Stop conditions

- Test failure that escapes the current slice → revert the slice, report.
- A slice needs a design decision the roadmap does not settle → make the
  smallest reasonable choice, document it in the commit body, and continue;
  only stop if the choice is destructive or outward-facing.
- Backlog empty after R8 → final report and stop.
