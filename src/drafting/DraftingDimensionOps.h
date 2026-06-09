#pragma once

#include "drafting/DraftingDocument.h"

#include <optional>
#include <string>

namespace edi::drafting {

struct DraftingDimensionPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DimensionGeometry geometry;

    static DraftingDimensionPlan accepted(DimensionGeometry geometry);
    static DraftingDimensionPlan rejected(DraftingResultCode code, std::string message);
};

std::optional<DimensionKind> draftingDimensionKindFromId(const std::string &kindId);
DraftingDimensionPlan planDimensionKindChange(const DimensionGeometry &dimension, DimensionKind kind);

} // namespace edi::drafting
