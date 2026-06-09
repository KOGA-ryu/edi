#include "drafting/DraftingPhysicalGeometry.h"

#include <cassert>
#include <cmath>

using namespace edi::drafting;

namespace {

bool near(double a, double b, double epsilon = 0.000001)
{
    return std::abs(a - b) <= epsilon;
}

DraftingGridProjection grid(double width = 12.0, double height = 8.0)
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.width = width;
    settings.height = height;
    return projectDraftingGrid(settings);
}

} // namespace

int main()
{
    const DraftingGridProjection projected = grid();
    assert(validPhysicalGrid(projected));
    assert(near(physicalX({0.25, 0.5}, projected), 3.0));
    assert(near(physicalY({0.25, 0.5}, projected), 4.0));
    assert(near(physicalWidth(0.5, projected), 6.0));
    assert(near(physicalHeight(0.5, projected), 4.0));
    assert(near(physicalDistance({0.25, 0.25}, {0.75, 0.75}, projected), std::sqrt(6.0 * 6.0 + 4.0 * 4.0)));
    assert(near(physicalAngleDegrees({0.25, 0.25}, {0.75, 0.75}, projected), 33.69006752598));
    DimensionGeometry horizontalDimension{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.2}, 0.05};
    Point2D horizontalOffset = dimensionOffsetVector(horizontalDimension);
    assert(near(horizontalOffset.x, 0.0));
    assert(near(horizontalOffset.y, 0.05));
    assert(near(physicalDimensionOffset(horizontalDimension, projected), 0.4));

    DimensionGeometry angledDimension{DimensionKind::Distance, {0.1, 0.2}, {0.4, 0.6}, 0.04};
    Point2D angledOffset = dimensionOffsetVector(angledDimension);
    assert(near(angledOffset.x, -0.032));
    assert(near(angledOffset.y, 0.024));
    assert(near(physicalDimensionOffset(angledDimension, projected), 0.42932505167996));

    DimensionGeometry negativeOffsetDimension{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.2}, -0.05};
    Point2D negativeOffset = dimensionOffsetVector(negativeOffsetDimension);
    assert(near(negativeOffset.x, 0.0));
    assert(near(negativeOffset.y, -0.05));
    assert(near(physicalDimensionOffset(negativeOffsetDimension, projected), -0.4));

    DimensionGeometry collapsedDimension{DimensionKind::Distance, {0.1, 0.2}, {0.1, 0.2}, 0.05};
    Point2D collapsedOffset = dimensionOffsetVector(collapsedDimension);
    assert(near(collapsedOffset.x, 0.0));
    assert(near(collapsedOffset.y, 0.0));
    assert(near(physicalDimensionOffset(collapsedDimension, projected), 0.0));

    DraftingGridProjection badWidth = projected;
    badWidth.settings.width = 0.0;
    assert(!validPhysicalGrid(badWidth));

    DraftingGridProjection badHeight = projected;
    badHeight.settings.height = -1.0;
    assert(!validPhysicalGrid(badHeight));

    return 0;
}
