# CSV Map Editor Integration

## Identity

Feature name: CSV Map Editor

Feature id: `csv_map_editor`

Owning project/profile: `data/project_profiles/draftsman_game_guy_map_editor.json`

Human goal: schematic dungeon/map token editing for Game Guy maps.

Design profile: `visual_asset_review` with `dense_review` behavior for the inspector.

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
left_panel: map summary and token palette counts
main_workspace: editable CSV-like grid surface
right_panel: selected cell inspector document
bottom_panel: validation/log rows
status_bar: unchanged shared shell mode state
settings: unchanged shared settings surface
```

Activity mode: `map_editor`

Default panel state: left, right, and bottom open for this profile only.

## Data Contract

Project profile:

```text
data/project_profiles/draftsman_game_guy_map_editor.json
```

Data files:

```text
data/maps/game_guy/starter_grid.csv
data/maps/game_guy/starter_cells.jsonl
```

The CSV stores visible schematic tokens. The JSONL sidecar stores optional selected-cell intent metadata keyed by `row` and `col`.

## Code Plan

Feature directory:

```text
src/features/csv_map_editor/
```

Shell routing edits:

```text
src/regions/LeftPanel.qml
src/regions/MainWorkspace.qml
src/regions/RightPanel.qml
src/regions/BottomPanel.qml
```

Runtime additions:

```text
src/runtime/RuntimeController.qml
app/main.cpp
```

## UX Rules

Content to show: small grid, token counts, selected cell facts, intent, tags, validation status, source paths, read-only validation/log rows.

Content to hide: generator controls, save/persistence claims, disabled future buttons.

Density requirements: no duplicate titles, compact cells, compact inspector rows.

Theme behavior: use `UiStyle` tokens only.

Small-window behavior: grid scrolls; right/bottom panels remain controlled by existing panel policy.

## Verification

```sh
scripts/validate_csv_map_editor.js data/project_profiles/draftsman_game_guy_map_editor.json
scripts/validate_project_profiles.js data/project_profiles/draftsman_blank.json data/project_profiles/draftsman_ui_taxonomy.json data/project_profiles/draftsman_game_guy_map_editor.json
scripts/validate_shell_surface_map.js data/shell_surface_map.json
cmake --build build
scripts/capture_proof.sh
```

Proof screenshot:

```text
docs/proof/csv_map_editor_1280x820.png
```

Known gaps: no generator, no persistence, no CSV write-back, no token editing palette actions yet.
