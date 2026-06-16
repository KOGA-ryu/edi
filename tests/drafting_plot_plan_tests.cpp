#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <string>

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
    inkLine.stroke.opacity = 0.6; // per-object opacity must ride its segments
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
    assert(plan.orderMode == DraftingPlotOrderMode::LayerOrder);
    assert(plan.directionMode == DraftingPlotDirectionMode::PreserveDirection);
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
    assert(plan.segments[0].opacity == 1.0); // default stays fully opaque
    assert(plan.segments[1].opacity == 0.6); // the object's alpha, not the layer's
    assert(nearlyEqual(plan.segments[1].a.x, 0.4));
    assert(nearlyEqual(plan.segments[1].b.y, 0.5));
    assert(plan.segments[2].objectId == "outside_line");
    assert(plan.travelSegments.size() == 2);
    assert(plan.travelSegments[0].fromObjectId == "default_line");
    assert(plan.travelSegments[0].toObjectId == "ink_line");
    assert(nearlyEqual(plan.travelSegments[0].a.x, 0.3));
    assert(nearlyEqual(plan.travelSegments[0].b.x, 0.4));
    assert(plan.travelSegments[1].fromObjectId == "ink_line");
    assert(plan.travelSegments[1].toObjectId == "outside_line");
    assert(nearlyEqual(plan.travelDistance, std::sqrt(0.02) + std::sqrt(0.5)));
    assert(plan.layerStats.size() == 3);
    assert(plan.layerStats[0].layerId == "default");
    assert(plan.layerStats[0].objectCount == 1);
    assert(plan.layerStats[0].segmentCount == 1);
    assert(nearlyEqual(plan.layerStats[0].strokeDistance, std::sqrt(0.02)));
    assert(nearlyEqual(plan.layerStats[0].travelDistance, 0.0));
    assert(plan.layerStats[0].ready);
    assert(plan.layerStats[0].blockedReason == "ready");
    assert(plan.layerStats[1].layerId == "ink");
    assert(plan.layerStats[1].objectCount == 2);
    assert(plan.layerStats[1].segmentCount == 2);
    assert(nearlyEqual(plan.layerStats[1].strokeDistance, std::sqrt(0.02) + std::sqrt(0.005)));
    assert(nearlyEqual(plan.layerStats[1].travelDistance, plan.travelDistance));
    assert(!plan.layerStats[1].ready);
    assert(plan.layerStats[1].blockedReason == "raw_out_of_drawable_bounds");
    assert(plan.layerStats[2].layerId == "disabled");
    assert(plan.layerStats[2].objectCount == 1);
    assert(plan.layerStats[2].segmentCount == 0);
    assert(!plan.layerStats[2].ready);
    assert(plan.layerStats[2].blockedReason == "plot_disabled");
    assert(plan.penStats.size() == 2);
    assert(plan.penStats[0].penId == "pen_black");
    assert(plan.penStats[0].objectCount == 2);
    assert(plan.penStats[0].segmentCount == 1);
    assert(nearlyEqual(plan.penStats[0].strokeDistance, std::sqrt(0.02)));
    assert(nearlyEqual(plan.penStats[0].travelDistance, 0.0));
    assert(plan.penStats[0].ready);
    assert(plan.penStats[0].blockedReason == "ready");
    assert(plan.penStats[1].penId == "pen_blue");
    assert(plan.penStats[1].objectCount == 2);
    assert(plan.penStats[1].segmentCount == 2);
    assert(nearlyEqual(plan.penStats[1].strokeDistance, std::sqrt(0.02) + std::sqrt(0.005)));
    assert(nearlyEqual(plan.penStats[1].travelDistance, plan.travelDistance));
    assert(plan.penStats[1].ready);
    assert(plan.penStats[1].blockedReason == "ready");
    assert(plan.warnings.size() == 1);
    assert(plan.warnings.front().objectId == "outside_line");
    assert(plan.warnings.front().kind == "raw_out_of_drawable_bounds");

    assert(updateLayerFlags(document, "ink", false, false).ok);
    const DraftingPlotPlan hiddenInkPlan = buildDraftingPlotPlan(document, plotGrid());
    assert(hiddenInkPlan.objects.size() == 1);
    assert(hiddenInkPlan.objects.front().objectId == "default_line");
    assert(hiddenInkPlan.travelSegments.empty());
    assert(nearlyEqual(hiddenInkPlan.travelDistance, 0.0));
    assert(hiddenInkPlan.layerStats.size() == 3);
    assert(hiddenInkPlan.layerStats[0].layerId == "default");
    assert(hiddenInkPlan.layerStats[0].ready);
    assert(hiddenInkPlan.layerStats[1].layerId == "ink");
    assert(!hiddenInkPlan.layerStats[1].ready);
    assert(hiddenInkPlan.layerStats[1].blockedReason == "hidden");
    assert(hiddenInkPlan.layerStats[2].layerId == "disabled");
    assert(!hiddenInkPlan.layerStats[2].ready);
    assert(hiddenInkPlan.layerStats[2].blockedReason == "plot_disabled");
    assert(hiddenInkPlan.penStats.size() == 2);
    assert(hiddenInkPlan.penStats[0].penId == "pen_black");
    assert(hiddenInkPlan.penStats[0].objectCount == 2);
    assert(hiddenInkPlan.penStats[0].ready);
    assert(hiddenInkPlan.penStats[1].penId == "pen_blue");
    assert(!hiddenInkPlan.penStats[1].ready);
    assert(hiddenInkPlan.penStats[1].blockedReason == "no_assigned_segments");
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
    for (const DraftingPlotTravelSegment &segment : segmentPlan.travelSegments) {
        assert(std::isfinite(segment.a.x));
        assert(std::isfinite(segment.a.y));
        assert(std::isfinite(segment.b.x));
        assert(std::isfinite(segment.b.y));
        assert(std::isfinite(segment.distance));
        assert(segment.distance > 0.0);
    }
    assert(std::isfinite(segmentPlan.travelDistance));
    assert(segmentPlan.travelDistance > 0.0);
    // Default objects carry no fill (FillStyle::opacity defaults to 0), so the
    // fill channel stays empty — the byte-identical default the boundary requires.
    assert(segmentPlan.fills.empty());

    // Fill plumbing: only closed fillable kinds with opacity>0 and a valid
    // colour collect a fill ring; line/polyline never do, and fill rides a
    // SEPARATE channel from the stroke segments.
    {
        DraftingDocument fillDocument = makeDraftingDocument("fill_doc");
        DraftingObject filledRect = makeObject("frect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.3, 0.3}, 0.2, 0.1});
        filledRect.fill.opacity = 0.5;
        filledRect.fill.color = "#ff0000";
        assert(addObject(fillDocument, filledRect).ok);
        DraftingObject filledCircle = makeObject("fcircle", DraftingShapeKind::Circle, CircleGeometry{{0.6, 0.6}, 0.1});
        filledCircle.fill.opacity = 1.0;
        filledCircle.fill.color = "#00ff00";
        assert(addObject(fillDocument, filledCircle).ok);
        DraftingObject filledPolygon = makeObject("fpoly", DraftingShapeKind::Polygon, PolygonGeometry{{{0.1, 0.6}, {0.2, 0.6}, {0.2, 0.7}}});
        filledPolygon.fill.opacity = 0.25;
        filledPolygon.fill.color = "#0000ff";
        assert(addObject(fillDocument, filledPolygon).ok);
        // opacity 0 (the default) -> no fill, even on a fillable kind.
        DraftingObject unfilledRect = makeObject("urect", DraftingShapeKind::Rectangle, RectangleGeometry{{0.05, 0.05}, 0.05, 0.05});
        assert(addObject(fillDocument, unfilledRect).ok);
        // A fill colour set on an OPEN kind never reaches the fill channel.
        DraftingObject filledLine = makeObject("fline", DraftingShapeKind::Line, LineGeometry{{0.4, 0.1}, {0.5, 0.1}});
        filledLine.fill.opacity = 1.0;
        filledLine.fill.color = "#ffffff";
        assert(addObject(fillDocument, filledLine).ok);

        const DraftingPlotPlan fillPlan = buildDraftingPlotPlan(fillDocument, projectDraftingGrid(defaultDraftingGridSettings()));
        assert(fillPlan.fills.size() == 3);
        assert(fillPlan.fills[0].objectId == "frect");
        assert(fillPlan.fills[0].color == "#ff0000");
        assert(nearlyEqual(fillPlan.fills[0].opacity, 0.5));
        assert(fillPlan.fills[0].points.size() == 4); // rectangle corners
        assert(fillPlan.fills[1].objectId == "fcircle");
        assert(fillPlan.fills[1].points.size() == 32); // matches the stroke tessellation
        assert(fillPlan.fills[2].objectId == "fpoly");
        assert(fillPlan.fills[2].points.size() == 3);
        // Fills are additive: stroke segments still flow as before.
        assert(!fillPlan.segments.empty());
    }

    DraftingDocument travelDocument = makeDraftingDocument("travel_doc");
    assert(addObject(travelDocument, makeObject("line_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}})).ok);
    assert(addObject(travelDocument, makeObject("line_b", DraftingShapeKind::Line, LineGeometry{{0.2, 0.1}, {0.3, 0.1}})).ok);
    assert(addObject(travelDocument, makeObject("line_c", DraftingShapeKind::Line, LineGeometry{{0.7, 0.7}, {0.8, 0.8}})).ok);
    assert(addObject(travelDocument, makeObject("travel_guide", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.5})).ok);
    DraftingObject travelHidden = makeObject("travel_hidden", DraftingShapeKind::Line, LineGeometry{{0.9, 0.9}, {1.0, 1.0}});
    travelHidden.visible = false;
    assert(addObject(travelDocument, travelHidden).ok);
    const DraftingPlotPlan travelPlan = buildDraftingPlotPlan(travelDocument, projectDraftingGrid(defaultDraftingGridSettings()));
    assert(travelPlan.objects.size() == 3);
    assert(travelPlan.segments.size() == 3);
    assert(travelPlan.travelSegments.size() == 1);
    assert(travelPlan.travelSegments.front().fromObjectId == "line_b");
    assert(travelPlan.travelSegments.front().toObjectId == "line_c");
    assert(nearlyEqual(travelPlan.travelSegments.front().a.x, 0.3));
    assert(nearlyEqual(travelPlan.travelSegments.front().a.y, 0.1));
    assert(nearlyEqual(travelPlan.travelSegments.front().b.x, 0.7));
    assert(nearlyEqual(travelPlan.travelSegments.front().b.y, 0.7));
    assert(nearlyEqual(travelPlan.travelDistance, std::sqrt(0.52)));

    DraftingDocument orderDocument = makeDraftingDocument("order_doc");
    assert(addObject(orderDocument, makeObject("far", DraftingShapeKind::Line, LineGeometry{{0.8, 0.8}, {0.9, 0.8}})).ok);
    assert(addObject(orderDocument, makeObject("near_a", DraftingShapeKind::Line, LineGeometry{{0.1, 0.1}, {0.2, 0.1}})).ok);
    assert(addObject(orderDocument, makeObject("near_b", DraftingShapeKind::Line, LineGeometry{{0.2, 0.1}, {0.3, 0.1}})).ok);
    assert(addObject(orderDocument, makeObject("mid", DraftingShapeKind::Line, LineGeometry{{0.4, 0.4}, {0.5, 0.4}})).ok);
    const DraftingGridProjection orderGrid = projectDraftingGrid(defaultDraftingGridSettings());
    const DraftingPlotPlan layerOrderPlan = buildDraftingPlotPlan(orderDocument, orderGrid);
    DraftingPlotSettings nearestSettings = defaultDraftingPlotSettings();
    nearestSettings.orderMode = DraftingPlotOrderMode::NearestNext;
    const DraftingPlotPlan nearestPlan = buildDraftingPlotPlan(orderDocument, orderGrid, nearestSettings);
    assert(layerOrderPlan.orderMode == DraftingPlotOrderMode::LayerOrder);
    assert(nearestPlan.orderMode == DraftingPlotOrderMode::NearestNext);
    assert(layerOrderPlan.segments.size() == nearestPlan.segments.size());
    assert(layerOrderPlan.segments.front().objectId == "far");
    assert(nearestPlan.segments[0].objectId == "near_a");
    assert(nearestPlan.segments[1].objectId == "near_b");
    assert(nearestPlan.segments[2].objectId == "mid");
    assert(nearestPlan.segments[3].objectId == "far");
    assert(nearestPlan.travelDistance < layerOrderPlan.travelDistance);
    assert(draftingPlotOrderModeFromName("nearest_next") == DraftingPlotOrderMode::NearestNext);
    assert(draftingPlotOrderModeFromName("unknown") == DraftingPlotOrderMode::LayerOrder);
    assert(std::string(draftingPlotOrderModeName(DraftingPlotOrderMode::NearestNext)) == "nearest_next");

    DraftingDocument directionDocument = makeDraftingDocument("direction_doc");
    assert(addObject(directionDocument, makeObject("first", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.2, 0.0}})).ok);
    assert(addObject(directionDocument, makeObject("second", DraftingShapeKind::Line, LineGeometry{{0.9, 0.0}, {0.3, 0.0}})).ok);
    DraftingPlotSettings preserveDirection = defaultDraftingPlotSettings();
    preserveDirection.orderMode = DraftingPlotOrderMode::NearestNext;
    const DraftingPlotPlan preserveDirectionPlan = buildDraftingPlotPlan(directionDocument, orderGrid, preserveDirection);
    DraftingPlotSettings reverseDirection = preserveDirection;
    reverseDirection.directionMode = DraftingPlotDirectionMode::AllowReverseSegments;
    const DraftingPlotPlan reverseDirectionPlan = buildDraftingPlotPlan(directionDocument, orderGrid, reverseDirection);
    assert(preserveDirectionPlan.directionMode == DraftingPlotDirectionMode::PreserveDirection);
    assert(reverseDirectionPlan.directionMode == DraftingPlotDirectionMode::AllowReverseSegments);
    assert(preserveDirectionPlan.segments.size() == reverseDirectionPlan.segments.size());
    assert(reverseDirectionPlan.segments[0].objectId == "first");
    assert(reverseDirectionPlan.segments[1].objectId == "second");
    assert(nearlyEqual(preserveDirectionPlan.segments[1].a.x, 0.9));
    assert(nearlyEqual(preserveDirectionPlan.segments[1].b.x, 0.3));
    assert(nearlyEqual(reverseDirectionPlan.segments[1].a.x, 0.3));
    assert(nearlyEqual(reverseDirectionPlan.segments[1].b.x, 0.9));
    assert(reverseDirectionPlan.segments[1].layerId == preserveDirectionPlan.segments[1].layerId);
    assert(reverseDirectionPlan.segments[1].penId == preserveDirectionPlan.segments[1].penId);
    assert(reverseDirectionPlan.travelDistance < preserveDirectionPlan.travelDistance);
    assert(draftingPlotDirectionModeFromName("allow_reverse_segments") == DraftingPlotDirectionMode::AllowReverseSegments);
    assert(draftingPlotDirectionModeFromName("unknown") == DraftingPlotDirectionMode::PreserveDirection);
    assert(std::string(draftingPlotDirectionModeName(DraftingPlotDirectionMode::AllowReverseSegments)) == "allow_reverse_segments");

    DraftingDocument invalidPenDocument = makeDraftingDocument("invalid_pen_doc");
    assert(addLayer(invalidPenDocument, makeDraftingLayer("bad_pen", "Bad Pen", 1), true).ok);
    DraftingLayer *badPenLayer = findLayer(invalidPenDocument, "bad_pen");
    assert(badPenLayer != nullptr);
    badPenLayer->plot.penId.clear();
    badPenLayer->plot.strokeWidth = 0.0;
    DraftingObject badPenLine = makeObject("bad_pen_line", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.4, 0.2}});
    badPenLine.layerId = "bad_pen";
    assert(addObject(invalidPenDocument, badPenLine).ok);
    const DraftingPlotPlan invalidPenPlan = buildDraftingPlotPlan(invalidPenDocument, orderGrid);
    assert(invalidPenPlan.layerStats.size() == 1);
    assert(invalidPenPlan.layerStats.front().layerId == "bad_pen");
    assert(!invalidPenPlan.layerStats.front().ready);
    assert(invalidPenPlan.layerStats.front().blockedReason == "invalid_plot_style");
    assert(invalidPenPlan.penStats.size() == 1);
    assert(invalidPenPlan.penStats.front().penId.empty());
    assert(!invalidPenPlan.penStats.front().ready);
    assert(invalidPenPlan.penStats.front().blockedReason == "missing_pen_id");

    DraftingDocument calibrationDocument = makeDraftingDocument("calibrated_plot_doc");
    assert(addObject(calibrationDocument, makeObject("scaled_line", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {0.5, 0.0}})).ok);
    assert(addObject(calibrationDocument, makeObject("scaled_line_2", DraftingShapeKind::Line, LineGeometry{{0.75, 0.0}, {1.0, 0.0}})).ok);
    DraftingPlotSettings calibratedSettings = defaultDraftingPlotSettings();
    calibratedSettings.calibrationScale = 2.0;
    const DraftingPlotPlan calibratedPlan = buildDraftingPlotPlan(calibrationDocument, orderGrid, calibratedSettings);
    assert(nearlyEqual(calibratedPlan.calibrationScale, 2.0));
    assert(calibratedPlan.segments.size() == 2);
    assert(nearlyEqual(calibratedPlan.segments.front().rawA.x, 0.0));
    assert(nearlyEqual(calibratedPlan.segments.front().rawB.x, 0.5));
    assert(nearlyEqual(calibratedPlan.segments.front().a.x, 0.0));
    assert(nearlyEqual(calibratedPlan.segments.front().b.x, 1.0));
    assert(calibratedPlan.travelSegments.size() == 1);
    assert(nearlyEqual(calibratedPlan.travelSegments.front().rawA.x, 0.5));
    assert(nearlyEqual(calibratedPlan.travelSegments.front().rawB.x, 0.75));
    assert(nearlyEqual(calibratedPlan.travelSegments.front().a.x, 1.0));
    assert(nearlyEqual(calibratedPlan.travelSegments.front().b.x, 1.5));
    assert(nearlyEqual(calibratedPlan.travelDistance, 0.5));
    assert(calibratedPlan.layerStats.size() == 1);
    assert(nearlyEqual(calibratedPlan.layerStats.front().strokeDistance, 1.5));
    assert(nearlyEqual(calibratedPlan.layerStats.front().travelDistance, 0.5));
    assert(calibratedPlan.penStats.size() == 1);
    assert(nearlyEqual(calibratedPlan.penStats.front().strokeDistance, 1.5));
    assert(nearlyEqual(calibratedPlan.penStats.front().travelDistance, 0.5));

    calibratedSettings.calibrationScale = -2.0;
    const DraftingPlotPlan invalidScalePlan = buildDraftingPlotPlan(calibrationDocument, orderGrid, calibratedSettings);
    assert(nearlyEqual(invalidScalePlan.calibrationScale, 1.0));
    assert(nearlyEqual(invalidScalePlan.segments.front().rawB.x, 0.5));
    assert(nearlyEqual(invalidScalePlan.segments.front().b.x, 0.5));

    DraftingDocument calibratedBoundsDocument = makeDraftingDocument("calibrated_bounds_doc");
    assert(addObject(calibratedBoundsDocument, makeObject("inside_raw_scaled_out", DraftingShapeKind::Line, LineGeometry{{0.2, 0.2}, {0.5, 0.2}})).ok);
    DraftingPlotSettings calibratedBoundsSettings = defaultDraftingPlotSettings();
    calibratedBoundsSettings.calibrationScale = 2.0;
    const DraftingPlotPlan calibratedBoundsPlan = buildDraftingPlotPlan(calibratedBoundsDocument, orderGrid, calibratedBoundsSettings);
    assert(calibratedBoundsPlan.hasPlotBounds);
    assert(nearlyEqual(calibratedBoundsPlan.plotBounds.x, 0.4));
    assert(nearlyEqual(calibratedBoundsPlan.plotBounds.y, 0.4));
    assert(nearlyEqual(calibratedBoundsPlan.plotBounds.width, 0.6));
    assert(nearlyEqual(calibratedBoundsPlan.plotBounds.height, 0.0));
    assert(calibratedBoundsPlan.warnings.size() == 1);
    assert(calibratedBoundsPlan.warnings.front().objectId == "inside_raw_scaled_out");
    assert(calibratedBoundsPlan.warnings.front().kind == "calibrated_plot_out_of_drawable_bounds");
    assert(calibratedBoundsPlan.layerStats.size() == 1);
    assert(!calibratedBoundsPlan.layerStats.front().ready);
    assert(calibratedBoundsPlan.layerStats.front().blockedReason == "calibrated_plot_out_of_drawable_bounds");

    // Review regression: an object whose preset color selects a DIFFERENT
    // pen than its layer's must not block the job — objects and segments
    // count under the same resolved pen, seeded with the resolved stroke.
    {
        DraftingDocument remapDocument = makeDraftingDocument("pen_remap_doc");
        DraftingObject blueLine = makeObject("blue_line", DraftingShapeKind::Line, LineGeometry{{0.2, 0.3}, {0.8, 0.3}});
        blueLine.stroke.color = "#75c7ff"; // the pen_blue preset color on a pen_black layer
        assert(addObject(remapDocument, blueLine).ok);
        const DraftingPlotPlan remapPlan = buildDraftingPlotPlan(remapDocument, orderGrid);
        assert(remapPlan.penStats.size() == 1);
        assert(remapPlan.penStats.front().penId == "pen_blue");
        assert(remapPlan.penStats.front().objectCount == 1);
        assert(remapPlan.penStats.front().segmentCount == 1);
        assert(remapPlan.penStats.front().ready);
    }

    return 0;
}
