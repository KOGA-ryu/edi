#pragma once

#include "drafting/DraftingTypes.h"

namespace edi::drafting {

// Epsilon for AABB overlap tolerance — touching edges (delta <= eps) are NOT
// considered an overlap so corridor-separated rooms that share a boundary wall
// remain valid. This is a NUMERICAL TOLERANCE, not a dimension (exempt from the
// no-hardcoded-dims rule per CLAUDE.md, but named here for clarity and to give
// callers a single authoritative value rather than repeating 1e-9).
inline constexpr double kFootprintOverlapEps = 1e-9;

// Axis-aligned bounding-box intersection test, eps-tolerant.
// Returns true if the two rectangles overlap in their INTERIORS (i.e. the
// intersection area is strictly greater than the eps strip). Edge-touching
// (shared boundary within the eps band) returns false.
//
// WHY this lives in src/drafting (not src/io)?
//   The AABB test is a pure geometric primitive with no IO or Qt dependency.
//   Moving it here makes it the SHARED SUBSTRATE (decision 3): the TOML parser,
//   the controller's interactive authoring guard, and the future Phase-3 overlap
//   resolver all call this one function instead of each carrying a local copy of
//   the same formula. A single definition means a single place to fix if the
//   tolerance logic ever needs adjustment.
//
// Arguments mirror DraftingMapRoom's layout: origin (NW corner in canvas units),
// width (x-axis), height (y-axis).
bool footprintsOverlap(Point2D originA, double widthA, double heightA,
                       Point2D originB, double widthB, double heightB,
                       double eps = kFootprintOverlapEps);

} // namespace edi::drafting
