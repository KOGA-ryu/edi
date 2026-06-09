#pragma once

#include "drafting/DraftingDocument.h"

namespace edi::drafting {

void clearSelection(DraftingDocument &document);
void selectOnly(DraftingDocument &document, DraftingObjectId id);
void selectMany(DraftingDocument &document, std::vector<DraftingObjectId> ids);
void toggleSelection(DraftingDocument &document, DraftingObjectId id);
bool isSelected(const DraftingDocument &document, const DraftingObjectId &id);
std::vector<DraftingObjectId> selectableObjectsInBounds(const DraftingDocument &document, Bounds2D bounds);
void normalizeSelection(DraftingDocument &document);

} // namespace edi::drafting
