#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <string>

namespace edi::recipe {

// The closed measurement vocabulary (width / height / length / radius),
// extracted here as the ONE home both pipelines share. Pipeline A
// (RecipeDocument's resolveMeasurement) is now a thin adapter over this; the
// op pipeline (resolveRecipeOps, R1-B03) calls it directly. Two copies of a
// closed vocabulary drift apart silently — the exact disease R1 exists to
// cure — so the field logic lives once.
//
// Physical scaling follows the binding contract
// (docs/recipe_binding_contract.md §3): width/radius scale along the grid's X
// axis, height along Y, length is a line's physical endpoint distance.
struct MeasureFieldResult {
    bool ok = false;
    double value = 0.0;
    std::string message; // B01 wordings, verbatim; empty on ok
};

// One drafted object's one measurement field -> one physical number. Field
// validity depends on the object's kind; an unanswerable field (length on a
// circle, radius on a line, an unknown name) fails with a message the UI can
// show next to the binding, as does a missing object.
MeasureFieldResult resolveMeasurementField(
    const edi::drafting::DraftingDocument &drafting,
    const edi::drafting::DraftingGridProjection &grid,
    const std::string &objectId,
    const std::string &field);

} // namespace edi::recipe
