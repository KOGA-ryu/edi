# EDI

EDI is a C++ Qt Widgets shell for the drawing-core workbench and related project profiles.

The current runtime rule is simple: app behavior lives in typed C++ contracts and controllers. JSON remains data and projection format. JavaScript and QML are not part of the repo runtime.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
./build/edi
./build/edi --project-profile data/project_profiles/draftsman_drawing_tool_blank.json
./build/edi --project-profile data/project_profiles/draftsman_text_editor.json
./build/edi --project-profile data/project_profiles/draftsman_game_guy_map_editor.json
```

## Validate

```sh
build/edi_validate ui-theme data/ui_theme.json
build/edi_validate project-profiles data/project_profiles/draftsman_blank.json data/project_profiles/draftsman_drawing_tool_blank.json
build/edi_validate shell-layout data/shell_layout.json
build/edi_validate shell-surface-map data/shell_surface_map.json
build/edi_validate design-principles data/design_principles.json
build/drawing_control_workflow_report --all --compare-baseline --failures-only
ctest --test-dir build --output-on-failure
```

## Source Map

- App shell and widget composition: `app/main.cpp`
- Drawing canvas widget: `src/widgets/DrawingCanvasWidget.cpp`
- Drawing model contracts: `src/core/`
- Canvas behavior contracts: `src/canvas/`
- Runtime catalogs and workflow planning: `src/runtime/`
- Validation and report CLIs: `src/tools/`
- Project data and profiles: `data/`
- Contract and work-order documentation: `docs/`

## Zero JavaScript Guard

The required guard is `no_javascript_files_tests`. It fails if source-control-visible JavaScript, TypeScript, JSX, TSX, or QML files appear outside `.git` and `build`.

Manual scan:

```sh
find . -path './.git' -prune -o -path './build' -prune -o \
  \( -name '*.js' -o -name '*.mjs' -o -name '*.cjs' -o -name '*.jsx' -o -name '*.ts' -o -name '*.tsx' -o -name '*.qml' \) -print
```
