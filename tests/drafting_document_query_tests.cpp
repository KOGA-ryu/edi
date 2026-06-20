#include "drafting/DraftingDocument.h"

#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <utility>

using namespace edi::drafting;

static_assert(shapeKindOf<PointGeometry>() == DraftingShapeKind::Point);
static_assert(shapeKindOf<LineGeometry>() == DraftingShapeKind::Line);
static_assert(shapeKindOf<RectangleGeometry>() == DraftingShapeKind::Rectangle);
static_assert(shapeKindOf<CircleGeometry>() == DraftingShapeKind::Circle);
static_assert(shapeKindOf<ArcGeometry>() == DraftingShapeKind::Arc);
static_assert(shapeKindOf<PolygonGeometry>() == DraftingShapeKind::Polygon);
static_assert(shapeKindOf<PolylineGeometry>() == DraftingShapeKind::Polyline);
static_assert(shapeKindOf<GuideGeometry>() == DraftingShapeKind::Guide);
static_assert(shapeKindOf<ConstructionLineGeometry>() == DraftingShapeKind::ConstructionLine);
static_assert(shapeKindOf<DimensionGeometry>() == DraftingShapeKind::Dimension);
static_assert(shapeKindOf<WallGeometry>() == DraftingShapeKind::Wall);

namespace {

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

} // namespace

int main()
{
    DraftingDocument document = makeDraftingDocument("query_doc");
    EDI_CHECK(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 1.0}})).ok);
    EDI_CHECK(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25})).ok);

    // No active object selected.
    EDI_CHECK(activeObject(document) == nullptr);
    EDI_CHECK(activeObjectOfKind(document, DraftingShapeKind::Line) == nullptr);

    // Active id points at a missing object.
    document.activeObjectId = "missing";
    EDI_CHECK(activeObject(document) == nullptr);
    EDI_CHECK(activeObjectOfKind(document, DraftingShapeKind::Line) == nullptr);

    // Active line resolves; kind match returns the same object, mismatch returns null.
    document.activeObjectId = "line_1";
    const DraftingObject *activeLine = activeObject(document);
    EDI_CHECK(activeLine != nullptr);
    EDI_CHECK(activeLine->id == "line_1");
    EDI_CHECK(activeObjectOfKind(document, DraftingShapeKind::Line) == activeLine);
    EDI_CHECK(activeObjectOfKind(document, DraftingShapeKind::Guide) == nullptr);

    // Active guide resolves under its own kind only.
    document.activeObjectId = "guide_1";
    const DraftingObject *activeGuide = activeObjectOfKind(document, DraftingShapeKind::Guide);
    EDI_CHECK(activeGuide != nullptr);
    EDI_CHECK(activeGuide->id == "guide_1");
    EDI_CHECK(activeObjectOfKind(document, DraftingShapeKind::Line) == nullptr);

    return 0;
}
