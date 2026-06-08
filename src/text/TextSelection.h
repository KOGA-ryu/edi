// TextSelection.h
//
// Purpose:
//   Declares cursor, range, and selection helpers for text documents.
//
// Expected contracts:
//   - TextCursor.
//   - TextRange.
//   - TextSelection.
//   - Clamp, collapse, normalize, and selected-text helper functions.
//
// Ownership rule:
//   Selection owns positions in text. It does not own document storage, command
//   mutation, clipboard, or UI rendering.
//
// Must not depend on:
//   - Widgets.
//   - Format adapters.
//   - Scripting runtime.
//
// Preserve later:
//   Range math must be deterministic and explicit about byte/codepoint/cell
//   semantics before ASCII grid modes rely on it.
