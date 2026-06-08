// DraftingStore.h
//
// Purpose:
//   Declares storage operations over a DraftingDocument.
//
// Expected contracts:
//   - Add object.
//   - Remove object.
//   - Update geometry.
//   - Update style and metadata.
//   - Find mutable/const object references by stable ID.
//   - Return explicit result objects for accepted/rejected operations.
//
// Ownership rule:
//   The store owns object storage invariants inside a document: ID uniqueness,
//   kind/geometry consistency, valid layers, and bounds recomputation.
//
// Preserve later:
//   Store operations should be deterministic and testable without launching the
//   app.
//
// Surface contract:
//   - Primary responsibility: declare document object storage operations.
//   - Allowed data: DraftingDocument references, DraftingObject values, object
//     IDs, layer IDs, geometry/style/metadata update payloads.
//   - Call direction: commands call store; app/UI/scripts reach store through
//     command paths.
//   - Mutation authority: may mutate document object storage through explicit
//     store operations.
//   - Unit convention: accepts document-space geometry values.
//   - Identity policy: public APIs address objects by stable ID.
//   - Lifetime: the caller owns the document; store functions mutate that value.
//   - Composition boundary: storage shape is separated from user intent.
//   - Promotion path: dense object arrays and ID-to-index maps belong here when
//     data-oriented performance work starts.
