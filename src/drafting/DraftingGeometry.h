#pragma once

#include "drafting/DraftingTypes.h"

#include <string>
#include <vector>

namespace edi::drafting {

struct HandleAnchor {
    std::string id;
    Point2D point;
};

struct GeometryValidationResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::InvalidGeometry;
    std::string message;

    static GeometryValidationResult accepted();
    static GeometryValidationResult rejected(std::string message);
};

bool isFinite(Point2D point);
bool isFinite(const Bounds2D &bounds);
GeometryValidationResult validateGeometry(const DraftingGeometry &geometry);
Bounds2D computeBounds(const DraftingGeometry &geometry);
DraftingGeometry translateGeometry(const DraftingGeometry &geometry, double dx, double dy);
Point2D translatePoint(Point2D point, double dx, double dy);
double distance(Point2D a, Point2D b);
double lineAngleDegrees(const LineGeometry &line);
double dimensionAngleDegrees(const DimensionGeometry &dimension);
double displayedDimensionLength(double storedLength, DimensionKind kind);
double storedDimensionLength(double displayedLength, DimensionKind kind);
double area(const DraftingGeometry &geometry);
std::vector<HandleAnchor> handleAnchors(const DraftingGeometry &geometry);

} // namespace edi::drafting
