#include "drafting/DraftingSelection.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <unordered_set>

namespace edi::drafting {

namespace {

bool boundsIntersect(Bounds2D a, Bounds2D b)
{
    if (!isFinite(a) || !isFinite(b)) {
        return false;
    }
    return a.x <= b.x + b.width
        && a.x + a.width >= b.x
        && a.y <= b.y + b.height
        && a.y + a.height >= b.y;
}

} // namespace

void clearSelection(DraftingDocument &document)
{
    document.selectedObjectIds.clear();
    document.activeObjectId.reset();
}

void selectOnly(DraftingDocument &document, DraftingObjectId id)
{
    document.selectedObjectIds.clear();
    if (containsObject(document, id)) {
        document.activeObjectId = id;
        document.selectedObjectIds.push_back(std::move(id));
    } else {
        document.activeObjectId.reset();
    }
}

void selectMany(DraftingDocument &document, std::vector<DraftingObjectId> ids)
{
    document.selectedObjectIds.clear();
    document.activeObjectId.reset();
    // Hash sets for both membership tests: containsObject per id re-scans
    // the document and isSelected re-scans the growing selection — together
    // O(N^2) for select-everything-sized inputs (measured ~1s at 9800 ids).
    const std::unordered_set<DraftingObjectId> existing = objectIdSet(document);
    std::unordered_set<DraftingObjectId> seen;
    seen.reserve(ids.size());
    for (DraftingObjectId &id : ids) {
        if (existing.find(id) == existing.end() || !seen.insert(id).second) {
            continue;
        }
        document.activeObjectId = id;
        document.selectedObjectIds.push_back(std::move(id));
    }
}

void toggleSelection(DraftingDocument &document, DraftingObjectId id)
{
    auto it = std::find(document.selectedObjectIds.begin(), document.selectedObjectIds.end(), id);
    if (it == document.selectedObjectIds.end()) {
        if (containsObject(document, id)) {
            document.selectedObjectIds.push_back(id);
            document.activeObjectId = std::move(id);
        }
        return;
    }

    document.selectedObjectIds.erase(it);
    if (document.activeObjectId == id) {
        if (document.selectedObjectIds.empty()) {
            document.activeObjectId.reset();
        } else {
            document.activeObjectId = document.selectedObjectIds.back();
        }
    }
}

bool isSelected(const DraftingDocument &document, const DraftingObjectId &id)
{
    return std::find(document.selectedObjectIds.begin(), document.selectedObjectIds.end(), id) != document.selectedObjectIds.end();
}

std::vector<DraftingObjectId> selectableObjectsInBounds(const DraftingDocument &document, Bounds2D bounds)
{
    if (!isFinite(bounds)) {
        return {};
    }

    std::vector<DraftingObjectId> objectIds;
    for (const DraftingObject &object : document.objects) {
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (object.visible
            && layer != nullptr
            && layer->visible
            && boundsIntersect(object.bounds, bounds)) {
            objectIds.push_back(object.id);
        }
    }
    return objectIds;
}

void normalizeSelection(DraftingDocument &document)
{
    document.selectedObjectIds.erase(
        std::remove_if(document.selectedObjectIds.begin(), document.selectedObjectIds.end(), [&](const DraftingObjectId &id) {
            return !containsObject(document, id);
        }),
        document.selectedObjectIds.end());

    if (document.activeObjectId && !isSelected(document, *document.activeObjectId)) {
        document.activeObjectId.reset();
    }
    if (!document.activeObjectId && !document.selectedObjectIds.empty()) {
        document.activeObjectId = document.selectedObjectIds.back();
    }
}

} // namespace edi::drafting
