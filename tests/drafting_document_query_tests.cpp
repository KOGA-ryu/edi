#include "drafting/DraftingDocument.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <utility>

using namespace edi::drafting;

static_assert(shapeKindOf<PointGeometry>() == DraftingShapeKind::Point);
static_assert(shapeKindOf<LineGeometry>() == DraftingShapeKind::Line);
static_assert(shapeKindOf<RectangleGeometry>() == DraftingShapeKind::Rectangle);
static_assert(shapeKindOf<CircleGeometry>() == DraftingShapeKind::Circle);
static_assert(shapeKindOf<PolygonGeometry>() == DraftingShapeKind::Polygon);
static_assert(shapeKindOf<PolylineGeometry>() == DraftingShapeKind::Polyline);
static_assert(shapeKindOf<GuideGeometry>() == DraftingShapeKind::Guide);
static_assert(shapeKindOf<ConstructionLineGeometry>() == DraftingShapeKind::ConstructionLine);
static_assert(shapeKindOf<DimensionGeometry>() == DraftingShapeKind::Dimension);

namespace {

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    DraftingDocument document = makeDraftingDocument("query_doc");
    assert(addObject(document, object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {1.0, 1.0}})).ok);
    assert(addObject(document, object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25})).ok);

    // No active object selected.
    assert(activeObject(document) == nullptr);
    assert(activeObjectOfKind(document, DraftingShapeKind::Line) == nullptr);

    // Active id points at a missing object.
    document.activeObjectId = "missing";
    assert(activeObject(document) == nullptr);
    assert(activeObjectOfKind(document, DraftingShapeKind::Line) == nullptr);

    // Active line resolves; kind match returns the same object, mismatch returns null.
    document.activeObjectId = "line_1";
    const DraftingObject *activeLine = activeObject(document);
    assert(activeLine != nullptr);
    assert(activeLine->id == "line_1");
    assert(activeObjectOfKind(document, DraftingShapeKind::Line) == activeLine);
    assert(activeObjectOfKind(document, DraftingShapeKind::Guide) == nullptr);

    // Active guide resolves under its own kind only.
    document.activeObjectId = "guide_1";
    const DraftingObject *activeGuide = activeObjectOfKind(document, DraftingShapeKind::Guide);
    assert(activeGuide != nullptr);
    assert(activeGuide->id == "guide_1");
    assert(activeObjectOfKind(document, DraftingShapeKind::Line) == nullptr);

    return 0;
}
