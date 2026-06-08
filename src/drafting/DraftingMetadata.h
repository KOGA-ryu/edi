#pragma once

#include "drafting/DraftingTypes.h"

#include <string>

namespace edi::drafting {

struct DraftingMetadataValidationResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingMetadataValidationResult accepted();
    static DraftingMetadataValidationResult rejected(DraftingResultCode code, std::string message);
};

DraftingMetadataValidationResult validateObjectMetadata(const ObjectMetadata &metadata);

} // namespace edi::drafting
