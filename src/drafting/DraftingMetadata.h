#pragma once

#include "drafting/DraftingTypes.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace edi::drafting {

constexpr std::size_t kMetadataShortTextLimit = 128;
constexpr std::size_t kMetadataMeasurementNoteLimit = 512;

struct DraftingMetadataValidationResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingMetadataValidationResult accepted();
    static DraftingMetadataValidationResult rejected(DraftingResultCode code, std::string message);
};

bool isValidMetadataText(std::string_view value, std::size_t limit);
bool isValidMetadataTimestamp(std::string_view value);
bool isValidMeasurementMetadata(const MeasurementMetadata &measurement);
bool isValidGuideVisualColor(std::string_view value);
bool isValidGuideVisualDashStyle(std::string_view value);
DraftingMetadataValidationResult validateObjectMetadata(const ObjectMetadata &metadata);

} // namespace edi::drafting
