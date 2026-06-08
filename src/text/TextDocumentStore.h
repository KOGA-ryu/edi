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
// Preserve later:
//   Store operations should be deterministic and valid without launching the
//   application.
//
// Surface contract:
//   - Primary responsibility: declare multi-document text storage operations.
//   - Allowed data: text document collection, active document ID, document
//     values, roles, titles, and metadata update payloads.
//   - Call direction: app/workspace/controllers and text commands call the
//     store; the store calls document helpers.
//   - Mutation authority: collection membership and direct document property
//     updates.
//   - Unit convention: document-level state, not edit ranges.
//   - Identity policy: stable IDs at API boundary; indexes are internal.
//   - Lifetime: store owns the documents in the collection.
//   - Composition boundary: collection management stays separate from editing.
//   - Promotion path: large document sets can gain ID maps behind this API.
