// DraftingCommands.cpp
//
// Implementation responsibility:
//   Applies drafting commands to a DraftingDocument by validating intent and
//   delegating storage work to DraftingStore.
//
// Belongs here:
//   - Command precondition checks.
//   - Command-to-store mutation sequencing.
//   - Revision updates on accepted mutation.
//   - Producing command diagnostics.
//
// Must be delegated elsewhere:
//   - Storage invariants belong in DraftingStore.
//   - Math belongs in DraftingGeometry.
//   - UI gesture lifecycle belongs outside the drafting core.
//   - Script parsing belongs in scripting/format layers.
//
// Boundary note:
//   This file should make mutation explicit. No hidden document edits from
//   renderers, format adapters, or scripts.
//
// Surface contract:
//   - Primary responsibility: turn validated drafting intent into document
//     changes.
//   - Allowed data: mutable document references, command values, store results,
//     geometry helpers, and selection helpers.
//   - Call direction: called by controllers or script bridges; calls store and
//     pure domain helpers.
//   - Mutation authority: may mutate through public store/selection operations.
//   - Unit convention: normalize command units before applying storage changes.
//   - Identity policy: resolves stable IDs at execution time.
//   - Lifetime: command values can be transient or stored for replay.
//   - Composition boundary: orchestration lives here; raw storage details remain
//     in DraftingStore.
//   - Promotion path: command logs and undo history can build on this layer.
