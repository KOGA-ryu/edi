# EDI Product Blueprint

EDI is a data-oriented creative workbench for drafting and text work. The product direction is a C++-owned runtime where durable state, validation, commands, and data contracts are explicit before higher-level tooling is layered on top.

## Product Shape

EDI has two primary work surfaces:

- Drafting: a structured canvas for drawing objects, measurement, and build-plan planning.
- Text editing: a document workspace for notes, prompts, references, context, and planning documents.

These surfaces should share project state, command discipline, and format ownership. The drafting app should not become an isolated drawing toy, and the text editor should not become a separate note pad. Both should serve the same creative planning workflow.

## Current State

- The app shell is C++ Qt Widgets.
- Drafting geometry, snap, hit-test, and edit contracts live under `src/drafting`; canvas interaction and painting live under `src/widgets`.
- Core drawing and storage files are present but partially rebuilt as stubs after cleanup.
- Runtime behavior is C++ owned.

## Next State

- Rebuild drafting object contracts around typed geometry, style, metadata, selection, and command mutation.
- Rebuild the text document model around plain text, document roles, document metadata, and export/handoff boundaries.
- Define project/workspace settings before wiring persistent storage.
- Add format readers and writers only after the C++ contracts they serve are clear.
- Use `docs/feature_research_map.md` as the broad research inventory before promoting features into the compact feature matrix.
- Use `docs/phase1_real_world_tool_research.md` as the Phase 1 reference for grid, unit, origin, bounds, and cursor-readout decisions.

## Future State

- Drafting and text surfaces operate as one project workspace.
- Measurements and build-plan notes can be derived from typed drafting objects.
- Automation can compose commands through Lua recipes without owning app state.
- AI-facing packets can be exported through TOON without becoming runtime truth.
- Compact machine state can be stored through MessagePack with inspection tools.

## Product Principles

- C++ owns app truth.
- Commands mutate state; renderers and exporters project state.
- Formats are boundaries, not internal models.
- A feature is not complete until its data ownership and failure modes are known.
- Planning documents should be written so the next implementation slice can be chosen from the feature matrix.
