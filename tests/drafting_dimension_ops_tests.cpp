#include "drafting/DraftingDimensionOps.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    assert(draftingDimensionKindFromId("distance") == DimensionKind::Distance);
    assert(draftingDimensionKindFromId("width") == DimensionKind::Width);
    assert(draftingDimensionKindFromId("height") == DimensionKind::Height);
    assert(draftingDimensionKindFromId("radius") == DimensionKind::Radius);
    assert(draftingDimensionKindFromId("diameter") == DimensionKind::Diameter);
    assert(!draftingDimensionKindFromId("ordinal"));

    DimensionGeometry angled{DimensionKind::Distance, {0.1, 0.2}, {0.4, 0.6}, 0.04};
    const double angledLength = std::hypot(0.3, 0.4);

    auto width = planDimensionKindChange(angled, DimensionKind::Width);
    assert(width.ok);
    assert(width.geometry.kind == DimensionKind::Width);
    assert(near(width.geometry.a.x, 0.1));
    assert(near(width.geometry.a.y, 0.2));
    assert(near(width.geometry.b.x, 0.1 + angledLength));
    assert(near(width.geometry.b.y, 0.2));
    assert(near(width.geometry.offset, 0.04));

    DimensionGeometry reversedWidth{DimensionKind::Distance, {0.5, 0.2}, {0.1, 0.6}, 0.04};
    auto widthNegative = planDimensionKindChange(reversedWidth, DimensionKind::Width);
    assert(widthNegative.ok);
    assert(near(widthNegative.geometry.b.x, 0.5 - std::hypot(0.4, 0.4)));
    assert(near(widthNegative.geometry.b.y, 0.2));

    auto height = planDimensionKindChange(angled, DimensionKind::Height);
    assert(height.ok);
    assert(height.geometry.kind == DimensionKind::Height);
    assert(near(height.geometry.b.x, 0.1));
    assert(near(height.geometry.b.y, 0.2 + angledLength));

    DimensionGeometry reversedHeight{DimensionKind::Distance, {0.1, 0.6}, {0.4, 0.2}, 0.04};
    auto heightNegative = planDimensionKindChange(reversedHeight, DimensionKind::Height);
    assert(heightNegative.ok);
    assert(near(heightNegative.geometry.b.x, 0.1));
    assert(near(heightNegative.geometry.b.y, 0.6 - std::hypot(0.3, 0.4)));

    auto diameter = planDimensionKindChange(angled, DimensionKind::Diameter);
    assert(diameter.ok);
    assert(diameter.geometry.kind == DimensionKind::Diameter);
    assert(near(diameter.geometry.b.x, 0.4));
    assert(near(diameter.geometry.b.y, 0.6));

    auto collapsed = planDimensionKindChange(
        DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.1, 0.2}, 0.04},
        DimensionKind::Width);
    assert(!collapsed.ok);
    assert(collapsed.code == DraftingResultCode::InvalidGeometry);

    auto nonFinite = planDimensionKindChange(
        DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {std::numeric_limits<double>::infinity(), 0.2}, 0.04},
        DimensionKind::Width);
    assert(!nonFinite.ok);
    assert(nonFinite.code == DraftingResultCode::InvalidGeometry);

    return 0;
}
