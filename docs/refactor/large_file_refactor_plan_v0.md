# Large-file refactor plan (v0)

## Scan context
- Scope: `/Users/kogaryu/draft/draftsman/qt_qml_region_split`
- Sort: file size descending by source-relevant files (`*.qml`, `*.cpp`, `*.h`, `*.js`, `*.json`, `*.md`), excluding build artifacts.

## Top large files
1. `src/runtime/RuntimeController.qml` — 50,886 bytes
2. `src/features/text_editor/TextEditorWorkspace.qml` — 19,577 bytes
3. `src/features/settings/PanelsSettingsPage.qml` — 16,386 bytes
4. `src/regions/WindowTitleBar.qml` — 16,040 bytes
5. `src/runtime/TextEditorSession.qml` — 15,672 bytes
6. `src/features/settings/ThemeSettingsWorkspace.qml` — 14,313 bytes
7. `src/regions/WindowTitleBarToolbar.qml` — 12,220 bytes
8. `src/Main.qml` — 10,363 bytes
9. `src/features/inspector/RightInspector.qml` — 7,011 bytes
10. `src/style/UiStyle.qml` — 6,883 bytes
11. `src/regions/LeftPanel.qml` — 4,765 bytes

## Current refactor priority (pragmatic sequence)
1. `RuntimeController.qml` — ownership cleanup boundary
   - Keep this as orchestration facade.
   - Remove remaining app/project-specific ownership and legacy wrappers.
   - Verify wrappers are either still required by QML compatibility or eliminated.

2. `WindowTitleBar.qml` / `WindowTitleBarToolbar.qml` — UI responsibility split
   - Keep `WindowTitleBar.qml` as structural shell.
   - Move menu/toolbar actions and button wiring fully into dedicated subcomponents.
   - Ensure compatibility signals/slots remain stable.

3. `src/features/text_editor/TextEditorWorkspace.qml`
   - If behavior dominates rendering, split command/state handlers into helper components or controllers.
   - Keep QML bindings readable and avoid growing `RuntimeController` coupling.

4. Settings pages (`PanelsSettingsPage.qml`, `ThemeSettingsWorkspace.qml`)
   - Split repeated section logic if duplicated control groups are found.
   - Prefer extracted delegates/partials over monolithic QML property/method clusters.

5. Inspect `LeftPanel.qml` and `RightInspector.qml`
   - Extract tightly coupled subpanels where repeated toggles/rows/settings sections are independent.

6. Data JSONs (`data/*.json`, `docs/*.md`)
   - Do not split; these are not runtime ownership boundaries.
   - Keep normalized and shared under existing contracts.

## Planned stop criteria
- No file in `src/` should cross 50 KB without justification.
- New splits must reduce cross-file coupling and improve ownership clarity.
- Refactors should preserve current behavior and be gated by focused smoke checks.

## Non-goals for this slice
- No behavioral feature expansion in this refactor pass.
- No UI polish.
- No C++ changes.

