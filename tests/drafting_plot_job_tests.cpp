#include "drafting/DraftingPlotJob.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

DraftingObject makeObject(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

DraftingGridProjection wideOpenGrid()
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.marginLeft = 0.0;
    settings.marginTop = 0.0;
    settings.marginRight = 0.0;
    settings.marginBottom = 0.0;
    return projectDraftingGrid(settings);
}

DraftingGridProjection plotGrid()
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.marginLeft = 0.1 * settings.width;
    settings.marginTop = 0.1 * settings.height;
    settings.marginRight = 0.1 * settings.width;
    settings.marginBottom = 0.1 * settings.height;
    return projectDraftingGrid(settings);
}

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    DraftingDocument readyDocument = makeDraftingDocument("ready_plot_job");
    EDI_CHECK(addObject(readyDocument, makeObject("line_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}})).ok);
    EDI_CHECK(addObject(readyDocument, makeObject("line_b", DraftingShapeKind::Line, LineGeometry{{0.3, 0.1}, {0.4, 0.1}})).ok);
    DraftingPlotSettings settings = defaultDraftingPlotSettings();
    settings.calibrationScale = 2.0;
    const DraftingPlotJob readyJob = buildDraftingPlotJob(readyDocument, wideOpenGrid(), settings);
    EDI_CHECK(readyJob.ready);
    EDI_CHECK(readyJob.blockedReasons.empty());
    EDI_CHECK(nearlyEqual(readyJob.calibrationScale, 2.0));
    EDI_CHECK(readyJob.strokeSegments.size() == 2);
    EDI_CHECK(readyJob.travelSegments.size() == 1);
    EDI_CHECK(readyJob.layerStats.size() == 1);
    EDI_CHECK(readyJob.penStats.size() == 1);
    EDI_CHECK(readyJob.warnings.empty());
    EDI_CHECK(readyJob.hasPlotBounds);
    EDI_CHECK(nearlyEqual(readyJob.plotBounds.x, 0.2));
    EDI_CHECK(nearlyEqual(readyJob.plotBounds.width, 0.6));
    EDI_CHECK(nearlyEqual(readyJob.strokeSegments.front().rawA.x, 0.1));
    EDI_CHECK(nearlyEqual(readyJob.strokeSegments.front().rawB.x, 0.2));
    EDI_CHECK(nearlyEqual(readyJob.strokeSegments.front().a.x, 0.2));
    EDI_CHECK(nearlyEqual(readyJob.strokeSegments.front().b.x, 0.4));
    EDI_CHECK(nearlyEqual(readyJob.travelSegments.front().rawA.x, 0.2));
    EDI_CHECK(nearlyEqual(readyJob.travelSegments.front().rawB.x, 0.3));
    EDI_CHECK(nearlyEqual(readyJob.travelSegments.front().a.x, 0.4));
    EDI_CHECK(nearlyEqual(readyJob.travelSegments.front().b.x, 0.6));

    DraftingDocument blockedDocument = makeDraftingDocument("blocked_plot_job");
    EDI_CHECK(addObject(blockedDocument, makeObject("scaled_out", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.5, 0.2}})).ok);
    const DraftingPlotJob blockedJob = buildDraftingPlotJob(blockedDocument, plotGrid(), settings);
    EDI_CHECK(!blockedJob.ready);
    EDI_CHECK(blockedJob.warnings.size() == 1);
    EDI_CHECK(blockedJob.warnings.front().kind == "calibrated_plot_out_of_drawable_bounds");
    EDI_CHECK(blockedJob.blockedReasons.size() == 1);
    EDI_CHECK(blockedJob.blockedReasons.front() == "calibrated_plot_out_of_drawable_bounds");
    EDI_CHECK(blockedJob.hasPlotBounds);
    EDI_CHECK(nearlyEqual(blockedJob.plotBounds.x, 0.4));
    EDI_CHECK(nearlyEqual(blockedJob.plotBounds.width, 0.6));
    EDI_CHECK(!blockedJob.layerStats.empty());
    EDI_CHECK(!blockedJob.layerStats.front().ready);
    EDI_CHECK(blockedJob.layerStats.front().blockedReason == "calibrated_plot_out_of_drawable_bounds");

    return 0;
}
