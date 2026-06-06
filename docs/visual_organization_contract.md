# Draftsman Visual Organization Contract

This document defines how Draftsman surfaces are organized and what each surface is allowed to show.

The goal is to keep Draftsman reusable across projects without letting feature slices turn the shell into a pile of duplicated titles, panels, fake controls, and demo content.

## Primary Rule

The main screen shows active work.

Settings shows configuration.

Project profiles decide what feature is active.

Feature data decides what content exists.

The shell decides where content can appear.

## Surface Roles

### Top Chrome

Allowed:

- macOS window controls
- panel collapse controls
- simple navigation controls
- application menus
- current activity signal when compact

Not allowed:

- project documentation
- long status text
- duplicate page titles
- feature-specific forms
- logs

Top chrome is for global controls only. If a control only matters inside one feature, it should usually live in that feature workspace.

### Activity Rail

Allowed:

- one icon/button per enabled activity mode
- settings entry
- compact active state

Not allowed:

- labels longer than one short token
- project tree rows
- disabled future modes
- nested feature navigation

The activity rail is a mode switcher, not a file tree.

### Left Panel

Allowed:

- the one necessary title for the active project/profile
- primary navigation for the active feature
- compact groups such as tools, layers, documents, maps, routes, assets
- selected state
- short metadata when it helps scanning

Not allowed:

- duplicate main workspace titles
- fake disabled buttons
- long descriptions
- receipts/logs
- full documents
- raw JSON

The left panel answers: what am I working inside, and what can I select next?

### Main Workspace

Allowed:

- the active artifact or work surface
- the actual editor/canvas/grid/review object
- direct manipulation controls needed for that artifact
- compact local toolbars when they directly operate on the artifact
- one focused empty state for true blank surfaces

Not allowed:

- repeated project title blocks
- broad documentation
- route facts that belong in the inspector
- settings forms
- logs or receipts
- decorative cards
- boxes inside boxes just to create visual separation

The main workspace answers: what is the human actively looking at or manipulating?

### Right Panel

Allowed:

- project-defined secondary work surface
- selected object inspection when useful
- selected tool settings when useful
- selected document facts when useful
- selected cell/route/artifact metadata when useful
- short validation state when useful
- code refs or source refs when useful

Not allowed:

- duplicate full content from the main workspace
- duplicate bottom logs
- vague global settings with no project-specific purpose
- long documents unless the selected project flow explicitly needs them there
- broad project explanations that are not tied to the active selection/workflow

The right panel is to be decided per project as the project evolves. Each feature must state what the right panel is for, or leave it closed.

### Bottom Panel

Allowed:

- project-defined secondary/output surface
- logs after a run when useful
- validation rows after validation when useful
- export receipts after export when useful
- command output when useful
- compact history tied to the active feature when useful

Not allowed:

- duplicated right panel inspection
- always-visible empty shelves
- broad documentation
- feature navigation
- fake tabs with no content

The bottom panel is to be decided per project as the project evolves. Each feature must state what the bottom panel is for, or leave it closed.

### Status Bar

Allowed:

- current mode
- dirty/saved state
- compact command result
- file/path/status hints

Not allowed:

- instructions
- multi-line text
- duplicated logs
- feature forms

The status bar is low-priority state, not a help panel.

### Settings

Allowed:

- theme controls
- panel behavior
- feature enable/disable
- project profile selection
- write/export safety policy
- per-feature configuration that changes behavior
- visibility toggles for optional UI surfaces

Not allowed:

- active project work
- generated output
- logs
- review notes
- text editor documents
- drawing canvas content

Settings answers: how should Draftsman behave and what should be enabled?

## Main Screen Showcase Rules

Only these content types are allowed to be showcased on the main screen:

- active canvas
- active text document
- active grid/map
- active review subject route
- active visual asset
- active tool recipe preview
- active blank workspace

Everything else must be routed elsewhere:

- configuration goes to settings
- object facts go to a project-declared inspection surface
- logs and receipts go to a project-declared output surface
- navigation goes to left panel
- low-priority state goes to status bar
- long planning notes go to text editor documents

## Feature Visibility Rules

A feature may show a control only when at least one of these is true:

- it performs a working action now
- it selects real content now
- it changes visible state now
- it configures real behavior now
- it exposes real imported data now

Do not show future tools as disabled placeholders. Put planned tools in documentation, settings, or feature registry data until they work.

## Tool Type Exclusivity

Only one tool type can be active at a time.

Tool types include:

```text
map_generator / map_editor
drawing_drafting / drawing_tool
blender_scripts / tool_workspace
```

These are mutually exclusive activity modes. Selecting one must replace the active main workspace, left panel, right panel, bottom panel content, and status context for the previous tool type.

Use `exclusive_group: "tool_type"` in project profile `activity_modes` for these modes.

Do not make map generation, drawing/drafting, and Blender scripts appear as simultaneously open panels inside one screen. They can share data later, but one owns the shell at a time.

## Settings Ownership

Settings should own configurable choices, not active work.

Examples:

- theme colors and fonts
- panel default states and sizes
- right/bottom auto-hide behavior
- enabled activities/features
- text editor export policy
- drawing tool visibility
- runner safety limits
- external tool paths

Settings should not become a second main workspace. If a settings page needs a preview, keep it compact and only show what changes.

## Visualization Rules

Use the densest clear representation.

- Tools: compact buttons, icon+short label when possible.
- Layers: stacked rows with visibility/select state.
- Documents: tabs or short rows with role metadata.
- Maps/grids: cells in the main workspace, selected cell details wherever the project declares the inspection surface.
- Drawing objects: visible on canvas, editable facts wherever the project declares the inspection surface.
- Logs: bottom rows only after output exists.
- Receipts: bottom or inspector only when tied to a completed action.
- External scripts: recipe cards or rows, never giant raw command walls by default.

Avoid visual noise:

- no redundant headers
- no decorative cards
- no nested panels without a reason
- no always-open empty output surfaces
- no hard-coded colors outside theme/style tokens

## Feature Plan Requirement

Every new feature must declare:

```text
main_showcase:
left_navigation:
right_inspection:
bottom_output:
settings_controls:
hidden_until_working:
```

If a feature cannot explain where its content belongs, it is not ready to build.

## Default Startup Rule

Fresh Draftsman startup should show:

- top chrome
- activity rail
- left panel
- main blank or active profile surface
- status bar

Right and bottom panels should start from the selected project profile defaults. If the project has not decided their job yet, keep them closed.

No startup screen should show demo content unless the user explicitly selects a demo/profile/proof mode.
