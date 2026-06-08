#pragma once

#include "drafting/DraftingTypes.h"

#include <vector>

namespace edi::drafting {

struct HandleAnchor {
    std::string id;
    Point2D point;
};

Bounds2D computeBounds(const DraftingGeometry &geometry);
DraftingGeometry translateGeometry(const DraftingGeometry &geometry, double dx, double dy);
Point2D translatePoint(Point2D point, double dx, double dy);
double distance(Point2D a, Point2D b);
double area(const DraftingGeometry &geometry);
std::vector<HandleAnchor> handleAnchors(const DraftingGeometry &geometry);

} // namespace edi::drafting
