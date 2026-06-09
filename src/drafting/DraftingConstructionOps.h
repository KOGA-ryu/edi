#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

struct DraftingConstructionLinePlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    ConstructionLineGeometry geometry;

    static DraftingConstructionLinePlan accepted(ConstructionLineGeometry geometry);
    static DraftingConstructionLinePlan rejected(DraftingResultCode code, std::string message);
};

bool isHorizontalConstructionLine(const ConstructionLineGeometry &line);
bool isVerticalConstructionLine(const ConstructionLineGeometry &line);
DraftingConstructionLinePlan fitConstructionLineToDrawable(const ConstructionLineGeometry &line, Bounds2D drawable);

} // namespace edi::drafting
