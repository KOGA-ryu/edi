#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingAlignmentMode {
    Left,
    Right,
    Top,
    Bottom,
    CenterX,
    CenterY,
    DistributeX,
    DistributeY
};

struct DraftingTranslation {
    DraftingObjectId objectId;
    double dx = 0.0;
    double dy = 0.0;
};

struct DraftingAlignmentResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingTranslation> translations;

    static DraftingAlignmentResult accepted(std::vector<DraftingTranslation> translations);
    static DraftingAlignmentResult rejected(DraftingResultCode code, std::string message);
};

const char *draftingAlignmentModeName(DraftingAlignmentMode mode);
std::optional<DraftingAlignmentMode> draftingAlignmentModeFromId(const std::string &modeId);
std::optional<DraftingAlignmentMode> draftingDistributeModeFromAxisId(const std::string &axisId);

DraftingAlignmentResult planDraftingAlignment(
    const DraftingDocument &document,
    const std::vector<DraftingObjectId> &objectIds,
    DraftingAlignmentMode mode);

} // namespace edi::drafting
