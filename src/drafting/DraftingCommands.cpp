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
