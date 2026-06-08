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
// Must not depend on:
//   - UI text widgets.
//   - Clipboard APIs.
//   - Raw TOON/TOML/MessagePack/Lua objects.
//
// Preserve later:
//   Text commands should be replayable and suitable for undo/redo once history
//   is added.
