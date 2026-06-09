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

DraftingGridProjection sampleGrid()
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.marginLeft = 0.0;
    settings.marginTop = 0.0;
    settings.marginRight = 0.0;
    settings.marginBottom = 0.0;
    return projectDraftingGrid(settings);
}

DraftingPlotJob buildSamplePlotJob()
{
    DraftingDocument document = makeDraftingDocument("sample_plot_job");
    const DraftingObject lineA = makeObject("line_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}});
    const DraftingObject lineB = makeObject("line_b", DraftingShapeKind::Line, LineGeometry{{0.3, 0.1}, {0.4, 0.1}});
    addObject(document, lineA);
    addObject(document, lineB);

    DraftingPlotSettings settings = defaultDraftingPlotSettings();
    settings.calibrationScale = 2.0;
    return buildDraftingPlotJob(document, sampleGrid(), settings);
}

} // namespace

int main()
{
    std::cout << formatDraftingPlotJobReport(buildSamplePlotJob());
    return 0;
}
