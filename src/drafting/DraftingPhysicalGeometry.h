#pragma once

#include "drafting/DraftingGrid.h"

namespace edi::drafting {

bool validPhysicalGrid(const DraftingGridProjection &grid);
double physicalX(Point2D point, const DraftingGridProjection &grid);
double physicalY(Point2D point, const DraftingGridProjection &grid);
double physicalWidth(double normalizedWidth, const DraftingGridProjection &grid);
double physicalHeight(double normalizedHeight, const DraftingGridProjection &grid);
double physicalDistance(Point2D a, Point2D b, const DraftingGridProjection &grid);
double physicalAngleDegrees(Point2D a, Point2D b, const DraftingGridProjection &grid);
Point2D dimensionOffsetVector(const DimensionGeometry &dimension);
double physicalDimensionOffset(const DimensionGeometry &dimension, const DraftingGridProjection &grid);

} // namespace edi::drafting
