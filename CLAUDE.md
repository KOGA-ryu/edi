# edi

Qt6/C++20 2D drafting (CAD) application. CMake build, widget-based UI — deliberately no QML.

## Hard rules

- **No JSON** in project source — no config files, no serialization. (`.claude/` harness files are exempt.)
- **No `.js` and no `.qml`** — the codebase must stay free of QtQml/QtQuick references.
- **Data-oriented design.** Pure logic lives in `src/drafting/` as free functions over plain
  structs (`Drafting*Ops`). Variation points are data — enums, kinds, plan structs, member
  pointers — or plan callables. No subclassing for behavior, no stateful logic objects.
- **Behavior-preserving refactors only** unless a change is explicitly requested.

## Architecture

- `src/drafting/` — pure C++ core (`edi_drafting_core`), no Qt types. Plan functions return
  plan structs (`ok` + payload); commands are a `DraftingCommand` variant applied via
  `applyDraftingCommand`.
- `src/core/DrawingDocumentController.*` (class in `DrawingCore.h`) — thin Qt orchestration:
  resolve inputs → delegate planning → apply command → emit `modelChanged`. Shared
  orchestration goes through kind-and-callable helpers (`applyActiveObjectMetadataUpdate`,
  `applyActiveObjectGeometryUpdate`, `applyCommandAndEmit`, `applyLayerFlagsUpdate`, …) —
  extend these rather than re-inlining the resolve/plan/apply/emit sequence.
- `src/widgets/` — Qt widgets shell (`EdiShellWindow`, `DrawingCanvasWidget`).
  `edi_shell_window_tests` covers the window's wiring (offscreen platform, widgets
  driven by objectName, controller state asserted); extend it when adding controls.
- `tests/` — one focused test file per ops slice, registered in `CMakeLists.txt`.

## Build & verify

Every change must pass this loop before commit:

```
cmake --build build
ctest --test-dir build --output-on-failure   # must be fully green
```

Plus the scan: no `.js`/`.qml` files anywhere, no `.json` outside `.claude/`, no
`QtQml`/`QtQuick` references in sources or CMake.

## Commits

Style: `claude: <imperative summary>` plus a short body explaining what was consolidated and
why it is behavior-preserving. One vein/slice per commit; working tree clean between slices.
(History before 2026-06-10 uses the older `codex:` prefix.)

If `.git/index.lock` or `.git/HEAD.lock` blocks a commit, verify it is a zero-byte stale file
(only `fsmonitor--daemon` git processes running) before removing it.

## Gotchas

- `DraftingGeometry` is a `std::variant`; converting a concrete geometry into
  `std::optional<DraftingGeometry>` needs an explicit `DraftingGeometry{...}` (two implicit
  user-defined conversions won't chain).
- Plan structs carry concrete geometry types; `UpdateGeometryCommand` takes the variant.
- The `/goal` command (`.claude/commands/goal.md`) encodes the refactor-campaign protocol.
