# Qt/QML Draftsman Shell Architecture

`Main.qml` composes major regions only: activity rail, left panel, main workspace, right panel, bottom panel, and status bar. Region internals live in `src/regions/`, reusable controls live in `src/components/`, feature surfaces live in `src/features/`, and shared state lives in `src/runtime/`.

The first feature is the UI taxonomy review gate. The route taxonomy is loaded from `data/review_subjects/draftsman_ui_taxonomy.json` by the C++ launcher and passed into QML before `Main.qml` is created. Route navigation, status overrides, and notes remain in-memory during the session and are not written to disk.

## Boundaries

- QML is the human-editable UI surface.
- `UiStyle.qml` owns colors, spacing, panel sizes, and typography.
- `data/ui_theme.json` owns the default editable theme values loaded at startup.
- Runtime state belongs in `RuntimeController.qml`.
- Review subjects belong in `data/review_subjects/` and should pass `scripts/validate_review_subjects.js`.
- No persistence is implemented until the notes/write receipt contract is approved.
