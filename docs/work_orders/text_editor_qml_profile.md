# Text Editor QML Profile

## Identity

Feature name: Text Editor

Feature id: `text_editor`

Owning project/profile: `data/project_profiles/draftsman_text_editor.json`

Human goal: provide a native Draftsman text editor surface for scratch text, review notes, and code snippets without turning the shell into a source-code IDE.

Design principles inherited:

```text
low_light_first
panels_are_tools
content_must_have_a_job
dense_not_decorative
no_dead_controls
```

## Surface Plan

```text
left_panel: document list and local commands only
main_workspace: native QML text editor surface, active tabs, and cursor-local status
right_panel: document facts only
bottom_panel: action/event rows only when an event exists
status_bar: shared shell mode state
settings: shared theme/layout settings
```

Activity mode: `text_editor`

Default panel state: left and right open, bottom closed until action/event output is useful.

## Scope

Implemented behavior:

- editable in-memory text surface
- in-memory document model owned by `RuntimeController.qml`
- active document tabs in the main workspace
- workspace-local `Undo`, `Redo`, `All`, `Clear`, `Find`, `Wrap`, and `Lines` icon controls
- local `New`, `Rename`, `Dup`, and `Close` icon controls
- delayed hover tooltips for text editor icon controls
- keyboard shortcuts for undo, redo, select all, find, and escape-close-find
- top `File` menu routes to document commands when `activityMode` is `text_editor`
- top `Edit` menu routes to editor commands and editor options when `activityMode` is `text_editor`
- menu-driven editor actions use a controller command bridge consumed by `TextEditorWorkspace.qml`
- inline rename field in the left panel
- close guard that blocks closing a modified in-memory document
- document facts in the right inspector
- line/column cursor and selection status in the editor surface
- hidden-by-default find field with previous/next match selection
- in-memory wrap and line-number visibility options
- modified/clean state tracking against each document's initial text
- bottom event shelf with no mirrored inspector facts
- no persistence or runner claims

Explicit non-goals:

- no source file editing
- no file persistence
- no external command runner
- no clipboard write behavior
- no syntax-highlighting dependency yet
- no discard confirmation dialog yet; modified close is blocked instead

## Future Hooks

If code highlighting is needed, prefer native QML plus KDE `KSyntaxHighlighting` before considering WebEngine-based editors.

If project persistence is needed, add a data contract first. Do not write arbitrary repo files from this surface.

## Verification

```sh
scripts/validate_project_profiles.js data/project_profiles/draftsman_text_editor.json
scripts/validate_ui_theme.js data/ui_theme.json
scripts/validate_shell_layout.js data/shell_layout.json
scripts/validate_shell_surface_map.js data/shell_surface_map.json
cmake --build build
./build/qt_qml_region_split \
  --screenshot docs/proof/text_editor_1280x820.png \
  --project-profile data/project_profiles/draftsman_text_editor.json \
  --width 1280 \
  --height 820
```

Proof screenshot:

```text
docs/proof/text_editor_1280x820.png
```
