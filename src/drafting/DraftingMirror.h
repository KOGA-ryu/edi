#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

enum class DraftingMirrorAxis {
    Horizontal,
    Vertical
};

struct DraftingMirrorResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingObject object;

    static DraftingMirrorResult accepted(DraftingObject object);
    static DraftingMirrorResult rejected(DraftingResultCode code, std::string message);
};

DraftingMirrorAxis draftingMirrorAxisFromId(const std::string &axisId);
DraftingMirrorResult mirrorDraftingObject(
    const DraftingObject &source,
    DraftingObjectId newObjectId,
    DraftingMirrorAxis axis);

} // namespace edi::drafting
