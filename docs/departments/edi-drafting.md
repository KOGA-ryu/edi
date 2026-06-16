# Department charter — edi-drafting

The pure 2D drafting/CAD core: the geometry, the operations, the command
pipeline, and the thin Qt orchestration that drives the document. The drafting
tool is feature #1 of the shell, but the LOGIC is this department's.

## Scope (what this department owns)
- `src/drafting/` — the pure C++ core (`edi_drafting_core`, **no Qt types**):
  `Drafting*Ops` free functions, the `DraftingGeometry` variant, the
  `DraftingCommand` variant + `applyDraftingCommand`, and the slices
  (snap, hit-test, mirror, numeric-edit, object-edit, quick-measure, plot-plan,
  serialize, geometry).
- `src/core/` — `DrawingDocumentController` (class in `DrawingCore.h`): the thin
  Qt orchestration that resolves inputs → delegates planning → applies a command
  → emits `modelChanged`.
- The canvas INTERACTION logic in `src/widgets/DrawingCanvas*` (mouse paths,
  gestures) is this department's behavior — but it lives in widgets, so
  coordinate with **edi-ui**, which owns the chrome and the colors.

Do NOT touch: the shell chrome / theming (edi-ui), the recipe op pipeline
(edi-blender-lab), or the map graph (edi-dungeon-map) — though those consume
this department's geometry and measurements.

## Architecture (the rules to obey)
Pure logic lives as **free functions over plain structs**. Plan functions return
plan structs (`ok` + payload); commands are a `DraftingCommand` variant applied
via `applyDraftingCommand`. The controller goes resolve → plan → apply → emit
through the **kind-and-callable helpers** (`applyActiveObjectMetadataUpdate`,
`applyActiveObjectGeometryUpdate`, `applyCommandAndEmit`, `applyLayerFlagsUpdate`,
…) — extend these, never re-inline the sequence. Variation is data (enums, kinds,
plan structs, member pointers, callables); **no subclassing for behavior**. Every
`std::visit` over `DraftingGeometry`/`DraftingCommand` with an overload set is
exhaustive (a missing arm won't compile); `DraftingTypes.h` carries an
`always_false_v` exhaustiveness guard.

Gotchas: a concrete geometry → `std::optional<DraftingGeometry>` needs an explicit
`DraftingGeometry{...}` (two implicit user conversions won't chain). Plan structs
carry concrete geometry; `UpdateGeometryCommand` takes the variant. Hold document
data by VALUE across an undo/redo.

## Read first
- `CLAUDE.md` (the Architecture section is this department's law).
- `docs/adding-a-drafting-tool.md` — the recipe for a new tool/slice.
- `docs/drafting-gaps.md` — the audited backlog of missing draft/draw tools.
- `docs/project-map.md` — the board (P5 = drafting depth).
- Memories: `edi-drafting-gaps`, `edi-drafting-surface-priority`.

## Verify (the green gate for this department)
```
cmake --build build && ctest --test-dir build --output-on-failure   # fully green
```
plus the scan (no `.js`/`.qml`, no `.json` outside `.claude/`, no QtQml/QtQuick).
One focused test file per ops slice, registered in `CMakeLists.txt`. Controller
wiring → `drawing_document_controller_tests`; canvas mouse paths →
`drawing_canvas_widget_tests` (synthesized `QMouseEvent`s).

## Backlog
`/goal` drives the campaign; the gap list is `docs/drafting-gaps.md`; the board is
`docs/project-map.md` (P5). A quick win on the board: wire fill into SVG export
(writes `fill='none'` today).
