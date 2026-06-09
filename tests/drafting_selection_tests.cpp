#include "drafting/DraftingSelection.h"

#include "drafting/DraftingStore.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <utility>
#include <vector>

using namespace edi::drafting;

namespace {

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

bool contains(const std::vector<DraftingObjectId> &ids, const DraftingObjectId &id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

int main()
{
    DraftingDocument document = makeDraftingDocument("selection_doc");
    assert(addLayer(document, makeDraftingLayer("hidden", "Hidden", 2)).ok);
    assert(addLayer(document, makeDraftingLayer("locked_visible", "Locked Visible", 3)).ok);

    assert(addObject(document, object("inside_line", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.2}})).ok);
    assert(addObject(document, object("crossing_line", DraftingShapeKind::Line, LineGeometry{{0.45, 0.45}, {0.7, 0.7}})).ok);
    assert(addObject(document, object("outside_line", DraftingShapeKind::Line, LineGeometry{{0.8, 0.8}, {0.9, 0.9}})).ok);

    DraftingObject hiddenObject = object("hidden_object", DraftingShapeKind::Line, LineGeometry{{0.1, 0.3}, {0.2, 0.3}});
    hiddenObject.visible = false;
    assert(addObject(document, hiddenObject).ok);

    DraftingObject hiddenLayerObject = object("hidden_layer_object", DraftingShapeKind::Line, LineGeometry{{0.1, 0.35}, {0.2, 0.35}});
    hiddenLayerObject.layerId = "hidden";
    assert(addObject(document, hiddenLayerObject).ok);
    assert(updateLayerFlags(document, "hidden", false, false).ok);

    DraftingObject lockedVisibleLayerObject = object("locked_visible_layer_object", DraftingShapeKind::Line, LineGeometry{{0.1, 0.4}, {0.2, 0.4}});
    lockedVisibleLayerObject.layerId = "locked_visible";
    assert(addObject(document, lockedVisibleLayerObject).ok);
    assert(updateLayerFlags(document, "locked_visible", true, true).ok);

    const std::vector<DraftingObjectId> selected = selectableObjectsInBounds(document, Bounds2D{0.0, 0.0, 0.5, 0.5});
    assert(contains(selected, "inside_line"));
    assert(contains(selected, "crossing_line"));
    assert(contains(selected, "locked_visible_layer_object"));
    assert(!contains(selected, "outside_line"));
    assert(!contains(selected, "hidden_object"));
    assert(!contains(selected, "hidden_layer_object"));

    const std::vector<DraftingObjectId> emptySelection = selectableObjectsInBounds(document, Bounds2D{0.0, 0.0, 0.0, 0.0});
    assert(emptySelection.empty());

    const std::vector<DraftingObjectId> invalidSelection = selectableObjectsInBounds(
        document,
        Bounds2D{0.0, 0.0, std::numeric_limits<double>::infinity(), 0.5});
    assert(invalidSelection.empty());

    return 0;
}
