// DraftingMeasurement.h
//
// Purpose:
//   Declares measurement contracts for drafting and build-planning work.
//
// Expected contracts:
//   - Scale calibration from canvas units to real-world units.
//   - Distance measurements.
//   - Object dimensions.
//   - Area measurements where geometry supports it.
//   - Measurement metadata references for annotated drafting objects.
//
// Ownership rule:
//   Measurement owns derived quantities. It does not own source geometry or
//   mutate documents.
//
// Must not depend on:
//   - UI rulers or inspector widgets.
//   - File format parser types.
//   - Scripting runtime state.
//
// Preserve later:
//   Measurement should be explicit about units, scale, precision, and whether a
//   result is exact, approximate, or unsupported.
