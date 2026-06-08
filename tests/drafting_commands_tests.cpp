#include "drafting/DraftingCommands.h"

#include <cassert>

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

    DraftingObject point;
    point.id = "point_1";
    point.kind = DraftingShapeKind::Point;
    point.geometry = PointGeometry{{4.0, 5.0}};

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

    DraftingObject invalidPolyline;
    invalidPolyline.id = "polyline_1";
    invalidPolyline.kind = DraftingShapeKind::Polyline;
    invalidPolyline.geometry = PolylineGeometry{{{0.0, 0.0}}};
    auto invalidCreate = applyDraftingCommand(document, CreateObjectCommand{invalidPolyline});
    assert(!invalidCreate.ok);
    assert(invalidCreate.code == DraftingResultCode::InvalidGeometry);
    assert(document.revision == revisionAfterDelete);

    return 0;
}
