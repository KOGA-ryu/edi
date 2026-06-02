# Reusable Shell Contract

Draftsman shell code is shared source. Project apps should not copy `src/` into their own repos.

## Rule

Use one authoritative Draftsman shell source tree:

```text
qt_qml_region_split/src/
```

Consumer apps keep their own data:

```text
data/project_profiles/*.json
data/review_subjects/*.json
data/ui_theme.json
data/shell_layout.json
```

When the shared shell source changes, every app that builds against that source gets the update.

## CMake Integration

Consumer apps can point the launcher at the shared shell source and their own data root:

```cmake
set(DRAFTSMAN_SHELL_TARGET_NAME my_project_shell CACHE STRING "" FORCE)
set(DRAFTSMAN_SHELL_QML_SOURCE_DIR "/Users/kogaryu/draft/draftsman/qt_qml_region_split/src" CACHE PATH "" FORCE)
set(DRAFTSMAN_SHELL_DATA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "" FORCE)
add_subdirectory("/Users/kogaryu/draft/draftsman/qt_qml_region_split" draftsman_shell_build)
```

The consumer app should treat the shared Draftsman repo like a submodule, sibling checkout, or other pinned dependency. The important part is that the QML source path points to the shared source, not a copy.

## Runtime Overrides

The executable can also load per-app data paths directly:

```sh
my_project_shell \
  --project-profile /path/to/app/data/project_profiles/project.json \
  --review-subject /path/to/app/data/review_subjects/project_taxonomy.json \
  --theme /path/to/app/data/ui_theme.json \
  --shell-layout /path/to/app/data/shell_layout.json
```

For a true blank app, set `main_workspace.feature` to `blank_canvas` and leave `data_sources.review_subject` empty. Review features should be enabled by an explicit project profile, not by the default shell.

## Ownership

- Shared shell source: regions, controls, runtime shell behavior, settings surfaces.
- Consumer app data: project identity, routes, feature toggles, theme values, layout defaults.
- Consumer app custom features: add as new feature surfaces with data contracts, not by forking shared shell files.

If a project needs a shell improvement, make it in the shared source first. If it needs project-specific content, put it in the consumer data or a feature module with a clear contract.

## Dex Planning Entry Point

Any Dex preparing an integration plan should read `docs/design_philosophy.md`, `data/design_principles.json`, `data/shell_surface_map.json`, and `docs/surface_contract.md` before inspecting QML. The design files define taste and review checks; the map defines stable surface ids, owner files, proof hooks, and edit boundaries.
