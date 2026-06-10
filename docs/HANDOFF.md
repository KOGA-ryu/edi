# Handoff — edi (2026-06-10, session 2)

Cold-start handoff for the next session/model. Read this, then `CLAUDE.md`,
then the governing docs named below. The repo is `/Users/kogaryu/edi` — confirm
with `git rev-parse --show-toplevel` before any work; never touch
`/Users/kogaryu/draft/draftsman_STALE_PARENT_DO_NOT_USE`. Shell work happens in
the worktree `/Users/kogaryu/edi-ui` on branch `ui-restoration` (rebase on
master per slice, merge to master at phase DoD).

## What edi is (and isn't)

A Qt6/C++20 widgets-based 2D drafting app, **and** a C++/data-oriented-design
learning project for the user — teaching value and clear reasoning outrank
speed and optimization. NOT related to the user's separate stock-trading work.
Hard rules (enforced every commit): **no JSON in our own data, no `.js`/`.qml`,
data-oriented design** (plain structs + free functions; variation as data or
plan callables; no subclassing for behavior). Commits: `claude: <summary>` with
a **teaching body** (why this design, what alternative lost) + the
`Co-Authored-By` trailer. Full memory context lives in
`~/.claude/projects/-Users-kogaryu-edi/memory/`.

## State at handoff

- `master` green: **62 tests passing**, working tree clean, H1/H2/H3/H5-partial
  merged. Verify with `cmake -S . -B build && cmake --build build && ctest
  --test-dir build --output-on-failure`.
- **The user owns the look.** They said the UI is far from what they want and
  they will iterate it themselves. Do structural/mechanical shell work; do NOT
  spend slices polishing visuals, and do not finalize any visible change
  without flagging it.

## The two programs (both cold-executable)

1. **`/goal`** — drafting features. R1–R8 done. Next backlog: **N1 copy/paste →
   N2 polyline+arrow → N3 object metadata → N4 rectangle variants → N5 G-code**.
2. **`/ui-goal`** — the modular **feature-host shell**. Governing design:
   `docs/shell_architecture.md` (READ FIRST). Look spec:
   `docs/ui_restoration_spec.md`; targets `docs/ui_reference/*.png`; legacy via
   `git show ce0b751:<path>` (read-only).

## Shell backlog status (H1–H8)

- **H1 — DONE.** Shell QSS renders from `ShellTheme` tokens
  (`buildShellStyleSheet`); canvas chrome colors live in
  `DrawingCanvasPalette` derived from the theme. Zero hard-coded hex in
  rendering paths outside the two palette-definition files. Known fossil: the
  spec §1 *value column* lists `textMuted #9aa8b6`, but that was the legacy
  QML's pre-`applyTheme()` declared default; the derivation column
  (`surface ⊕ text 62%` → `#9199a1`) is canonical and implemented.
- **H2 — DONE.** `ShellHost.{h,cpp}`: `ShellSlot` / `FeatureDescriptor` /
  `FeatureRegistry` / `WorkspaceLayout` / `mountWorkspaceLayout`. The window
  assembles from a registry + a drafting-in-all-slots layout;
  `shell_host_tests` mounts a fake 2-slot feature. Gotcha: the descriptor
  member is `supportedSlots` because Qt #defines `slots`.
- **H3 — core DONE.** `ShellPanels.{h,cpp}`: panel state =
  f(manual collapse, auto-hide, presets); QSplitters with 8px handles and
  min/max bands from `panelSpec()`. Initial state per spec: left open,
  right+bottom collapsed; window min is now 520x420. Splitter-handle *look*
  (1px line treatment) deliberately left minimal — user owns look.
- **H4 — NOT STARTED, deliberately deferred.** Frameless chrome + traffic
  lights + title bar + rail restyle is the most look-sensitive phase; do it
  when the user is in the loop. The *mechanics* (toggle buttons consuming
  `panelVisibility`, `startSystemMove`, plain-frame fallback boolean) are
  spec'd in `docs/ui_restoration_spec.md` §3.
- **H5 — half DONE.** `ShellLayoutStore` is real (TOML encode/decode +
  round-trip tests; forgiving decode: named slots, clamped sizes, dropped bad
  rows, plan-struct `ok`). Panel geometry persists across restarts
  (`workspace.toml` beside `edi.toml`; seams `loadWorkspaceLayout`/
  `saveWorkspaceLayout`, saved in `closeEvent`, loaded in `main()`).
  **Remaining: runtime workspace switching** — BLOCKED on extracting the
  drafting feature out of the window first: panel builders append to member
  collections (`m_conditionalButtons`, `m_geometryFields`, …) under a
  built-once assumption, so tearing down/rebuilding slots would dangle
  pointers behind `refreshInspector`. Extract the drafting feature into a
  module whose lifetime matches its widgets (the H2 commit body records this
  asymmetry), THEN switching is a small slice.
- **H6 component pass / H7 review / H8 replenish** — untouched.

## Working method that proved out

- **Verified slices**: one concern per commit, full `ctest` green +
  `.js`/`.qml`/`.json` scan + diff read before each commit.
- **Mutation-check every new test target once** (sabotage, confirm abort,
  restore). Two hard-won rules: (1) restore from a `/tmp` backup, NEVER
  `git checkout` — it nukes uncommitted slice work; (2) force the rebuild by
  deleting the target's `.o` (delete only the object subtree, not the whole
  `.dir` — that holds makefiles), because same-second mtimes make `make` run
  stale binaries. `strings` can't see `QStringLiteral` content (UTF-16) —
  verify binaries with a UTF-16 search if it matters.
- Offscreen widget tests: Qt defers resize events for hidden widgets —
  `window.show()` before resize-driven assertions; `isVisibleTo(&window)`
  reads panel visibility without a shown window.
- Test gotcha: `rebuildGeometryEditor` retires spins with `deleteLater()` —
  flush `QEvent::DeferredDelete` before `findChild` lookups.

## Resume

Run `/ui-goal` (shell; next: drafting-feature extraction → workspace
switching, or H4 with the user present) or `/goal` (features, N1→).
