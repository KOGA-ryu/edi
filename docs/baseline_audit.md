# Baseline Audit

Human-authored UI source lives under `src/`.

- `src/Main.qml` composes the shell regions.
- `src/regions/` contains activity rail, left panel, main workspace, right panel, bottom panel, and status bar.
- `src/components/` contains reusable controls.
- `src/style/UiStyle.qml` is a QML singleton for colors, spacing, sizing, and fonts.
- `src/runtime/RuntimeController.qml` owns first-pass in-memory review state.
- `src/features/ui_taxonomy/` contains the first meta review workspace.

The app is now a CMake Qt Quick project. It uses source-directory QML loading during development so Qt Creator edits are immediately visible after rebuild/relaunch.

No legacy C++ Draftsman source was imported.
