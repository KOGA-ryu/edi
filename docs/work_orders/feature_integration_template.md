# Feature Integration Work Order Template

Use this template before coding a new Draftsman shell feature.

## Identity

Feature name:

Feature id:

Owning project/profile:

Human goal:

## Surface Plan

Target surfaces:

```text
left_panel:
main_workspace:
right_panel:
bottom_panel:
status_bar:
settings:
```

Activity mode:

Default panel state:

## Data Contract

Project profile changes:

Data files added or changed:

Runtime controller properties needed:

Validation script changes:

## Code Plan

Files allowed to edit:

Files not allowed to edit:

New feature directory:

Shell routing edits:

Reusable components needed:

## UX Rules

Content to show:

Content to hide:

Density requirements:

Theme behavior:

Small-window behavior:

## Verification

Commands:

```sh
scripts/validate_shell_surface_map.js data/shell_surface_map.json
scripts/validate_project_profiles.js data/project_profiles/draftsman_blank.json data/project_profiles/draftsman_ui_taxonomy.json
scripts/validate_ui_theme.js data/ui_theme.json
scripts/validate_shell_layout.js data/shell_layout.json
cmake --build build
scripts/capture_proof.sh
```

Proof screenshots to add or update:

Risks:

Known gaps:

Builder sign-off:
