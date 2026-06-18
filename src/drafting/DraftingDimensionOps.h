#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>

namespace edi::drafting {

// Default radius (canvas units) from the vertex to the arc/label of a freshly
// placed Angular dimension. The user can later drag `b` to resize it.
constexpr double kDefaultAngularArc = 0.1;

struct DraftingDimensionPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DimensionGeometry geometry;

    static DraftingDimensionPlan accepted(DimensionGeometry geometry);
    static DraftingDimensionPlan rejected(DraftingResultCode code, std::string message);
};

std::optional<DimensionKind> draftingDimensionKindFromId(const std::string &kindId);
DraftingDimensionPlan planDimensionKindChange(const DimensionGeometry &dimension, DimensionKind kind);

// Plan an Angular dimension from the intersection of two lines.
// Encoding (Candidate A, settled by reviewer gate):
//   geometry.a      = vertex V (intersection point)
//   geometry.b      = V + normalize(l1 direction) * kDefaultAngularArc   (ray1 tip)
//   geometry.offset = signed angle in degrees FROM ray1 TO ray2
// Rejects parallel / non-intersecting lines with InvalidSelectionTarget.
DraftingDimensionPlan planAngularDimension(const LineGeometry &l1, const LineGeometry &l2);

// Convenience planners that build a correct DimensionGeometry from a single
// circle or arc pick — no new DimensionKind is introduced; each reuses an
// existing kind with the right encoding.

// Radius dimension for an arc: kind=Radius, a=center, b=midpoint on arc perimeter
// (arcPointAtAngle at the mid-sweep angle), offset=0.04 (standoff).
// Rejects zero or non-finite radius.
DraftingDimensionPlan planRadialDimensionForArc(const ArcGeometry &arc);

// Diameter dimension for a circle: kind=Diameter, a=center,
// b=center+(radius,0) so stored |b−a|=radius while displayedDimensionLength
// (kind=Diameter) doubles it to show the full diameter. offset=0.04.
// Rejects zero or non-finite radius.
DraftingDimensionPlan planRadialDimensionForCircle(const CircleGeometry &circle);

// Arc-sweep dimension: reuses DimensionKind::Angular (DR-13 Candidate A encoding).
//   a      = arc.center (vertex)
//   b      = arcPointAtAngle(center, kDefaultAngularArc, startAngleDeg)  (ray1 tip)
//   offset = arc.endAngleDeg − arc.startAngleDeg  (signed sweep in degrees)
// dimensionMeasuredAngle of the result equals the arc's sweep angle.
// Rejects zero/non-finite radius or zero sweep.
DraftingDimensionPlan planArcSweepDimension(const ArcGeometry &arc);

} // namespace edi::drafting
