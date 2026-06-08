// DraftingMeasurement.cpp
//
// Implementation responsibility:
//   Implements measurement and unit conversion helpers for typed drafting
//   geometry.
//
// Belongs here:
//   - Applying scale calibration.
//   - Computing distance, dimensions, and area from geometry.
//   - Returning unsupported/invalid measurement diagnostics.
//
// Must be delegated elsewhere:
//   - Editing geometry belongs in drafting commands/store.
//   - Persisting measurement records belongs in format adapters.
//   - Presenting measurements belongs in UI widgets.
//
// Boundary note:
//   Measurements are derived from geometry and calibration. They are not a
//   second source of geometry truth.
//
// Surface contract:
//   - Primary responsibility: implement unit conversion and derived measurement
//     calculations.
//   - Allowed data: typed geometry, calibration records, unit records, and
//     derived measurement outputs.
//   - Call direction: called by drafting commands, inspectors, and planning
//     tools.
//   - Mutation authority: compute-only.
//   - Unit convention: document-space input, calibrated unit output.
//   - Identity policy: object IDs may label results but do not drive math.
//   - Lifetime: no ownership of source geometry.
//   - Composition boundary: this file does not create build plans by itself.
//   - Promotion path: precision/rounding policy can split into a measurement
//     formatting module later.
