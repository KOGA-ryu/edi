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

} // namespace edi::drafting
