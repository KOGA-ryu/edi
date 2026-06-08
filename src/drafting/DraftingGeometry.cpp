// DraftingGeometry.cpp
//
// Implementation responsibility:
//   Implements pure drafting geometry math.
//
// Belongs here:
//   - Bounds for point, line, rectangle, circle, polygon, and polyline.
//   - Translation and transform helpers.
//   - Distance/area primitives.
//   - Hit-test scoring and handle anchor calculations when they are pure.
//
// Must be delegated elsewhere:
//   - Applying geometry changes belongs in DraftingStore or DraftingCommands.
//   - Selection decisions belong in DraftingSelection or command logic.
//   - Drawing pixels belongs in widgets/renderers.
//
// Boundary note:
//   This file should never edit a document. It computes facts from inputs.
//
// Surface contract:
//   - Primary responsibility: implement pure geometry calculations.
//   - Allowed data: value geometry, numeric tolerances, transforms, and derived
//     output structs.
//   - Call direction: called by domain modules and render/input projection code.
//   - Mutation authority: compute-only.
//   - Unit convention: document-space in and document-space out unless a helper
//     explicitly names another unit.
//   - Identity policy: works on geometry values, not object IDs.
//   - Lifetime: no caching unless an explicit acceleration structure is added.
//   - Composition boundary: geometry remains independent from storage.
//   - Promotion path: spatial indexing can wrap these functions without moving
//     object ownership into this file.
