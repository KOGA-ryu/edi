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
// Preserve later:
//   Range math must be deterministic and explicit about byte/codepoint/cell
//   semantics before ASCII grid modes rely on it.
//
// Surface contract:
//   - Primary responsibility: declare cursor/range/selection value contracts.
//   - Allowed data: offsets, anchor/focus positions, range length, collapsed
//     state, and selected text projections.
//   - Call direction: text commands and UI adapters call selection helpers.
//   - Mutation authority: compute-only over selection values.
//   - Unit convention: v1 offset units must be explicit; ASCII cell units get a
//     separate contract when introduced.
//   - Identity policy: selections reference document content by position.
//   - Lifetime: selection values are snapshots, not document owners.
//   - Composition boundary: selection math is separate from document storage.
//   - Promotion path: Unicode/cell-aware selection can extend this file later.
