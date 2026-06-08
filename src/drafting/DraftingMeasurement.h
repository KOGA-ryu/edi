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
// Preserve later:
//   Measurement should be explicit about units, scale, precision, and whether a
//   result is exact, approximate, or unsupported.
//
// Surface contract:
//   - Primary responsibility: declare real-world measurement contracts over
//     drafting geometry.
//   - Allowed data: calibration records, unit names/enums, geometry values,
//     distance/dimension/area result records, and measurement metadata.
//   - Call direction: commands, inspectors, build-plan generators, and UI
//     presentation layers may call measurement helpers.
//   - Mutation authority: compute-only.
//   - Unit convention: converts document-space values to calibrated real-world
//     units.
//   - Identity policy: measurements may reference object IDs but do not own
//     objects.
//   - Lifetime: derived measurement values are snapshots.
//   - Composition boundary: measurement calculates; build-plan sequencing lives
//     elsewhere.
//   - Promotion path: domain-specific measurement systems can layer on this.
