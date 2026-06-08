// drafting_store_tests.cpp
//
// Contract family:
//   Drafting document storage invariants.
//
// Surface contract:
//   - Primary responsibility: document the storage contract examples that will
//     later become executable checks.
//   - Allowed setup data: minimal documents, layers, object IDs, typed geometry,
//     style/metadata payloads, and store operation requests.
//   - Call direction: test code calls DraftingStore APIs directly.
//   - Mutation authority: tests may mutate local fixtures only.
//   - Unit convention: document-space geometry values.
//   - Identity policy: fixtures use stable IDs and avoid depending on indexes.
//   - Lifetime: each scenario owns its fixture document.
//   - Composition boundary: focused on storage examples, not command/UI flows.
//   - Promotion path: convert these notes into small arrange/act/assert cases.
