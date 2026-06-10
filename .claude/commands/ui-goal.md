---
description: Shell program — build the modular feature host (and its look) in pure widgets, one verified slice at a time
---

# Goal: shell / feature-host program

You are the dedicated shell session. Build edi's **modular feature host**: a
shell that mounts composable features (drafting, text editor, ASCII preview,
asset taxonomy…) into slots, where the slot→feature layout is data. The drafting
tool is feature #1, not the app.

READ FIRST, in order:
1. `docs/shell_architecture.md` — the host model (Feature/Slot/Workspace/
   ShellHost), the feature-context bus, the data-format policy, the migration
   plan, and the H1–H8 backlog. This is the governing design.
2. `docs/ui_restoration_spec.md` — exact theme tokens, layout constants, and
   component treatments (validated against the canonical `UiStyle.qml`).
3. `CLAUDE.md` — conventions (no JSON, no `.js`/`.qml`, data-oriented design,
   teaching documentation in commits and non-obvious code).

Visual targets: `docs/ui_reference/*.png`. Behavioral reference for the legacy
shell: `git show ce0b751:<path>` (never check out).

The backlog is **H1–H8 in `docs/shell_architecture.md`**, not the U-phases
below (kept only as the look-and-feel detail H1/H3/H4/H6 draw on).

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
