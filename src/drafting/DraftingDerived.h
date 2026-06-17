#pragma once

#include "drafting/DraftingTypes.h"

#include <string>

namespace edi::drafting {

// DERIVED geometry: shapes CONSTRUCTED from input points rather than authored
// directly — the compass-and-straightedge constructions. This file is the home for
// that family and is meant to grow (DR-06 adds circle division + inscribed N-gon/
// star). Each op is a pure free function returning a plan struct (ok + payload),
// mirroring DraftingOffsetResult.

struct DraftingDerivedCircleResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    CircleGeometry geometry;

    static DraftingDerivedCircleResult accepted(CircleGeometry geometry);
    static DraftingDerivedCircleResult rejected(DraftingResultCode code, std::string message);
};

// The unique circle through three points (the circumcircle). Rejects collinear
// points (no finite circle passes through three collinear points) and non-finite
// input with DraftingResultCode::InvalidGeometry.
DraftingDerivedCircleResult circleThroughThreePoints(Point2D a, Point2D b, Point2D c);

// The circle with a and b as the ENDS of a diameter: center = midpoint(a,b),
// radius = distance(a,b)/2. Rejects coincident (zero-diameter) and non-finite input.
DraftingDerivedCircleResult circleThroughTwoPoints(Point2D a, Point2D b);

} // namespace edi::drafting
