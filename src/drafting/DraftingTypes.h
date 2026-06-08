// DraftingTypes.h
//
// Purpose:
//   Defines the primitive typed vocabulary for the drafting system.
//
// Expected contracts:
//   - Stable IDs: drafting object IDs, layer IDs, style IDs, document IDs.
//   - Numeric geometry primitives: Point2D, Bounds2D, Transform2D.
//   - Style primitives: stroke, fill, opacity, line width, line style.
//   - Metadata fields: author/source, created time, tool provenance, measurement
//     information.
//   - Shape kind enum for point, line, rectangle, circle, polygon, and polyline.
//   - Typed geometry structs, later collected under a variant-style geometry
//     contract.
//
// Ownership rule:
//   This file owns names and shapes of basic data only. It does not own object
//   lifecycle, command mutation, rendering, or persistence.
//
// Must not depend on:
//   - Qt Widgets.
//   - Format parser types.
//   - Scripting/runtime command bridges.
//
// Preserve later:
//   Keep these types plain and stable. They are the load-bearing vocabulary used
//   by geometry, store, commands, measurement, and formats.
