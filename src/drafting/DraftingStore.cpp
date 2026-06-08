// DraftingStore.cpp
//
// Implementation responsibility:
//   Implements direct document storage operations and storage-level validation.
//
// Belongs here:
//   - Rejecting duplicate object IDs.
//   - Rejecting kind/geometry mismatches.
//   - Verifying target layer existence/editability when required.
//   - Recomputing bounds after geometry/style edits.
//   - Returning structured success/failure information.
//
// Must be delegated elsewhere:
//   - User-facing command interpretation belongs in DraftingCommands.
//   - Hit testing, snapping, and handle math belong in geometry/input contracts.
//   - Persistence belongs in format adapters.
//
// Boundary note:
//   This file mutates document storage, but only through explicit store APIs.
//
// Surface contract:
//   - Primary responsibility: implement object storage updates on documents.
//   - Allowed data: mutable documents, object records, IDs, layer references,
//     and geometry/style/metadata payloads.
//   - Call direction: called by drafting commands and focused domain utilities.
//   - Mutation authority: storage mutation for objects/layers only.
//   - Unit convention: document-space geometry in; derived document-space bounds
//     out through stored object fields.
//   - Identity policy: stable IDs at API boundary; internal maps/indexes may be
//     introduced for speed.
//   - Lifetime: never owns the document allocation; operates on references.
//   - Composition boundary: storage changes do not decide UI behavior.
//   - Promotion path: this is the natural home for data-oriented object tables.
