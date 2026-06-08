#include "drafting/DraftingCommands.h"

#include <cassert>

using namespace edi::drafting;

int main()
{
    DraftingDocument document = makeDraftingDocument("doc");

    DraftingObject point;
    point.id = "point_1";
    point.kind = DraftingShapeKind::Point;
    point.geometry = PointGeometry{{4.0, 5.0}};

    auto create = applyDraftingCommand(document, CreateObjectCommand{point});
    assert(create.ok);
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

    return 0;
}
