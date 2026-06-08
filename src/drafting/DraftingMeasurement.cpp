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
