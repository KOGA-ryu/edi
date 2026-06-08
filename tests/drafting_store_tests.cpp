#include "drafting/DraftingStore.h"

#include <cassert>

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
    assert(document.objects.size() == 1);

    auto move = moveObject(document, "line_1", 2.0, 3.0);
    assert(move.ok);
    const auto *line = findObject(document, "line_1");
    assert(line != nullptr);
    assert(line->bounds.x == 2.0);
    assert(line->bounds.y == 3.0);

    auto remove = removeObject(document, "line_1");
    assert(remove.ok);
    assert(document.objects.empty());

    return 0;
}
