# Project Profile Contract

Project profiles tell the Draftsman shell what project it is hosting without rewriting QML.

Default profile:

`data/project_profiles/draftsman_blank.json`

Validation:

```sh
scripts/validate_project_profiles.js \
  data/project_profiles/draftsman_blank.json \
  data/project_profiles/draftsman_ui_taxonomy.json
```

## Contract

The default profile is a true blank shell. It starts in `blank` activity, renders no review subject, and keeps project-specific content out of the startup canvas.

```js
{
  schema_version: 1,
  profile: {
    profile_id: "draftsman_blank",
    label: "Draftsman",
    type: "blank_shell",
    root_path: "",
    summary: "Reusable blank Draftsman shell.",
    default_activity: "blank"
  },
  activity_modes: [
    { id: "blank", label: "Blank", icon: "B", tooltip: "Blank workspace", enabled: true },
    { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface", enabled: true }
  ],
  left_panel: {
    project_rows: [],
    settings_label: "Theme and layout"
  },
  main_workspace: {
    feature: "blank_canvas"
  },
  right_inspector: {
    source: "none",
    sections: {
      facts: false,
      selection: false,
      code_refs: false,
      notes: false,
      receipts: false,
      actions: false
    }
  },
  bottom_panel: {
    tabs: []
  },
  data_sources: {
    review_subject: "",
    review_notes: ""
  },
  write_policy: {
    writes_enabled: false,
    note_storage: "memory"
  }
}
```

The meta UI taxonomy review is still available as an explicit profile:

```sh
./build/qt_qml_region_split --project-profile data/project_profiles/draftsman_ui_taxonomy.json
```

## Builder Rules

- Keep profile files data-only.
- Use profile paths for project-specific review subjects.
- Keep `draftsman_blank` free of project content.
- Keep theme colors in `data/ui_theme.json`.
- Keep panel geometry in `data/shell_layout.json`.
- Keep project-specific right-panel content shaped by `docs/right_inspector_contract.md`.
- Do not enable writes until a persistence and receipt contract is approved.
