#pragma once

#include "drafting/DraftingDocument.h"

#include <string>

namespace edi::drafting {

struct DraftingNumericEditResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    DraftingGeometry geometry = PointGeometry{};

    static DraftingNumericEditResult accepted(DraftingGeometry geometry);
    static DraftingNumericEditResult rejected(DraftingResultCode code, std::string message);
};

DraftingNumericEditResult applyNumericGeometryEdit(
    const DraftingObject &object,
    const std::string &fieldId,
    double value);

double lineAngleDegrees(const LineGeometry &line);
double dimensionAngleDegrees(const DimensionGeometry &dimension);

} // namespace edi::drafting
