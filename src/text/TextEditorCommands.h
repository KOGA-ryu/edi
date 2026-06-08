// TextEditorCommands.h
//
// Purpose:
//   Declares text editing commands as explicit mutation requests.
//
// Expected contracts:
//   - Create document.
//   - Rename document.
//   - Set document role.
//   - Insert text.
//   - Replace range.
//   - Delete range.
//   - Command result with accepted/rejected status and diagnostic text.
//
// Ownership rule:
//   Commands own validation and mutation sequencing. The store owns document
//   membership. TextSelection owns cursor/range math.
//
// Preserve later:
//   Text commands should be replayable and suitable for undo/redo once history
//   is added.
//
// Surface contract:
//   - Primary responsibility: declare explicit text mutation requests.
//   - Allowed data: command kind, document ID, text payloads, ranges, roles,
//     names, and command diagnostics.
//   - Call direction: UI/controllers/scripts create commands; command executor
//     calls store and selection helpers.
//   - Mutation authority: public text mutation gate.
//   - Unit convention: ranges use the offset convention declared by
//     TextSelection.
//   - Identity policy: commands address documents by stable ID.
//   - Lifetime: command values can be replayed or stored for undo history.
//   - Composition boundary: command intent is separated from text storage.
//   - Promotion path: undo/redo and macro recording can build from commands.
