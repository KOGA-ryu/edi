#include "drafting/DraftingGeometry.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    DraftingGeometry line = LineGeometry{{0.0, 0.0}, {3.0, 4.0}};
    Bounds2D bounds = computeBounds(line);
    assert(bounds.width == 3.0);
    assert(bounds.height == 4.0);
    assert(distance({0.0, 0.0}, {3.0, 4.0}) == 5.0);
    assert(nearlyEqual(lineAngleDegrees(LineGeometry{{0.0, 0.0}, {3.0, 4.0}}), 53.1301023542));

    DraftingGeometry rect = RectangleGeometry{{2.0, 3.0}, 10.0, 5.0};
    Bounds2D rectBounds = computeBounds(rect);
    assert(rectBounds.x == 2.0);
    assert(rectBounds.y == 3.0);
    assert(area(rect) == 50.0);

    auto handles = handleAnchors(line);
    assert(handles.size() == 2);
    assert(handles.front().id == "line_start");

    assert(draftingResultCodeName(DraftingResultCode::InvalidGeometry) == std::string("invalid_geometry"));
    assert(shapeKindName(DraftingShapeKind::Guide) == std::string("guide"));
    assert(shapeKindName(DraftingShapeKind::ConstructionLine) == std::string("construction_line"));
    assert(shapeKindName(DraftingShapeKind::Dimension) == std::string("dimension"));
    assert(guideOrientationName(GuideOrientation::Vertical) == std::string("vertical"));
    assert(validateGeometry(line).ok);
    assert(validateGeometry(GuideGeometry{GuideOrientation::Horizontal, 0.5}).ok);
    assert(!validateGeometry(GuideGeometry{GuideOrientation::Vertical, 2.0}).ok);
    assert(validateGeometry(ConstructionLineGeometry{{0.0, 0.25}, {1.0, 0.75}}).ok);
    assert(!validateGeometry(ConstructionLineGeometry{{0.5, 0.5}, {0.5, 0.5}}).ok);
    assert(validateGeometry(DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {0.3, 0.4}, 0.04}).ok);
    assert(!validateGeometry(DimensionGeometry{DimensionKind::Distance, {0.2, 0.2}, {0.2, 0.2}, 0.04}).ok);
    assert(nearlyEqual(dimensionAngleDegrees(DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {0.3, 0.4}, 0.04}), 53.1301023542));
    assert(nearlyEqual(displayedDimensionLength(0.5, DimensionKind::Distance), 0.5));
    assert(nearlyEqual(displayedDimensionLength(0.5, DimensionKind::Diameter), 1.0));
    assert(nearlyEqual(storedDimensionLength(1.0, DimensionKind::Distance), 1.0));
    assert(nearlyEqual(storedDimensionLength(1.0, DimensionKind::Diameter), 0.5));
    assert(!validateGeometry(CircleGeometry{{0.0, 0.0}, -1.0}).ok);
    assert(!validateGeometry(RectangleGeometry{{0.0, 0.0}, -1.0, 2.0}).ok);
    assert(!validateGeometry(PolygonGeometry{{{0.0, 0.0}, {1.0, 1.0}}}).ok);
    assert(!validateGeometry(PolylineGeometry{{{0.0, 0.0}}}).ok);
    assert(!validateGeometry(PointGeometry{{std::numeric_limits<double>::infinity(), 0.0}}).ok);

    DraftingGeometry horizontalGuide = GuideGeometry{GuideOrientation::Horizontal, 0.25};
    Bounds2D guideBounds = computeBounds(horizontalGuide);
    assert(guideBounds.x == 0.0);
    assert(guideBounds.y == 0.25);
    assert(guideBounds.width == 1.0);
    assert(guideBounds.height == 0.0);
    DraftingGeometry movedGuide = translateGeometry(horizontalGuide, 0.0, 0.25);
    const auto *movedGuideGeometry = std::get_if<GuideGeometry>(&movedGuide);
    assert(movedGuideGeometry != nullptr);
    assert(movedGuideGeometry->position == 0.5);

    DraftingGeometry construction = ConstructionLineGeometry{{0.2, 0.3}, {0.8, 0.9}};
    Bounds2D constructionBounds = computeBounds(construction);
    assert(nearlyEqual(constructionBounds.x, 0.2));
    assert(nearlyEqual(constructionBounds.y, 0.3));
    assert(nearlyEqual(constructionBounds.width, 0.6));
    assert(nearlyEqual(constructionBounds.height, 0.6));
    DraftingGeometry movedConstruction = translateGeometry(construction, 0.1, -0.2);
    const auto *movedConstructionGeometry = std::get_if<ConstructionLineGeometry>(&movedConstruction);
    assert(movedConstructionGeometry != nullptr);
    assert(nearlyEqual(movedConstructionGeometry->a.x, 0.3));
    assert(nearlyEqual(movedConstructionGeometry->a.y, 0.1));

    DraftingGeometry dimension = DimensionGeometry{DimensionKind::Distance, {0.2, 0.3}, {0.8, 0.3}, 0.1};
    Bounds2D dimensionBounds = computeBounds(dimension);
    assert(nearlyEqual(dimensionBounds.x, 0.2));
    assert(nearlyEqual(dimensionBounds.y, 0.3));
    assert(nearlyEqual(dimensionBounds.width, 0.6));
    assert(nearlyEqual(dimensionBounds.height, 0.1));
    DraftingGeometry movedDimension = translateGeometry(dimension, 0.1, 0.2);
    const auto *movedDimensionGeometry = std::get_if<DimensionGeometry>(&movedDimension);
    assert(movedDimensionGeometry != nullptr);
    assert(nearlyEqual(movedDimensionGeometry->a.x, 0.3));
    assert(nearlyEqual(movedDimensionGeometry->a.y, 0.5));

    return 0;
}
