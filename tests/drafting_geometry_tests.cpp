#include "drafting/DraftingGeometry.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

using namespace edi::drafting;

int main()
{
    DraftingGeometry line = LineGeometry{{0.0, 0.0}, {3.0, 4.0}};
    Bounds2D bounds = computeBounds(line);
    assert(bounds.width == 3.0);
    assert(bounds.height == 4.0);
    assert(distance({0.0, 0.0}, {3.0, 4.0}) == 5.0);

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
    assert(guideOrientationName(GuideOrientation::Vertical) == std::string("vertical"));
    assert(validateGeometry(line).ok);
    assert(validateGeometry(GuideGeometry{GuideOrientation::Horizontal, 0.5}).ok);
    assert(!validateGeometry(GuideGeometry{GuideOrientation::Vertical, 2.0}).ok);
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

    return 0;
}
