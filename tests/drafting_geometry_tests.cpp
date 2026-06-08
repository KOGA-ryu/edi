// drafting_geometry_tests.cpp
//
// Contract family:
//   Pure drafting geometry math.
//
// Surface contract:
//   - Primary responsibility: document pure geometry example cases.
//   - Allowed setup data: geometry values, transforms, tolerances, expected
//     bounds/distances/handles, and style inputs where needed.
//   - Call direction: test code calls DraftingGeometry functions directly.
//   - Mutation authority: none beyond local values.
//   - Unit convention: document-space input and output unless named otherwise.
//   - Identity policy: geometry examples use values, not document ownership.
//   - Lifetime: no shared fixture state.
//   - Composition boundary: focused on math examples.
//   - Promotion path: add dense/hot-path comparison cases if kernels split out.
