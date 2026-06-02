# Draftsman Design Philosophy

Draftsman should feel like a quiet draftsman table, binder, and inspector. It is a work surface for humans and Dex workers to organize, review, and adapt projects without rebuilding the shell for every project.

## Core Philosophy

Dense, not decorative.

The UI should be compact, scannable, and useful. Avoid marketing layouts, hero sections, ornamental panels, and empty title zones.

One title is enough.

If the left panel already identifies the selected area, the main workspace should not repeat the same title unless it adds new information. Breadcrumbs, headers, focus cards, and status rows must not all say the same thing.

No dead controls.

Do not show disabled `New`, `Add`, `Help`, or future buttons as placeholders. If a control is visible, it needs a current job.

Blank stays blank.

The default shell is not a demo. It must not load sample review subjects, route cards, notes, documents, project rows, or hidden taxonomy state.

Low-light first.

The UI is designed for sensitive eyes and long sessions. Large bright surfaces are not acceptable defaults. Use theme tokens and keep high-energy colors constrained to meaningful state or action.

Panels are tools.

The left panel navigates, the main workspace hosts the selected feature, the right panel inspects, the bottom panel holds secondary output/receipts/logs, and the status bar gives low-priority state. Panels are not decoration.

Content must have a job.

Every visible object should support navigation, inspection, action, review, feedback, or configuration. If it does not, remove it or move it to documentation.

Shared shell first.

Reusable UI improvements belong in the shared shell. Project-specific behavior belongs in profiles, data files, or feature modules.

## Dex Roles

Architect Dex defines the boundary before code exists.

It should produce target surfaces, data contracts, owner files, proof requirements, and explicit no-touch zones. It should not invent a new aesthetic or code against an undefined surface.

Builder Dex implements only the approved surfaces.

It should read the work order, the relevant `data/shell_surface_map.json` entries, and this design philosophy. It should not read the whole repo unless blocked. It must not add placeholder UI.

Review Dex checks for regressions and slop.

It should lead with findings: duplicate titles, dead controls, excessive borders, box nesting, hidden route leaks, theme escapes, missing proof, and contract drift.

Research Dex brings project knowledge.

It should classify findings into shell surface, feature surface, project profile, data contract, or out-of-scope. It should not dump raw repo content into the UI.

Theme Dex protects visual comfort.

It should work through `data/ui_theme.json` and `src/style/UiStyle.qml`. It should not hard-code colors in feature QML unless a missing token is documented and added.

## Design Profiles

Different projects can demand different UI shapes, but they inherit the same philosophy.

`blank_shell`:

The default shell. Left panel and main empty workspace are visible. Right and bottom panels are closed. No project content appears.

`dense_review`:

Use for route, artifact, decision, or implementation review. The left panel navigates, the main workspace reviews the selected object, the right panel inspects facts, and the bottom panel stays closed until useful.

`repo_knowledge_binder`:

Use for research, AI agent notes, signed session receipts, documents, code snippets, and repo knowledge. The right panel should become the inspector for selected facts, not a duplicate document.

`visual_asset_review`:

Use for generated images, palettes, layers, taxonomy drills, and review notes. The main workspace should give priority to the visual artifact; controls belong in side panels.

`text_editor_workbench`:

Use for read-only documents, scratch text, code snippets, and alternate code options. It should feel like an editor surface without becoming a full IDE.

`settings_admin`:

Use for theme, panel, feature, project, and write-rule controls. Settings should stay compact and avoid redundant page titles.

## Review Checks

Before sign-off, a Dex should answer:

```text
Are there duplicate titles?
Are there dead controls?
Are there boxes inside boxes without a job?
Does blank mode load or show project content?
Did feature code hard-code theme colors?
Did the change touch only the approved surfaces?
Is there proof for the affected view and at least one constrained window size?
```

Any failed answer should become either a fix or an explicit known gap in the work order.
