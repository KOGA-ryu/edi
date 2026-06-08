// TextDocumentStore.cpp
//
// Implementation responsibility:
//   Implements text document collection operations.
//
// Belongs here:
//   - Enforcing unique text document IDs.
//   - Adding/removing documents.
//   - Maintaining active document consistency.
//   - Applying direct document property updates.
//
// Must be delegated elsewhere:
//   - Text range mutation belongs in TextEditorCommands.
//   - Cursor/selection math belongs in TextSelection.
//   - Persistence belongs in format adapters.
//
// Boundary note:
//   This file owns collection shape, not editor gestures or rendering.
