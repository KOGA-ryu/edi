// DraftingDocument.h
//
// Purpose:
//   Defines the in-memory drafting document contract.
//
// Expected contracts:
//   - DraftingObject: id, kind, typed geometry, style reference, layer, metadata,
//     derived bounds, and flags.
//   - DraftingLayer: id, name, visibility, ordering, locked/editable state.
//   - DraftingDocument: document id, object collection, layer collection,
//     selection state, revision, and document metadata.
//
// Ownership rule:
//   The document owns object membership and document-level state. Bounds are
//   cached projection data and must be recomputed from geometry/style.
//
// Preserve later:
//   Serialization must convert to/from this typed model. Serialization must not
//   become the model.
//
// Surface contract:
//   - Primary responsibility: describe a complete typed drafting document.
//   - Allowed data: document ID, ordered objects, layers, selection state,
//     metadata, revision, and cached derived bounds.
//   - Call direction: stores and commands mutate documents; renderers and
//     format adapters read public document shape.
//   - Mutation authority: document values can be mutated only through store or
//     command APIs once implementation begins.
//   - Unit convention: geometry is document-space; viewport/screen pixels live
//     outside this contract.
//   - Identity policy: object/layer IDs are stable handles; object ordering is
//     a document concern.
//   - Lifetime: document owns objects/layers; styles/assets may be referenced.
//   - Composition boundary: object membership lives here, object math does not.
//   - Promotion path: large documents can later gain dense object tables while
//     preserving this public document contract.
