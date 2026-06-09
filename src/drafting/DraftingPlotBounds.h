#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingPlotPlan.h"

#include <vector>

namespace edi::drafting {

enum class DraftingPlotBoundsStatus {
    Unavailable,
    InsideDrawable,
    OutsideDrawable,
};

struct DraftingPlotBoundsResult {
    bool ok = false;
    Bounds2D bounds;
    DraftingPlotBoundsStatus status = DraftingPlotBoundsStatus::Unavailable;
};

const char *draftingPlotBoundsStatusName(DraftingPlotBoundsStatus status);
bool boundsInsideDrawable(Bounds2D bounds, Bounds2D drawable);
Bounds2D translateBounds(Bounds2D bounds, double dx, double dy);
DraftingPlotBoundsResult rawPlotOutputBounds(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings = defaultDraftingPlotSettings());
DraftingPlotBoundsResult selectedRawPlotOutputBounds(
    const DraftingDocument &document,
    const std::vector<DraftingObjectId> &objectIds,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings = defaultDraftingPlotSettings());

} // namespace edi::drafting
