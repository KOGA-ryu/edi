#include "drafting/DraftingDimensionOps.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingDimensionPlan DraftingDimensionPlan::accepted(DimensionGeometry geometry)
{
    DraftingDimensionPlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = geometry;
    return result;
}

DraftingDimensionPlan DraftingDimensionPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingDimensionPlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

std::optional<DimensionKind> draftingDimensionKindFromId(const std::string &kindId)
{
    if (kindId == "distance") {
        return DimensionKind::Distance;
    }
    if (kindId == "width") {
        return DimensionKind::Width;
    }
    if (kindId == "height") {
        return DimensionKind::Height;
    }
    if (kindId == "radius") {
        return DimensionKind::Radius;
    }
    if (kindId == "diameter") {
        return DimensionKind::Diameter;
    }
    return std::nullopt;
}

DraftingDimensionPlan planDimensionKindChange(const DimensionGeometry &dimension, DimensionKind kind)
{
    const double currentLength = distance(dimension.a, dimension.b);
    if (!std::isfinite(currentLength) || currentLength <= 0.000001) {
        return DraftingDimensionPlan::rejected(DraftingResultCode::InvalidGeometry, "dimension requires two distinct finite points");
    }

    DimensionGeometry next = dimension;
    next.kind = kind;
    if (next.kind == DimensionKind::Width) {
        const double sign = next.b.x < next.a.x ? -1.0 : 1.0;
        next.b = {next.a.x + sign * currentLength, next.a.y};
    } else if (next.kind == DimensionKind::Height) {
        const double sign = next.b.y < next.a.y ? -1.0 : 1.0;
        next.b = {next.a.x, next.a.y + sign * currentLength};
    }

    return DraftingDimensionPlan::accepted(next);
}

} // namespace edi::drafting
