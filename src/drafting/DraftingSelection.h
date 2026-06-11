#pragma once

#include "drafting/DraftingDocument.h"

namespace edi::drafting {

void clearSelection(DraftingDocument &document);
void selectOnly(DraftingDocument &document, DraftingObjectId id);
// existingIds is the document's id set (objectIdSet), passed in because the
// single caller (SelectObjectsCommand) already built it for validation —
// building it twice per select would re-pay the O(M) the set exists to save.
void selectMany(DraftingDocument &document, std::vector<DraftingObjectId> ids,
                const std::unordered_set<DraftingObjectId> &existingIds);
void toggleSelection(DraftingDocument &document, DraftingObjectId id);
bool isSelected(const DraftingDocument &document, const DraftingObjectId &id);
std::vector<DraftingObjectId> selectableObjectsInBounds(const DraftingDocument &document, Bounds2D bounds);
void normalizeSelection(DraftingDocument &document);

} // namespace edi::drafting
