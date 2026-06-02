# Project Profile Contract

Project profiles tell the Draftsman shell what project it is hosting without rewriting QML.

Default profile:

`data/project_profiles/draftsman_blank.json`

Validation:

```sh
scripts/validate_project_profiles.js data/project_profiles/draftsman_blank.json
```

## Contract

```js
{
  schema_version: 1,
  profile: {
    profile_id: "draftsman_blank",
    label: "Draftsman",
    type: "blank_shell",
    root_path: "",
    summary: "Reusable blank Draftsman shell.",
    default_activity: "review"
  },
  activity_modes: [
    { id: "review", label: "Review", icon: "R", tooltip: "Review gate workspace", enabled: true }
  ],
  left_panel: {
    project_rows: [
      { label: "Scratch", meta: "workflow" }
    ],
    settings_label: "Theme and layout"
  },
  main_workspace: {
    feature: "ui_taxonomy_review"
  },
  right_inspector: {
    source: "selected_route",
    sections: {
      facts: true,
      selection: true,
      code_refs: true,
      notes: true,
      receipts: true,
      actions: true
    }
  },
  bottom_panel: {
    tabs: ["Output", "Proof", "Receipts", "Log"]
  },
  data_sources: {
    review_subject: "data/review_subjects/draftsman_ui_taxonomy.json",
    review_notes: "data/review_notes/draftsman_ui_taxonomy_notes.json"
  },
  write_policy: {
    writes_enabled: false,
    note_storage: "memory"
  }
}
```

## Builder Rules

- Keep profile files data-only.
- Use profile paths for project-specific review subjects.
- Keep theme colors in `data/ui_theme.json`.
- Keep panel geometry in `data/shell_layout.json`.
- Keep project-specific right-panel content shaped by `docs/right_inspector_contract.md`.
- Do not enable writes until a persistence and receipt contract is approved.
