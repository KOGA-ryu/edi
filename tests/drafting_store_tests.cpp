// drafting_store_tests.cpp
//
// Contract family:
//   Drafting document storage invariants.
//
// Intended coverage:
//   - Object ID uniqueness.
//   - Kind/geometry consistency.
//   - Add/remove/update storage operations.
//   - Bounds recomputation after geometry/style changes.
//   - Rejection paths that leave the document unchanged.
//
// Should not test here:
//   - UI gestures.
//   - Rendering.
//   - File formats.
//   - Script command translation.
//
// Later role:
//   These tests should become executable documentation for what the drafting
//   store is allowed to mutate and what invariants it must protect.
