#include "drafting/DraftingPlotJob.h"

#include <algorithm>
#include <utility>

namespace edi::drafting {
namespace {

void appendUniqueReason(std::vector<std::string> &reasons, std::string reason)
{
    if (reason.empty()) {
        return;
    }
    if (std::find(reasons.begin(), reasons.end(), reason) != reasons.end()) {
        return;
    }
    reasons.push_back(std::move(reason));
}

std::vector<std::string> blockedReasonsForPlan(const DraftingPlotPlan &plan)
{
    std::vector<std::string> reasons;
    for (const DraftingPlotWarning &warning : plan.warnings) {
        appendUniqueReason(reasons, warning.kind);
    }
    for (const DraftingPlotLayerStats &stats : plan.layerStats) {
        if (!stats.ready) {
            appendUniqueReason(reasons, stats.blockedReason);
        }
    }
    for (const DraftingPlotPenStats &stats : plan.penStats) {
        if (!stats.ready) {
            appendUniqueReason(reasons, stats.blockedReason);
        }
    }
    return reasons;
}

bool planIsReady(const DraftingPlotPlan &plan, const std::vector<std::string> &blockedReasons)
{
    if (!plan.warnings.empty() || !blockedReasons.empty() || plan.segments.empty()) {
        return false;
    }
    return std::all_of(plan.layerStats.begin(), plan.layerStats.end(), [](const DraftingPlotLayerStats &stats) {
        return stats.ready;
    }) && std::all_of(plan.penStats.begin(), plan.penStats.end(), [](const DraftingPlotPenStats &stats) {
        return stats.ready;
    });
}

} // namespace

DraftingPlotJob buildDraftingPlotJob(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings)
{
    return buildDraftingPlotJob(buildDraftingPlotPlan(document, grid, settings));
}

DraftingPlotJob buildDraftingPlotJob(const DraftingPlotPlan &plan)
{
    DraftingPlotJob job;
    job.strokeSegments = plan.segments;
    job.travelSegments = plan.travelSegments;
    job.layerStats = plan.layerStats;
    job.penStats = plan.penStats;
    job.warnings = plan.warnings;
    job.plotBounds = plan.plotBounds;
    job.hasPlotBounds = plan.hasPlotBounds;
    job.calibrationScale = plan.calibrationScale;
    job.orderMode = plan.orderMode;
    job.directionMode = plan.directionMode;
    job.travelDistance = plan.travelDistance;
    job.blockedReasons = blockedReasonsForPlan(plan);
    job.ready = planIsReady(plan, job.blockedReasons);
    return job;
}

} // namespace edi::drafting
