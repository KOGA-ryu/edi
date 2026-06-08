// TextDocument.h
//
// Purpose:
//   Defines the core text document model for EDI.
//
// Expected contracts:
//   - Stable text document ID.
//   - Document role: scratch, prompt, context, reference, build note, or later
//     ASCII-specific roles.
//   - Plain text content.
//   - Structured side metadata.
//   - Dirty flag and revision counter.
//
// Ownership rule:
//   A TextDocument owns one document's text and metadata. It does not own the
//   document collection, persistence format, or editor UI.
//
// Must not depend on:
//   - Qt text widgets.
//   - TOML, TOON, MessagePack, or Lua parser/runtime types.
//   - Drafting object storage.
//
// Preserve later:
//   Plain text remains the base truth. ASCII-aware modes should layer on top of
//   explicit text/grid contracts rather than hidden UI state.
