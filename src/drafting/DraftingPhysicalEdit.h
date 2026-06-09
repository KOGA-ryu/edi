#pragma once

#include "drafting/DraftingCommands.h"
#include "drafting/DraftingGrid.h"

#include <optional>
#include <string>

namespace edi::drafting {

struct DraftingPhysicalGeometryEditPlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::optional<DraftingCommand> command;

    static DraftingPhysicalGeometryEditPlan accepted(DraftingCommand command);
    static DraftingPhysicalGeometryEditPlan rejected(DraftingResultCode code, std::string message);
};

DraftingPhysicalGeometryEditPlan planPhysicalGeometryEdit(
    const DraftingObject &object,
    const DraftingGridProjection &grid,
    const std::string &fieldId,
    double value);

} // namespace edi::drafting
