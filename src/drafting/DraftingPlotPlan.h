#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingPlotOrderMode {
    LayerOrder,
    NearestNext,
};

enum class DraftingPlotDirectionMode {
    PreserveDirection,
    AllowReverseSegments,
};

struct DraftingPlotSettings {
    DraftingPlotOrderMode orderMode = DraftingPlotOrderMode::LayerOrder;
    DraftingPlotDirectionMode directionMode = DraftingPlotDirectionMode::PreserveDirection;
};

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
    LayerId toLayerId;
    std::string toPenId;
    Point2D a;
    Point2D b;
    double distance = 0.0;
};

struct DraftingPlotLayerStats {
    LayerId layerId;
    std::string layerName;
    int objectCount = 0;
    int segmentCount = 0;
    double strokeDistance = 0.0;
    double travelDistance = 0.0;
    bool ready = false;
    std::string blockedReason = "no_plotted_objects";
};

struct DraftingPlotPenStats {
    std::string penId;
    std::string strokeColor;
    double strokeWidth = 0.0;
    int objectCount = 0;
    int segmentCount = 0;
    double strokeDistance = 0.0;
    double travelDistance = 0.0;
    bool ready = false;
    std::string blockedReason = "no_assigned_segments";
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
    std::vector<DraftingPlotLayerStats> layerStats;
    std::vector<DraftingPlotPenStats> penStats;
    std::vector<DraftingPlotWarning> warnings;
    DraftingPlotOrderMode orderMode = DraftingPlotOrderMode::LayerOrder;
    DraftingPlotDirectionMode directionMode = DraftingPlotDirectionMode::PreserveDirection;
    double travelDistance = 0.0;
};

DraftingPlotSettings defaultDraftingPlotSettings();
const char *draftingPlotOrderModeName(DraftingPlotOrderMode mode);
DraftingPlotOrderMode draftingPlotOrderModeFromName(const std::string &name);
const char *draftingPlotDirectionModeName(DraftingPlotDirectionMode mode);
DraftingPlotDirectionMode draftingPlotDirectionModeFromName(const std::string &name);
bool draftingShapeCanPlot(DraftingShapeKind kind);
DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid);
DraftingPlotPlan buildDraftingPlotPlan(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings);

} // namespace edi::drafting
