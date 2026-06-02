# Qt/QML Region Split Shell

This is a clean editable QML shell foundation for Draftsman.

Main rule: `src/Main.qml` composes major regions only. Region internals live in `src/regions/`. Shared styling lives in `src/style/UiStyle.qml`.

Reuse rule: other apps should consume this shell source instead of copying it. See `docs/reusable_shell_contract.md`.

## Edit map

- Change colors, spacing, fonts, region widths: `src/style/UiStyle.qml`
- Change shell composition: `src/Main.qml`
- Change left panel internals: `src/regions/LeftPanel.qml`
- Change right panel internals: `src/regions/RightPanel.qml`
- Change main workspace internals: `src/regions/MainWorkspace.qml`
- Change bottom panel internals: `src/regions/BottomPanel.qml`
- Change activity rail internals: `src/regions/ActivityRail.qml`
- Change status bar internals: `src/regions/StatusBar.qml`
- Change reusable controls: `src/components/`

## Intended repo placement

Recommended target:

```text
/Users/kogaryu/ui/renderers/qt_qml_static/src/
```

If generation is still used, update the generator to emit this structure instead of one giant `Main.qml`.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

From the built app:

```sh
./build/qt_qml_region_split
```

Load a specific review subject JSON:

```sh
./build/qt_qml_region_split --review-subject data/review_subjects/draftsman_ui_taxonomy.json
```

Load a specific theme JSON:

```sh
./build/qt_qml_region_split --theme data/ui_theme.json
```

Load a specific shell layout JSON:

```sh
./build/qt_qml_region_split --shell-layout data/shell_layout.json
```

Build another app against this shared UI source:

```cmake
set(DRAFTSMAN_SHELL_QML_SOURCE_DIR "/Users/kogaryu/draft/draftsman/qt_qml_region_split/src" CACHE PATH "" FORCE)
set(DRAFTSMAN_SHELL_DATA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "" FORCE)
add_subdirectory("/Users/kogaryu/draft/draftsman/qt_qml_region_split" draftsman_shell_build)
```

For quick source-mode QML iteration:

```sh
/opt/homebrew/bin/qml --verbose -I src src/Main.qml
```

The C++ launcher is required for JSON review-subject loading. Direct `qml` runs are useful for layout smoke checks only.

## Qt Creator

Open this folder or `CMakeLists.txt` in Qt Creator:

```text
/Users/kogaryu/draft/qt_qml_region_split
```

The source authority is `src/`. Do not create a competing `qml/` tree unless the imports are moved intentionally.

## Where To Edit

- Overall region composition: `src/Main.qml`
- Shared colors, sizing, fonts: `src/style/UiStyle.qml`
- Left/right/main/bottom/status regions: `src/regions/`
- Reusable controls: `src/components/`
- UI taxonomy review feature: `src/features/ui_taxonomy/`
- Runtime state and navigation: `src/runtime/RuntimeController.qml`

## How To Change Colors

Theme defaults live in `data/ui_theme.json`. Open Settings in the activity rail to preview changes to:

- theme mode
- base color
- surface color
- accent color
- text/font color
- UI font
- code font
- UI font size
- code font size

The running UI updates live. Saving is intentionally disabled until the backup, validation, and receipt contract is approved.

Validate a theme file:

```sh
scripts/validate_ui_theme.js data/ui_theme.json
```

## How To Add A Region

1. Add the region file under `src/regions/`.
2. Compose it from `src/Main.qml`.
3. Add any reusable controls under `src/components/`.
4. Add taxonomy routes and code refs in `src/runtime/RuntimeController.qml`.

## How To Add A Review Subject

Review subject taxonomy lives in JSON under `data/review_subjects/`. The app reads the selected JSON at launch and normalizes it into the runtime route model.

Do not add persistence or file writes until the write contract is approved.

Validate a review subject before launching it:

```sh
scripts/validate_review_subjects.js data/review_subjects/draftsman_ui_taxonomy.json
```

## Proof

Capture proof screenshots from the app window:

```sh
scripts/capture_proof.sh
```

Single screenshot example:

```sh
./build/qt_qml_region_split --screenshot docs/proof/default_shell_1280x820.png --width 1280 --height 820
```
