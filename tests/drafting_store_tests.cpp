#include "drafting/DraftingStore.h"

#include <cassert>
#include <limits>

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

    auto add = addObject(document, makeLine("line_1"));
    assert(add.ok);
    assert(document.objects.size() == 1);
    assert(document.objects.front().bounds.width == 10.0);

    auto duplicate = addObject(document, makeLine("line_1"));
    assert(!duplicate.ok);
    assert(duplicate.code == DraftingResultCode::DuplicateObjectId);
    assert(document.objects.size() == 1);
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

    auto remove = removeObject(document, "line_1");
    assert(remove.ok);
    assert(document.objects.empty());

    auto missingRemove = removeObject(document, "line_1");
    assert(!missingRemove.ok);
    assert(missingRemove.code == DraftingResultCode::ObjectNotFound);

    return 0;
}
