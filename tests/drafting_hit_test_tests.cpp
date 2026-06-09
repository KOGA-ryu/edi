#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    assert(nearlyEqual(hitDistance(PointGeometry{{0.0, 0.0}}, {3.0, 4.0}), 5.0));
    assert(nearlyEqual(hitDistance(LineGeometry{{0.0, 0.0}, {1.0, 0.0}}, {0.5, 0.25}), 0.25));
    assert(nearlyEqual(hitDistance(RectangleGeometry{{0.25, 0.25}, 0.25, 0.25}, {0.3, 0.3}), 0.0));
    assert(nearlyEqual(hitDistance(CircleGeometry{{0.5, 0.5}, 0.2}, {0.7, 0.5}), 0.0));
    assert(nearlyEqual(hitDistance(GuideGeometry{GuideOrientation::Horizontal, 0.25}, {0.7, 0.35}), 0.1));
    assert(nearlyEqual(hitDistance(ConstructionLineGeometry{{0.0, 0.25}, {1.0, 0.75}}, {0.5, 0.6}), 0.0894427));
    assert(nearlyEqual(hitDistance(DimensionGeometry{{0.0, 0.25}, {1.0, 0.25}, 0.1}, {0.5, 0.35}), 0.0));
    assert(hitDistance(PointGeometry{{0.0, 0.0}}, {std::numeric_limits<double>::infinity(), 0.0}) > 1.0e100);

    DraftingDocument document = makeDraftingDocument("hit_doc");
    assert(addObject(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})).ok);
    assert(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.5}, {1.0, 0.5}})).ok);
    assert(addObject(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.45, 0.45}, 0.2, 0.2})).ok);
    assert(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25})).ok);
    assert(addObject(document, object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.7, 0.0}, {0.7, 1.0}})).ok);
    assert(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{{0.1, 0.85}, {0.5, 0.85}, 0.04})).ok);

    DraftingHitTestResult lineHit = hitTestDocument(document, {0.4, 0.51});
    assert(lineHit.ok);
    assert(lineHit.objectId == "line_1");

    DraftingHitTestResult guideHit = hitTestDocument(document, {0.255, 0.8});
    assert(guideHit.ok);
    assert(guideHit.objectId == "guide_1");

    DraftingHitTestResult constructionHit = hitTestDocument(document, {0.705, 0.8});
    assert(constructionHit.ok);
    assert(constructionHit.objectId == "construction_1");

    DraftingHitTestResult dimensionHit = hitTestDocument(document, {0.3, 0.89});
    assert(dimensionHit.ok);
    assert(dimensionHit.objectId == "dimension_1");

    DraftingHitTestResult rectHit = hitTestDocument(document, {0.5, 0.5});
    assert(rectHit.ok);
    assert(rectHit.objectId == "rect_1");

    DraftingHitTestResult miss = hitTestDocument(document, {0.9, 0.9}, DraftingHitTestSettings{0.01});
    assert(!miss.ok);

    document.objects[2].visible = false;
    DraftingHitTestResult hiddenMiss = hitTestDocument(document, {0.5, 0.5});
    assert(hiddenMiss.ok);
    assert(hiddenMiss.objectId == "line_1");

    assert(!hitTestDocument(document, {0.5, 0.5}, DraftingHitTestSettings{-1.0}).ok);

    return 0;
}
