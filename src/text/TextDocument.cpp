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
