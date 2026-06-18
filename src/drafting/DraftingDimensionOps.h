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

} // namespace edi::drafting
