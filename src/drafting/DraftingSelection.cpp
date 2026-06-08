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
