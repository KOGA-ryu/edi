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
//
// Surface contract:
//   - Primary responsibility: implement collection operations for text docs.
//   - Allowed data: store values, document values, IDs, roles, active document
//     references, and metadata updates.
//   - Call direction: called by text commands and app/workspace code.
//   - Mutation authority: collection and document property updates.
//   - Unit convention: no text range units; command layer handles ranges.
//   - Identity policy: stable document IDs; internal ordering can be vector.
//   - Lifetime: owns document values inside the store.
//   - Composition boundary: avoids UI and persistence orchestration.
//   - Promotion path: document indexing/search cache can attach here later.
