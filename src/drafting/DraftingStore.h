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
// Must not depend on:
//   - Mouse/keyboard events.
//   - Canvas rendering.
//   - Lua scripts.
//   - Raw format parser objects.
//
// Preserve later:
//   Store operations should be deterministic and testable without launching the
//   app.
