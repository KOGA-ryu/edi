#include "drafting/DraftingCommands.h"
#include "drafting/DraftingSelection.h"

#include <cassert>
#include <limits>

using namespace edi::drafting;

int main()
{
    DraftingDocument document = makeDraftingDocument("doc");

    auto acceptedResult = DraftingCommandResult::accepted();
    assert(acceptedResult.ok);
    assert(acceptedResult.code == DraftingResultCode::None);
    auto rejectedResult = DraftingCommandResult::rejected(DraftingResultCode::ObjectNotFound, "missing");
    assert(!rejectedResult.ok);
    assert(rejectedResult.code == DraftingResultCode::ObjectNotFound);

    auto builtPoint = buildDraftingObject("point_1", DraftingShapeKind::Point, PointGeometry{{4.0, 5.0}});
    assert(builtPoint.ok);
    DraftingObject point = builtPoint.object;

    auto create = applyDraftingCommand(document, CreateObjectCommand{point});
    assert(create.ok);
    assert(create.code == DraftingResultCode::None);
    assert(containsObject(document, "point_1"));

    auto select = applyDraftingCommand(document, SelectObjectCommand{"point_1"});
    assert(select.ok);
    assert(document.activeObjectId == "point_1");

    auto move = applyDraftingCommand(document, MoveObjectCommand{"point_1", 1.0, 1.0});
    assert(move.ok);
    const auto *moved = findObject(document, "point_1");
    assert(moved != nullptr);
    assert(moved->bounds.x == 5.0);
    assert(moved->bounds.y == 6.0);

    auto del = applyDraftingCommand(document, DeleteObjectCommand{"point_1"});
    assert(del.ok);
    assert(!containsObject(document, "point_1"));

    const auto revisionAfterDelete = document.revision;
    auto missingSelect = applyDraftingCommand(document, SelectObjectCommand{"point_1"});
    assert(!missingSelect.ok);
    assert(missingSelect.code == DraftingResultCode::InvalidSelectionTarget);
    assert(document.revision == revisionAfterDelete);
    assert(!document.activeObjectId);

    DraftingObject invalidPolyline = makeDraftingObject("polyline_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.0, 0.0}}});
    auto invalidCreate = applyDraftingCommand(document, CreateObjectCommand{invalidPolyline});
    assert(!invalidCreate.ok);
    assert(invalidCreate.code == DraftingResultCode::InvalidGeometry);
    assert(document.revision == revisionAfterDelete);

    DraftingDocument editDocument = makeDraftingDocument("edit_doc");
    auto builtLine = buildDraftingObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {10.0, 10.0}});
    assert(builtLine.ok);
    assert(applyDraftingCommand(editDocument, CreateObjectCommand{builtLine.object}).ok);
    const auto revisionAfterLineCreate = editDocument.revision;
    auto editLine = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "line_end", {20.0, 5.0}});
    assert(editLine.ok);
    const auto *editedLineObject = findObject(editDocument, "line_1");
    assert(editedLineObject != nullptr);
    const auto *editedLine = std::get_if<LineGeometry>(&editedLineObject->geometry);
    assert(editedLine != nullptr);
    assert(editedLine->b.x == 20.0);
    assert(editedLine->b.y == 5.0);
    assert(editDocument.revision == revisionAfterLineCreate + 1);

    const auto revisionAfterLineEdit = editDocument.revision;
    auto badHandle = applyDraftingCommand(editDocument, EditObjectHandleCommand{"line_1", "missing_handle", {1.0, 1.0}});
    assert(!badHandle.ok);
    assert(badHandle.code == DraftingResultCode::InvalidSelectionTarget);
    assert(editDocument.revision == revisionAfterLineEdit);

    auto builtRect = buildDraftingObject("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{1.0, 1.0}, 3.0, 4.0});
    assert(builtRect.ok);
    assert(applyDraftingCommand(editDocument, CreateObjectCommand{builtRect.object}).ok);
    assert(applyDraftingCommand(editDocument, SelectObjectCommand{"line_1"}).ok);
    toggleSelection(editDocument, "rect_1");
    const auto revisionBeforeSelectionMove = editDocument.revision;
    auto moveSelection = applyDraftingCommand(editDocument, MoveSelectionCommand{2.0, 3.0});
    assert(moveSelection.ok);
    assert(editDocument.revision == revisionBeforeSelectionMove + 1);
    const auto *movedLineObject = findObject(editDocument, "line_1");
    const auto *movedRectObject = findObject(editDocument, "rect_1");
    assert(movedLineObject != nullptr);
    assert(movedRectObject != nullptr);
    const auto *movedLine = std::get_if<LineGeometry>(&movedLineObject->geometry);
    const auto *movedRect = std::get_if<RectangleGeometry>(&movedRectObject->geometry);
    assert(movedLine != nullptr);
    assert(movedRect != nullptr);
    assert(movedLine->a.x == 2.0);
    assert(movedLine->a.y == 3.0);
    assert(movedRect->origin.x == 3.0);
    assert(movedRect->origin.y == 4.0);

    const auto revisionBeforeBadMove = editDocument.revision;
    auto badSelectionMove = applyDraftingCommand(editDocument, MoveSelectionCommand{std::numeric_limits<double>::infinity(), 0.0});
    assert(!badSelectionMove.ok);
    assert(badSelectionMove.code == DraftingResultCode::InvalidGeometry);
    assert(editDocument.revision == revisionBeforeBadMove);

    return 0;
}
