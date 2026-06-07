# Blender Recipe Lab Content Layout

Blender Recipe Lab composes existing Draftsman tools.

```text
main_workspace: existing drawing/drafting workspace
right_panel: existing drawing/drafting right-context tools
bottom_panel: existing text editor workspace with Blender recipe/script docs
left_panel: blank/minimal
```

Do not rebuild these surfaces from scratch for this profile. Wire data into the
existing drawing, text, and external script/ASCII pipeline.

## Runtime Boundary

Blender Recipe Lab owns Blender-specific state under:

```text
src/features/blender_recipe_lab/BlenderRecipeLabSession.qml
src/features/blender_recipe_lab/BlenderRecipeLabSessionStore.js
```

Use that session for operation-chain selection, script dry-run state, ASCII
preview state, materialize status, and selected recipe nodes. Do not add that
state to the shell runtime controller. If a feature appears to require new shell
controller API, stop and refactor the feature boundary first.
