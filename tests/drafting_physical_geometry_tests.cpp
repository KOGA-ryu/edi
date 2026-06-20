#include "drafting/DraftingPhysicalGeometry.h"

#include "EdiAssert.h"
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
    EDI_CHECK(validPhysicalGrid(projected));
    EDI_CHECK(near(physicalX({0.25, 0.5}, projected), 3.0));
    EDI_CHECK(near(physicalY({0.25, 0.5}, projected), 4.0));
    EDI_CHECK(near(physicalWidth(0.5, projected), 6.0));
    EDI_CHECK(near(physicalHeight(0.5, projected), 4.0));
    EDI_CHECK(near(physicalDistance({0.25, 0.25}, {0.75, 0.75}, projected), std::sqrt(6.0 * 6.0 + 4.0 * 4.0)));
    EDI_CHECK(near(physicalAngleDegrees({0.25, 0.25}, {0.75, 0.75}, projected), 33.69006752598));
    DimensionGeometry horizontalDimension{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.2}, 0.05};
    Point2D horizontalOffset = dimensionOffsetVector(horizontalDimension);
    EDI_CHECK(near(horizontalOffset.x, 0.0));
    EDI_CHECK(near(horizontalOffset.y, 0.05));
    EDI_CHECK(near(physicalDimensionOffset(horizontalDimension, projected), 0.4));

    DimensionGeometry angledDimension{DimensionKind::Distance, {0.1, 0.2}, {0.4, 0.6}, 0.04};
    Point2D angledOffset = dimensionOffsetVector(angledDimension);
    EDI_CHECK(near(angledOffset.x, -0.032));
    EDI_CHECK(near(angledOffset.y, 0.024));
    EDI_CHECK(near(physicalDimensionOffset(angledDimension, projected), 0.42932505167996));

    DimensionGeometry negativeOffsetDimension{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.2}, -0.05};
    Point2D negativeOffset = dimensionOffsetVector(negativeOffsetDimension);
    EDI_CHECK(near(negativeOffset.x, 0.0));
    EDI_CHECK(near(negativeOffset.y, -0.05));
    EDI_CHECK(near(physicalDimensionOffset(negativeOffsetDimension, projected), -0.4));

    DimensionGeometry collapsedDimension{DimensionKind::Distance, {0.1, 0.2}, {0.1, 0.2}, 0.05};
    Point2D collapsedOffset = dimensionOffsetVector(collapsedDimension);
    EDI_CHECK(near(collapsedOffset.x, 0.0));
    EDI_CHECK(near(collapsedOffset.y, 0.0));
    EDI_CHECK(near(physicalDimensionOffset(collapsedDimension, projected), 0.0));

    DraftingGridProjection badWidth = projected;
    badWidth.settings.width = 0.0;
    EDI_CHECK(!validPhysicalGrid(badWidth));

    DraftingGridProjection badHeight = projected;
    badHeight.settings.height = -1.0;
    EDI_CHECK(!validPhysicalGrid(badHeight));

    return 0;
}
