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
