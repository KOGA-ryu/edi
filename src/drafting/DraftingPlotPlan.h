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

struct DraftingPlotSegment {
    DraftingObjectId objectId;
    LayerId layerId;
    Point2D a;
    Point2D b;
    std::string penId;
    std::string strokeColor;
    double strokeWidth = 0.0;
};

struct DraftingPlotTravelSegment {
    DraftingObjectId fromObjectId;
    DraftingObjectId toObjectId;
    Point2D a;
    Point2D b;
    double distance = 0.0;
};

struct DraftingPlotWarning {
    DraftingObjectId objectId;
    std::string kind;
    std::string message;
};

struct DraftingPlotPlan {
    std::vector<DraftingPlotObject> objects;
    std::vector<DraftingPlotSegment> segments;
    std::vector<DraftingPlotTravelSegment> travelSegments;
    std::vector<DraftingPlotWarning> warnings;
    double travelDistance = 0.0;
};

bool draftingShapeCanPlot(DraftingShapeKind kind);
DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid);

} // namespace edi::drafting
