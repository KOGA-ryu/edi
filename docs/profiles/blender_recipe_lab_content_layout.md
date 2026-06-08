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

Blender Recipe Lab state is data-driven under:

```text
data/features/blender_recipe_lab/
```

Use C++ controllers/widgets for operation-chain selection, script dry-run state,
ASCII preview state, materialize status, and selected recipe nodes. Do not add
that state to unrelated shell code. If a feature appears to require new shell
controller API, stop and refactor the feature boundary first.
