#pragma once

#include "drafting/DraftingTypes.h"   // Point2D

namespace edi::drafting {

// Span-room sizing DIMENSIONS as NAMED DATA (no-hardcoded-dims rule). These are the op's STANDALONE
// FALLBACK defaults; the generator passes scene-appropriate values later (a span-spec field). Canvas units.
// kDefaultSpanBandWidth = the "wide": the span's breadth ACROSS the connector axis (a wide main room).
// kDefaultSpanEndPad    = the "big":  how far the span extends PAST each connector ALONG the axis.
inline constexpr double kDefaultSpanBandWidth = 0.2;   // FALLBACK breadth; generator overrides per scene
inline constexpr double kDefaultSpanEndPad    = 0.05;  // FALLBACK end extension; generator overrides

// Tolerance for the degenerate-length guard AND the axis-aligned classification. A NUMERICAL TOLERANCE,
// not a dimension (exempt from the no-hardcoded-dims rule, but named per CLAUDE.md — mirrors
// DraftingOverlap's kFootprintOverlapEps).
inline constexpr double kSpanAxisEps = 1e-9;

// Result: an axis-aligned bounding rect that drops straight into DraftingMapRoom origin/width/height.
// `origin` is the NW corner (min x, min y — y is south-positive, same convention as RoomSpec.origin).
// `axisAligned` = the two anchors share an x OR a y (the AABB is then TIGHT/exact); false for a DIAGONAL
// pair (the AABB is a correct-but-LOOSE bound of the rotated band — the caller knows to revisit; the
// rotated-rect / polygon ring is the fast-follow).
struct SpanFootprint {
    Point2D origin;          // NW corner, canvas units
    double  width  = 0.0;
    double  height = 0.0;
    bool    axisAligned = true;
};

// PURE: two connector anchors + sizing in, AABB out. The CALLER resolves node ids → anchors (decision 3).
SpanFootprint deriveSpanFootprint(Point2D anchorA, Point2D anchorB,
                                  double bandWidth = kDefaultSpanBandWidth,
                                  double endPad    = kDefaultSpanEndPad);

} // namespace edi::drafting
