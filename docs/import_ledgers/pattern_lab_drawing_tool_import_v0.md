# Pattern Lab Drawing Tool Import V0

## Purpose

Port the useful native drawing surface from Pattern Lab into Draftsman without making Pattern Lab the product authority.

Draftsman remains the primary product shell:

```text
/Users/kogaryu/draft/draftsman
```

Pattern Lab remains a donor/reference repo:

```text
/Users/kogaryu/draft/pattern_lab_2d_native_clean
```

## Imported Source Files

Exact donor files copied into Draftsman:

```text
src/core/DrawingCore.h
src/core/DrawingCore.cpp
src/runtime/DrawingSession.qml
src/runtime/DrawingCanvasHitTest.js
src/runtime/DrawingRuntimeRows.js
src/runtime/DrawingToolCatalog.js
src/features/drawing_tool/DrawingCanvasObjectRenderer.qml
src/features/drawing_tool/DrawingCanvasPreviewRenderer.qml
src/features/drawing_tool/DrawingCanvasSnapResolver.qml
src/features/drawing_tool/DrawingToolWorkspace.qml
src/features/drawing_tool/DrawingToolLeftPanel.qml
src/features/drawing_tool/DrawingToolRightContext.qml
src/features/drawing_tool/DrawingToolBottomPanel.qml
data/features/drawing_tool/tool_registry.json
```

## Draftsman Targets

```text
qt_qml_region_split/src/core/
qt_qml_region_split/src/runtime/
qt_qml_region_split/src/features/drawing_tool/
qt_qml_region_split/data/features/drawing_tool/tool_registry.json
```

## Rewrites

- `qt_qml_region_split/CMakeLists.txt`
  - Added `DrawingCore` sources.
  - Added `src/` as an include root.
- `qt_qml_region_split/app/main.cpp`
  - Added a `DrawingDocumentController`.
  - Exposed `nativeDrawingController`, `initialDrawingModel`, and drawing tool registry context to QML.
- `qt_qml_region_split/src/runtime/RuntimeController.qml`
  - Mounted `DrawingSession`.
  - Added drawing aliases and thin wrapper functions used by the imported QML.
- `qt_qml_region_split/src/features/drawing_tool/DrawingToolWorkspace.qml`
  - Removed the donor canvas HUD because it duplicated state and cluttered the Draftsman canvas.

## Kept Out

- Pattern Lab repo ownership.
- Pattern Lab project profile defaults.
- Script replay CLI options.
- SVG/model export CLI options.
- Blender tool workspace and runner artifacts.
- Generated proof/demo geometry in startup state.

## Current Behavior

- The Draftsman drawing profile opens a real native canvas.
- The left panel shows donor drawing tool taxonomy.
- Clicking/drawing routes through the C++ drawing controller in memory.
- The default canvas remains blank except for artboard/grid/guide surfaces.

## Verification

```sh
scripts/validate_project_profiles.js data/project_profiles/draftsman_drawing_tool_blank.json data/project_profiles/draftsman_blank.json data/project_profiles/draftsman_text_editor.json data/project_profiles/draftsman_game_guy_map_editor.json
scripts/validate_shell_surface_map.js data/shell_surface_map.json
scripts/validate_ui_theme.js data/ui_theme.json
scripts/validate_shell_layout.js data/shell_layout.json
cmake --build build
./build/qt_qml_region_split --project-profile data/project_profiles/draftsman_drawing_tool_blank.json --activity drawing_tool --screenshot /tmp/draftsman_drawing_import.png --width 1280 --height 820
```
