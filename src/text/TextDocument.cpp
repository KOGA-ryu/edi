// TextDocument.cpp
//
// Implementation responsibility:
//   Implements small helpers for TextDocument construction and metadata state.
//
// Belongs here:
//   - Default document creation.
//   - Role normalization helpers.
//   - Dirty/revision helper functions.
//
// Must be delegated elsewhere:
//   - Multi-document collection logic belongs in TextDocumentStore.
//   - Insert/replace/delete command validation belongs in TextEditorCommands.
//   - Persistence belongs in format adapters.
//
// Boundary note:
//   This file should not grow editor behavior or UI concerns.
//
// Surface contract:
//   - Primary responsibility: implement small helpers for text document values.
//   - Allowed data: document values, role values, titles, metadata, dirty flag,
//     and revision.
//   - Call direction: called by text store and command modules.
//   - Mutation authority: value construction and simple state helpers only.
//   - Unit convention: no offset math beyond whole-document text values.
//   - Identity policy: preserves document IDs as opaque values.
//   - Lifetime: helpers operate on caller-owned document values.
//   - Composition boundary: document value helpers stay below editor commands.
//   - Promotion path: richer document metadata helpers can split out later.
