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
// Must not depend on:
//   - UI control state.
//   - Raw pointer/mouse events.
//   - Format or scripting adapters.
//
// Preserve later:
//   Selection should be stable under deleted/missing object IDs by normalizing
//   against the document.
