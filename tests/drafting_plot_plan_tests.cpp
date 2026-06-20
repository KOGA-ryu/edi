#include "drafting/DraftingPlotPlan.h"
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
    EDI_CHECK(addLayer(document, makeDraftingLayer("ink", "Ink", 1), true).ok);
    EDI_CHECK(addLayer(document, makeDraftingLayer("disabled", "Disabled", 2)).ok);

    LayerPlotStyle inkPlot;
    inkPlot.penId = "pen_blue";
    inkPlot.strokeColor = "#75c7ff";
    inkPlot.strokeWidth = 1.0;
    EDI_CHECK(updateLayerPlotStyle(document, "ink", inkPlot).ok);

    LayerPlotStyle disabledPlot;
    disabledPlot.plotEnabled = false;
    EDI_CHECK(updateLayerPlotStyle(document, "disabled", disabledPlot).ok);

    DraftingObject defaultLine = makeObject("default_line", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.3, 0.3}});
    EDI_CHECK(addObject(document, defaultLine).ok);

    DraftingObject inkLine = makeObject("ink_line", DraftingShapeKind::Line, LineGeometry{{0.4, 0.4}, {0.5, 0.5}});
    inkLine.layerId = "ink";
    inkLine.stroke.opacity = 0.6; // per-object opacity must ride its segments
    EDI_CHECK(addObject(document, inkLine).ok);

    DraftingObject disabledLine = makeObject("disabled_line", DraftingShapeKind::Line, LineGeometry{{0.6, 0.6}, {0.7, 0.7}});
    disabledLine.layerId = "disabled";
    EDI_CHECK(addObject(document, disabledLine).ok);

    DraftingObject hiddenLine = makeObject("hidden_line", DraftingShapeKind::Line, LineGeometry{{0.7, 0.7}, {0.8, 0.8}});
    hiddenLine.visible = false;
    EDI_CHECK(addObject(document, hiddenLine).ok);

    DraftingObject guide = makeObject("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Horizontal, 0.25});
    guide.layerId = "ink";
    EDI_CHECK(addObject(document, guide).ok);

    DraftingObject outsideLine = makeObject("outside_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.05, 0.05}});
    outsideLine.layerId = "ink";
    EDI_CHECK(addObject(document, outsideLine).ok);

    const DraftingPlotPlan plan = buildDraftingPlotPlan(document, plotGrid());
    EDI_CHECK(plan.orderMode == DraftingPlotOrderMode::LayerOrder);
    EDI_CHECK(plan.directionMode == DraftingPlotDirectionMode::PreserveDirection);
    EDI_CHECK(plan.objects.size() == 3);
    EDI_CHECK(plan.objects[0].objectId == "default_line");
    EDI_CHECK(plan.objects[0].penId == "pen_black");
    EDI_CHECK(plan.objects[1].objectId == "ink_line");
    EDI_CHECK(plan.objects[1].penId == "pen_blue");
    EDI_CHECK(plan.objects[1].strokeColor == "#75c7ff");
    EDI_CHECK(plan.objects[1].strokeWidth == 1.0);
    EDI_CHECK(plan.objects[2].objectId == "outside_line");
    EDI_CHECK(plan.segments.size() == 3);
    EDI_CHECK(plan.segments[0].objectId == "default_line");
    EDI_CHECK(plan.segments[1].objectId == "ink_line");
    EDI_CHECK(plan.segments[1].penId == "pen_blue");
    EDI_CHECK(plan.segments[0].opacity == 1.0); // default stays fully opaque
    EDI_CHECK(plan.segments[1].opacity == 0.6); // the object's alpha, not the layer's
    EDI_CHECK(nearlyEqual(plan.segments[1].a.x, 0.4));
    EDI_CHECK(nearlyEqual(plan.segments[1].b.y, 0.5));
    EDI_CHECK(plan.segments[2].objectId == "outside_line");
    EDI_CHECK(plan.travelSegments.size() == 2);
    EDI_CHECK(plan.travelSegments[0].fromObjectId == "default_line");
    EDI_CHECK(plan.travelSegments[0].toObjectId == "ink_line");
    EDI_CHECK(nearlyEqual(plan.travelSegments[0].a.x, 0.3));
    EDI_CHECK(nearlyEqual(plan.travelSegments[0].b.x, 0.4));
    EDI_CHECK(plan.travelSegments[1].fromObjectId == "ink_line");
    EDI_CHECK(plan.travelSegments[1].toObjectId == "outside_line");
    EDI_CHECK(nearlyEqual(plan.travelDistance, std::sqrt(0.02) + std::sqrt(0.5)));
    EDI_CHECK(plan.layerStats.size() == 3);
    EDI_CHECK(plan.layerStats[0].layerId == "default");
    EDI_CHECK(plan.layerStats[0].objectCount == 1);
    EDI_CHECK(plan.layerStats[0].segmentCount == 1);
    EDI_CHECK(nearlyEqual(plan.layerStats[0].strokeDistance, std::sqrt(0.02)));
    EDI_CHECK(nearlyEqual(plan.layerStats[0].travelDistance, 0.0));
    EDI_CHECK(plan.layerStats[0].ready);
    EDI_CHECK(plan.layerStats[0].blockedReason == "ready");
    EDI_CHECK(plan.layerStats[1].layerId == "ink");
    EDI_CHECK(plan.layerStats[1].objectCount == 2);
    EDI_CHECK(plan.layerStats[1].segmentCount == 2);
    EDI_CHECK(nearlyEqual(plan.layerStats[1].strokeDistance, std::sqrt(0.02) + std::sqrt(0.005)));
    EDI_CHECK(nearlyEqual(plan.layerStats[1].travelDistance, plan.travelDistance));
    EDI_CHECK(!plan.layerStats[1].ready);
    EDI_CHECK(plan.layerStats[1].blockedReason == "raw_out_of_drawable_bounds");
    EDI_CHECK(plan.layerStats[2].layerId == "disabled");
    EDI_CHECK(plan.layerStats[2].objectCount == 1);
    EDI_CHECK(plan.layerStats[2].segmentCount == 0);
    EDI_CHECK(!plan.layerStats[2].ready);
    EDI_CHECK(plan.layerStats[2].blockedReason == "plot_disabled");
    EDI_CHECK(plan.penStats.size() == 2);
    EDI_CHECK(plan.penStats[0].penId == "pen_black");
    EDI_CHECK(plan.penStats[0].objectCount == 2);
    EDI_CHECK(plan.penStats[0].segmentCount == 1);
    EDI_CHECK(nearlyEqual(plan.penStats[0].strokeDistance, std::sqrt(0.02)));
    EDI_CHECK(nearlyEqual(plan.penStats[0].travelDistance, 0.0));
    EDI_CHECK(plan.penStats[0].ready);
    EDI_CHECK(plan.penStats[0].blockedReason == "ready");
    EDI_CHECK(plan.penStats[1].penId == "pen_blue");
    EDI_CHECK(plan.penStats[1].objectCount == 2);
    EDI_CHECK(plan.penStats[1].segmentCount == 2);
    EDI_CHECK(nearlyEqual(plan.penStats[1].strokeDistance, std::sqrt(0.02) + std::sqrt(0.005)));
    EDI_CHECK(nearlyEqual(plan.penStats[1].travelDistance, plan.travelDistance));
    EDI_CHECK(plan.penStats[1].ready);
    EDI_CHECK(plan.penStats[1].blockedReason == "ready");
    EDI_CHECK(plan.warnings.size() == 1);
    EDI_CHECK(plan.warnings.front().objectId == "outside_line");
    EDI_CHECK(plan.warnings.front().kind == "raw_out_of_drawable_bounds");

    EDI_CHECK(updateLayerFlags(document, "ink", false, false).ok);
    const DraftingPlotPlan hiddenInkPlan = buildDraftingPlotPlan(document, plotGrid());
    EDI_CHECK(hiddenInkPlan.objects.size() == 1);
    EDI_CHECK(hiddenInkPlan.objects.front().objectId == "default_line");
    EDI_CHECK(hiddenInkPlan.travelSegments.empty());
    EDI_CHECK(nearlyEqual(hiddenInkPlan.travelDistance, 0.0));
    EDI_CHECK(hiddenInkPlan.layerStats.size() == 3);
    EDI_CHECK(hiddenInkPlan.layerStats[0].layerId == "default");
    EDI_CHECK(hiddenInkPlan.layerStats[0].ready);
    EDI_CHECK(hiddenInkPlan.layerStats[1].layerId == "ink");
    EDI_CHECK(!hiddenInkPlan.layerStats[1].ready);
    EDI_CHECK(hiddenInkPlan.layerStats[1].blockedReason == "hidden");
    EDI_CHECK(hiddenInkPlan.layerStats[2].layerId == "disabled");
    EDI_CHECK(!hiddenInkPlan.layerStats[2].ready);
    EDI_CHECK(hiddenInkPlan.layerStats[2].blockedReason == "plot_disabled");
    EDI_CHECK(hiddenInkPlan.penStats.size() == 2);
    EDI_CHECK(hiddenInkPlan.penStats[0].penId == "pen_black");
    EDI_CHECK(hiddenInkPlan.penStats[0].objectCount == 2);
    EDI_CHECK(hiddenInkPlan.penStats[0].ready);
    EDI_CHECK(hiddenInkPlan.penStats[1].penId == "pen_blue");
    EDI_CHECK(!hiddenInkPlan.penStats[1].ready);
    EDI_CHECK(hiddenInkPlan.penStats[1].blockedReason == "no_assigned_segments");
    EDI_CHECK(hiddenInkPlan.warnings.empty());

    DraftingDocument segmentDocument = makeDraftingDocument("segment_doc");
    EDI_CHECK(addObject(segmentDocument, makeObject("point_1", DraftingShapeKind::Point, PointGeometry{{0.2, 0.2}})).ok);
    EDI_CHECK(addObject(segmentDocument, makeObject("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.2}})).ok);
    EDI_CHECK(addObject(segmentDocument, makeObject("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.3, 0.3}, 0.2, 0.1})).ok);
    EDI_CHECK(addObject(segmentDocument, makeObject("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.6, 0.6}, 0.1})).ok);
    EDI_CHECK(addObject(segmentDocument, makeObject("polygon_1", DraftingShapeKind::Polygon, PolygonGeometry{{{0.1, 0.6}, {0.2, 0.6}, {0.2, 0.7}}})).ok);
    EDI_CHECK(addObject(segmentDocument, makeObject("polyline_1", DraftingShapeKind::Polyline, PolylineGeometry{{{0.6, 0.1}, {0.7, 0.1}, {0.7, 0.2}}})).ok);
    const DraftingPlotPlan segmentPlan = buildDraftingPlotPlan(segmentDocument, projectDraftingGrid(defaultDraftingGridSettings()));
    EDI_CHECK(segmentPlan.objects.size() == 6);
    EDI_CHECK(segmentPlan.segments.size() == 44);
    EDI_CHECK(segmentPlan.segments[0].objectId == "point_1");
    EDI_CHECK(segmentPlan.segments[1].objectId == "point_1");
    EDI_CHECK(segmentPlan.segments[2].objectId == "line_1");
    EDI_CHECK(segmentPlan.segments[3].objectId == "rect_1");
    EDI_CHECK(segmentPlan.segments[6].objectId == "rect_1");
    EDI_CHECK(segmentPlan.segments[7].objectId == "circle_1");
    EDI_CHECK(segmentPlan.segments[39].objectId == "polygon_1");
    EDI_CHECK(segmentPlan.segments[42].objectId == "polyline_1");
    for (const DraftingPlotSegment &segment : segmentPlan.segments) {
        EDI_CHECK(std::isfinite(segment.a.x));
        EDI_CHECK(std::isfinite(segment.a.y));
        EDI_CHECK(std::isfinite(segment.b.x));
        EDI_CHECK(std::isfinite(segment.b.y));
    }
    for (const DraftingPlotTravelSegment &segment : segmentPlan.travelSegments) {
        EDI_CHECK(std::isfinite(segment.a.x));
        EDI_CHECK(std::isfinite(segment.a.y));
        EDI_CHECK(std::isfinite(segment.b.x));
        EDI_CHECK(std::isfinite(segment.b.y));
        EDI_CHECK(std::isfinite(segment.distance));
        EDI_CHECK(segment.distance > 0.0);
    }
    EDI_CHECK(std::isfinite(segmentPlan.travelDistance));
    EDI_CHECK(segmentPlan.travelDistance > 0.0);
    // Default objects carry no fill (FillStyle::opacity defaults to 0), so the
    // fill channel stays empty — the byte-identical default the boundary requires.
    EDI_CHECK(segmentPlan.fills.empty());

    // Fill plumbing: only closed fillable kinds with opacity>0 and a valid
    // colour collect a fill ring; line/polyline never do, and fill rides a
    // SEPARATE channel from the stroke segments.
    {
        DraftingDocument fillDocument = makeDraftingDocument("fill_doc");
        DraftingObject filledRect = makeObject("frect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.3, 0.3}, 0.2, 0.1});
        filledRect.fill.opacity = 0.5;
        filledRect.fill.color = "#ff0000";
        EDI_CHECK(addObject(fillDocument, filledRect).ok);
        DraftingObject filledCircle = makeObject("fcircle", DraftingShapeKind::Circle, CircleGeometry{{0.6, 0.6}, 0.1});
        filledCircle.fill.opacity = 1.0;
        filledCircle.fill.color = "#00ff00";
        EDI_CHECK(addObject(fillDocument, filledCircle).ok);
        DraftingObject filledPolygon = makeObject("fpoly", DraftingShapeKind::Polygon, PolygonGeometry{{{0.1, 0.6}, {0.2, 0.6}, {0.2, 0.7}}});
        filledPolygon.fill.opacity = 0.25;
        filledPolygon.fill.color = "#0000ff";
        EDI_CHECK(addObject(fillDocument, filledPolygon).ok);
        // opacity 0 (the default) -> no fill, even on a fillable kind.
        DraftingObject unfilledRect = makeObject("urect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.05, 0.05}, 0.05, 0.05});
        EDI_CHECK(addObject(fillDocument, unfilledRect).ok);
        // A fill colour set on an OPEN kind never reaches the fill channel.
        DraftingObject filledLine = makeObject("fline", DraftingShapeKind::Line, LineGeometry{{0.4, 0.1}, {0.5, 0.1}});
        filledLine.fill.opacity = 1.0;
        filledLine.fill.color = "#ffffff";
        EDI_CHECK(addObject(fillDocument, filledLine).ok);

        const DraftingPlotPlan fillPlan = buildDraftingPlotPlan(fillDocument, projectDraftingGrid(defaultDraftingGridSettings()));
        EDI_CHECK(fillPlan.fills.size() == 3);
        EDI_CHECK(fillPlan.fills[0].objectId == "frect");
        EDI_CHECK(fillPlan.fills[0].color == "#ff0000");
        EDI_CHECK(nearlyEqual(fillPlan.fills[0].opacity, 0.5));
        EDI_CHECK(fillPlan.fills[0].points.size() == 4); // rectangle corners
        EDI_CHECK(fillPlan.fills[1].objectId == "fcircle");
        EDI_CHECK(fillPlan.fills[1].points.size() == 32); // matches the stroke tessellation
        EDI_CHECK(fillPlan.fills[2].objectId == "fpoly");
        EDI_CHECK(fillPlan.fills[2].points.size() == 3);
        // Fills are additive: stroke segments still flow as before.
        EDI_CHECK(!fillPlan.segments.empty());
    }

    DraftingDocument travelDocument = makeDraftingDocument("travel_doc");
    EDI_CHECK(addObject(travelDocument, makeObject("line_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}})).ok);
    EDI_CHECK(addObject(travelDocument, makeObject("line_b", DraftingShapeKind::Line, LineGeometry{{0.2, 0.1}, {0.3, 0.1}})).ok);
    EDI_CHECK(addObject(travelDocument, makeObject("line_c", DraftingShapeKind::Line, LineGeometry{{0.7, 0.7}, {0.8, 0.8}})).ok);
    EDI_CHECK(addObject(travelDocument, makeObject("travel_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.5})).ok);
    DraftingObject travelHidden = makeObject("travel_hidden", DraftingShapeKind::Line, LineGeometry{{0.9, 0.9}, {1.0, 1.0}});
    travelHidden.visible = false;
    EDI_CHECK(addObject(travelDocument, travelHidden).ok);
    const DraftingPlotPlan travelPlan = buildDraftingPlotPlan(travelDocument, projectDraftingGrid(defaultDraftingGridSettings()));
    EDI_CHECK(travelPlan.objects.size() == 3);
    EDI_CHECK(travelPlan.segments.size() == 3);
    EDI_CHECK(travelPlan.travelSegments.size() == 1);
    EDI_CHECK(travelPlan.travelSegments.front().fromObjectId == "line_b");
    EDI_CHECK(travelPlan.travelSegments.front().toObjectId == "line_c");
    EDI_CHECK(nearlyEqual(travelPlan.travelSegments.front().a.x, 0.3));
    EDI_CHECK(nearlyEqual(travelPlan.travelSegments.front().a.y, 0.1));
    EDI_CHECK(nearlyEqual(travelPlan.travelSegments.front().b.x, 0.7));
    EDI_CHECK(nearlyEqual(travelPlan.travelSegments.front().b.y, 0.7));
    EDI_CHECK(nearlyEqual(travelPlan.travelDistance, std::sqrt(0.52)));

    DraftingDocument orderDocument = makeDraftingDocument("order_doc");
    EDI_CHECK(addObject(orderDocument, makeObject("far", DraftingShapeKind::Line, LineGeometry{{0.8, 0.8}, {0.9, 0.8}})).ok);
    EDI_CHECK(addObject(orderDocument, makeObject("near_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}})).ok);
    EDI_CHECK(addObject(orderDocument, makeObject("near_b", DraftingShapeKind::Line, LineGeometry{{0.2, 0.1}, {0.3, 0.1}})).ok);
    EDI_CHECK(addObject(orderDocument, makeObject("mid", DraftingShapeKind::Line, LineGeometry{{0.4, 0.4}, {0.5, 0.4}})).ok);
    const DraftingGridProjection orderGrid = projectDraftingGrid(defaultDraftingGridSettings());
    const DraftingPlotPlan layerOrderPlan = buildDraftingPlotPlan(orderDocument, orderGrid);
    DraftingPlotSettings nearestSettings = defaultDraftingPlotSettings();
    nearestSettings.orderMode = DraftingPlotOrderMode::NearestNext;
    const DraftingPlotPlan nearestPlan = buildDraftingPlotPlan(orderDocument, orderGrid, nearestSettings);
    EDI_CHECK(layerOrderPlan.orderMode == DraftingPlotOrderMode::LayerOrder);
    EDI_CHECK(nearestPlan.orderMode == DraftingPlotOrderMode::NearestNext);
    EDI_CHECK(layerOrderPlan.segments.size() == nearestPlan.segments.size());
    EDI_CHECK(layerOrderPlan.segments.front().objectId == "far");
    EDI_CHECK(nearestPlan.segments[0].objectId == "near_a");
    EDI_CHECK(nearestPlan.segments[1].objectId == "near_b");
    EDI_CHECK(nearestPlan.segments[2].objectId == "mid");
    EDI_CHECK(nearestPlan.segments[3].objectId == "far");
    EDI_CHECK(nearestPlan.travelDistance < layerOrderPlan.travelDistance);
    EDI_CHECK(draftingPlotOrderModeFromName("nearest_next") == DraftingPlotOrderMode::NearestNext);
    EDI_CHECK(draftingPlotOrderModeFromName("unknown") == DraftingPlotOrderMode::LayerOrder);
    EDI_CHECK(std::string(draftingPlotOrderModeName(DraftingPlotOrderMode::NearestNext)) == "nearest_next");

    DraftingDocument directionDocument = makeDraftingDocument("direction_doc");
    EDI_CHECK(addObject(directionDocument, makeObject("first", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.2, 0.0}})).ok);
    EDI_CHECK(addObject(directionDocument, makeObject("second", DraftingShapeKind::Line, LineGeometry{{0.9, 0.0}, {0.3, 0.0}})).ok);
    DraftingPlotSettings preserveDirection = defaultDraftingPlotSettings();
    preserveDirection.orderMode = DraftingPlotOrderMode::NearestNext;
    const DraftingPlotPlan preserveDirectionPlan = buildDraftingPlotPlan(directionDocument, orderGrid, preserveDirection);
    DraftingPlotSettings reverseDirection = preserveDirection;
    reverseDirection.directionMode = DraftingPlotDirectionMode::AllowReverseSegments;
    const DraftingPlotPlan reverseDirectionPlan = buildDraftingPlotPlan(directionDocument, orderGrid, reverseDirection);
    EDI_CHECK(preserveDirectionPlan.directionMode == DraftingPlotDirectionMode::PreserveDirection);
    EDI_CHECK(reverseDirectionPlan.directionMode == DraftingPlotDirectionMode::AllowReverseSegments);
    EDI_CHECK(preserveDirectionPlan.segments.size() == reverseDirectionPlan.segments.size());
    EDI_CHECK(reverseDirectionPlan.segments[0].objectId == "first");
    EDI_CHECK(reverseDirectionPlan.segments[1].objectId == "second");
    EDI_CHECK(nearlyEqual(preserveDirectionPlan.segments[1].a.x, 0.9));
    EDI_CHECK(nearlyEqual(preserveDirectionPlan.segments[1].b.x, 0.3));
    EDI_CHECK(nearlyEqual(reverseDirectionPlan.segments[1].a.x, 0.3));
    EDI_CHECK(nearlyEqual(reverseDirectionPlan.segments[1].b.x, 0.9));
    EDI_CHECK(reverseDirectionPlan.segments[1].layerId == preserveDirectionPlan.segments[1].layerId);
    EDI_CHECK(reverseDirectionPlan.segments[1].penId == preserveDirectionPlan.segments[1].penId);
    EDI_CHECK(reverseDirectionPlan.travelDistance < preserveDirectionPlan.travelDistance);
    EDI_CHECK(draftingPlotDirectionModeFromName("allow_reverse_segments") == DraftingPlotDirectionMode::AllowReverseSegments);
    EDI_CHECK(draftingPlotDirectionModeFromName("unknown") == DraftingPlotDirectionMode::PreserveDirection);
    EDI_CHECK(std::string(draftingPlotDirectionModeName(DraftingPlotDirectionMode::AllowReverseSegments)) == "allow_reverse_segments");

    DraftingDocument invalidPenDocument = makeDraftingDocument("invalid_pen_doc");
    EDI_CHECK(addLayer(invalidPenDocument, makeDraftingLayer("bad_pen", "Bad Pen", 1), true).ok);
    DraftingLayer *badPenLayer = findLayer(invalidPenDocument, "bad_pen");
    EDI_CHECK(badPenLayer != nullptr);
    badPenLayer->plot.penId.clear();
    badPenLayer->plot.strokeWidth = 0.0;
    DraftingObject badPenLine = makeObject("bad_pen_line", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.4, 0.2}});
    badPenLine.layerId = "bad_pen";
    EDI_CHECK(addObject(invalidPenDocument, badPenLine).ok);
    const DraftingPlotPlan invalidPenPlan = buildDraftingPlotPlan(invalidPenDocument, orderGrid);
    EDI_CHECK(invalidPenPlan.layerStats.size() == 1);
    EDI_CHECK(invalidPenPlan.layerStats.front().layerId == "bad_pen");
    EDI_CHECK(!invalidPenPlan.layerStats.front().ready);
    EDI_CHECK(invalidPenPlan.layerStats.front().blockedReason == "invalid_plot_style");
    EDI_CHECK(invalidPenPlan.penStats.size() == 1);
    EDI_CHECK(invalidPenPlan.penStats.front().penId.empty());
    EDI_CHECK(!invalidPenPlan.penStats.front().ready);
    EDI_CHECK(invalidPenPlan.penStats.front().blockedReason == "missing_pen_id");

    DraftingDocument calibrationDocument = makeDraftingDocument("calibrated_plot_doc");
    EDI_CHECK(addObject(calibrationDocument, makeObject("scaled_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.5, 0.0}})).ok);
    EDI_CHECK(addObject(calibrationDocument, makeObject("scaled_line_2", DraftingShapeKind::Line, LineGeometry{{0.75, 0.0}, {1.0, 0.0}})).ok);
    DraftingPlotSettings calibratedSettings = defaultDraftingPlotSettings();
    calibratedSettings.calibrationScale = 2.0;
    const DraftingPlotPlan calibratedPlan = buildDraftingPlotPlan(calibrationDocument, orderGrid, calibratedSettings);
    EDI_CHECK(nearlyEqual(calibratedPlan.calibrationScale, 2.0));
    EDI_CHECK(calibratedPlan.segments.size() == 2);
    EDI_CHECK(nearlyEqual(calibratedPlan.segments.front().rawA.x, 0.0));
    EDI_CHECK(nearlyEqual(calibratedPlan.segments.front().rawB.x, 0.5));
    EDI_CHECK(nearlyEqual(calibratedPlan.segments.front().a.x, 0.0));
    EDI_CHECK(nearlyEqual(calibratedPlan.segments.front().b.x, 1.0));
    EDI_CHECK(calibratedPlan.travelSegments.size() == 1);
    EDI_CHECK(nearlyEqual(calibratedPlan.travelSegments.front().rawA.x, 0.5));
    EDI_CHECK(nearlyEqual(calibratedPlan.travelSegments.front().rawB.x, 0.75));
    EDI_CHECK(nearlyEqual(calibratedPlan.travelSegments.front().a.x, 1.0));
    EDI_CHECK(nearlyEqual(calibratedPlan.travelSegments.front().b.x, 1.5));
    EDI_CHECK(nearlyEqual(calibratedPlan.travelDistance, 0.5));
    EDI_CHECK(calibratedPlan.layerStats.size() == 1);
    EDI_CHECK(nearlyEqual(calibratedPlan.layerStats.front().strokeDistance, 1.5));
    EDI_CHECK(nearlyEqual(calibratedPlan.layerStats.front().travelDistance, 0.5));
    EDI_CHECK(calibratedPlan.penStats.size() == 1);
    EDI_CHECK(nearlyEqual(calibratedPlan.penStats.front().strokeDistance, 1.5));
    EDI_CHECK(nearlyEqual(calibratedPlan.penStats.front().travelDistance, 0.5));

    calibratedSettings.calibrationScale = -2.0;
    const DraftingPlotPlan invalidScalePlan = buildDraftingPlotPlan(calibrationDocument, orderGrid, calibratedSettings);
    EDI_CHECK(nearlyEqual(invalidScalePlan.calibrationScale, 1.0));
    EDI_CHECK(nearlyEqual(invalidScalePlan.segments.front().rawB.x, 0.5));
    EDI_CHECK(nearlyEqual(invalidScalePlan.segments.front().b.x, 0.5));

    DraftingDocument calibratedBoundsDocument = makeDraftingDocument("calibrated_bounds_doc");
    EDI_CHECK(addObject(calibratedBoundsDocument, makeObject("inside_raw_scaled_out", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.5, 0.2}})).ok);
    DraftingPlotSettings calibratedBoundsSettings = defaultDraftingPlotSettings();
    calibratedBoundsSettings.calibrationScale = 2.0;
    const DraftingPlotPlan calibratedBoundsPlan = buildDraftingPlotPlan(calibratedBoundsDocument, orderGrid, calibratedBoundsSettings);
    EDI_CHECK(calibratedBoundsPlan.hasPlotBounds);
    EDI_CHECK(nearlyEqual(calibratedBoundsPlan.plotBounds.x, 0.4));
    EDI_CHECK(nearlyEqual(calibratedBoundsPlan.plotBounds.y, 0.4));
    EDI_CHECK(nearlyEqual(calibratedBoundsPlan.plotBounds.width, 0.6));
    EDI_CHECK(nearlyEqual(calibratedBoundsPlan.plotBounds.height, 0.0));
    EDI_CHECK(calibratedBoundsPlan.warnings.size() == 1);
    EDI_CHECK(calibratedBoundsPlan.warnings.front().objectId == "inside_raw_scaled_out");
    EDI_CHECK(calibratedBoundsPlan.warnings.front().kind == "calibrated_plot_out_of_drawable_bounds");
    EDI_CHECK(calibratedBoundsPlan.layerStats.size() == 1);
    EDI_CHECK(!calibratedBoundsPlan.layerStats.front().ready);
    EDI_CHECK(calibratedBoundsPlan.layerStats.front().blockedReason == "calibrated_plot_out_of_drawable_bounds");

    // Review regression: an object whose preset color selects a DIFFERENT
    // pen than its layer's must not block the job — objects and segments
    // count under the same resolved pen, seeded with the resolved stroke.
    {
        DraftingDocument remapDocument = makeDraftingDocument("pen_remap_doc");
        DraftingObject blueLine = makeObject("blue_line", DraftingShapeKind::Line, LineGeometry{{0.2, 0.3}, {0.8, 0.3}});
        blueLine.stroke.color = "#75c7ff"; // the pen_blue preset color on a pen_black layer
        EDI_CHECK(addObject(remapDocument, blueLine).ok);
        const DraftingPlotPlan remapPlan = buildDraftingPlotPlan(remapDocument, orderGrid);
        EDI_CHECK(remapPlan.penStats.size() == 1);
        EDI_CHECK(remapPlan.penStats.front().penId == "pen_blue");
        EDI_CHECK(remapPlan.penStats.front().objectCount == 1);
        EDI_CHECK(remapPlan.penStats.front().segmentCount == 1);
        EDI_CHECK(remapPlan.penStats.front().ready);
    }

    return 0;
}
