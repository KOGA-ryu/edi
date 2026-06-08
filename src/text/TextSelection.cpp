// TextSelection.cpp
//
// Implementation responsibility:
//   Implements pure text cursor/range/selection helpers.
//
// Belongs here:
//   - Normalizing start/end offsets.
//   - Clamping ranges to document length.
//   - Detecting collapsed selections.
//   - Extracting selected text by validated range.
//
// Must be delegated elsewhere:
//   - Applying edits belongs in TextEditorCommands.
//   - Managing many documents belongs in TextDocumentStore.
//   - Drawing selections belongs in UI widgets.
//
// Boundary note:
//   This file computes selection facts. It should not mutate document storage.
