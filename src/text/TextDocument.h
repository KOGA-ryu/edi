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
// Preserve later:
//   Plain text remains the base truth. ASCII-aware modes should layer on top of
//   explicit text/grid contracts rather than hidden UI state.
//
// Surface contract:
//   - Primary responsibility: define one text document's durable in-memory
//     value.
//   - Allowed data: document ID, title, role, plain text content, side metadata,
//     dirty flag, and revision.
//   - Call direction: stores own collections of documents; commands mutate text
//     through public document/store contracts.
//   - Mutation authority: direct value changes are wrapped by store/commands.
//   - Unit convention: text content is plain string data; offset/cell semantics
//     are declared by TextSelection and later ASCII contracts.
//   - Identity policy: stable document ID, role as typed enum/value.
//   - Lifetime: document owns its text and metadata values.
//   - Composition boundary: no multi-document or UI state here.
//   - Promotion path: ASCII cell layers can reference this base text model.
