#include "drafting/DraftingPlotJobReport.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
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

} // namespace

int main()
{
    DraftingPlotSettings settings = defaultDraftingPlotSettings();
    settings.calibrationScale = 2.0;

    DraftingDocument readyDocument = makeDraftingDocument("ready_report");
    EDI_CHECK(addObject(readyDocument, makeObject("line_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}})).ok);
    EDI_CHECK(addObject(readyDocument, makeObject("line_b", DraftingShapeKind::Line, LineGeometry{{0.3, 0.1}, {0.4, 0.1}})).ok);
    const std::string readyReport = formatDraftingPlotJobReport(buildDraftingPlotJob(readyDocument, wideOpenGrid(), settings));
    const std::string expectedReady =
        "edi_plot_job_report\n"
        "status: ready\n"
        "calibration_scale: 2.000000\n"
        "order_mode: layer_order\n"
        "direction_mode: preserve_direction\n"
        "plot_bounds: x=0.200000 y=0.200000 w=0.600000 h=0.000000\n"
        "stroke_segments: 2\n"
        "travel_segments: 1\n"
        "travel_distance: 0.200000\n"
        "warnings: 0\n"
        "blocked_reasons: 0\n"
        "layers: 1\n"
        "- layer id=default ready=true objects=2 segments=2 stroke=0.400000 travel=0.200000 reason=ready\n"
        "pens: 1\n"
        "- pen id=pen_black ready=true objects=2 segments=2 stroke=0.400000 travel=0.200000 reason=ready\n";
    EDI_CHECK(readyReport == expectedReady);

    DraftingDocument blockedDocument = makeDraftingDocument("blocked_report");
    EDI_CHECK(addObject(blockedDocument, makeObject("scaled_out", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.5, 0.2}})).ok);
    const std::string blockedReport = formatDraftingPlotJobReport(buildDraftingPlotJob(blockedDocument, plotGrid(), settings));
    const std::string expectedBlocked =
        "edi_plot_job_report\n"
        "status: blocked\n"
        "calibration_scale: 2.000000\n"
        "order_mode: layer_order\n"
        "direction_mode: preserve_direction\n"
        "plot_bounds: x=0.400000 y=0.400000 w=0.600000 h=0.000000\n"
        "stroke_segments: 1\n"
        "travel_segments: 0\n"
        "travel_distance: 0.000000\n"
        "warnings: 1\n"
        "- warning object=scaled_out kind=calibrated_plot_out_of_drawable_bounds message=\"calibrated plot output is outside drawable bounds\"\n"
        "blocked_reasons: 1\n"
        "- reason=calibrated_plot_out_of_drawable_bounds\n"
        "layers: 1\n"
        "- layer id=default ready=false objects=1 segments=1 stroke=0.600000 travel=0.000000 reason=calibrated_plot_out_of_drawable_bounds\n"
        "pens: 1\n"
        "- pen id=pen_black ready=true objects=1 segments=1 stroke=0.600000 travel=0.000000 reason=ready\n";
    EDI_CHECK(blockedReport == expectedBlocked);

    return 0;
}
