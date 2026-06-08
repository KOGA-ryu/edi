# Feature Integration Work Order Template

Use this template before coding a new Draftsman shell feature.

## Identity

Feature name:

Feature id:

Owning project/profile:

Human goal:

Design profile:

Design principles inherited:

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

Exclusive group:

Default panel state:

Main showcase:

Left navigation:

Right inspection:

Bottom output:

Settings controls:

Hidden until working:

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

Slop checks:

```text
duplicate_titles:
dead_controls:
box_nesting:
blank_leak:
theme_escape:
```

## Verification

Commands:

```sh
build/edi_validate shell-surface-map data/shell_surface_map.json
build/edi_validate design-principles data/design_principles.json
build/edi_validate project-profiles data/project_profiles/draftsman_blank.json data/project_profiles/draftsman_ui_taxonomy.json
build/edi_validate ui-theme data/ui_theme.json
build/edi_validate shell-layout data/shell_layout.json
cmake --build build
scripts/capture_proof.sh
```

Proof screenshots to add or update:

Risks:

Known gaps:

Builder sign-off:
