// DraftingSelection.cpp
//
// Implementation responsibility:
//   Implements deterministic selection operations over drafting object IDs.
//
// Belongs here:
//   - Selecting/toggling/clearing IDs.
//   - Maintaining active object consistency.
//   - Dropping missing IDs when normalizing against a document.
//
// Must be delegated elsewhere:
//   - Hit-test target choice belongs in geometry/input logic.
//   - Object mutation belongs in store/commands.
//   - UI highlight drawing belongs in widgets/renderers.
//
// Boundary note:
//   Selection changes are state changes, but they are not geometry mutations.
//
// Surface contract:
//   - Primary responsibility: implement ID-based selection operations.
//   - Allowed data: selection values, object IDs, and document membership reads.
//   - Call direction: called by drafting commands and controller adapters.
//   - Mutation authority: selection state only.
//   - Unit convention: ID sets/lists only.
//   - Identity policy: active object must be one of the selected IDs when set.
//   - Lifetime: stores copied IDs, not object references.
//   - Composition boundary: keeps selection logic separate from object edits.
//   - Promotion path: add selection groups or named sets here if needed later.
