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
  custom_actions: [],
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
- Feature profiles that do not use review routes can leave `review_subject` empty when their feature validator covers their own data sources.
- Keep theme colors in `data/ui_theme.json`.
- Keep panel geometry in `data/shell_layout.json`.
- Keep project-specific right-panel content shaped by `docs/right_inspector_contract.md`.
- Add project commands through `custom_actions` when a known safe handler exists; do not use arbitrary scripts for first-pass integrations.
- Do not enable writes until a persistence and receipt contract is approved.

## Activity Mode Exclusivity

`activity_modes` are exclusive shell modes. Only one mode owns the active workspace and panels at a time.

Tool-type modes must declare:

```js
exclusive_group: "tool_type"
```

Use that group for project modes such as:

```text
map_generator / map_editor
drawing_drafting / drawing_tool
blender_scripts / tool_workspace
```

Settings may declare `exclusive_group: "system"` or omit the field. Do not place map generation, drawing/drafting, and Blender scripts into simultaneously visible panels; switch activity mode instead.

## Custom Actions

`custom_actions` are optional declarative menu actions. They let a profile add project commands without forking shell QML.

```js
custom_actions: [
  {
    id: "export_text_editor_bundle",
    label: "Export Bundle",
    menu: "File",
    activity: "text_editor",
    handler: "export_text_bundle",
    enabled: true,
    args: {
      packet_type: "text_editor_bundle"
    }
  },
  {
    id: "export_dex_handoff",
    label: "Export Dex Handoff",
    menu: "File",
    activity: "text_editor",
    handler: "export_text_bundle",
    enabled: true,
    args: {
      packet_type: "dex_handoff"
    }
  }
]
```

Supported first-pass handlers:

- `export_text_bundle`: exports the open text editor documents into a timestamped packet.
- `text_editor_command`: routes `args.command` through the existing text editor command bridge.
- `switch_activity`: switches to `args.activity`.

Custom actions can also be triggered for proof/automation runs:

```sh
./build/qt_qml_region_split \
  --project-profile data/project_profiles/draftsman_text_editor.json \
  --activity text_editor \
  --action export_dex_handoff
```

Export packets include `index.txt` and SHA-256 file records for handoff verification.

`packet_type: "dex_handoff"` adds `AGENT_README.txt`, `prompt.txt`, and `context.txt` to the normal export packet.

Text document roles control Dex handoff routing:

- `prompt`: preferred source for `prompt.txt`
- `context` and `reference`: included in `context.txt`
- `scratch`: skipped in Dex handoff packets
- `output`: exported as a document copy, but not added to prompt/context text

Validate a generated packet before handing it to another worker:

```sh
scripts/validate_export_packet.js data/text_editor/exports/<packet-dir>
```
