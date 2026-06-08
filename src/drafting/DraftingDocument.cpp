// DraftingDocument.cpp
//
// Implementation responsibility:
//   Implements default document construction and simple document lookup helpers.
//
// Belongs here:
//   - Creating an empty document with a default editable layer.
//   - Finding objects/layers by stable ID.
//   - Small document metadata/revision helpers that do not perform commands.
//
// Must be delegated elsewhere:
//   - Add/remove/update object validation belongs in DraftingStore.
//   - User intent and undoable mutation belongs in DraftingCommands.
//   - Geometry calculations belong in DraftingGeometry.
//   - File conversion belongs in format adapters.
//
// Boundary note:
//   This file should not grow into a command processor.
//
// Surface contract:
//   - Primary responsibility: implement small document construction and lookup
//     helpers.
//   - Allowed data: document values, object IDs, layer IDs, revision metadata,
//     and default layer/document seeds.
//   - Call direction: called by store, command, and workspace code.
//   - Mutation authority: limited to construction and simple document-local
//     helper updates.
//   - Unit convention: passes through document-space data without conversion.
//   - Identity policy: lookup by stable ID; no public reliance on vector index.
//   - Lifetime: helpers operate on caller-owned document values.
//   - Composition boundary: document helpers stay lower than command behavior.
//   - Promotion path: lookup acceleration can move into DraftingStore if needed.
