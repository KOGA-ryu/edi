#include "drafting/DraftingPlotPlan.h"

#include "drafting/DraftingLayerOps.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace edi::drafting {
namespace {

// The number of segments a circle is tessellated into — for BOTH its plotted
// stroke outline (appendPlotSegments) and its SVG fill ring (closedFillRing).
// One constant because the two MUST stay equal: a fill ring smoother than the
// stroke would peek past the outline (per the frozen fill side-channel boundary).
constexpr int kCircleSegments = 32;

int layerOrderForObject(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return layer == nullptr ? std::numeric_limits<int>::max() : layer->order;
}

bool isHexDigit(char value)
{
    return (value >= '0' && value <= '9')
        || (value >= 'a' && value <= 'f')
        || (value >= 'A' && value <= 'F');
}

bool isValidStrokeColor(const std::string &value)
{
    if (value.size() != 7 || value.front() != '#') {
        return false;
    }
    return std::all_of(value.begin() + 1, value.end(), isHexDigit);
}

bool isValidLayerPlotStyle(const LayerPlotStyle &plot)
{
    return !plot.penId.empty()
        && isValidStrokeColor(plot.strokeColor)
        && std::isfinite(plot.strokeWidth)
        && plot.strokeWidth > 0.0;
}

double safeCalibrationScale(double value)
{
    return std::isfinite(value) && value > 0.0 ? value : 1.0;
}

Point2D scalePoint(Point2D point, double scale)
{
    return {point.x * scale, point.y * scale};
}

Bounds2D boundsForPoints(Point2D a, Point2D b)
{
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.x, b.x);
    const double bottom = std::max(a.y, b.y);
    return {left, top, right - left, bottom - top};
}

Bounds2D includePoint(Bounds2D bounds, Point2D point)
{
    const double left = std::min(bounds.x, point.x);
    const double top = std::min(bounds.y, point.y);
    const double right = std::max(bounds.x + bounds.width, point.x);
    const double bottom = std::max(bounds.y + bounds.height, point.y);
    return {left, top, right - left, bottom - top};
}

double pointDistance(Point2D a, Point2D b)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

// The pen an object's segments actually ride: a color the OBJECT chose
// maps to its preset pen where one exists; an inherited color keeps the
// layer's pen verbatim (including a deliberately empty one — the
// readiness checks must still see it).
std::string resolvedObjectPenId(const DraftingObject &object, const DraftingLayer &layer)
{
    return object.stroke.color.empty()
        ? layer.plot.penId
        : penIdForStrokeColor(effectiveObjectStroke(object, layer).color, layer.plot.penId);
}

void appendSegment(
    DraftingPlotPlan &plan,
    const DraftingObject &object,
    const DraftingLayer &layer,
    Point2D a,
    Point2D b)
{
    // Segments carry the RESOLVED stroke (object wins, layer fills) and the
    // physically honest pen — through the same helper the plot stats use,
    // because the review found the half-shared version blocking whole plot
    // jobs: objects counted under the layer's pen, segments under the
    // remapped pen, and both pens flagged unready for missing the other.
    const StrokeStyle stroke = effectiveObjectStroke(object, layer);
    const std::string penId = resolvedObjectPenId(object, layer);
    plan.segments.push_back({
        object.id,
        object.layerId,
        a,
        b,
        a,
        b,
        penId,
        stroke.color,
        stroke.width,
        stroke.lineStyle,
        stroke.opacity,
    });
}

void appendVertexSegments(
    DraftingPlotPlan &plan,
    const DraftingObject &object,
    const DraftingLayer &layer,
    const std::vector<Point2D> &vertices,
    bool closed)
{
    if (vertices.size() < 2) {
        return;
    }
    for (std::size_t index = 0; index + 1 < vertices.size(); ++index) {
        appendSegment(plan, object, layer, vertices[index], vertices[index + 1]);
    }
    if (closed) {
        appendSegment(plan, object, layer, vertices.back(), vertices.front());
    }
}

void appendPlotSegments(DraftingPlotPlan &plan, const DraftingObject &object, const DraftingLayer &layer)
{
    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            constexpr double halfSize = 0.005;
            appendSegment(
                plan,
                object,
                layer,
                {geometry.point.x - halfSize, geometry.point.y},
                {geometry.point.x + halfSize, geometry.point.y});
            appendSegment(
                plan,
                object,
                layer,
                {geometry.point.x, geometry.point.y - halfSize},
                {geometry.point.x, geometry.point.y + halfSize});
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            appendSegment(plan, object, layer, geometry.a, geometry.b);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const Point2D a = geometry.origin;
            const Point2D b{geometry.origin.x + geometry.width, geometry.origin.y};
            const Point2D c{geometry.origin.x + geometry.width, geometry.origin.y + geometry.height};
            const Point2D d{geometry.origin.x, geometry.origin.y + geometry.height};
            appendSegment(plan, object, layer, a, b);
            appendSegment(plan, object, layer, b, c);
            appendSegment(plan, object, layer, c, d);
            appendSegment(plan, object, layer, d, a);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            constexpr double pi = 3.14159265358979323846;
            Point2D previous{geometry.center.x + geometry.radius, geometry.center.y};
            for (int index = 1; index <= kCircleSegments; ++index) {
                const double angle = 2.0 * pi * static_cast<double>(index) / static_cast<double>(kCircleSegments);
                const Point2D next{
                    geometry.center.x + std::cos(angle) * geometry.radius,
                    geometry.center.y + std::sin(angle) * geometry.radius,
                };
                appendSegment(plan, object, layer, previous, next);
                previous = next;
            }
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            // Flatten the arc to an open chain at ~2deg steps (shared sampler).
            appendVertexSegments(plan, object, layer, sampleArc(geometry), false);
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            appendVertexSegments(plan, object, layer, sampleEllipse(geometry), true);
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            appendVertexSegments(plan, object, layer, geometry.vertices, true);
        } else if constexpr (std::is_same_v<Geometry, PolylineGeometry>) {
            appendVertexSegments(plan, object, layer, geometry.vertices, false);
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            // Plot the SAMPLED curve as an open chain.
            appendVertexSegments(plan, object, layer, sampleSpline(geometry), false);
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // v1: plot the CENTERLINE as a single segment, exactly like
            // LineGeometry. Honest minimal — emitting the band OUTLINE (a closed
            // rectangle following the a->b band) is a later plot option.
            appendSegment(plan, object, layer, geometry.a, geometry.b);
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>
                || std::is_same_v<Geometry, GuideGeometry>
                || std::is_same_v<Geometry, ConstructionLineGeometry>
                || std::is_same_v<Geometry, DimensionGeometry>) {
            // The four kinds that emit NO plot segments. Text annotations carry
            // none; guides/construction lines/dimensions are filtered out earlier
            // by draftingShapeCanPlot, so they never reach here — but the arm is
            // explicit so the guard below can hold. Behavior unchanged: these
            // previously fell through to nothing (no matching branch).
        } else {
            // This visit used to be silent (a new kind would simply never plot);
            // the guard makes a future kind a COMPILE error that names this site.
            static_assert(always_false_v<Geometry>, "appendPlotSegments: unhandled geometry kind — add an arm");
        }
    }, object.geometry);
}

// The closed vertex ring for a SOLID-fillable shape, or empty for any kind the
// canvas painter does NOT fill. This mirrors DrawingCanvasObjectPainter's
// closed-shape set exactly (rectangle, circle, ellipse, polygon) so SVG fill
// agrees with the canvas. The ring walks the same points as the stroke outline,
// so fill and stroke never disagree at the edge. Every kind has an explicit arm
// (a fillable ring, or an explicit empty for the ten non-fillable kinds) and a
// terminal always_false_v guard — so a future geometry kind must consciously pick
// "fillable" or "not", as a COMPILE error, rather than silently never filling.
std::vector<Point2D> closedFillRing(const DraftingGeometry &geometry)
{
    return std::visit([](const auto &shape) -> std::vector<Point2D> {
        using Geometry = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            return {
                shape.origin,
                {shape.origin.x + shape.width, shape.origin.y},
                {shape.origin.x + shape.width, shape.origin.y + shape.height},
                {shape.origin.x, shape.origin.y + shape.height},
            };
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            // kCircleSegments matches the circle stroke arm in appendPlotSegments
            // (NOT sampleEllipse's 64) so the fill ring lands on the same outline
            // the stroke draws — a smoother fill would peek past it.
            constexpr double pi = 3.14159265358979323846;
            std::vector<Point2D> ring;
            ring.reserve(kCircleSegments);
            for (int index = 0; index < kCircleSegments; ++index) {
                const double angle = 2.0 * pi * static_cast<double>(index) / static_cast<double>(kCircleSegments);
                ring.push_back({
                    shape.center.x + std::cos(angle) * shape.radius,
                    shape.center.y + std::sin(angle) * shape.radius,
                });
            }
            return ring;
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            // Shared sampler — same ring the ellipse stroke walks (closed, 64).
            return sampleEllipse(shape);
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            return shape.vertices;
        } else if constexpr (std::is_same_v<Geometry, PointGeometry>
                || std::is_same_v<Geometry, LineGeometry>
                || std::is_same_v<Geometry, ArcGeometry>
                || std::is_same_v<Geometry, PolylineGeometry>
                || std::is_same_v<Geometry, GuideGeometry>
                || std::is_same_v<Geometry, ConstructionLineGeometry>
                || std::is_same_v<Geometry, DimensionGeometry>
                || std::is_same_v<Geometry, TextAnnotationGeometry>
                || std::is_same_v<Geometry, SplineGeometry>
                || std::is_same_v<Geometry, WallGeometry>) {
            // The ten kinds the canvas painter does NOT fill: an empty ring, the
            // explicit form of the old catch-all `else`. The frozen fill boundary
            // (solid only; fillable set = rectangle/circle/ellipse/polygon) is what
            // keeps these OUT — this arm records that decision per kind.
            return {};
        } else {
            static_assert(always_false_v<Geometry>, "closedFillRing: unhandled geometry kind — add an arm");
        }
    }, geometry);
}

void appendPlotFill(DraftingPlotPlan &plan, const DraftingObject &object)
{
    // Same rule the canvas painter applies: a closed fillable kind with
    // opacity>0 and a valid #rrggbb colour. Reuses the plot's color gate
    // (isValidStrokeColor) — one vocabulary for "a colour this system accepts".
    if (object.fill.opacity <= 0.0 || !isValidStrokeColor(object.fill.color)) {
        return;
    }
    std::vector<Point2D> ring = closedFillRing(object.geometry);
    if (ring.size() < 3) {
        return; // not a closed region (non-fillable kind or degenerate)
    }
    plan.fills.push_back({object.id, std::move(ring), object.fill.color, object.fill.opacity});
}

void appendTravelSegments(DraftingPlotPlan &plan)
{
    constexpr double minimumTravel = 0.000001;
    if (plan.segments.size() < 2) {
        return;
    }

    for (std::size_t index = 0; index + 1 < plan.segments.size(); ++index) {
        const DraftingPlotSegment &from = plan.segments[index];
        const DraftingPlotSegment &to = plan.segments[index + 1];
        const double travelDistance = pointDistance(from.b, to.a);
        if (!std::isfinite(travelDistance) || travelDistance <= minimumTravel) {
            continue;
        }

        plan.travelSegments.push_back({
            from.objectId,
            to.objectId,
            to.layerId,
            to.penId,
            from.rawB,
            to.rawA,
            from.b,
            to.a,
            travelDistance,
        });
        plan.travelDistance += travelDistance;
    }
}

void reverseSegment(DraftingPlotSegment &segment)
{
    std::swap(segment.rawA, segment.rawB);
    std::swap(segment.a, segment.b);
}

void applyCalibrationScale(DraftingPlotPlan &plan)
{
    if (plan.calibrationScale == 1.0) {
        return;
    }
    for (DraftingPlotSegment &segment : plan.segments) {
        segment.a = scalePoint(segment.rawA, plan.calibrationScale);
        segment.b = scalePoint(segment.rawB, plan.calibrationScale);
    }
    // Fill rings are collected in raw space too, so they ride the same
    // calibration the segments do — otherwise a scaled plot's fill would
    // detach from its (scaled) outline.
    for (DraftingPlotFill &fill : plan.fills) {
        for (Point2D &point : fill.points) {
            point = scalePoint(point, plan.calibrationScale);
        }
    }
}

bool planHasWarning(const DraftingPlotPlan &plan, const DraftingObjectId &objectId, const std::string &kind)
{
    return std::any_of(plan.warnings.begin(), plan.warnings.end(), [&](const DraftingPlotWarning &warning) {
        return warning.objectId == objectId && warning.kind == kind;
    });
}

bool objectHasRawOutOfBoundsWarning(const DraftingPlotPlan &plan, const DraftingObjectId &objectId)
{
    return planHasWarning(plan, objectId, "raw_out_of_drawable_bounds");
}

void computeCalibratedPlotBounds(DraftingPlotPlan &plan)
{
    plan.hasPlotBounds = false;
    plan.plotBounds = {};
    for (const DraftingPlotSegment &segment : plan.segments) {
        if (!std::isfinite(segment.a.x)
            || !std::isfinite(segment.a.y)
            || !std::isfinite(segment.b.x)
            || !std::isfinite(segment.b.y)) {
            continue;
        }
        if (!plan.hasPlotBounds) {
            plan.plotBounds = boundsForPoints(segment.a, segment.b);
            plan.hasPlotBounds = true;
            continue;
        }
        plan.plotBounds = includePoint(includePoint(plan.plotBounds, segment.a), segment.b);
    }
}

void appendCalibratedPlotBoundsWarnings(DraftingPlotPlan &plan, const DraftingGridProjection &grid)
{
    for (const DraftingPlotSegment &segment : plan.segments) {
        if (objectHasRawOutOfBoundsWarning(plan, segment.objectId)
            || planHasWarning(plan, segment.objectId, "calibrated_plot_out_of_drawable_bounds")) {
            continue;
        }
        if (!boundsOutsideDrawableArea(boundsForPoints(segment.a, segment.b), grid)) {
            continue;
        }
        plan.warnings.push_back({
            segment.objectId,
            "calibrated_plot_out_of_drawable_bounds",
            "calibrated plot output is outside drawable bounds",
        });
    }
}

void reorderSegmentsNearestNext(
    std::vector<DraftingPlotSegment> &segments,
    Point2D start,
    DraftingPlotDirectionMode directionMode)
{
    if (segments.size() < 2) {
        return;
    }

    std::vector<DraftingPlotSegment> remaining = std::move(segments);
    std::vector<DraftingPlotSegment> ordered;
    ordered.reserve(remaining.size());
    Point2D current = start;

    while (!remaining.empty()) {
        std::size_t nearestIndex = 0;
        double nearestDistance = pointDistance(current, remaining.front().a);
        bool reverseNearest = false;
        if (directionMode == DraftingPlotDirectionMode::AllowReverseSegments) {
            const double reverseDistance = pointDistance(current, remaining.front().b);
            if (reverseDistance < nearestDistance) {
                nearestDistance = reverseDistance;
                reverseNearest = true;
            }
        }
        for (std::size_t index = 1; index < remaining.size(); ++index) {
            const double candidateDistance = pointDistance(current, remaining[index].a);
            if (candidateDistance < nearestDistance) {
                nearestIndex = index;
                nearestDistance = candidateDistance;
                reverseNearest = false;
            }
            if (directionMode == DraftingPlotDirectionMode::AllowReverseSegments) {
                const double reverseDistance = pointDistance(current, remaining[index].b);
                if (reverseDistance < nearestDistance) {
                    nearestIndex = index;
                    nearestDistance = reverseDistance;
                    reverseNearest = true;
                }
            }
        }

        DraftingPlotSegment selected = remaining[nearestIndex];
        if (reverseNearest) {
            reverseSegment(selected);
        }
        ordered.push_back(selected);
        current = ordered.back().b;
        remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(nearestIndex));
    }

    segments = std::move(ordered);
}

DraftingPlotLayerStats &ensureLayerStats(DraftingPlotPlan &plan, const DraftingDocument &document, const LayerId &layerId)
{
    const auto found = std::find_if(plan.layerStats.begin(), plan.layerStats.end(), [&](const DraftingPlotLayerStats &stats) {
        return stats.layerId == layerId;
    });
    if (found != plan.layerStats.end()) {
        return *found;
    }

    std::string layerName = layerId;
    if (const DraftingLayer *layer = findLayer(document, layerId)) {
        layerName = layer->name;
    }
    plan.layerStats.push_back({layerId, layerName});
    return plan.layerStats.back();
}

DraftingPlotPenStats &ensurePenStats(DraftingPlotPlan &plan, const DraftingPlotSegment &segment)
{
    const auto found = std::find_if(plan.penStats.begin(), plan.penStats.end(), [&](const DraftingPlotPenStats &stats) {
        return stats.penId == segment.penId;
    });
    if (found != plan.penStats.end()) {
        return *found;
    }

    plan.penStats.push_back({
        segment.penId,
        segment.strokeColor,
        segment.strokeWidth,
    });
    return plan.penStats.back();
}

DraftingPlotPenStats &ensurePenStats(DraftingPlotPlan &plan, const LayerPlotStyle &plot)
{
    const auto found = std::find_if(plan.penStats.begin(), plan.penStats.end(), [&](const DraftingPlotPenStats &stats) {
        return stats.penId == plot.penId;
    });
    if (found != plan.penStats.end()) {
        return *found;
    }

    plan.penStats.push_back({
        plot.penId,
        plot.strokeColor,
        plot.strokeWidth,
    });
    return plan.penStats.back();
}

DraftingPlotPenStats &ensurePenStats(DraftingPlotPlan &plan, const DraftingPlotTravelSegment &segment)
{
    const auto found = std::find_if(plan.penStats.begin(), plan.penStats.end(), [&](const DraftingPlotPenStats &stats) {
        return stats.penId == segment.toPenId;
    });
    if (found != plan.penStats.end()) {
        return *found;
    }

    plan.penStats.push_back({segment.toPenId});
    return plan.penStats.back();
}

std::string layerBoundsBlockedReason(const DraftingPlotPlan &plan, const DraftingDocument &document, const LayerId &layerId)
{
    for (const DraftingPlotWarning &warning : plan.warnings) {
        if (warning.kind != "raw_out_of_drawable_bounds" && warning.kind != "calibrated_plot_out_of_drawable_bounds") {
            continue;
        }
        const DraftingObject *object = findObject(document, warning.objectId);
        if (object != nullptr && object->layerId == layerId) {
            return warning.kind;
        }
    }
    return {};
}

void finalizePlotStatsReadiness(DraftingPlotPlan &plan, const DraftingDocument &document)
{
    for (DraftingPlotLayerStats &stats : plan.layerStats) {
        const DraftingLayer *layer = findLayer(document, stats.layerId);
        if (layer == nullptr) {
            stats.ready = false;
            stats.blockedReason = "missing_layer";
        } else if (!layer->visible) {
            stats.ready = false;
            stats.blockedReason = "hidden";
        } else if (!layer->plot.plotEnabled) {
            stats.ready = false;
            stats.blockedReason = "plot_disabled";
        } else if (!isValidLayerPlotStyle(layer->plot)) {
            stats.ready = false;
            stats.blockedReason = "invalid_plot_style";
        } else if (stats.objectCount <= 0) {
            stats.ready = false;
            stats.blockedReason = "no_plotted_objects";
        } else if (stats.segmentCount <= 0) {
            stats.ready = false;
            stats.blockedReason = "no_assigned_segments";
        } else if (const std::string blockedReason = layerBoundsBlockedReason(plan, document, stats.layerId); !blockedReason.empty()) {
            stats.ready = false;
            stats.blockedReason = blockedReason;
        } else {
            stats.ready = true;
            stats.blockedReason = "ready";
        }
    }

    for (DraftingPlotPenStats &stats : plan.penStats) {
        if (stats.penId.empty()) {
            stats.ready = false;
            stats.blockedReason = "missing_pen_id";
        } else if (!std::isfinite(stats.strokeWidth) || stats.strokeWidth <= 0.0) {
            stats.ready = false;
            stats.blockedReason = "invalid_stroke_width";
        } else if (!isValidStrokeColor(stats.strokeColor)) {
            stats.ready = false;
            stats.blockedReason = "invalid_stroke_color";
        } else if (stats.objectCount <= 0) {
            stats.ready = false;
            stats.blockedReason = "no_assigned_objects";
        } else if (stats.segmentCount <= 0) {
            stats.ready = false;
            stats.blockedReason = "no_assigned_segments";
        } else {
            stats.ready = true;
            stats.blockedReason = "ready";
        }
    }
}

void appendPlotStats(DraftingPlotPlan &plan, const DraftingDocument &document)
{
    for (const DraftingObject &object : document.objects) {
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !object.visible || !draftingShapeCanPlot(object.kind)) {
            continue;
        }

        DraftingPlotLayerStats &layerStats = ensureLayerStats(plan, document, layer->id);
        ++layerStats.objectCount;

        // Count the object under the pen its segments will actually ride —
        // seeding the stats with the RESOLVED stroke, so the color/width
        // readiness checks judge what plots, not what the layer declares.
        LayerPlotStyle resolvedPlot = layer->plot;
        const StrokeStyle resolved = effectiveObjectStroke(object, *layer);
        resolvedPlot.penId = resolvedObjectPenId(object, *layer);
        resolvedPlot.strokeColor = resolved.color;
        resolvedPlot.strokeWidth = resolved.width;
        DraftingPlotPenStats &penStats = ensurePenStats(plan, resolvedPlot);
        ++penStats.objectCount;
    }

    for (const DraftingPlotSegment &segment : plan.segments) {
        const double strokeDistance = pointDistance(segment.a, segment.b);
        DraftingPlotLayerStats &layerStats = ensureLayerStats(plan, document, segment.layerId);
        ++layerStats.segmentCount;
        layerStats.strokeDistance += strokeDistance;

        DraftingPlotPenStats &penStats = ensurePenStats(plan, segment);
        ++penStats.segmentCount;
        penStats.strokeDistance += strokeDistance;
    }

    for (const DraftingPlotTravelSegment &segment : plan.travelSegments) {
        DraftingPlotLayerStats &layerStats = ensureLayerStats(plan, document, segment.toLayerId);
        layerStats.travelDistance += segment.distance;

        DraftingPlotPenStats &penStats = ensurePenStats(plan, segment);
        penStats.travelDistance += segment.distance;
    }

    finalizePlotStatsReadiness(plan, document);
}

} // namespace

bool draftingStrokeColorIsValid(const std::string &value)
{
    return isValidStrokeColor(value);
}

DraftingPlotSettings defaultDraftingPlotSettings()
{
    return {};
}

const char *draftingPlotOrderModeName(DraftingPlotOrderMode mode)
{
    switch (mode) {
    case DraftingPlotOrderMode::NearestNext:
        return "nearest_next";
    case DraftingPlotOrderMode::LayerOrder:
        return "layer_order";
    }
    return "layer_order";
}

DraftingPlotOrderMode draftingPlotOrderModeFromName(const std::string &name)
{
    if (name == "nearest_next") {
        return DraftingPlotOrderMode::NearestNext;
    }
    return DraftingPlotOrderMode::LayerOrder;
}

const char *draftingPlotDirectionModeName(DraftingPlotDirectionMode mode)
{
    switch (mode) {
    case DraftingPlotDirectionMode::AllowReverseSegments:
        return "allow_reverse_segments";
    case DraftingPlotDirectionMode::PreserveDirection:
        return "preserve_direction";
    }
    return "preserve_direction";
}

DraftingPlotDirectionMode draftingPlotDirectionModeFromName(const std::string &name)
{
    if (name == "allow_reverse_segments") {
        return DraftingPlotDirectionMode::AllowReverseSegments;
    }
    return DraftingPlotDirectionMode::PreserveDirection;
}

bool draftingShapeCanPlot(DraftingShapeKind kind)
{
    return kind != DraftingShapeKind::Guide
        && kind != DraftingShapeKind::ConstructionLine
        && kind != DraftingShapeKind::Dimension;
}

DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid)
{
    return buildDraftingPlotPlan(document, grid, defaultDraftingPlotSettings());
}

DraftingPlotPlan buildDraftingPlotPlan(
    const DraftingDocument &document,
    const DraftingGridProjection &grid,
    const DraftingPlotSettings &settings)
{
    DraftingPlotPlan plan;
    plan.orderMode = settings.orderMode;
    plan.directionMode = settings.directionMode;
    plan.calibrationScale = safeCalibrationScale(settings.calibrationScale);

    std::vector<const DraftingObject *> sortedObjects;
    sortedObjects.reserve(document.objects.size());
    for (const DraftingObject &object : document.objects) {
        sortedObjects.push_back(&object);
    }
    std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [&](const DraftingObject *a, const DraftingObject *b) {
        return layerOrderForObject(document, *a) < layerOrderForObject(document, *b);
    });

    for (const DraftingObject *objectPointer : sortedObjects) {
        const DraftingObject &object = *objectPointer;
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !layer->visible || !layer->plot.plotEnabled || !object.visible || !draftingShapeCanPlot(object.kind)) {
            continue;
        }

        plan.objects.push_back({
            object.id,
            object.layerId,
            layer->plot.penId,
            layer->plot.strokeColor,
            layer->plot.strokeWidth,
        });
        appendPlotSegments(plan, object, *layer);
        appendPlotFill(plan, object);

        if (boundsOutsideDrawableArea(object.bounds, grid)) {
            plan.warnings.push_back({
                object.id,
                "raw_out_of_drawable_bounds",
                "plot object is outside drawable bounds",
            });
        }
    }

    if (settings.orderMode == DraftingPlotOrderMode::NearestNext) {
        reorderSegmentsNearestNext(plan.segments, grid.origin, settings.directionMode);
    }
    applyCalibrationScale(plan);
    computeCalibratedPlotBounds(plan);
    appendCalibratedPlotBoundsWarnings(plan, grid);
    appendTravelSegments(plan);
    appendPlotStats(plan, document);

    return plan;
}

} // namespace edi::drafting
