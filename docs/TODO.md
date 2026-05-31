# Qt/QML Draftsman Shell TODO

Status: foundation implemented

This file tracks the first clean QML shell pass. Future persistence, agent writes, plugins, and generated UI are intentionally out of scope until their contracts are approved.

## Ground Rules

- [x] Do not import legacy C++ Draftsman source.
- [x] Keep `Main.qml` as a composer.
- [x] Keep major regions in `src/regions/`.
- [x] Keep reusable controls in `src/components/`.
- [x] Keep style tokens in `src/style/UiStyle.qml`.
- [x] Keep runtime state in `src/runtime/`.
- [x] Keep review feature UI in `src/features/`.
- [x] Do not write review notes or statuses to disk.
- [x] Do not push remote changes.

## Done

- [x] Add `CMakeLists.txt`.
- [x] Add `app/main.cpp`.
- [x] Build a Qt Quick executable.
- [x] Use `src/` as the QML source authority.
- [x] Load `src/Main.qml` directly during development.
- [x] Add `.gitignore` for build and Qt Creator local files.
- [x] Document build, run, Qt Creator, edit map, color edits, regions, review subjects, and proof commands.
- [x] Add `docs/baseline_audit.md`.
- [x] Add `docs/architecture.md`.
- [x] Add `docs/ui_taxonomy.md`.
- [x] Add `docs/coding_methods.md`.
- [x] Add `data/review_subjects/draftsman_ui_taxonomy.json`.
- [x] Add `data/review_notes/draftsman_ui_taxonomy_notes.json`.
- [x] Load review subject JSON through C++ before QML startup.
- [x] Add `--review-subject <path>` launch option for worker-provided subjects.
- [x] Add `scripts/validate_review_subjects.js`.
- [x] Add low-glare style tokens for base, surface, raised surface, accent, text, muted text, borders, focus, disabled, and statuses.
- [x] Add `data/ui_theme.json`.
- [x] Load theme JSON through C++ before QML startup.
- [x] Add `--theme <path>` launch option.
- [x] Add Settings workspace for four-color palette and font sizes.
- [x] Add live in-memory theme preview.
- [x] Add `scripts/validate_ui_theme.js`.
- [x] Replace missing external fonts with common macOS/Linux fallbacks.
- [x] Compose activity rail, left panel, main workspace, right panel, bottom panel, and status bar.
- [x] Add reusable buttons, icon buttons, tabs, status chips, list rows, cards, breadcrumbs, note cards, text field/area, toggle, code ref row, and splitter placeholder.
- [x] Centralize selected mode, selected subject, selected route, local tab, shelf tab, navigation stacks, status overrides, and notes in `RuntimeController.qml`.
- [x] Add UI taxonomy routes for shell regions, settings, feature surfaces, data contracts, and proof/review.
- [x] Add child routes for top chrome, activity rail, left panel, main workspace, right panel, bottom panel, settings, and feature surfaces.
- [x] Render current route facts.
- [x] Render child route cards.
- [x] Render code references.
- [x] Render review prompts.
- [x] Render route status chips and status counts.
- [x] Add in-memory human note entry.
- [x] Add in-memory status selection.
- [x] Show note counts in cards, left panel, right panel, bottom shelf, and status bar.
- [x] Show write-disabled state visibly.
- [x] Add route navigation: home, child route selection, back, forward, breadcrumb.
- [x] Add right context inspector tied to selected route.
- [x] Add bottom shelf tabs for output, proof, receipts, and log.
- [x] Add app-window screenshot mode.
- [x] Add `scripts/capture_proof.sh`.
- [x] Capture default shell proof.
- [x] Capture activity rail drilldown proof.
- [x] Capture note entry proof.
- [x] Capture settings theme proof.
- [x] Capture minimum-size proof.

## Deferred By Contract

- [ ] Add formal JSON Schema files for review subjects and notes.
- [ ] Add persistence only after the save/write contract is approved.
- [ ] Add Save Notes and Save Status buttons only after persistence is approved.
- [ ] Add receipt files, backups, validation, and human confirmation only after persistence is approved.
- [ ] Add agent read/write integration only after the agent boundary is approved.
- [ ] Add open-in-editor and copy-path commands only after command execution rules are approved.
- [ ] Add real draggable panel resizing and saved layout only after the layout contract is approved.
- [ ] Add Save Theme once settings persistence, backup, validation, and receipt behavior are approved.

## Verification

```sh
cmake -S . -B build
cmake --build build
./build/qt_qml_region_split
scripts/capture_proof.sh
```
