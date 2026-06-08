// DraftingSelection.h
//
// Purpose:
//   Declares selection state and selection helpers for drafting documents.
//
// Expected contracts:
//   - Selected object ID set/list.
//   - Active object ID.
//   - Select one, select many, toggle, clear, and normalize helpers.
//
// Ownership rule:
//   Selection owns which object IDs are selected. It does not own object
//   geometry, storage, hit testing, or rendering.
//
// Preserve later:
//   Selection should be stable under deleted/missing object IDs by normalizing
//   against the document.
//
// Surface contract:
//   - Primary responsibility: declare selected-object state and operations.
//   - Allowed data: selected ID list/set, active object ID, selection mode, and
//     document lookup access for normalization.
//   - Call direction: commands and controllers may use selection helpers.
//   - Mutation authority: may mutate selection state only.
//   - Unit convention: no coordinates; selection is ID-based.
//   - Identity policy: stable object IDs define selection.
//   - Lifetime: selection owns ID values, not objects.
//   - Composition boundary: selection does not choose hit-test targets.
//   - Promotion path: large selections can use sorted vectors or bitsets behind
//     the same public contract.
