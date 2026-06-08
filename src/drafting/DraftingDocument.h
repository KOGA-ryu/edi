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
// Must not depend on:
//   - UI selection widgets or canvas painting.
//   - TOML, TOON, MessagePack, or Lua.
//   - Concrete command dispatch implementations.
//
// Preserve later:
//   Serialization must convert to/from this typed model. Serialization must not
//   become the model.
