#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

struct DraftingStoreResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingStoreResult accepted();
    static DraftingStoreResult rejected(DraftingResultCode code, std::string message);
};

DraftingStoreResult addObject(DraftingDocument &document, DraftingObject object);
DraftingStoreResult removeObject(DraftingDocument &document, const DraftingObjectId &id);
DraftingStoreResult updateObjectGeometry(DraftingDocument &document, const DraftingObjectId &id, DraftingGeometry geometry);
DraftingStoreResult updateObjectMetadata(DraftingDocument &document, const DraftingObjectId &id, ObjectMetadata metadata);
DraftingStoreResult updateObjectFlags(DraftingDocument &document, const DraftingObjectId &id, bool locked, bool visible);
DraftingStoreResult moveObject(DraftingDocument &document, const DraftingObjectId &id, double dx, double dy);

} // namespace edi::drafting
