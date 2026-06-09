#include "drafting/DraftingPlotJobReport.h"
#include "drafting/DraftingStore.h"

#include <iostream>
#include <string>
#include <utility>

namespace {

using namespace edi::drafting;

DraftingObject makeObject(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    if (!built.ok) {
        return {};
    }
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

DraftingPlotSettings calibratedSettings()
{
    DraftingPlotSettings settings = defaultDraftingPlotSettings();
    settings.calibrationScale = 2.0;
    return settings;
}

DraftingPlotJob buildReadySample()
{
    DraftingDocument document = makeDraftingDocument("sample_plot_job");
    const DraftingObject lineA = makeObject("line_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}});
    const DraftingObject lineB = makeObject("line_b", DraftingShapeKind::Line, LineGeometry{{0.3, 0.1}, {0.4, 0.1}});
    addObject(document, lineA);
    addObject(document, lineB);
    return buildDraftingPlotJob(document, wideOpenGrid(), calibratedSettings());
}

DraftingPlotJob buildCalibratedBlockedSample()
{
    DraftingDocument document = makeDraftingDocument("sample_calibrated_blocked_plot_job");
    const DraftingObject line = makeObject("scaled_out", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.5, 0.2}});
    addObject(document, line);
    return buildDraftingPlotJob(document, plotGrid(), calibratedSettings());
}

DraftingPlotJob buildRawBlockedSample()
{
    DraftingDocument document = makeDraftingDocument("sample_raw_blocked_plot_job");
    const DraftingObject line = makeObject("raw_out", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.05, 0.05}});
    addObject(document, line);
    return buildDraftingPlotJob(document, plotGrid(), defaultDraftingPlotSettings());
}

DraftingPlotJob buildSamplePlotJob(const std::string &sampleId)
{
    if (sampleId == "calibrated-blocked") {
        return buildCalibratedBlockedSample();
    }
    if (sampleId == "raw-blocked") {
        return buildRawBlockedSample();
    }
    return buildReadySample();
}

void printUsage()
{
    std::cerr << "usage: edi_plot_job_report [--sample ready|calibrated-blocked|raw-blocked]\n";
}

} // namespace

int main(int argc, char **argv)
{
    std::string sampleId = "ready";
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        if (arg == "--sample" && index + 1 < argc) {
            sampleId = argv[++index];
            continue;
        }
        printUsage();
        return 2;
    }

    if (sampleId != "ready" && sampleId != "calibrated-blocked" && sampleId != "raw-blocked") {
        printUsage();
        return 2;
    }

    std::cout << formatDraftingPlotJobReport(buildSamplePlotJob(sampleId));
    return 0;
}
