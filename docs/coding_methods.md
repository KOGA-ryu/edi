# Coding Methods

- Keep `Main.qml` as a composer only.
- Put region internals in `src/regions/`.
- Put reusable controls in `src/components/`.
- Put custom feature surfaces in `src/features/`.
- Put shared runtime state in `src/runtime/`.
- Keep style constants in `src/style/UiStyle.qml`.
- Do not scatter hardcoded colors, spacing, fonts, or panel sizes.
- Prefer simple QML properties and functions before creating framework abstractions.
- Keep review taxonomy in `data/review_subjects/` JSON.
- Validate review subjects with `scripts/validate_review_subjects.js`.
- Keep first-pass notes and statuses in memory; do not write files casually.
- Add proof screenshots for visible UI changes.
