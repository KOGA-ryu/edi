#pragma once

#include "drafting/DraftingTypes.h"

#include <string>
#include <vector>

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

// N points evenly spaced around the circle, at startAngleDeg + i·360/n
// (i = 0..n-1), in the model's angle convention. Returns empty when n < 2 (the
// inscribe ops below turn that into a rejection) — pure point list, no validation.
std::vector<Point2D> divideCirclePoints(const CircleGeometry &circle, int n, double startAngleDeg = 0.0);

struct DraftingInscribedResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    PolygonGeometry geometry; // closed (a PolygonGeometry is implicitly closed), editable

    static DraftingInscribedResult accepted(PolygonGeometry geometry);
    static DraftingInscribedResult rejected(DraftingResultCode code, std::string message);
};

// The regular N-gon inscribed in the circle: divideCirclePoints in order as one
// closed polygon. Rejects n < 3 (and a degenerate circle / non-finite input).
DraftingInscribedResult inscribeRegularPolygon(const CircleGeometry &circle, int n, double startAngleDeg = 0.0);

// The {n/step} star polygon: connect every step-th of the n perimeter points. This
// is a SINGLE closed path only when gcd(n, step) == 1 (otherwise tracing yields a
// disjoint compound, e.g. {6/2} = two triangles, which is not one PolygonGeometry).
// Valid iff n >= 5, 2 <= step, step < n/2, and gcd(n, step) == 1; every other case
// is rejected with a message naming why.
DraftingInscribedResult inscribeStarPolygon(const CircleGeometry &circle, int n, int step, double startAngleDeg = 0.0);

} // namespace edi::drafting
