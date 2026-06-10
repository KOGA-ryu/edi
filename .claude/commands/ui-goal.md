---
description: UI restoration program — reproduce the legacy shell look in pure widgets, one verified slice at a time
---

# Goal: UI restoration program

You are the dedicated UI session. Reproduce the legacy QML shell's look and
behavior in pure C++ Qt Widgets. Spec: `docs/ui_restoration_spec.md` (READ IT
FIRST — exact tokens, constants, behaviors). Visual targets:
`docs/ui_reference/*.png`. Conventions: `CLAUDE.md` (no JSON, no `.js`/`.qml`,
data-oriented design, teaching documentation in commits and non-obvious code).

## Isolation — a feature session works this repo in parallel

- Confirm identity: `git rev-parse --show-toplevel` = `/Users/kogaryu/edi`
  (or your worktree of it). Never touch
  `/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE`.
- **Work in a dedicated worktree+branch**, not the main checkout: if missing,
  `git worktree add /Users/kogaryu/edi-ui -b ui-restoration` (needs its own
  `build/` — `cmake -S . -B build` once). All slices commit to
  `ui-restoration`.
- Rebase on `origin/master`/`master` at the START of every iteration and
  MERGE to master only at a phase DoD with the full suite green. The feature
  session also edits `EdiShellWindow.*` and `CMakeLists.txt` — small slices
  and frequent rebases are your conflict strategy; at conflicts, master's
  feature changes win, your styling re-applies on top.
- File ownership: you own shell chrome — `EdiShellWindow.*`, new
  `src/widgets/Shell*.{h,cpp}`, `applyShellStyle`, `tests/edi_shell_window_tests.cpp`
  extensions. You do NOT redesign the controller, the drafting core, or the
  canvas interaction logic (styling canvas *colors* onto theme tokens is yours).

## Phase backlog

- **U1 — Theme tokens as data.** `src/widgets/ShellTheme.{h,cpp}`: a plain
  `ShellTheme` struct (base/surface/accent/text inputs + fixed semantics) +
  pure `derived-token` function (the ⊕ mixing from the spec §1) + a QSS
  builder replacing the hard-coded hex in `applyShellStyle`. Unify canvas
  painter colors onto the same tokens (pass or expose tokens — keep the canvas
  family Qt-light). Tests: token derivation golden values from spec §1
  (`shell_theme_tests`, pure, no widgets). DoD: app renders from tokens,
  zero hard-coded hex outside ShellTheme defaults, suite green,
  mutation-checked.
- **U2 — Frameless chrome + title bar.** Spec §3: traffic lights, drag via
  `startSystemMove()`, 42px bar, divider. Keep a plain-frame fallback behind a
  single boolean (data) so offscreen tests and debugging stay easy. DoD:
  screenshot-comparable to `default_shell_1280x820.png` chrome; window
  close/min/max work; tests still pass offscreen.
- **U3 — Panel system.** Collapsible left/right/bottom with 8px splitter hit
  zones, sizes/limits/auto-hide rules and the four presets from spec §2 —
  model panel state as data (`panelState()` from collapse+auto-hide inputs).
  Panel toggle buttons in the title bar per spec §3. Persisting sizes joins
  R6 (TOML) later — design the state struct to be serializable. DoD: presets
  + drag-resize + auto-hide verified in shell tests (resize the window
  offscreen and assert states).
- **U4 — Rail + status bar restyle.** Activity rail to 52px/34px-buttons spec;
  status bar 28px with mode + dirty indicator (dirty state exists since R1 —
  `isDocumentDirty()`). DoD: matches spec measurements; tests assert status
  text reacts to dirty flag.
- **U5 — Component treatment pass.** Restyle existing factory output to spec
  §4 (button/tab/row metrics, chip widget if a consumer exists). Only build
  components something consumes. DoD: visual pass against the reference
  screenshots; suite green.
- **U6 — Review cycle** over `ui-restoration` since branch point (find→verify
  protocol; apply CONFIRMED/PLAUSIBLE; skip with reasons), then merge to
  master.
- **U7 — Replenish or terminate**: re-derive remaining UI gaps from the spec
  + screenshots; new phases need objective DoD; else final report and stop.

## Per-slice protocol

1. Rebase on master; `git status --short` clean in YOUR worktree, else stop.
2. Smallest slice; pure data/functions (+ tests) before widget wiring.
3. Verify: build clean; full ctest green; no `.js`/`.qml`; no `.json` outside
   `.claude/`; diff read. Mutation-check new test targets once (hard rebuilds
   around mutate/restore — mtime granularity runs stale binaries).
4. Commit `claude: <imperative summary>` + teaching body (why this design,
   what alternative lost) + Co-Authored-By trailer.
5. At phase DoD: one-paragraph result, merge to master if suite green, continue.

## Stop conditions

- Escaped test failure → revert slice, report.
- A look/feel decision the spec + screenshots cannot settle → smallest
  reasonable choice, documented in the commit; stop only if outward-facing or
  destructive.
- Backlog empty after U7 → final report, stop.
