# Handoff — edi (2026-06-10)

Cold-start handoff for the next session/model. Read this, then `CLAUDE.md`,
then the governing docs named below. The repo is `/Users/kogaryu/edi` — confirm
with `git rev-parse --show-toplevel` before any work; never touch
`/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE`.

## What edi is (and isn't)

A Qt6/C++20 widgets-based 2D drafting app, **and** a C++/data-oriented-design
learning project for the user — teaching value and clear reasoning outrank
speed and optimization. NOT related to the user's separate stock-trading work.
Hard rules (enforced every commit): **no JSON in our own data, no `.js`/`.qml`,
data-oriented design** (plain structs + free functions; variation as data or
plan callables; no subclassing for behavior). Commits: `claude: <summary>` with
a **teaching body** (why this design, what alternative lost) + the
`Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>` trailer. Full memory
context lives in `~/.claude/projects/-Users-kogaryu-edi/memory/`.

## State at handoff

- Branch `master`, last commit `e8530cb`. Working tree clean.
- **Tests: 58 passing** at the last full `ctest` run (a stale build cache may
  report fewer to `ctest -N` — run `cmake -S . -B build && cmake --build build
  && ctest --test-dir build --output-on-failure` to confirm).
- `git log origin/master..HEAD` reported 0, i.e. level with the remote — but
  **verify and `git push`** before trusting that; a lot of work landed locally
  this session.

## The two programs (both cold-executable)

1. **`/goal`** — drafting features. R1–R8 done (save/open, undo/redo,
   zoom/pan/keyboard, arc+polygon, SVG/HPGL, TOML settings, review, replenish).
   Next backlog: **N1 copy/paste → N2 polyline+arrow → N3 object metadata →
   N4 rectangle variants → N5 G-code**. Spec: `docs/rebuild_roadmap.md` (R-phases,
   historical) + `.claude/commands/goal.md` (live N-backlog).
2. **`/ui-goal`** — the modular **feature-host shell**. Governing design:
   `docs/shell_architecture.md` (READ FIRST for any shell work). Backlog
   **H1–H8**. The shell is a host that mounts composable *features* (drafting is
   feature #1) into *slots*; slot→feature layout is data (a `WorkspaceLayout`),
   redefinable per job. Look spec: `docs/ui_restoration_spec.md`; visual targets
   `docs/ui_reference/*.png`; legacy source to mine read-only via
   `git show ce0b751:<path>`.

## What this session did (most recent first)

- **God-file decomposition** of `EdiShellWindow.cpp` (2351 → 385 lines) into four
  behaviour-identical TUs of the same class — `EdiShellWindowPanels.cpp` (1035,
  the build* methods), `EdiShellWindowInspector.cpp` (434), `EdiShellWindowIo.cpp`
  (131) — plus the shared `ShellWidgetHelpers.{h,cpp}` (the 25 free helpers).
  These file boundaries ARE H2's feature seams. Done via subagents doing the
  mechanical cut/paste; each verified as a pure-deletion diff + full green.
- **H1 (theme model)**: `src/widgets/ShellTheme.{h,cpp}` — palette as a pure
  function of four inputs (base/surface/accent/text), porting the legacy
  `mix()`/`applyTheme()` exactly, golden-tested (`shell_theme_tests`,
  mutation-checked). NOT yet wired into `applyShellStyle` — that is H1b.
- **Scroll-area fix** (`3f0161e`): side panels were squashing controls into
  unclickable slivers (one column, ~185 widgets, no scroll). `makeScrollablePanel`
  fixed it. This is why the app is usable again.
- **Two correctness fixes** from reviewing the prior builder session: the
  interactive-edit bracket leaking across undo/redo/open (broke undo silently),
  and false-clean dirty tracking via revision aliasing — now an O(1) monotonic
  epoch. Both mutation-checked.

## Next steps, in priority order

1. **Push** (verify `git status` first).
2. **H1b — wire `ShellTheme` into `applyShellStyle`.** This is a LOOK decision and
   the user is sensitive about the UI. Default theme should reproduce today's
   look; the derived tokens differ slightly from the current hand-tuned literals
   (borders run less blue). The model can't see rendered colors — **have the
   user eyeball it**; do not finalize blind. Isolated single commit.
3. **H2 — host seam.** Promote `EdiShellWindowPanels.cpp` into a drafting
   `FeatureDescriptor`; turn `EdiShellWindow` into the generic `ShellHost` that
   mounts a one-binding `WorkspaceLayout`, behavior-identical. The decomposition
   already drew the seams. Fresh-context work — don't rush at the tail of a session.
4. **H3 — panel system** (real `QSplitter` resize/collapse/auto-hide/presets,
   replacing the scroll-area stopgap).

Decided (in `docs/shell_architecture.md`): activity rail = workspace switcher,
distinct from Top Chrome (which holds window controls + File/Edit menu + the
panel-collapse toggles); features are multi-slot. Still open (non-blocking):
per-job default layout, tool-tree content (full taxonomy vs wired-only).

## Working method that proved out

- **Verified slices**: one concern per commit, full `ctest` green + `.js`/`.qml`/
  `.json` scan + diff read before each commit. Revert-the-slice on any escaped
  failure.
- **Mutation-check every new test target once** (sabotage code, confirm abort,
  restore) — and force a hard rebuild around it (`rm` the target's `.o` + binary)
  because make's mtime granularity can run stale binaries on fast cycles.
- **Delegate mechanical bulk to subagents, own the verification gate** — keeps
  the main (cached) context lean. Always re-verify the subagent's claim
  independently (build/test + inspect the diff); a "pure move" is where subtle
  changes hide.
- Test gotcha: `rebuildGeometryEditor` retires spins with `deleteLater()` — flush
  `QEvent::DeferredDelete` before `findChild` lookups in widget tests.
- Stale `.git/index.lock`/`HEAD.lock` from a crashed commit: confirm zero-byte +
  only `fsmonitor--daemon` running, then remove.

## Resume

Run `/ui-goal` (shell, H1b→) or `/goal` (features, N1→). Both read their own
governing docs and need nothing from this conversation. `/loop /ui-goal` or
`/loop /goal` for an autonomous multi-iteration run.
