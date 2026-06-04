# Drawing Tool Blank Profile

## Identity

Feature name: Drawing Tool Blank

Feature id: `drawing_tool_blank`

Owning project/profile: `data/project_profiles/draftsman_drawing_tool_blank.json`

Human goal: provide a clean Draftsman shell copy for a drawing tool without forking the shared UI.

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
left_panel: project identity only, no fake tools
main_workspace: empty drawing canvas host
right_panel: empty inspector host, closed by default
bottom_panel: empty shelf host, closed by default
status_bar: shared shell mode state
settings: shared theme/layout settings
```

Activity mode: `drawing_tool`

Default panel state: left open, right closed, bottom closed.

## Code Plan

Feature directory:

```text
src/features/drawing_tool/
```

Shell routing edits:

```text
src/regions/LeftPanel.qml
src/regions/MainWorkspace.qml
src/regions/RightPanel.qml
src/regions/BottomPanel.qml
```

No drawing engine, persistence, layer stack, brush toolbar, asset browser, or export workflow is implemented in this blank copy.

## UX Rules

Content to show: only the shared shell identity and blank workspace surface.

Content to hide: placeholder palette buttons, fake layers, disabled export/save controls, broad drawing claims.

Theme behavior: use `UiStyle` tokens only.

Future drawing-tool work should add real controls only when their behavior and data contract exist.

## Verification

```sh
scripts/validate_project_profiles.js data/project_profiles/draftsman_drawing_tool_blank.json
scripts/validate_shell_surface_map.js data/shell_surface_map.json
cmake --build build
scripts/capture_proof.sh
```

Proof screenshot:

```text
docs/proof/drawing_tool_blank_1280x820.png
```
