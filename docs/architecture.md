# Qt/QML Draftsman Shell Architecture

`Main.qml` composes major regions only: activity rail, left panel, main workspace, right panel, bottom panel, and status bar. Region internals live in `src/regions/`, reusable controls live in `src/components/`, feature surfaces live in `src/features/`, and shared state lives in `src/runtime/`.

The shell is intended to be consumed as shared source by other project apps. Those apps should point CMake at this `src/` tree and keep their own data root for project profiles, review subjects, themes, and shell layout. Do not fork or copy the shell when a project needs the same UI foundation.

The first feature is the UI taxonomy review gate. The active project profile is loaded from `data/project_profiles/draftsman_blank.json`; it defines project identity, enabled activity modes, left-panel project rows, bottom tabs, write policy, and the review subject data source. The route taxonomy is loaded from the profile data source unless `--review-subject` overrides it. Route navigation, status overrides, and notes remain in-memory during the session and are not written to disk.

## Boundaries

- QML is the human-editable UI surface.
- `UiStyle.qml` owns colors, spacing, panel sizes, and typography.
- `data/ui_theme.json` owns the default editable theme values loaded at startup. It uses the same flat contract as the C++ Draftsman theme: `theme_mode`, `base`, `surface`, `accent`, `text`, `ui_font`, `code_font`, `ui_font_size`, and `code_font_size`.
- Runtime state belongs in `RuntimeController.qml`.
- Project profiles belong in `data/project_profiles/` and should pass `scripts/validate_project_profiles.js`.
- Review subjects belong in `data/review_subjects/` and should pass `scripts/validate_review_subjects.js`.
- Reuse integration belongs in the consumer app build/data configuration, not copied QML files.
- No persistence is implemented until the notes/write receipt contract is approved.
