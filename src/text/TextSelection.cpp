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
//
// Surface contract:
//   - Primary responsibility: implement pure cursor/range helpers.
//   - Allowed data: text length, offsets, ranges, selections, and string views
//     for selected-text projection.
//   - Call direction: called by text commands and UI adapters.
//   - Mutation authority: compute-only.
//   - Unit convention: follows TextSelection.h offset policy.
//   - Identity policy: no document IDs needed for pure range math.
//   - Lifetime: no retained references.
//   - Composition boundary: no store or command dispatch.
//   - Promotion path: cell-grid selection math can split into an ASCII module.
