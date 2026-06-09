#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>

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

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
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
    assert(plan.segments.size() == 3);
    assert(plan.segments[0].objectId == "default_line");
    assert(plan.segments[1].objectId == "ink_line");
    assert(plan.segments[1].penId == "pen_blue");
    assert(nearlyEqual(plan.segments[1].a.x, 0.4));
    assert(nearlyEqual(plan.segments[1].b.y, 0.5));
    assert(plan.segments[2].objectId == "outside_line");
    assert(plan.warnings.size() == 1);
    assert(plan.warnings.front().objectId == "outside_line");
    assert(plan.warnings.front().kind == "out_of_drawable_bounds");

    assert(updateLayerFlags(document, "ink", false, false).ok);
    const DraftingPlotPlan hiddenInkPlan = buildDraftingPlotPlan(document, plotGrid());
    assert(hiddenInkPlan.objects.size() == 1);
    assert(hiddenInkPlan.objects.front().objectId == "default_line");
    assert(hiddenInkPlan.warnings.empty());

    DraftingDocument segmentDocument = makeDraftingDocument("segment_doc");
    assert(addObject(segmentDocument, makeObject("point_1", DraftingShapeKind::Point, PointGeometry{{0.2, 0.2}})).ok);
    assert(addObject(segmentDocument, makeObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.2}})).ok);
    assert(addObject(segmentDocument, makeObject("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.3, 0.3}, 0.2, 0.1})).ok);
    assert(addObject(segmentDocument, makeObject("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.6, 0.6}, 0.1})).ok);
    assert(addObject(segmentDocument, makeObject("polygon_1", DraftingShapeKind::Polygon, PolygonGeometry{{{0.1, 0.6}, {0.2, 0.6}, {0.2, 0.7}}})).ok);
    assert(addObject(segmentDocument, makeObject("polyline_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.6, 0.1}, {0.7, 0.1}, {0.7, 0.2}}})).ok);
    const DraftingPlotPlan segmentPlan = buildDraftingPlotPlan(segmentDocument, projectDraftingGrid(defaultDraftingGridSettings()));
    assert(segmentPlan.objects.size() == 6);
    assert(segmentPlan.segments.size() == 44);
    assert(segmentPlan.segments[0].objectId == "point_1");
    assert(segmentPlan.segments[1].objectId == "point_1");
    assert(segmentPlan.segments[2].objectId == "line_1");
    assert(segmentPlan.segments[3].objectId == "rect_1");
    assert(segmentPlan.segments[6].objectId == "rect_1");
    assert(segmentPlan.segments[7].objectId == "circle_1");
    assert(segmentPlan.segments[39].objectId == "polygon_1");
    assert(segmentPlan.segments[42].objectId == "polyline_1");
    for (const DraftingPlotSegment &segment : segmentPlan.segments) {
        assert(std::isfinite(segment.a.x));
        assert(std::isfinite(segment.a.y));
        assert(std::isfinite(segment.b.x));
        assert(std::isfinite(segment.b.y));
    }

    return 0;
}
