#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
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
    EDI_CHECK(bounds.width == 3.0);
    EDI_CHECK(bounds.height == 4.0);
    Bounds2D combined = includeBounds({0.25, 0.25, 0.25, 0.25}, {0.1, 0.4, 0.3, 0.3});
    EDI_CHECK(nearlyEqual(combined.x, 0.1));
    EDI_CHECK(nearlyEqual(combined.y, 0.25));
    EDI_CHECK(nearlyEqual(combined.width, 0.4));
    EDI_CHECK(nearlyEqual(combined.height, 0.45));
    EDI_CHECK(boundsContainsPoint({0.25, 0.25, 0.5, 0.5}, {0.5, 0.5}));
    EDI_CHECK(!boundsContainsPoint({0.25, 0.25, 0.5, 0.5}, {0.1, 0.5}));
    EDI_CHECK(distance({0.0, 0.0}, {3.0, 4.0}) == 5.0);
    EDI_CHECK(nearlyEqual(lineAngleDegrees(LineGeometry{{0.0, 0.0}, {3.0, 4.0}}), 53.1301023542));

    DraftingGeometry rect = RectangleGeometry{{2.0, 3.0}, 10.0, 5.0};
    Bounds2D rectBounds = computeBounds(rect);
    EDI_CHECK(rectBounds.x == 2.0);
    EDI_CHECK(rectBounds.y == 3.0);
    EDI_CHECK(area(rect) == 50.0);

    auto handles = handleAnchors(line);
    EDI_CHECK(handles.size() == 2);
    EDI_CHECK(handles.front().id == "line_start");

    EDI_CHECK(draftingResultCodeName(DraftingResultCode::InvalidGeometry) == std::string("invalid_geometry"));
    EDI_CHECK(shapeKindName(DraftingShapeKind::Guide) == std::string("guide"));
    EDI_CHECK(shapeKindName(DraftingShapeKind::ConstructionLine) == std::string("construction_line"));
    EDI_CHECK(shapeKindName(DraftingShapeKind::Dimension) == std::string("dimension"));
    EDI_CHECK(guideOrientationName(GuideOrientation::Vertical) == std::string("vertical"));
    EDI_CHECK(validateGeometry(line).ok);
    EDI_CHECK(validateGeometry(GuideGeometry{GuideOrientation::Horizontal, 0.5}).ok);
    EDI_CHECK(!validateGeometry(GuideGeometry{GuideOrientation::Vertical, 2.0}).ok);
    EDI_CHECK(validateGeometry(ConstructionLineGeometry{{0.0, 0.25}, {1.0, 0.75}}).ok);
    EDI_CHECK(!validateGeometry(ConstructionLineGeometry{{0.5, 0.5}, {0.5, 0.5}}).ok);
    EDI_CHECK(validateGeometry(DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {0.3, 0.4}, 0.04}).ok);
    EDI_CHECK(!validateGeometry(DimensionGeometry{DimensionKind::Distance, {0.2, 0.2}, {0.2, 0.2}, 0.04}).ok);
    EDI_CHECK(nearlyEqual(dimensionAngleDegrees(DimensionGeometry{DimensionKind::Distance, {0.0, 0.0}, {0.3, 0.4}, 0.04}), 53.1301023542));
    EDI_CHECK(nearlyEqual(displayedDimensionLength(0.5, DimensionKind::Distance), 0.5));
    EDI_CHECK(nearlyEqual(displayedDimensionLength(0.5, DimensionKind::Diameter), 1.0));
    EDI_CHECK(nearlyEqual(storedDimensionLength(1.0, DimensionKind::Distance), 1.0));
    EDI_CHECK(nearlyEqual(storedDimensionLength(1.0, DimensionKind::Diameter), 0.5));
    EDI_CHECK(!validateGeometry(CircleGeometry{{0.0, 0.0}, -1.0}).ok);
    EDI_CHECK(!validateGeometry(RectangleGeometry{{0.0, 0.0}, -1.0, 2.0}).ok);
    EDI_CHECK(!validateGeometry(PolygonGeometry{{{0.0, 0.0}, {1.0, 1.0}}}).ok);
    EDI_CHECK(!validateGeometry(PolylineGeometry{{{0.0, 0.0}}}).ok);
    EDI_CHECK(!validateGeometry(PointGeometry{{std::numeric_limits<double>::infinity(), 0.0}}).ok);

    DraftingGeometry horizontalGuide = GuideGeometry{GuideOrientation::Horizontal, 0.25};
    Bounds2D guideBounds = computeBounds(horizontalGuide);
    EDI_CHECK(guideBounds.x == 0.0);
    EDI_CHECK(guideBounds.y == 0.25);
    EDI_CHECK(guideBounds.width == 1.0);
    EDI_CHECK(guideBounds.height == 0.0);
    DraftingGeometry movedGuide = translateGeometry(horizontalGuide, 0.0, 0.25);
    const auto *movedGuideGeometry = std::get_if<GuideGeometry>(&movedGuide);
    EDI_CHECK(movedGuideGeometry != nullptr);
    EDI_CHECK(movedGuideGeometry->position == 0.5);
    EDI_CHECK(!translationHasEffect(0.0, 0.0));
    EDI_CHECK(!translationHasEffect(0.00000001, 0.0));
    EDI_CHECK(translationHasEffect(0.0000001, 0.0));
    EDI_CHECK(translationHasEffect(0.0, -0.0000001));
    EDI_CHECK(!translationHasEffect(std::numeric_limits<double>::infinity(), 0.0));

    DraftingGeometry construction = ConstructionLineGeometry{{0.2, 0.3}, {0.8, 0.9}};
    Bounds2D constructionBounds = computeBounds(construction);
    EDI_CHECK(nearlyEqual(constructionBounds.x, 0.2));
    EDI_CHECK(nearlyEqual(constructionBounds.y, 0.3));
    EDI_CHECK(nearlyEqual(constructionBounds.width, 0.6));
    EDI_CHECK(nearlyEqual(constructionBounds.height, 0.6));
    DraftingGeometry movedConstruction = translateGeometry(construction, 0.1, -0.2);
    const auto *movedConstructionGeometry = std::get_if<ConstructionLineGeometry>(&movedConstruction);
    EDI_CHECK(movedConstructionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(movedConstructionGeometry->a.x, 0.3));
    EDI_CHECK(nearlyEqual(movedConstructionGeometry->a.y, 0.1));

    DraftingGeometry dimension = DimensionGeometry{DimensionKind::Distance, {0.2, 0.3}, {0.8, 0.3}, 0.1};
    Bounds2D dimensionBounds = computeBounds(dimension);
    EDI_CHECK(nearlyEqual(dimensionBounds.x, 0.2));
    EDI_CHECK(nearlyEqual(dimensionBounds.y, 0.3));
    EDI_CHECK(nearlyEqual(dimensionBounds.width, 0.6));
    EDI_CHECK(nearlyEqual(dimensionBounds.height, 0.1));
    DraftingGeometry movedDimension = translateGeometry(dimension, 0.1, 0.2);
    const auto *movedDimensionGeometry = std::get_if<DimensionGeometry>(&movedDimension);
    EDI_CHECK(movedDimensionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(movedDimensionGeometry->a.x, 0.3));
    EDI_CHECK(nearlyEqual(movedDimensionGeometry->a.y, 0.5));

    return 0;
}
