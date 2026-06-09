#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

enum class DraftingOffsetSide {
    Left,
    Right
};

struct DraftingOffsetResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingObject object;

    static DraftingOffsetResult accepted(DraftingObject object);
    static DraftingOffsetResult rejected(DraftingResultCode code, std::string message);
};

DraftingOffsetSide draftingOffsetSideFromId(const std::string &sideId);
DraftingOffsetResult offsetDraftingObject(
    const DraftingObject &source,
    DraftingObjectId newObjectId,
    double distance,
    DraftingOffsetSide side);

} // namespace edi::drafting
