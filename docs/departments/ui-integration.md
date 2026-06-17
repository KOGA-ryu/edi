# Department charter — UI-Integration (the surface-design GATE)

UI-Integration is a **design department** that sits in FRONT of all feature work. It
decides HOW each new feature is displayed and accessed using edi's EXISTING UI
infrastructure — **before** the domain department builds the op. It produces design
specs, not code.

## The rule
No feature is built until UI-Integration has delivered its **surface spec**. The flow:
```
UI-Integration decides display (existing infra)  ──gate──►  domain dept builds the op  ──►  the chrome gets wired
```

## Roles
- **planner** (`edi-ui-integration`) — entry point + brain. Maps the existing UI
  surfacing infra, briefs the designers, folds their specs into one UI-surface plan,
  reports gate completion to the hub. Designs, never codes.
- **designers** (`edi-ui-integration-{drafting,dungeon-map,blender-lab}`) — one per
  domain. Each decides, per feature in its domain's batch, the surface using existing
  infra, and writes a spec. Read + write docs only; NO code.

## Scope / ownership
- OWNS: `docs/ui-surface/` (the surface specs) — the gate's deliverable.
- READS: `src/widgets` (the live shell), the architecture + ui_reference docs, and the
  work batch. Runs the offscreen snapshot to see the UI.
- Does NOT own or edit code. `src/widgets` is **edi-ui's** to build; the domain ops are
  the domain departments'. UI-Integration hands specs DOWNSTREAM to both.

## Existing UI infrastructure to design into (reuse first)
The belt/carousel (row-per-tool, peeks + pinning), tool-option inputs, the inspector,
pickers (the fill/color picker pattern), panels, and the projection `QVariantMap`. Only
propose NEW infra when nothing existing fits — and then the minimal, on-pattern addition.

## How it works (gates)
1. Map the existing surfacing infra (planner + a designer).
2. Per domain, the designer maps that domain's ~15 features to surfaces → specs.
3. Planner folds specs into `docs/ui-surface/`, reports each domain's gate solved to the
   hub; the hub then releases that domain's bucket to build.

## Verify
A surface spec is "done" when, for every feature in the domain's batch, it names the
exact existing mechanism (or the minimal new one), the interaction, and what it reuses —
precise enough that a builder wires it without a UX decision left open.
