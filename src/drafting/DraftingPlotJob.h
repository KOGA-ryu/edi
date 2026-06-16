#pragma once

#include "drafting/DraftingPlotPlan.h"

#include <string>
#include <vector>

namespace edi::drafting {

struct DraftingPlotJob {
    std::vector<DraftingPlotSegment> strokeSegments;
    // Per-object solid fills (see DraftingPlotFill). The SVG writer paints
    // them; HPGL/G-code ignore the field — a pen plotter has no fill.
    std::vector<DraftingPlotFill> fills;
    std::vector<DraftingPlotTravelSegment> travelSegments;
    std::vector<DraftingPlotLayerStats> layerStats;
    std::vector<DraftingPlotPenStats> penStats;
    std::vector<DraftingPlotWarning> warnings;
    Bounds2D plotBounds;
    bool hasPlotBounds = false;
    double calibrationScale = 1.0;
    DraftingPlotOrderMode orderMode = DraftingPlotOrderMode::LayerOrder;
    DraftingPlotDirectionMode directionMode = DraftingPlotDirectionMode::PreserveDirection;
    double travelDistance = 0.0;
    bool ready = false;
    std::vector<std::string> blockedReasons;
};

DraftingPlotJob buildDraftingPlotJob(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings = defaultDraftingPlotSettings());

DraftingPlotJob buildDraftingPlotJob(const DraftingPlotPlan &plan);

} // namespace edi::drafting
