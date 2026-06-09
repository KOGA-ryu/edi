#include "drafting/DraftingConstructionOps.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingConstructionLinePlan DraftingConstructionLinePlan::accepted(ConstructionLineGeometry geometry)
{
    DraftingConstructionLinePlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.geometry = geometry;
    return result;
}

DraftingConstructionLinePlan DraftingConstructionLinePlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingConstructionLinePlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
}

bool isHorizontalConstructionLine(const ConstructionLineGeometry &line)
{
    constexpr double epsilon = 0.0000001;
    return std::abs(line.a.y - line.b.y) < epsilon;
}

bool isVerticalConstructionLine(const ConstructionLineGeometry &line)
{
    constexpr double epsilon = 0.0000001;
    return std::abs(line.a.x - line.b.x) < epsilon;
}

DraftingConstructionLinePlan fitConstructionLineToDrawable(const ConstructionLineGeometry &line, Bounds2D drawable)
{
    if (!isFinite(line.a) || !isFinite(line.b)) {
        return DraftingConstructionLinePlan::rejected(DraftingResultCode::InvalidGeometry, "construction line endpoints must be finite");
    }
    if (!isFinite(drawable) || drawable.width <= 0.0 || drawable.height <= 0.0) {
        return DraftingConstructionLinePlan::rejected(DraftingResultCode::InvalidGeometry, "drawable bounds must be positive and finite");
    }

    const bool horizontal = isHorizontalConstructionLine(line);
    const bool vertical = isVerticalConstructionLine(line);
    if (!horizontal && !vertical) {
        return DraftingConstructionLinePlan::rejected(DraftingResultCode::InvalidGeometry, "construction line must be horizontal or vertical");
    }

    ConstructionLineGeometry next = line;
    if (horizontal) {
        next.a = {drawable.x, line.a.y};
        next.b = {drawable.x + drawable.width, line.a.y};
    } else {
        next.a = {line.a.x, drawable.y};
        next.b = {line.a.x, drawable.y + drawable.height};
    }
    return DraftingConstructionLinePlan::accepted(next);
}

} // namespace edi::drafting
