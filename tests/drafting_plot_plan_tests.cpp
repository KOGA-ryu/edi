#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingStore.h"

#include <cassert>

using namespace edi::drafting;

namespace {

DraftingObject makeObject(std::string id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
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
    DraftingDocument document = makeDraftingDocument("plot_doc");
    assert(addLayer(document, makeDraftingLayer("ink", "Ink", 1), true).ok);
    assert(addLayer(document, makeDraftingLayer("disabled", "Disabled", 2)).ok);

    LayerPlotStyle inkPlot;
    inkPlot.penId = "pen_blue";
    inkPlot.strokeColor = "#75c7ff";
    inkPlot.strokeWidth = 1.0;
    assert(updateLayerPlotStyle(document, "ink", inkPlot).ok);

    LayerPlotStyle disabledPlot;
    disabledPlot.plotEnabled = false;
    assert(updateLayerPlotStyle(document, "disabled", disabledPlot).ok);

    DraftingObject defaultLine = makeObject("default_line", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.3, 0.3}});
    assert(addObject(document, defaultLine).ok);

    DraftingObject inkLine = makeObject("ink_line", DraftingShapeKind::Line, LineGeometry{{0.4, 0.4}, {0.5, 0.5}});
    inkLine.layerId = "ink";
    assert(addObject(document, inkLine).ok);

    DraftingObject disabledLine = makeObject("disabled_line", DraftingShapeKind::Line, LineGeometry{{0.6, 0.6}, {0.7, 0.7}});
    disabledLine.layerId = "disabled";
    assert(addObject(document, disabledLine).ok);

    DraftingObject hiddenLine = makeObject("hidden_line", DraftingShapeKind::Line, LineGeometry{{0.7, 0.7}, {0.8, 0.8}});
    hiddenLine.visible = false;
    assert(addObject(document, hiddenLine).ok);

    DraftingObject guide = makeObject("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25});
    guide.layerId = "ink";
    assert(addObject(document, guide).ok);

    DraftingObject outsideLine = makeObject("outside_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.05, 0.05}});
    outsideLine.layerId = "ink";
    assert(addObject(document, outsideLine).ok);

    const DraftingPlotPlan plan = buildDraftingPlotPlan(document, plotGrid());
    assert(plan.objects.size() == 3);
    assert(plan.objects[0].objectId == "default_line");
    assert(plan.objects[0].penId == "pen_black");
    assert(plan.objects[1].objectId == "ink_line");
    assert(plan.objects[1].penId == "pen_blue");
    assert(plan.objects[1].strokeColor == "#75c7ff");
    assert(plan.objects[1].strokeWidth == 1.0);
    assert(plan.objects[2].objectId == "outside_line");
    assert(plan.warnings.size() == 1);
    assert(plan.warnings.front().objectId == "outside_line");
    assert(plan.warnings.front().kind == "out_of_drawable_bounds");

    assert(updateLayerFlags(document, "ink", false, false).ok);
    const DraftingPlotPlan hiddenInkPlan = buildDraftingPlotPlan(document, plotGrid());
    assert(hiddenInkPlan.objects.size() == 1);
    assert(hiddenInkPlan.objects.front().objectId == "default_line");
    assert(hiddenInkPlan.warnings.empty());

    return 0;
}
