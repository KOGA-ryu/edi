// DraftingGeometry.h
//
// Purpose:
//   Declares pure geometry operations for drafting objects.
//
// Expected contracts:
//   - Compute bounds from typed geometry and style.
//   - Translate geometry.
//   - Transform points.
//   - Measure distances and dimensions.
//   - Compute hit-test scores.
//   - Generate handle anchors for legal edit intents.
//
// Ownership rule:
//   Geometry owns math only. It does not own document storage, selection, command
//   mutation, rendering, or persistence.
//
// Preserve later:
//   Every function here should be deterministic, finite for valid input, and
//   testable as a pure function.
//
// Surface contract:
//   - Primary responsibility: declare geometry calculations over typed drafting
//     values.
//   - Allowed data: geometry structs, style/calibration inputs where needed,
//     points, bounds, transforms, tolerances, and handle descriptors.
//   - Call direction: store, commands, measurement, hit testing, and rendering
//     may call geometry helpers.
//   - Mutation authority: none; compute-only.
//   - Unit convention: document-space coordinates by default; viewport pixels
//     are converted before reaching this layer.
//   - Identity policy: geometry uses values, not object ownership.
//   - Lifetime: returns derived values; does not retain inputs.
//   - Composition boundary: math does not know project/workspace state.
//   - Promotion path: hot math can split into dense/vectorized kernels later.
