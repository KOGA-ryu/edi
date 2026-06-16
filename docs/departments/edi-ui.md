# Department charter — edi-ui

The widget SHELL and its LOOK: a modular feature host that mounts composable
features into slots, where the slot→feature layout is data. Pure widgets,
deliberately no QML. **The look is the user's** — implement to spec, flag
anything the spec can't settle.

## Scope (what this department owns)
- `src/widgets/` — `EdiShellWindow.*`, the `Shell*` widget family, the
  `DrawingCanvas*` chrome/colors, `applyShellStyle`, `ShellTheme`, the workspace
  layouts (`draftingWorkspaceLayout`, `mapWorkspaceLayout`, the Blender lab
  layout), and the FeatureContext bus.
- `tests/edi_shell_window_tests.cpp` extensions (offscreen, widgets by
  objectName) and `drawing_canvas_widget_tests.cpp` (chrome/paint).

You own the HOST + chrome + theme. You do NOT redesign the controller, the
drafting core, or canvas INTERACTION logic (that's edi-drafting; styling canvas
COLORS onto theme tokens is yours). Other departments own the CONTENT of their
feature panels (the recipe lab panels are edi-blender-lab's; the map browser is
edi-dungeon-map's) — you own where and how panels mount, and the look.

## Architecture (the rules to obey)
DATA-ORIENTED shell: the slot→feature layout is DATA; theme is a plain
`ShellTheme` struct + a pure derived-token function feeding a QSS builder (no
hard-coded hex outside `ShellTheme` defaults); panel state is DATA from
collapse/auto-hide inputs. Keep the canvas family Qt-light (paint from theme
tokens). Pure WIDGETS — no `.qml`/`.js`/QtQml/QtQuick, ever. Variation is data,
never subclassing a widget for behavior.

Signal-safety is the shell's subtle part: a widget commits only on USER signals
(`editingFinished`/`activated`/`clicked`), never on a programmatic refresh, so a
re-render can't loop; rebuild field widgets only on a selection / field-shape
change, never on a value commit; defer a context-menu action via
`QTimer::singleShot(0)` so it doesn't delete a widget mid-event. `rebuildGeometryEditor`
retires spins with `deleteLater()` — flush `QEvent::DeferredDelete` before widget
lookups. Features are rebuilt per mount, so watch for stale connections / leaked
panels across a workspace switch.

## Read first
- `docs/shell_architecture.md` — the governing host design (Feature / Slot /
  Workspace / ShellHost, the FeatureContext bus, the H1–H8 backlog).
- `docs/ui_restoration_spec.md` — exact theme tokens, layout constants, component
  treatments (validated against the reference `UiStyle.qml` — REFERENCE ONLY;
  never add QML).
- `docs/ui_reference/*.png` — the visual targets.
- Memories: `edi-figma-restructure`, `edi-chrome-layout`,
  `edi-drafting-surface-priority`.

## Verify (the green gate for this department)
```
cmake --build build && ctest --test-dir build --output-on-failure   # incl. edi_shell_window_tests
```
plus the scan. Verify the LOOK, not just the tests:
`QT_QPA_PLATFORM=offscreen ./build/edi --snapshot /tmp/x.png` (add
`--workspace <mode>` / `--probe x,y`). If a slice INTENTIONALLY changes a golden,
re-bless once — `EDI_BLESS_GOLDEN=1 ./build/edi_shell_window_tests` — and say why.
Never re-bless to hide an unintended change. Screenshot-fidelity to
`docs/ui_reference`.

## Backlog
`/ui-goal` drives the campaign; the backlog is H1–H8 in `docs/shell_architecture.md`.
Standing look items flagged in critique: low text contrast, flat/borderless
palette buttons, and the activity rail not highlighting the active workspace.
