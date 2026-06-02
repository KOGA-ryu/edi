# Right Inspector Contract

The right panel renders a generic inspector document. Project-specific code should build this shape and pass it to `RightInspector.qml`; the renderer should not know about a project domain.

```js
{
  id: "route_inspector",
  targetId: "right_panel",
  targetLabel: "Right Panel",
  targetType: "region",
  status: "pending",
  sections: [
    {
      id: "facts",
      label: "Facts",
      type: "rows",
      visible: true,
      rows: [
        { label: "Type", value: "region" }
      ]
    },
    {
      id: "selection",
      label: "Selection",
      type: "text",
      visible: true,
      items: [
        { label: "Purpose", value: "Explain the selected object." }
      ]
    },
    {
      id: "code_refs",
      label: "Code Refs",
      type: "code_refs",
      visible: true,
      items: [
        { path: "src/regions/RightPanel.qml", role: "source" }
      ]
    },
    {
      id: "notes",
      label: "Notes",
      type: "notes",
      visible: true,
      items: [
        { status: "pending", body: "Human note text.", createdAt: "session time" }
      ]
    },
    {
      id: "receipts",
      label: "Receipts",
      type: "rows",
      visible: true,
      rows: [
        { label: "Writes", value: "disabled" }
      ]
    },
    {
      id: "actions",
      label: "Actions",
      type: "actions",
      visible: true,
      actions: [
        { id: "status_accepted", label: "Accept", kind: "status", value: "accepted" }
      ]
    }
  ]
}
```

Current section visibility is stored in `data/shell_layout.json` under `right_panel.sections`.
