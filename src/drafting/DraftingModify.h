#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace edi::drafting {

// Intersection of two line SEGMENTS in canvas space. Returns the crossing point
// only when it lies within BOTH segments (the parametric t and u each in
// [0,1]); a parallel/collinear pair or a crossing that falls off either segment
// returns nullopt. This is the first real intersection math in the core — the
// trim verb is its first caller.
std::optional<Point2D> segmentIntersection(const LineGeometry &a, const LineGeometry &b);

// Intersection of the two INFINITE lines through these segments — the crossing
// even when it lies beyond an endpoint. Only a parallel/collinear pair (or a
// zero-length segment) returns nullopt. Fillet needs this: the corner vertex of
// two segments can sit past where either one actually reaches.
std::optional<Point2D> lineIntersection(const LineGeometry &a, const LineGeometry &b);

struct DraftingTrimResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    LineGeometry geometry; // the trimmed line — valid only when ok

    static DraftingTrimResult accepted(LineGeometry geometry);
    static DraftingTrimResult rejected(DraftingResultCode code, std::string message);
};

// Trim a target line back to where a boundary line crosses it, removing the end
// the user clicked nearer. Among the boundaries, the one whose crossing sits
// nearest the pick wins (so clicking near a particular cut selects it). Rejects
// when no boundary crosses the target, or when the trim would collapse the line
// to zero length. Pure: the controller supplies the other lines as boundaries.
DraftingTrimResult trimLineAtPoint(const LineGeometry &target,
                                   const std::vector<LineGeometry> &boundaries,
                                   Point2D pick);

struct DraftingFilletResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    // On success: the two lines trimmed/extended to their tangent points, plus
    // the rounding arc that joins them. Caller applies all three atomically.
    LineGeometry line1;
    LineGeometry line2;
    ArcGeometry arc;

    static DraftingFilletResult accepted(LineGeometry line1, LineGeometry line2, ArcGeometry arc);
    static DraftingFilletResult rejected(DraftingResultCode code, std::string message);
};

// Round the corner between two lines with an arc of the given radius, tangent to
// both. `pick` selects which of the (up to four) corners to fillet — the arc
// sits in the corner the pick points at — and which other line to use. Each line
// is trimmed/extended so its corner-side end meets the arc's tangent point.
// Rejects parallel/collinear lines, a non-positive radius, or a fillet so large
// it would collapse a line. Pure: the controller supplies both lines + radius.
DraftingFilletResult filletLines(const LineGeometry &line1,
                                 const LineGeometry &line2,
                                 double radius,
                                 Point2D pick);

} // namespace edi::drafting
