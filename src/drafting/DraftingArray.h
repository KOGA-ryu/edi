#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

struct DraftingArrayResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects;

    static DraftingArrayResult accepted(std::vector<DraftingObject> objects);
    static DraftingArrayResult rejected(DraftingResultCode code, std::string message);
};

DraftingArrayResult repeatDraftingObject(
    const DraftingObject &source,
    const std::vector<DraftingObjectId> &newObjectIds,
    double spacingX,
    double spacingY);

} // namespace edi::drafting
