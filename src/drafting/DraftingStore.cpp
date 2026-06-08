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
