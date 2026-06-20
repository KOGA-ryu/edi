#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <cmath>
#include <limits>

using namespace edi::drafting;

namespace {

DraftingObject object(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    EDI_CHECK(nearlyEqual(hitDistance(PointGeometry{{0.0, 0.0}}, {3.0, 4.0}), 5.0));
    EDI_CHECK(nearlyEqual(hitDistance(LineGeometry{{0.0, 0.0}, {1.0, 0.0}}, {0.5, 0.25}), 0.25));
    EDI_CHECK(nearlyEqual(hitDistance(RectangleGeometry{{0.25, 0.25}, 0.25, 0.25}, {0.3, 0.3}), 0.0));
    EDI_CHECK(nearlyEqual(hitDistance(CircleGeometry{{0.5, 0.5}, 0.2}, {0.7, 0.5}), 0.0));
    EDI_CHECK(nearlyEqual(hitDistance(GuideGeometry{GuideOrientation::Horizontal, 0.25}, {0.7, 0.35}), 0.1));
    EDI_CHECK(nearlyEqual(hitDistance(ConstructionLineGeometry{{0.0, 0.25}, {1.0, 0.75}}, {0.5, 0.6}), 0.0894427));
    EDI_CHECK(nearlyEqual(hitDistance(DimensionGeometry{DimensionKind::Distance, {0.0, 0.25}, {1.0, 0.25}, 0.1}, {0.5, 0.35}), 0.0));
    EDI_CHECK(hitDistance(PointGeometry{{0.0, 0.0}}, {std::numeric_limits<double>::infinity(), 0.0}) > 1.0e100);

    DraftingDocument document = makeDraftingDocument("hit_doc");
    EDI_CHECK(addObject(document, object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.1}})).ok);
    EDI_CHECK(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.5}, {1.0, 0.5}})).ok);
    EDI_CHECK(addObject(document, object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.45, 0.45}, 0.2, 0.2})).ok);
    EDI_CHECK(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25})).ok);
    EDI_CHECK(addObject(document, object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.7, 0.0}, {0.7, 1.0}})).ok);
    EDI_CHECK(addObject(document, object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.85}, {0.5, 0.85}, 0.04})).ok);

    DraftingHitTestResult lineHit = hitTestDocument(document, {0.4, 0.51});
    EDI_CHECK(lineHit.ok);
    EDI_CHECK(lineHit.objectId == "line_1");

    DraftingHitTestResult guideHit = hitTestDocument(document, {0.255, 0.8});
    EDI_CHECK(guideHit.ok);
    EDI_CHECK(guideHit.objectId == "guide_1");

    DraftingHitTestResult constructionHit = hitTestDocument(document, {0.705, 0.8});
    EDI_CHECK(constructionHit.ok);
    EDI_CHECK(constructionHit.objectId == "construction_1");

    DraftingHitTestResult dimensionHit = hitTestDocument(document, {0.3, 0.89});
    EDI_CHECK(dimensionHit.ok);
    EDI_CHECK(dimensionHit.objectId == "dimension_1");

    DraftingHitTestResult rectHit = hitTestDocument(document, {0.5, 0.5});
    EDI_CHECK(rectHit.ok);
    EDI_CHECK(rectHit.objectId == "rect_1");

    DraftingHitTestResult miss = hitTestDocument(document, {0.9, 0.9}, DraftingHitTestSettings{0.01});
    EDI_CHECK(!miss.ok);

    DraftingDocument layeredHitDocument = makeDraftingDocument("layered_hit_doc");
    EDI_CHECK(addLayer(layeredHitDocument, makeDraftingLayer("top", "Top", 1)).ok);
    DraftingObject bottomRect = object("bottom_rect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.25, 0.25}, 0.5, 0.5});
    DraftingObject topRect = object("top_rect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.25, 0.25}, 0.5, 0.5});
    topRect.layerId = "top";
    EDI_CHECK(addObject(layeredHitDocument, bottomRect).ok);
    EDI_CHECK(addObject(layeredHitDocument, topRect).ok);
    DraftingHitTestResult topLayerHit = hitTestDocument(layeredHitDocument, {0.5, 0.5});
    EDI_CHECK(topLayerHit.ok);
    EDI_CHECK(topLayerHit.objectId == "top_rect");
    EDI_CHECK(updateLayerFlags(layeredHitDocument, "top", false, false).ok);
    DraftingHitTestResult hiddenTopLayerHit = hitTestDocument(layeredHitDocument, {0.5, 0.5});
    EDI_CHECK(hiddenTopLayerHit.ok);
    EDI_CHECK(hiddenTopLayerHit.objectId == "bottom_rect");

    document.objects[2].visible = false;
    DraftingHitTestResult hiddenMiss = hitTestDocument(document, {0.5, 0.5});
    EDI_CHECK(hiddenMiss.ok);
    EDI_CHECK(hiddenMiss.objectId == "line_1");

    EDI_CHECK(!hitTestDocument(document, {0.5, 0.5}, DraftingHitTestSettings{-1.0}).ok);

    return 0;
}
