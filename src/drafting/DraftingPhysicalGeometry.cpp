#include "drafting/DraftingPhysicalGeometry.h"

#include <cmath>

namespace edi::drafting {

bool validPhysicalGrid(const DraftingGridProjection &grid)
{
    return std::isfinite(grid.settings.width)
        && std::isfinite(grid.settings.height)
        && grid.settings.width > 0.0
        && grid.settings.height > 0.0;
}

double physicalX(Point2D point, const DraftingGridProjection &grid)
{
    return point.x * grid.settings.width;
}

double physicalY(Point2D point, const DraftingGridProjection &grid)
{
    return point.y * grid.settings.height;
}

double physicalWidth(double normalizedWidth, const DraftingGridProjection &grid)
{
    return normalizedWidth * grid.settings.width;
}

double physicalHeight(double normalizedHeight, const DraftingGridProjection &grid)
{
    return normalizedHeight * grid.settings.height;
}

double physicalDistance(Point2D a, Point2D b, const DraftingGridProjection &grid)
{
    const double dx = physicalWidth(b.x - a.x, grid);
    const double dy = physicalHeight(b.y - a.y, grid);
    return std::sqrt(dx * dx + dy * dy);
}

double physicalAngleDegrees(Point2D a, Point2D b, const DraftingGridProjection &grid)
{
    constexpr double pi = 3.14159265358979323846;
    const double dx = physicalWidth(b.x - a.x, grid);
    const double dy = physicalHeight(b.y - a.y, grid);
    return std::atan2(dy, dx) * 180.0 / pi;
}

Point2D dimensionOffsetVector(const DimensionGeometry &dimension)
{
    const double normalizedLength = std::sqrt(
        (dimension.b.x - dimension.a.x) * (dimension.b.x - dimension.a.x)
        + (dimension.b.y - dimension.a.y) * (dimension.b.y - dimension.a.y));
    if (normalizedLength <= 0.000001) {
        return {};
    }
    return {
        -(dimension.b.y - dimension.a.y) / normalizedLength * dimension.offset,
        (dimension.b.x - dimension.a.x) / normalizedLength * dimension.offset,
    };
}

double physicalDimensionOffset(const DimensionGeometry &dimension, const DraftingGridProjection &grid)
{
    const Point2D offset = dimensionOffsetVector(dimension);
    const double dx = physicalWidth(offset.x, grid);
    const double dy = physicalHeight(offset.y, grid);
    const double magnitude = std::sqrt(dx * dx + dy * dy);
    return dimension.offset < 0.0 ? -magnitude : magnitude;
}

} // namespace edi::drafting
