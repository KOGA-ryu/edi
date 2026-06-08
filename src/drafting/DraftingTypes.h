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
// Preserve later:
//   Keep these types plain and stable. They are the load-bearing vocabulary used
//   by geometry, store, commands, measurement, and formats.
//
// Surface contract:
//   - Primary responsibility: define the typed vocabulary shared by all drafting
//     modules.
//   - Allowed data: IDs, coordinates, dimensions, transforms, style values,
//     metadata values, shape kind enum, and typed geometry records.
//   - Call direction: included by drafting modules and format adapters that
//     convert typed drafting contracts.
//   - Mutation authority: none; this file defines value shapes.
//   - Unit convention: document-space coordinates unless a type name says
//     otherwise; real-world units belong in measurement types.
//   - Identity policy: stable IDs are public; dense indexes can be internal to
//     stores and acceleration structures.
//   - Lifetime: value types are copied/moved; ownership is decided by stores.
//   - Composition boundary: types stay data-only and do not call behavior.
//   - Promotion path: hot-path arrays can later be built from these stable value
//     contracts without changing public IDs.
