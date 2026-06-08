#include "drafting/DraftingStore.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingSelection.h"

#include <cassert>
#include <limits>
#include <optional>

using namespace edi::drafting;

namespace {

DraftingObject makeLine(const char *id)
{
    DraftingObject object;
    object.id = id;
    object.kind = DraftingShapeKind::Line;
    object.geometry = LineGeometry{{0.0, 0.0}, {10.0, 0.0}};
    return object;
}

} // namespace

int main()
{
    DraftingDocument document = makeDraftingDocument("doc");
    assert(layerIndexById(document, "default") == 0);
    assert(findLayer(document, "default") == &document.layers[0]);
    assert(containsLayer(document, "default"));
    assert(layerIndexById(document, "missing_layer") == std::nullopt);
    assert(findLayer(document, "missing_layer") == nullptr);
    assert(!containsLayer(document, "missing_layer"));

    auto add = addObject(document, makeLine("line_1"));
    assert(add.ok);
    assert(document.objects.size() == 1);
    assert(document.objects.front().bounds.width == 10.0);
    assert(objectIndexById(document, "line_1") == 0);

    auto addSecond = addObject(document, makeLine("line_2"));
    assert(addSecond.ok);
    auto addThird = addObject(document, makeLine("line_3"));
    assert(addThird.ok);
    assert(document.objects.size() == 3);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_2");
    assert(document.objects[2].id == "line_3");
    assert(objectIndexById(document, "line_2") == 1);
    assert(findObject(document, "line_2") == &document.objects[1]);
    assert(containsObject(document, "line_2"));
    assert(objectIndexById(document, "missing") == std::nullopt);
    assert(findObject(document, "missing") == nullptr);
    assert(!containsObject(document, "missing"));

    auto duplicate = addObject(document, makeLine("line_1"));
    assert(!duplicate.ok);
    assert(duplicate.code == DraftingResultCode::DuplicateObjectId);
    assert(document.objects.size() == 3);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_2");
    assert(document.objects[2].id == "line_3");
    const auto revisionAfterDuplicate = document.revision;

    DraftingObject emptyId = makeLine("");
    auto emptyIdResult = addObject(document, emptyId);
    assert(!emptyIdResult.ok);
    assert(emptyIdResult.code == DraftingResultCode::EmptyObjectId);
    assert(document.revision == revisionAfterDuplicate);

    DraftingObject invalidCircle;
    invalidCircle.id = "bad_circle";
    invalidCircle.kind = DraftingShapeKind::Circle;
    invalidCircle.geometry = CircleGeometry{{0.0, 0.0}, -4.0};
    auto invalidCircleResult = addObject(document, invalidCircle);
    assert(!invalidCircleResult.ok);
    assert(invalidCircleResult.code == DraftingResultCode::InvalidGeometry);
    assert(document.revision == revisionAfterDuplicate);

    DraftingObject badKind;
    badKind.id = "bad_kind";
    badKind.kind = DraftingShapeKind::Line;
    badKind.geometry = PointGeometry{{0.0, 0.0}};
    auto badKindResult = addObject(document, badKind);
    assert(!badKindResult.ok);
    assert(badKindResult.code == DraftingResultCode::KindGeometryMismatch);
    assert(document.revision == revisionAfterDuplicate);

    DraftingObject missingLayer = makeLine("missing_layer_object");
    missingLayer.layerId = "missing_layer";
    auto missingLayerResult = addObject(document, missingLayer);
    assert(!missingLayerResult.ok);
    assert(missingLayerResult.code == DraftingResultCode::LayerNotFound);
    assert(document.revision == revisionAfterDuplicate);
    assert(objectIndexById(document, "missing_layer_object") == std::nullopt);

    auto move = moveObject(document, "line_1", 2.0, 3.0);
    assert(move.ok);
    const auto *line = findObject(document, "line_1");
    assert(line != nullptr);
    assert(line->bounds.x == 2.0);
    assert(line->bounds.y == 3.0);
    const auto revisionAfterMove = document.revision;

    auto badMove = moveObject(document, "line_1", std::numeric_limits<double>::quiet_NaN(), 0.0);
    assert(!badMove.ok);
    assert(badMove.code == DraftingResultCode::InvalidGeometry);
    assert(document.revision == revisionAfterMove);

    selectOnly(document, "line_2");
    auto removeMiddle = removeObject(document, "line_2");
    assert(removeMiddle.ok);
    assert(document.objects.size() == 2);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_3");
    assert(objectIndexById(document, "line_3") == 1);
    assert(document.selectedObjectIds.empty());
    assert(!document.activeObjectId);

    const auto revisionAfterRemoveMiddle = document.revision;
    auto missingRemoveBeforeOrder = removeObject(document, "missing");
    assert(!missingRemoveBeforeOrder.ok);
    assert(missingRemoveBeforeOrder.code == DraftingResultCode::ObjectNotFound);
    assert(document.revision == revisionAfterRemoveMiddle);
    assert(document.objects[0].id == "line_1");
    assert(document.objects[1].id == "line_3");

    auto remove = removeObject(document, "line_1");
    assert(remove.ok);
    auto removeLastRemaining = removeObject(document, "line_3");
    assert(removeLastRemaining.ok);
    assert(document.objects.empty());

    auto missingRemove = removeObject(document, "line_1");
    assert(!missingRemove.ok);
    assert(missingRemove.code == DraftingResultCode::ObjectNotFound);

    return 0;
}
