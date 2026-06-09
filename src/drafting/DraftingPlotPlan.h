#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <vector>

namespace edi::drafting {

struct DraftingPlotObject {
    DraftingObjectId objectId;
    LayerId layerId;
    std::string penId;
    std::string strokeColor;
    double strokeWidth = 0.0;
};

struct DraftingPlotWarning {
    DraftingObjectId objectId;
    std::string kind;
    std::string message;
};

struct DraftingPlotPlan {
    std::vector<DraftingPlotObject> objects;
    std::vector<DraftingPlotWarning> warnings;
};

bool draftingShapeCanPlot(DraftingShapeKind kind);
DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid);

} // namespace edi::drafting
