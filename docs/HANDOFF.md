# Handoff — edi (2026-06-10, session 3)

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

- `master` green: **65 tests passing**, working tree clean. H1-H5 done, the
  settings program, and the **entire F-backlog (F1-F6) plus three feedback
  rounds** — see "Shell backlog status". Verify with `cmake -S . -B build &&
  cmake --build build && ctest --test-dir build --output-on-failure`.
- **The user owns the look.** Do structural/mechanical shell work; do NOT
  spend slices polishing visuals, and do not finalize any visible change
  without flagging it. The user gives screenshot-driven feedback rounds —
  apply them as small verified slices.

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
- **H4 — DONE** (built with the user in the loop, to their authoritative
  description in docs/shell_architecture.md): frameless title bar = traffic
  lights → left-panel toggle → back/forward (workspace-history trail, the
  "rabbit hole"; NOT undo/redo) → File/Edit/Settings menus → chrome status
  label → terminal + right toggles. Drag/double-click via event filter.
  POST-H4 LAYOUT REWORK (user feedback): only the LEFT panel is in-flow
  (2-section splitter); right + bottom panels are OVERLAYS inside the main
  area (layoutMainArea + grip strips) — they cover the grid, never resize it;
  the terminal (bottom) has no max size and can grow into the whole main
  area. The main slot is the canvas ONLY (workspace header deleted; status
  goes through ShellActions.setStatusText into the chrome).
- **H5 — DONE.** `ShellLayoutStore` is real (TOML encode/decode + round-trip
  tests; forgiving decode). Panel geometry persists across restarts
  (`workspace.toml` beside `edi.toml`). The drafting UI was extracted into
  `DraftingFeature` (talks to the shell only through `ShellActions`
  callables; window keeps chrome/IO/settings/panels), which unblocked
  **runtime workspace switching**: `switchWorkspaceLayout()` deletes the slot
  widgets, retires the feature instance (members die with it), recreates, and
  remounts; `loadWorkspaceLayout` applies bindings (switches when they
  differ). Splitter size apply/capture resolve sections by widget identity —
  positional assumptions break under partial layouts. The mutation check is
  the design proof: reusing the old feature instance across a switch
  SEGFAULTS the suite.
- **Settings program — DONE** (user-requested, post-H5): `setThemeInputs` is
  live theming (one derivation feeds shell QSS + canvas palette; theme.* keys
  in edi.toml via the shared codec in `io/ProfileStore`); `SettingsFeature`
  is the registry's second feature (rail "S" switches to it; page = 4 color
  rows + typography + profiles, stateless view over hooks); profiles are
  per-name TOML files under <AppConfigLocation>/profiles with sanitized
  names; edi.toml always holds the live theme so startup never needs a
  profile file.
- **H7 review cycle — DONE** (7-angle find->verify over the session diff;
  fixes applied: grip clamp, settings dedup, shared config-path helper;
  refuted claims recorded in the commit body of 83c6b2d).
- **Recipe lab slice 1 — DONE**: feature lifecycle is registry data
  (FeatureDescriptor.recreateInstance / instanceMounted; ctor and switches
  share one path; mutation = segfault proof).
- **F-backlog — ALL DONE (F1-F6 + feedback rounds).** Authoritative status
  block at the top of shell_architecture.md §"The Figma restructure".
  Summary: F1 object list; F2 contextual inspector (pure plan table
  `DraftingInspectorPlan`, interim "document" context under select tool);
  F3 belt as carousel — one row per tool, sub-features along the row,
  half-cell peeks, notch-accumulated scrolling, occupancy-aware navigation
  (skips empties), row PINNING ("+" nub freezes a quick bar, "×" kills it;
  pins not persisted — flagged); F4 chromeless floating palettes
  (`FloatingPalette`, `buildPalettes` seam, placements in workspace.toml);
  chrome work: Snap popup in the title bar (`buildChromePanels` seam), left
  panel = object list only, undo/redo + files + recents in the menus;
  F5 settings pop-out (Qt::Tool window; settings workspace deleted);
  F6 settings page table + Tool Belt checklist (hooks: toolInventory /
  beltToolIds / setBeltToolIds; live re-dress, no remount).
- **Open threads (deliberate)**: pinned-row persistence (joins belt in
  TOML when wanted); right panel's interim "document" context awaits F4/F5
  homes for layers/plot/calibration; free-form belt cell placement (format
  already supports it); belt glyphs are 2-letter placeholders (user owns
  look).
- **H6 superseded by the F-backlog; H8 replenish** — later.

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
- **QSS changes require launching the app**, not just ctest: Qt's chained
  multi-.arg() fills the LOWEST remaining numbered placeholder, so deleting a
  %N use silently shifts every later value with NO leftover "%" to assert on
  (the suite stayed green on scrambled styling). buildShellStyleSheet now
  uses @named@ tokens from an explicit table — keep it that way, and keep
  the per-value presence assertions in shell_theme_tests.

## Working method addenda (this session)

- **Registry seams now number three**: buildPanel (slots), buildPalettes
  (floating, F4), buildChromePanels (title-bar popups). A feature bound to
  several slots is served once per seam; all torn down/rebuilt per mount.
- **Live-edit contract**: settings edits (theme, belt) re-dress live
  widgets in place. NEVER remount from inside a settings control — the
  handler's own widget dies mid-click (the F6 commit body has the details).
- The belt component (`BeltState` + `BeltCrossWidget`) is deliberately
  reusable/game-grade: occupancy and pins are data, the widget knows
  nothing about drafting. Treat it as a library when feature #2 wants UI.

## Resume

Run `/ui-goal` — next: **feature #2, the Blender recipe lab**
(shell_architecture.md §"Feature #2"), which forces the first real
FeatureContext bus work (grid/canvas Main, text editor + ASCII render in
the terminal, scripts in Right). Or `/goal` (drafting features, N1
copy/paste →). Belt polish rounds (pin persistence, glyphs, free-form
placement) are quick slices if the user asks.
