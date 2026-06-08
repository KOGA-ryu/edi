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
