# Draftsman Surface Contract

This document is the starting point for any Dex building against the Draftsman shell.

Do not begin by reading the whole repo. Start with:

```text
data/shell_surface_map.json
data/design_principles.json
docs/design_philosophy.md
docs/project_profile_contract.md
docs/reusable_shell_contract.md
docs/right_inspector_contract.md
docs/work_orders/feature_integration_template.md
```

## Core Rule

The shell owns layout. Features own content.

Project-specific UI should enter through project profile data and feature modules. Do not fork shared shell files for one project.

The design philosophy owns taste and quality. Every feature plan should inherit `docs/design_philosophy.md` and `data/design_principles.json`, then declare which design profile it follows.

## Surface Names

Use these ids exactly in plans, docs, proof names, and data contracts:

```text
window
top_chrome
activity_rail
left_panel
main_workspace
right_panel
bottom_panel
status_bar
settings
blank_canvas
ui_taxonomy_review
csv_map_editor
```

## Edit Boundaries

Shell region files are shared infrastructure:

```text
src/Main.qml
src/regions/WindowTitleBar.qml
src/regions/ActivityRail.qml
src/regions/LeftPanel.qml
src/regions/MainWorkspace.qml
src/regions/RightPanel.qml
src/regions/BottomPanel.qml
src/regions/StatusBar.qml
src/runtime/RuntimeController.qml
```

Edit them only when the work requires shared routing, panel behavior, layout policy, or reusable shell behavior.

Feature files are safer places for project-specific content:

```text
src/features/blank/
src/features/settings/
src/features/ui_taxonomy/
```

New project features should get their own feature directory under `src/features/`.

## Blank Contract

The default profile is a true blank canvas:

```text
data/project_profiles/draftsman_blank.json
```

Blank mode must not load review subject data, hidden route state, disabled placeholder buttons, project text, route cards, or inspector rows.

The only always-visible blank surfaces are the shell controls, activity rail, left panel title, main empty canvas, and status mode.

## Feature Integration Pattern

A feature integration plan must answer:

```text
Which design profile governs it?
Which profile enables it?
Which activity mode opens it?
Which surfaces does it fill?
Which data files drive it?
Which owner files are touched?
Which proof screenshots prove it works?
```

Preferred integration order:

1. Add or update project profile data.
2. Add feature data contracts.
3. Add feature QML under `src/features/<feature_id>/`.
4. Route feature surfaces through the smallest possible shell host edit.
5. Update `data/shell_surface_map.json`.
6. Validate, build, and capture proof.

## Required Verification

Run these before handing work back:

```sh
scripts/validate_shell_surface_map.js data/shell_surface_map.json
scripts/validate_design_principles.js data/design_principles.json
scripts/validate_project_profiles.js data/project_profiles/draftsman_blank.json data/project_profiles/draftsman_ui_taxonomy.json
scripts/validate_ui_theme.js data/ui_theme.json
scripts/validate_shell_layout.js data/shell_layout.json
scripts/validate_review_subjects.js data/review_subjects/draftsman_ui_taxonomy.json
scripts/validate_csv_map_editor.js data/project_profiles/draftsman_game_guy_map_editor.json
cmake --build build
scripts/capture_proof.sh
```

If a project adds new profile/data files, validate those files explicitly too.
