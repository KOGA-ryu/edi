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
// trim verb is its first caller, and extend/fillet will reuse it.
std::optional<Point2D> segmentIntersection(const LineGeometry &a, const LineGeometry &b);

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

} // namespace edi::drafting
