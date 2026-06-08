#include "drafting/DraftingSelection.h"

#include <algorithm>

namespace edi::drafting {

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
    for (DraftingObjectId &id : ids) {
        if (!containsObject(document, id) || isSelected(document, id)) {
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
