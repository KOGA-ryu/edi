// TextDocumentStore.h
//
// Purpose:
//   Declares collection-level storage for text documents.
//
// Expected contracts:
//   - Add/remove/find documents by stable ID.
//   - Track active text document.
//   - Update document text, role, title, and metadata through explicit APIs.
//   - List documents by role.
//
// Ownership rule:
//   The store owns membership and active-document state. Individual documents own
//   their text and metadata.
//
// Must not depend on:
//   - Text editor widgets.
//   - Raw file formats.
//   - Lua scripts.
//
// Preserve later:
//   Store operations should be deterministic and valid without launching the
//   application.
