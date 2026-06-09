#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "core/DrawingDocumentProjection.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingArray.h"
#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingMirror.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingOffset.h"
#include "drafting/DraftingPlotBounds.h"
#include "drafting/DraftingPlotJob.h"
#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace {

using namespace edi::drafting;

double clamp01(double value)
{
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

QString nextObjectId(const QString &kind, int serial)
{
    return QStringLiteral("%1_%2").arg(kind, QString::number(serial).rightJustified(4, QLatin1Char('0')));
}

std::string toStdString(const QString &value)
{
    return value.toStdString();
}

DraftingToolKind toolKind(const QString &toolId)
{
    return draftingToolKindFromId(toStdString(toolId));
}

QString objectIdPrefix(DraftingToolKind kind)
{
    return QString::fromLatin1(draftingToolKindName(kind));
}

DraftingToolCreationRequest creationRequest(const QString &toolId, const QString &objectId, const LayerId &layerId, Point2D start, Point2D end)
{
    return {toolKind(toolId), toStdString(objectId), layerId, start, end, toStdString(toolId)};
}

QString nextLayerId(const DraftingDocument &document)
{
    int serial = static_cast<int>(document.layers.size()) + 1;
    while (containsLayer(document, toStdString(QStringLiteral("layer_%1").arg(serial)))) {
        ++serial;
    }
    return QStringLiteral("layer_%1").arg(serial);
}

LayerPlotStyle plotStyleForPenPreset(LayerPlotStyle plot, const QString &presetId)
{
    if (presetId == QStringLiteral("pen_blue")) {
        plot.penId = "pen_blue";
        plot.strokeColor = "#75c7ff";
    } else if (presetId == QStringLiteral("pen_red")) {
        plot.penId = "pen_red";
        plot.strokeColor = "#d98b8b";
    } else {
        plot.penId = "pen_black";
        plot.strokeColor = "#d7dde8";
    }
    return plot;
}

LayerPlotStyle plotStyleForWidthPreset(LayerPlotStyle plot, const QString &presetId)
{
    if (presetId == QStringLiteral("fine")) {
        plot.strokeWidth = 1.0;
    } else if (presetId == QStringLiteral("bold")) {
        plot.strokeWidth = 3.0;
    } else {
        plot.strokeWidth = 2.0;
    }
    return plot;
}

bool boundsIntersect(Bounds2D a, Bounds2D b)
{
    return a.x <= b.x + b.width
        && a.x + a.width >= b.x
        && a.y <= b.y + b.height
        && a.y + a.height >= b.y;
}

Bounds2D includeBounds(Bounds2D bounds, Bounds2D next)
{
    const double left = std::min(bounds.x, next.x);
    const double top = std::min(bounds.y, next.y);
    const double right = std::max(bounds.x + bounds.width, next.x + next.width);
    const double bottom = std::max(bounds.y + bounds.height, next.y + next.height);
    return {left, top, right - left, bottom - top};
}

bool containsId(const std::vector<DraftingObjectId> &ids, const DraftingObjectId &id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

struct MoveSnapAnchor {
    Point2D point;
    int rank = 3;
    QString label;
};

struct MoveSnapChoice {
    bool ok = false;
    bool intersection = false;
    double distance = std::numeric_limits<double>::max();
    int anchorRank = 3;
    double dx = 0.0;
    double dy = 0.0;
    Point2D intendedAnchor;
    Point2D snappedAnchor;
    DraftingObjectId sourceObjectId;
    QString anchorLabel;
};

bool guideSnapChoiceBetter(const MoveSnapChoice &candidate, const MoveSnapChoice &current)
{
    if (!current.ok) {
        return true;
    }
    if (candidate.intersection != current.intersection) {
        return candidate.intersection;
    }
    constexpr double epsilon = 0.000001;
    if (std::abs(candidate.distance - current.distance) > epsilon) {
        return candidate.distance < current.distance;
    }
    if (candidate.anchorRank != current.anchorRank) {
        return candidate.anchorRank < current.anchorRank;
    }
    return false;
}

QString moveAnchorLabelForHandle(const std::string &handleId)
{
    if (handleId == "point") {
        return QStringLiteral("point");
    }
    if (handleId == "line_start" || handleId == "line_end") {
        return QStringLiteral("endpoint");
    }
    if (handleId == "circle_center") {
        return QStringLiteral("center");
    }
    if (handleId == "circle_radius") {
        return QStringLiteral("radius");
    }
    if (handleId.rfind("rect_", 0) == 0) {
        return QStringLiteral("corner");
    }
    return QStringLiteral("handle");
}

void addUniqueAnchor(std::vector<MoveSnapAnchor> &anchors, Point2D point, int rank, QString label)
{
    if (!isFinite(point)) {
        return;
    }
    constexpr double epsilon = 0.000001;
    const auto duplicate = std::find_if(anchors.begin(), anchors.end(), [point](const MoveSnapAnchor &existing) {
        return std::abs(existing.point.x - point.x) < epsilon && std::abs(existing.point.y - point.y) < epsilon;
    });
    if (duplicate == anchors.end()) {
        anchors.push_back({point, rank, std::move(label)});
    }
}

std::vector<MoveSnapAnchor> moveSnapAnchorsForObject(const DraftingObject &object)
{
    std::vector<MoveSnapAnchor> anchors;
    for (const HandleAnchor &handle : handleAnchors(object.geometry)) {
        addUniqueAnchor(anchors, handle.point, 0, moveAnchorLabelForHandle(handle.id));
    }

    const Bounds2D bounds = object.bounds;
    if (isFinite(bounds)) {
        const double left = bounds.x;
        const double top = bounds.y;
        const double right = bounds.x + bounds.width;
        const double bottom = bounds.y + bounds.height;
        const double centerX = bounds.x + bounds.width / 2.0;
        const double centerY = bounds.y + bounds.height / 2.0;
        addUniqueAnchor(anchors, {centerX, centerY}, 1, QStringLiteral("center"));
        addUniqueAnchor(anchors, {left, centerY}, 2, QStringLiteral("edge"));
        addUniqueAnchor(anchors, {right, centerY}, 2, QStringLiteral("edge"));
        addUniqueAnchor(anchors, {centerX, top}, 2, QStringLiteral("edge"));
        addUniqueAnchor(anchors, {centerX, bottom}, 2, QStringLiteral("edge"));
        addUniqueAnchor(anchors, {left, top}, 3, QStringLiteral("corner"));
        addUniqueAnchor(anchors, {right, top}, 3, QStringLiteral("corner"));
        addUniqueAnchor(anchors, {right, bottom}, 3, QStringLiteral("corner"));
        addUniqueAnchor(anchors, {left, bottom}, 3, QStringLiteral("corner"));
    }
    return anchors;
}

QVariantMap boundsToMap(Bounds2D bounds)
{
    return {
        {QStringLiteral("x"), bounds.x},
        {QStringLiteral("y"), bounds.y},
        {QStringLiteral("width"), bounds.width},
        {QStringLiteral("height"), bounds.height},
    };
}

QVariantMap gridProjectionToMap(const DraftingGridProjection &grid)
{
    QVariantList lines;
    for (const DraftingGridLine &line : grid.lines) {
        lines.push_back(QVariantMap{
            {QStringLiteral("axis"), line.axis == DraftingGridLineAxis::Vertical ? QStringLiteral("vertical") : QStringLiteral("horizontal")},
            {QStringLiteral("position"), line.position},
            {QStringLiteral("major"), line.major},
        });
    }

    return {
        {QStringLiteral("preset"), QString::fromLatin1(draftingGridPresetName(grid.settings.preset))},
        {QStringLiteral("preset_label"), QString::fromLatin1(draftingGridPresetLabel(grid.settings.preset))},
        {QStringLiteral("unit"), QString::fromLatin1(draftingGridUnitName(grid.settings.unit))},
        {QStringLiteral("unit_label"), QString::fromLatin1(draftingGridUnitLabel(grid.settings.unit))},
        {QStringLiteral("width"), grid.settings.width},
        {QStringLiteral("height"), grid.settings.height},
        {QStringLiteral("margin_left"), grid.settings.marginLeft},
        {QStringLiteral("margin_top"), grid.settings.marginTop},
        {QStringLiteral("margin_right"), grid.settings.marginRight},
        {QStringLiteral("margin_bottom"), grid.settings.marginBottom},
        {QStringLiteral("minor_step"), grid.settings.minorStep},
        {QStringLiteral("major_line_every"), grid.settings.majorLineEvery},
        {QStringLiteral("visible"), grid.settings.visible},
        {QStringLiteral("page_bounds"), boundsToMap(grid.pageBounds)},
        {QStringLiteral("drawable_bounds"), boundsToMap(grid.drawableBounds)},
        {QStringLiteral("origin"), QVariantMap{{QStringLiteral("x"), grid.origin.x}, {QStringLiteral("y"), grid.origin.y}}},
        {QStringLiteral("lines"), lines},
    };
}

void applyGridToSnap(DraftingSnapSettings &snapSettings, const DraftingGridSettings &gridSettings)
{
    const DraftingGridSettings safe = sanitizeDraftingGridSettings(gridSettings);
    snapSettings.gridStepX = safe.minorStep / safe.width;
    snapSettings.gridStepY = safe.minorStep / safe.height;
    snapSettings.gridStep = snapSettings.gridStepX;
}

QString tolerancePresetId(double tolerance)
{
    if (tolerance <= 0.015) {
        return QStringLiteral("tight");
    }
    if (tolerance >= 0.06) {
        return QStringLiteral("loose");
    }
    return QStringLiteral("normal");
}

double toleranceForPreset(const QString &presetId)
{
    if (presetId == QStringLiteral("tight")) {
        return 0.015;
    }
    if (presetId == QStringLiteral("loose")) {
        return 0.06;
    }
    return 0.03;
}

DraftingGridUnit gridUnitFromId(const QString &unitId)
{
    const std::string id = unitId.toStdString();
    if (id == "millimeter") {
        return DraftingGridUnit::Millimeter;
    }
    if (id == "centimeter") {
        return DraftingGridUnit::Centimeter;
    }
    if (id == "inch") {
        return DraftingGridUnit::Inch;
    }
    if (id == "foot") {
        return DraftingGridUnit::Foot;
    }
    return DraftingGridUnit::CanvasUnit;
}

bool pointInsideBounds(Point2D point, Bounds2D bounds)
{
    return point.x >= bounds.x
        && point.y >= bounds.y
        && point.x <= bounds.x + bounds.width
        && point.y <= bounds.y + bounds.height;
}

QVariantMap pointToMap(Point2D point)
{
    return {
        {QStringLiteral("x"), point.x},
        {QStringLiteral("y"), point.y},
    };
}

QVariantMap calibrationMeasurementToMap(const DraftingCalibrationMeasurement &measurement)
{
    QVariantList objectIds;
    for (const DraftingObjectId &objectId : measurement.objectIds) {
        objectIds.push_back(drawing_core::qStringFromStdString(objectId));
    }

    return {
        {QStringLiteral("pattern_id"), drawing_core::qStringFromStdString(measurement.patternId)},
        {QStringLiteral("object_ids"), objectIds},
        {QStringLiteral("expected_value"), measurement.expectedValue},
        {QStringLiteral("measured_value"), measurement.measuredValue},
        {QStringLiteral("error_value"), measurement.errorValue},
        {QStringLiteral("percent_error"), measurement.percentError},
        {QStringLiteral("source"), drawing_core::qStringFromStdString(measurement.source)},
    };
}

QVariantMap calibrationCorrectionToMap(const DraftingCalibrationCorrectionPlan &correction)
{
    return {
        {QStringLiteral("ok"), correction.ok},
        {QStringLiteral("pattern_id"), drawing_core::qStringFromStdString(correction.patternId)},
        {QStringLiteral("expected_value"), correction.expectedValue},
        {QStringLiteral("measured_value"), correction.measuredValue},
        {QStringLiteral("scale_factor"), correction.scaleFactor},
        {QStringLiteral("correction_percent"), correction.correctionPercent},
        {QStringLiteral("message"), drawing_core::qStringFromStdString(correction.message)},
    };
}

const DraftingLayer *layerForObject(const DraftingDocument &document, const DraftingObject &object)
{
    return findLayer(document, object.layerId);
}

bool objectLayerLocked(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = layerForObject(document, object);
    return layer != nullptr && layer->locked;
}

bool objectEffectivelyVisible(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = layerForObject(document, object);
    return object.visible && layer != nullptr && layer->visible;
}

QVariantMap pointerProjectionToMap(Point2D rawPoint,
    const DraftingDocument &document,
    const DraftingSnapSettings &snapSettings,
    const DraftingGridProjection &grid)
{
    const Point2D raw = normalizeDraftingPoint(rawPoint);
    const DraftingSnapResult snap = resolveSnap(raw, document, snapSettings);
    return {
        {QStringLiteral("raw"), pointToMap(raw)},
        {QStringLiteral("snapped"), pointToMap(snap.point)},
        {QStringLiteral("kind"), QString::fromLatin1(draftingSnapKindName(snap.kind))},
        {QStringLiteral("source"), QString::fromLatin1(draftingSnapSourceKindName(snap.sourceKind))},
        {QStringLiteral("label"), QString::fromStdString(snap.label)},
        {QStringLiteral("source_object_id"), drawing_core::qStringFromStdString(snap.sourceObjectId)},
        {QStringLiteral("unit"), QString::fromLatin1(draftingGridUnitName(grid.settings.unit))},
        {QStringLiteral("unit_label"), QString::fromLatin1(draftingGridUnitLabel(grid.settings.unit))},
        {QStringLiteral("raw_unit_x"), raw.x * grid.settings.width},
        {QStringLiteral("raw_unit_y"), raw.y * grid.settings.height},
        {QStringLiteral("snapped_unit_x"), snap.point.x * grid.settings.width},
        {QStringLiteral("snapped_unit_y"), snap.point.y * grid.settings.height},
        {QStringLiteral("inside_drawable"), pointInsideBounds(snap.point, grid.drawableBounds)},
    };
}

QVariantList plotWarningsToList(const std::vector<DraftingPlotWarning> &plotWarnings)
{
    QVariantList warnings;
    for (const DraftingPlotWarning &warning : plotWarnings) {
        warnings.push_back(QVariantMap{
            {QStringLiteral("object_id"), drawing_core::qStringFromStdString(warning.objectId)},
            {QStringLiteral("kind"), drawing_core::qStringFromStdString(warning.kind)},
            {QStringLiteral("message"), drawing_core::qStringFromStdString(warning.message)},
        });
    }
    return warnings;
}

QVariantList blockedReasonsToList(const std::vector<std::string> &blockedReasons)
{
    QVariantList reasons;
    for (const std::string &reason : blockedReasons) {
        reasons.push_back(drawing_core::qStringFromStdString(reason));
    }
    return reasons;
}

void annotateProjectedObjectsWithPlotSafety(QVariantMap &model, const DraftingPlotPlan &plan)
{
    QVariantList objects = model.value(QStringLiteral("drawing_objects")).toList();
    for (QVariant &objectValue : objects) {
        QVariantMap object = objectValue.toMap();
        const QString objectId = object.value(QStringLiteral("id")).toString();
        QVariantList objectWarnings;
        bool rawOutsideDrawable = false;
        bool calibratedOutsideDrawable = false;

        for (const DraftingPlotWarning &warning : plan.warnings) {
            if (drawing_core::qStringFromStdString(warning.objectId) != objectId) {
                continue;
            }
            const QString kind = drawing_core::qStringFromStdString(warning.kind);
            objectWarnings.push_back(QVariantMap{
                {QStringLiteral("kind"), kind},
                {QStringLiteral("message"), drawing_core::qStringFromStdString(warning.message)},
            });
            rawOutsideDrawable = rawOutsideDrawable || kind == QStringLiteral("raw_out_of_drawable_bounds");
            calibratedOutsideDrawable = calibratedOutsideDrawable || kind == QStringLiteral("calibrated_plot_out_of_drawable_bounds");
        }

        const QVariantMap firstWarning = objectWarnings.isEmpty() ? QVariantMap{} : objectWarnings.front().toMap();
        object.insert(QStringLiteral("plot_warning_count"), objectWarnings.size());
        object.insert(QStringLiteral("plot_warnings"), objectWarnings);
        object.insert(QStringLiteral("plot_blocked"), !objectWarnings.isEmpty());
        object.insert(QStringLiteral("plot_safety_state"), objectWarnings.isEmpty() ? QStringLiteral("ready") : QStringLiteral("blocked"));
        object.insert(QStringLiteral("plot_warning_kind"), firstWarning.value(QStringLiteral("kind")).toString());
        object.insert(QStringLiteral("plot_warning_message"), firstWarning.value(QStringLiteral("message")).toString());
        object.insert(QStringLiteral("outside_drawable"), rawOutsideDrawable);
        object.insert(QStringLiteral("calibrated_outside_drawable"), calibratedOutsideDrawable);
        objectValue = object;
    }
    model.insert(QStringLiteral("drawing_objects"), objects);
}

QVariantMap plotPlanToMap(const DraftingPlotPlan &plan)
{
    const DraftingPlotJob job = buildDraftingPlotJob(plan);
    const QVariantList warnings = plotWarningsToList(plan.warnings);
    QVariantList segments;
    for (const DraftingPlotSegment &segment : plan.segments) {
        segments.push_back(QVariantMap{
            {QStringLiteral("object_id"), drawing_core::qStringFromStdString(segment.objectId)},
            {QStringLiteral("layer_id"), drawing_core::qStringFromStdString(segment.layerId)},
            {QStringLiteral("raw_x1"), segment.rawA.x},
            {QStringLiteral("raw_y1"), segment.rawA.y},
            {QStringLiteral("raw_x2"), segment.rawB.x},
            {QStringLiteral("raw_y2"), segment.rawB.y},
            {QStringLiteral("x1"), segment.a.x},
            {QStringLiteral("y1"), segment.a.y},
            {QStringLiteral("x2"), segment.b.x},
            {QStringLiteral("y2"), segment.b.y},
            {QStringLiteral("calibrated_x1"), segment.a.x},
            {QStringLiteral("calibrated_y1"), segment.a.y},
            {QStringLiteral("calibrated_x2"), segment.b.x},
            {QStringLiteral("calibrated_y2"), segment.b.y},
            {QStringLiteral("pen_id"), drawing_core::qStringFromStdString(segment.penId)},
            {QStringLiteral("stroke_color"), drawing_core::qStringFromStdString(segment.strokeColor)},
            {QStringLiteral("stroke_width"), segment.strokeWidth},
        });
    }
    QVariantList travelSegments;
    for (const DraftingPlotTravelSegment &segment : plan.travelSegments) {
        travelSegments.push_back(QVariantMap{
            {QStringLiteral("from_object_id"), drawing_core::qStringFromStdString(segment.fromObjectId)},
            {QStringLiteral("to_object_id"), drawing_core::qStringFromStdString(segment.toObjectId)},
            {QStringLiteral("to_layer_id"), drawing_core::qStringFromStdString(segment.toLayerId)},
            {QStringLiteral("to_pen_id"), drawing_core::qStringFromStdString(segment.toPenId)},
            {QStringLiteral("raw_x1"), segment.rawA.x},
            {QStringLiteral("raw_y1"), segment.rawA.y},
            {QStringLiteral("raw_x2"), segment.rawB.x},
            {QStringLiteral("raw_y2"), segment.rawB.y},
            {QStringLiteral("x1"), segment.a.x},
            {QStringLiteral("y1"), segment.a.y},
            {QStringLiteral("x2"), segment.b.x},
            {QStringLiteral("y2"), segment.b.y},
            {QStringLiteral("calibrated_x1"), segment.a.x},
            {QStringLiteral("calibrated_y1"), segment.a.y},
            {QStringLiteral("calibrated_x2"), segment.b.x},
            {QStringLiteral("calibrated_y2"), segment.b.y},
            {QStringLiteral("distance"), segment.distance},
        });
    }
    QVariantList layerStats;
    for (const DraftingPlotLayerStats &stats : plan.layerStats) {
        layerStats.push_back(QVariantMap{
            {QStringLiteral("layer_id"), drawing_core::qStringFromStdString(stats.layerId)},
            {QStringLiteral("layer_name"), drawing_core::qStringFromStdString(stats.layerName)},
            {QStringLiteral("object_count"), stats.objectCount},
            {QStringLiteral("segment_count"), stats.segmentCount},
            {QStringLiteral("stroke_distance"), stats.strokeDistance},
            {QStringLiteral("travel_distance"), stats.travelDistance},
            {QStringLiteral("ready"), stats.ready},
            {QStringLiteral("blocked_reason"), drawing_core::qStringFromStdString(stats.blockedReason)},
        });
    }
    QVariantList penStats;
    for (const DraftingPlotPenStats &stats : plan.penStats) {
        penStats.push_back(QVariantMap{
            {QStringLiteral("pen_id"), drawing_core::qStringFromStdString(stats.penId)},
            {QStringLiteral("stroke_color"), drawing_core::qStringFromStdString(stats.strokeColor)},
            {QStringLiteral("stroke_width"), stats.strokeWidth},
            {QStringLiteral("object_count"), stats.objectCount},
            {QStringLiteral("segment_count"), stats.segmentCount},
            {QStringLiteral("stroke_distance"), stats.strokeDistance},
            {QStringLiteral("travel_distance"), stats.travelDistance},
            {QStringLiteral("ready"), stats.ready},
            {QStringLiteral("blocked_reason"), drawing_core::qStringFromStdString(stats.blockedReason)},
        });
    }

    return {
        {QStringLiteral("plot_object_count"), static_cast<int>(plan.objects.size())},
        {QStringLiteral("order_mode"), QString::fromLatin1(draftingPlotOrderModeName(plan.orderMode))},
        {QStringLiteral("direction_mode"), QString::fromLatin1(draftingPlotDirectionModeName(plan.directionMode))},
        {QStringLiteral("calibration_scale"), plan.calibrationScale},
        {QStringLiteral("has_plot_bounds"), plan.hasPlotBounds},
        {QStringLiteral("plot_bounds"), boundsToMap(plan.plotBounds)},
        {QStringLiteral("segment_count"), static_cast<int>(plan.segments.size())},
        {QStringLiteral("travel_segment_count"), static_cast<int>(plan.travelSegments.size())},
        {QStringLiteral("travel_distance"), plan.travelDistance},
        {QStringLiteral("warning_count"), static_cast<int>(plan.warnings.size())},
        {QStringLiteral("ready"), job.ready},
        {QStringLiteral("blocked"), !job.ready},
        {QStringLiteral("status"), job.ready ? QStringLiteral("ready") : QStringLiteral("blocked")},
        {QStringLiteral("blocked_reason_count"), static_cast<int>(job.blockedReasons.size())},
        {QStringLiteral("blocked_reasons"), blockedReasonsToList(job.blockedReasons)},
        {QStringLiteral("layer_stats"), layerStats},
        {QStringLiteral("pen_stats"), penStats},
        {QStringLiteral("first_warning"), plan.warnings.empty() ? QString() : drawing_core::qStringFromStdString(plan.warnings.front().message)},
        {QStringLiteral("first_warning_kind"), plan.warnings.empty() ? QString() : drawing_core::qStringFromStdString(plan.warnings.front().kind)},
        {QStringLiteral("first_warning_object_id"), plan.warnings.empty() ? QString() : drawing_core::qStringFromStdString(plan.warnings.front().objectId)},
        {QStringLiteral("preview"), QVariantMap{
            {QStringLiteral("order_mode"), QString::fromLatin1(draftingPlotOrderModeName(plan.orderMode))},
            {QStringLiteral("direction_mode"), QString::fromLatin1(draftingPlotDirectionModeName(plan.directionMode))},
            {QStringLiteral("calibration_scale"), plan.calibrationScale},
            {QStringLiteral("segment_count"), static_cast<int>(plan.segments.size())},
            {QStringLiteral("travel_segment_count"), static_cast<int>(plan.travelSegments.size())},
            {QStringLiteral("travel_distance"), plan.travelDistance},
            {QStringLiteral("segments"), segments},
            {QStringLiteral("travel_segments"), travelSegments},
        }},
        {QStringLiteral("warnings"), warnings},
    };
}

double nudgeScaleForMode(const QString &stepMode)
{
    if (stepMode == QStringLiteral("fine")) {
        return 0.25;
    }
    if (stepMode == QStringLiteral("coarse")) {
        return 4.0;
    }
    return 1.0;
}

DraftingOffsetSide offsetSideFromId(const QString &sideId)
{
    return sideId == QStringLiteral("right") ? DraftingOffsetSide::Right : DraftingOffsetSide::Left;
}

DraftingMirrorAxis mirrorAxisFromId(const QString &axisId)
{
    return axisId == QStringLiteral("vertical") ? DraftingMirrorAxis::Vertical : DraftingMirrorAxis::Horizontal;
}

std::optional<DraftingAlignmentMode> alignmentModeFromId(const QString &modeId)
{
    if (modeId == QStringLiteral("left")) {
        return DraftingAlignmentMode::Left;
    }
    if (modeId == QStringLiteral("right")) {
        return DraftingAlignmentMode::Right;
    }
    if (modeId == QStringLiteral("top")) {
        return DraftingAlignmentMode::Top;
    }
    if (modeId == QStringLiteral("bottom")) {
        return DraftingAlignmentMode::Bottom;
    }
    if (modeId == QStringLiteral("center_x")) {
        return DraftingAlignmentMode::CenterX;
    }
    if (modeId == QStringLiteral("center_y")) {
        return DraftingAlignmentMode::CenterY;
    }
    return std::nullopt;
}

std::optional<DraftingAlignmentMode> distributeModeFromAxisId(const QString &axisId)
{
    if (axisId == QStringLiteral("x")) {
        return DraftingAlignmentMode::DistributeX;
    }
    if (axisId == QStringLiteral("y")) {
        return DraftingAlignmentMode::DistributeY;
    }
    return std::nullopt;
}

bool sameGuide(const GuideGeometry &a, const GuideGeometry &b)
{
    constexpr double epsilon = 0.000001;
    return a.orientation == b.orientation && std::abs(a.position - b.position) <= epsilon;
}

std::optional<DraftingObjectId> existingGuideId(const DraftingDocument &document, const GuideGeometry &guide)
{
    for (const DraftingObject &object : document.objects) {
        if (object.kind != DraftingShapeKind::Guide || !kindMatchesGeometry(object.kind, object.geometry)) {
            continue;
        }
        const auto *existing = std::get_if<GuideGeometry>(&object.geometry);
        if (existing != nullptr && sameGuide(*existing, guide)) {
            return object.id;
        }
    }
    return std::nullopt;
}

std::optional<double> nearestVisibleGuidePosition(const DraftingDocument &document, GuideOrientation orientation, double target)
{
    if (!std::isfinite(target)) {
        return std::nullopt;
    }

    bool found = false;
    double bestPosition = 0.0;
    double bestDistance = std::numeric_limits<double>::max();
    for (const DraftingObject &object : document.objects) {
        if (object.kind != DraftingShapeKind::Guide || !kindMatchesGeometry(object.kind, object.geometry)) {
            continue;
        }
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (!object.visible || layer == nullptr || !layer->visible) {
            continue;
        }
        const auto *guide = std::get_if<GuideGeometry>(&object.geometry);
        if (guide == nullptr || guide->orientation != orientation || !std::isfinite(guide->position)) {
            continue;
        }
        const double distance = std::abs(guide->position - target);
        if (!found || distance < bestDistance) {
            found = true;
            bestDistance = distance;
            bestPosition = guide->position;
        }
    }
    return found ? std::optional<double>{bestPosition} : std::nullopt;
}

} // namespace

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
    , m_document(makeDraftingDocument("active_drawing", "Active Drawing"))
    , m_gridSettings(defaultDraftingGridSettings())
    , m_plotSettings(defaultDraftingPlotSettings())
{
    applyGridToSnap(m_snapSettings, m_gridSettings);
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    QVariantMap model = drawing_core::draftingDocumentToModelProjection(m_document, m_snapSettings, &grid, m_previewObject ? &*m_previewObject : nullptr);
    QVariantMap snapProjection = model.value(QStringLiteral("snap")).toMap();
    snapProjection.insert(QStringLiteral("guide_move_enabled"), m_guideMoveSnapEnabled);
    model.insert(QStringLiteral("snap"), snapProjection);
    const DraftingPlotPlan plotPlan = buildDraftingPlotPlan(m_document, grid, m_plotSettings);
    model.insert(QStringLiteral("grid"), gridProjectionToMap(grid));
    model.insert(QStringLiteral("plot_summary"), plotPlanToMap(plotPlan));
    annotateProjectedObjectsWithPlotSafety(model, plotPlan);
    const DraftingPlotBoundsResult selectionPlotBounds = selectedRawPlotOutputBounds(
        m_document,
        m_document.selectedObjectIds,
        grid,
        m_plotSettings);
    model.insert(QStringLiteral("has_selection_plot_bounds"), selectionPlotBounds.ok);
    if (selectionPlotBounds.ok) {
        model.insert(QStringLiteral("selection_plot_bounds"), boundsToMap(selectionPlotBounds.bounds));
        model.insert(QStringLiteral("selection_plot_bounds_status"), QString::fromLatin1(draftingPlotBoundsStatusName(selectionPlotBounds.status)));
        model.insert(QStringLiteral("selection_drawable_relation"), QString::fromLatin1(draftingDrawableBoundsRelationName(selectionPlotBounds.relation)));
        model.insert(QStringLiteral("selection_plot_bounds_width"), selectionPlotBounds.bounds.width);
        model.insert(QStringLiteral("selection_plot_bounds_height"), selectionPlotBounds.bounds.height);
    } else {
        model.insert(QStringLiteral("selection_plot_bounds_status"), QStringLiteral("unavailable"));
        model.insert(QStringLiteral("selection_drawable_relation"), QStringLiteral("unavailable"));
        model.insert(QStringLiteral("selection_plot_bounds_width"), 0.0);
        model.insert(QStringLiteral("selection_plot_bounds_height"), 0.0);
    }
    if (m_pointerRawPoint) {
        model.insert(QStringLiteral("pointer"), pointerProjectionToMap(*m_pointerRawPoint, m_document, m_snapSettings, grid));
    }
    if (!m_lastGuideDragSnap.isEmpty()) {
        model.insert(QStringLiteral("guide_drag_snap"), m_lastGuideDragSnap);
    }
    if (m_latestCalibrationMeasurement) {
        model.insert(QStringLiteral("calibration_measurement"), calibrationMeasurementToMap(*m_latestCalibrationMeasurement));
    }
    if (m_pendingCalibrationCorrection) {
        model.insert(QStringLiteral("calibration_correction"), calibrationCorrectionToMap(*m_pendingCalibrationCorrection));
    }

    model.insert(QStringLiteral("warnings"), plotWarningsToList(plotPlan.warnings));
    return model;
}

QString DrawingDocumentController::selectedToolId() const
{
    return m_selectedToolId;
}

QString DrawingDocumentController::selectedObjectId() const
{
    return m_document.activeObjectId ? drawing_core::qStringFromStdString(*m_document.activeObjectId) : QString();
}

QString DrawingDocumentController::activeLayerId() const
{
    return drawing_core::qStringFromStdString(m_document.activeLayerId);
}

bool DrawingDocumentController::gridSnapEnabled() const
{
    return m_snapSettings.gridEnabled;
}

bool DrawingDocumentController::objectSnapEnabled() const
{
    return m_snapSettings.objectSnapEnabled;
}

bool DrawingDocumentController::endpointSnapEnabled() const
{
    return m_snapSettings.endpointEnabled;
}

bool DrawingDocumentController::vertexSnapEnabled() const
{
    return m_snapSettings.vertexEnabled;
}

bool DrawingDocumentController::midpointSnapEnabled() const
{
    return m_snapSettings.midpointEnabled;
}

bool DrawingDocumentController::centerSnapEnabled() const
{
    return m_snapSettings.centerEnabled;
}

bool DrawingDocumentController::guideSnapEnabled() const
{
    return m_snapSettings.guideEnabled;
}

bool DrawingDocumentController::guideMoveSnapEnabled() const
{
    return m_guideMoveSnapEnabled;
}

bool DrawingDocumentController::objectSnapPriorityBeforeGrid() const
{
    return m_snapSettings.objectPriorityBeforeGrid;
}

QString DrawingDocumentController::gridPresetId() const
{
    return QString::fromLatin1(draftingGridPresetName(m_gridSettings.preset));
}

QString DrawingDocumentController::objectSnapTolerancePresetId() const
{
    return tolerancePresetId(m_snapSettings.objectTolerance);
}

QString DrawingDocumentController::plotOrderModeId() const
{
    return QString::fromLatin1(draftingPlotOrderModeName(m_plotSettings.orderMode));
}

QString DrawingDocumentController::plotDirectionModeId() const
{
    return QString::fromLatin1(draftingPlotDirectionModeName(m_plotSettings.directionMode));
}

void DrawingDocumentController::setSelectedToolId(const QString &toolId)
{
    if (m_selectedToolId == toolId) {
        return;
    }
    m_selectedToolId = toolId;
    m_pendingCreation.reset();
    m_previewObject.reset();
    m_lastGuideDragSnap.clear();
    emit modelChanged();
}

void DrawingDocumentController::setGridSnapEnabled(bool enabled)
{
    if (m_snapSettings.gridEnabled == enabled) {
        return;
    }
    m_snapSettings.gridEnabled = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setObjectSnapEnabled(bool enabled)
{
    if (m_snapSettings.objectSnapEnabled == enabled) {
        return;
    }
    m_snapSettings.objectSnapEnabled = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setEndpointSnapEnabled(bool enabled)
{
    if (m_snapSettings.endpointEnabled == enabled) {
        return;
    }
    m_snapSettings.endpointEnabled = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setVertexSnapEnabled(bool enabled)
{
    if (m_snapSettings.vertexEnabled == enabled) {
        return;
    }
    m_snapSettings.vertexEnabled = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setMidpointSnapEnabled(bool enabled)
{
    if (m_snapSettings.midpointEnabled == enabled) {
        return;
    }
    m_snapSettings.midpointEnabled = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setCenterSnapEnabled(bool enabled)
{
    if (m_snapSettings.centerEnabled == enabled) {
        return;
    }
    m_snapSettings.centerEnabled = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setGuideSnapEnabled(bool enabled)
{
    if (m_snapSettings.guideEnabled == enabled) {
        return;
    }
    m_snapSettings.guideEnabled = enabled;
    m_lastGuideDragSnap.clear();
    emit modelChanged();
}

void DrawingDocumentController::setGuideMoveSnapEnabled(bool enabled)
{
    if (m_guideMoveSnapEnabled == enabled) {
        return;
    }
    m_guideMoveSnapEnabled = enabled;
    m_lastGuideDragSnap.clear();
    emit modelChanged();
}

void DrawingDocumentController::setObjectSnapPriorityBeforeGrid(bool enabled)
{
    if (m_snapSettings.objectPriorityBeforeGrid == enabled) {
        return;
    }
    m_snapSettings.objectPriorityBeforeGrid = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setObjectSnapTolerancePreset(QString presetId)
{
    const double tolerance = toleranceForPreset(presetId);
    if (m_snapSettings.objectTolerance == tolerance) {
        return;
    }
    m_snapSettings.objectTolerance = tolerance;
    emit modelChanged();
}

void DrawingDocumentController::setGridPresetId(const QString &presetId)
{
    const DraftingGridPreset preset = draftingGridPresetFromName(presetId.toStdString());
    if (m_gridSettings.preset == preset) {
        return;
    }
    m_gridSettings = draftingGridPresetSettings(preset);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setGridUnitId(const QString &unitId)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.preset = DraftingGridPreset::Custom;
    settings.unit = gridUnitFromId(unitId);
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setGridSize(double width, double height)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.preset = DraftingGridPreset::Custom;
    settings.width = width;
    settings.height = height;
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setGridMargins(double left, double top, double right, double bottom)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.preset = DraftingGridPreset::Custom;
    settings.marginLeft = left;
    settings.marginTop = top;
    settings.marginRight = right;
    settings.marginBottom = bottom;
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setGridStep(double minorStep)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.preset = DraftingGridPreset::Custom;
    settings.minorStep = minorStep;
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setGridMajorLineEvery(int majorLineEvery)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.preset = DraftingGridPreset::Custom;
    settings.majorLineEvery = majorLineEvery;
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setGridVisible(bool visible)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.preset = DraftingGridPreset::Custom;
    settings.visible = visible;
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyGridToSnap(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::setPlotOrderModeId(const QString &modeId)
{
    const DraftingPlotOrderMode mode = draftingPlotOrderModeFromName(modeId.toStdString());
    if (m_plotSettings.orderMode == mode) {
        return;
    }
    m_plotSettings.orderMode = mode;
    emit modelChanged();
}

void DrawingDocumentController::setPlotDirectionModeId(const QString &modeId)
{
    const DraftingPlotDirectionMode mode = draftingPlotDirectionModeFromName(modeId.toStdString());
    if (m_plotSettings.directionMode == mode) {
        return;
    }
    m_plotSettings.directionMode = mode;
    emit modelChanged();
}

void DrawingDocumentController::updatePointerNormalized(double x, double y)
{
    const Point2D point{clamp01(x), clamp01(y)};
    if (m_pointerRawPoint && m_pointerRawPoint->x == point.x && m_pointerRawPoint->y == point.y) {
        return;
    }
    m_pointerRawPoint = point;
    emit modelChanged();
}

bool DrawingDocumentController::updateSelectedObjectGeometryField(const QString &fieldId, double value)
{
    if (fieldId.isEmpty() || !m_document.activeObjectId || !std::isfinite(value)) {
        return false;
    }

    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }

    const DraftingNumericEditResult edit = applyNumericGeometryEdit(*object, toStdString(fieldId), value);
    if (!edit.ok) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, edit.geometry});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::updateSelectedObjectPhysicalGeometryField(const QString &fieldId, double value)
{
    if (fieldId.isEmpty() || !m_document.activeObjectId || !std::isfinite(value)) {
        return false;
    }

    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const double width = grid.settings.width;
    const double height = grid.settings.height;
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        return false;
    }

    if (fieldId == QStringLiteral("line_length") || fieldId == QStringLiteral("line_angle_deg")) {
        const auto *line = std::get_if<LineGeometry>(&object->geometry);
        if (object->kind != DraftingShapeKind::Line || line == nullptr) {
            return false;
        }

        const double ax = line->a.x * width;
        const double ay = line->a.y * height;
        const double bx = line->b.x * width;
        const double by = line->b.y * height;
        const double dx = bx - ax;
        const double dy = by - ay;
        constexpr double pi = 3.14159265358979323846;
        const double currentLength = std::sqrt(dx * dx + dy * dy);
        const double angle = fieldId == QStringLiteral("line_angle_deg")
            ? value * pi / 180.0
            : std::atan2(dy, dx);
        const double length = fieldId == QStringLiteral("line_length") ? value : currentLength;
        if (!std::isfinite(angle) || !std::isfinite(length) || length < 0.0) {
            return false;
        }

        const double normalizedX2 = (ax + std::cos(angle) * length) / width;
        const double normalizedY2 = (ay + std::sin(angle) * length) / height;
        const DraftingNumericEditResult xEdit = applyNumericGeometryEdit(*object, "x2", normalizedX2);
        if (!xEdit.ok) {
            return false;
        }
        DraftingObject partiallyEdited = *object;
        partiallyEdited.geometry = xEdit.geometry;
        const DraftingNumericEditResult yEdit = applyNumericGeometryEdit(partiallyEdited, "y2", normalizedY2);
        if (!yEdit.ok) {
            return false;
        }
        const DraftingCommandResult result = applyDraftingCommand(
            m_document,
            UpdateGeometryCommand{*m_document.activeObjectId, yEdit.geometry});
        if (!result.ok) {
            return false;
        }

        emit modelChanged();
        return true;
    }

    double normalizedValue = value;
    if (fieldId == QStringLiteral("position")) {
        const auto *guide = std::get_if<GuideGeometry>(&object->geometry);
        if (object->kind != DraftingShapeKind::Guide || guide == nullptr) {
            return false;
        }
        normalizedValue = guide->orientation == GuideOrientation::Horizontal
            ? value / height
            : value / width;
    } else if (fieldId == QStringLiteral("x")
        || fieldId == QStringLiteral("cx")
        || fieldId == QStringLiteral("x1")
        || fieldId == QStringLiteral("x2")
        || fieldId == QStringLiteral("width")
        || fieldId == QStringLiteral("radius")
        || fieldId == QStringLiteral("diameter")) {
        normalizedValue = value / width;
    } else if (fieldId == QStringLiteral("y")
        || fieldId == QStringLiteral("cy")
        || fieldId == QStringLiteral("y1")
        || fieldId == QStringLiteral("y2")
        || fieldId == QStringLiteral("height")) {
        normalizedValue = value / height;
    } else if (fieldId == QStringLiteral("rotation_deg")) {
        normalizedValue = value;
    } else {
        return false;
    }

    return updateSelectedObjectGeometryField(fieldId, normalizedValue);
}

bool DrawingDocumentController::setSelectedObjectLocked(bool locked)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateObjectFlagsCommand{*m_document.activeObjectId, locked, object->visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setSelectedObjectVisible(bool visible)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateObjectFlagsCommand{*m_document.activeObjectId, object->locked, visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setSelectedGuideLabel(const QString &label)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }

    ObjectMetadata metadata = object->metadata;
    metadata.guideVisual.label = toStdString(label);
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateMetadataCommand{*m_document.activeObjectId, metadata});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setSelectedGuideColor(const QString &color)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }

    ObjectMetadata metadata = object->metadata;
    metadata.guideVisual.color = toStdString(color);
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateMetadataCommand{*m_document.activeObjectId, metadata});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setSelectedGuideDashStyle(const QString &dashStyle)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }

    ObjectMetadata metadata = object->metadata;
    metadata.guideVisual.dashStyle = toStdString(dashStyle);
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateMetadataCommand{*m_document.activeObjectId, metadata});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setSelectedGuideLabelVisible(bool visible)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }

    ObjectMetadata metadata = object->metadata;
    metadata.guideVisual.showLabel = visible;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateMetadataCommand{*m_document.activeObjectId, metadata});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setSelectedDimensionLabelVisible(bool visible)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Dimension) {
        return false;
    }

    ObjectMetadata metadata = object->metadata;
    metadata.dimensionVisual.showLabel = visible;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateMetadataCommand{*m_document.activeObjectId, metadata});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setDefaultLayerLocked(bool locked)
{
    const DraftingLayer *layer = findLayer(m_document, "default");
    if (layer == nullptr) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerFlagsCommand{layer->id, locked, layer->visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setDefaultLayerVisible(bool visible)
{
    const DraftingLayer *layer = findLayer(m_document, "default");
    if (layer == nullptr) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerFlagsCommand{layer->id, layer->locked, visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setActiveLayerLocked(bool locked)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerFlagsCommand{layer->id, locked, layer->visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setActiveLayerVisible(bool visible)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerFlagsCommand{layer->id, layer->locked, visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setActiveLayerPlotEnabled(bool enabled)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr) {
        return false;
    }

    LayerPlotStyle plot = layer->plot;
    plot.plotEnabled = enabled;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerPlotStyleCommand{layer->id, plot});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setActiveLayerPenPreset(const QString &presetId)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr) {
        return false;
    }

    const LayerPlotStyle plot = plotStyleForPenPreset(layer->plot, presetId);
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerPlotStyleCommand{layer->id, plot});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setActiveLayerStrokeWidthPreset(const QString &presetId)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr) {
        return false;
    }

    const LayerPlotStyle plot = plotStyleForWidthPreset(layer->plot, presetId);
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateLayerPlotStyleCommand{layer->id, plot});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::createLayer()
{
    const QString id = nextLayerId(m_document);
    const QString name = QStringLiteral("Layer %1").arg(m_document.layers.size() + 1);
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        CreateLayerCommand{makeDraftingLayer(toStdString(id), toStdString(name), static_cast<int>(m_document.layers.size())), true});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::renameActiveLayer(const QString &name)
{
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        RenameLayerCommand{m_document.activeLayerId, toStdString(name)});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setActiveLayerId(const QString &layerId)
{
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        SetActiveLayerCommand{toStdString(layerId)});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::moveActiveLayer(const QString &direction)
{
    int delta = 0;
    if (direction == QStringLiteral("up")) {
        delta = 1;
    } else if (direction == QStringLiteral("down")) {
        delta = -1;
    } else {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        MoveLayerCommand{m_document.activeLayerId, delta});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::moveSelectedObjectToLayer(const QString &layerId)
{
    if (!m_document.activeObjectId) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        MoveObjectToLayerCommand{*m_document.activeObjectId, toStdString(layerId)});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::nudgeSelection(const QString &direction, const QString &stepMode)
{
    if (m_document.selectedObjectIds.empty()) {
        return false;
    }
    m_lastGuideDragSnap.clear();

    const double scale = nudgeScaleForMode(stepMode);
    const double stepX = std::max(0.000001, m_snapSettings.gridStepX > 0.0 ? m_snapSettings.gridStepX : m_snapSettings.gridStep);
    const double stepY = std::max(0.000001, m_snapSettings.gridStepY > 0.0 ? m_snapSettings.gridStepY : m_snapSettings.gridStep);

    double dx = 0.0;
    double dy = 0.0;
    if (direction == QStringLiteral("left")) {
        dx = -stepX * scale;
    } else if (direction == QStringLiteral("right")) {
        dx = stepX * scale;
    } else if (direction == QStringLiteral("up")) {
        dy = -stepY * scale;
    } else if (direction == QStringLiteral("down")) {
        dy = stepY * scale;
    } else {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{dx, dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::nudgeSelectionInsideDrawable(const QString &direction, const QString &stepMode)
{
    m_lastGuideDragSnap.clear();
    const double scale = nudgeScaleForMode(stepMode);
    const double stepX = std::max(0.000001, m_snapSettings.gridStepX > 0.0 ? m_snapSettings.gridStepX : m_snapSettings.gridStep);
    const double stepY = std::max(0.000001, m_snapSettings.gridStepY > 0.0 ? m_snapSettings.gridStepY : m_snapSettings.gridStep);

    double dx = 0.0;
    double dy = 0.0;
    if (direction == QStringLiteral("left")) {
        dx = -stepX * scale;
    } else if (direction == QStringLiteral("right")) {
        dx = stepX * scale;
    } else if (direction == QStringLiteral("up")) {
        dy = -stepY * scale;
    } else if (direction == QStringLiteral("down")) {
        dy = stepY * scale;
    } else {
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    const DraftingPlotBoundsResult selected = selectedRawPlotOutputBounds(m_document, m_document.selectedObjectIds, grid, m_plotSettings);
    if (!selected.ok
        || !isFinite(drawable)
        || selected.bounds.width > drawable.width
        || selected.bounds.height > drawable.height
        || !boundsInsideDrawable(translateBounds(selected.bounds, dx, dy), drawable)) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{dx, dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::offsetSelectedObject(const QString &sideId)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr || source->locked || objectLayerLocked(m_document, *source) || !objectEffectivelyVisible(m_document, *source)) {
        return false;
    }

    const QString id = nextObjectId(QStringLiteral("offset"), m_nextObjectSerial++);
    const DraftingOffsetResult offset = offsetDraftingObject(*source, toStdString(id), 0.05, offsetSideFromId(sideId));
    if (!offset.ok) {
        return false;
    }

    const DraftingCommandResult create = applyDraftingCommand(m_document, CreateObjectCommand{offset.object});
    if (!create.ok) {
        return false;
    }
    applyDraftingCommand(m_document, SelectObjectCommand{offset.object.id});
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::mirrorSelectedObject(const QString &axisId)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr || source->locked || objectLayerLocked(m_document, *source) || !objectEffectivelyVisible(m_document, *source)) {
        return false;
    }

    const QString id = nextObjectId(QStringLiteral("mirror"), m_nextObjectSerial++);
    const DraftingMirrorResult mirror = mirrorDraftingObject(*source, toStdString(id), mirrorAxisFromId(axisId));
    if (!mirror.ok) {
        return false;
    }

    const DraftingCommandResult create = applyDraftingCommand(m_document, CreateObjectCommand{mirror.object});
    if (!create.ok) {
        return false;
    }
    applyDraftingCommand(m_document, SelectObjectCommand{mirror.object.id});
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::repeatSelectedObject(const QString &axisId)
{
    if (axisId != QStringLiteral("x") && axisId != QStringLiteral("y")) {
        return false;
    }
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr || source->locked || objectLayerLocked(m_document, *source) || !objectEffectivelyVisible(m_document, *source)) {
        return false;
    }

    constexpr int copyCount = 3;
    std::vector<DraftingObjectId> objectIds;
    objectIds.reserve(copyCount);
    for (int index = 0; index < copyCount; ++index) {
        objectIds.push_back(toStdString(nextObjectId(QStringLiteral("repeat"), m_nextObjectSerial++)));
    }

    const double spacingX = axisId == QStringLiteral("y") ? 0.0 : 0.1;
    const double spacingY = axisId == QStringLiteral("y") ? 0.1 : 0.0;
    const DraftingArrayResult repeat = repeatDraftingObject(*source, objectIds, spacingX, spacingY);
    if (!repeat.ok) {
        return false;
    }

    std::vector<DraftingObjectId> selectedIds;
    selectedIds.reserve(repeat.objects.size());
    for (const DraftingObject &object : repeat.objects) {
        const DraftingCommandResult create = applyDraftingCommand(m_document, CreateObjectCommand{object});
        if (!create.ok) {
            return false;
        }
        selectedIds.push_back(object.id);
    }
    applyDraftingCommand(m_document, SelectObjectsCommand{selectedIds});
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::alignSelection(const QString &modeId)
{
    const std::optional<DraftingAlignmentMode> mode = alignmentModeFromId(modeId);
    if (!mode) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, AlignSelectionCommand{*mode});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::distributeSelection(const QString &axisId)
{
    const std::optional<DraftingAlignmentMode> mode = distributeModeFromAxisId(axisId);
    if (!mode) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, DistributeSelectionCommand{*mode});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::createCalibrationPattern(const QString &patternId)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr || layer->locked) {
        return false;
    }

    DraftingCalibrationPatternRequest request;
    request.kind = draftingCalibrationPatternKindFromId(toStdString(patternId));
    request.idPrefix = toStdString(nextObjectId(QStringLiteral("calibration"), m_nextObjectSerial++));
    request.layerId = m_document.activeLayerId;
    request.origin = {0.15, 0.15};
    request.size = 0.24;
    request.spacing = 0.04;
    request.lineCount = 5;

    const DraftingCalibrationPatternResult pattern = buildDraftingCalibrationPattern(request);
    if (!pattern.ok || pattern.objects.empty()) {
        return false;
    }

    std::vector<DraftingObjectId> selectedIds;
    selectedIds.reserve(pattern.objects.size());
    for (const DraftingObject &object : pattern.objects) {
        const DraftingCommandResult create = applyDraftingCommand(m_document, CreateObjectCommand{object});
        if (!create.ok) {
            return false;
        }
        selectedIds.push_back(object.id);
    }
    applyDraftingCommand(m_document, SelectObjectsCommand{selectedIds});
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::recordCalibrationMeasurement(double measuredValue)
{
    std::vector<DraftingObject> selectedObjects;
    selectedObjects.reserve(m_document.selectedObjectIds.size());
    for (const DraftingObjectId &objectId : m_document.selectedObjectIds) {
        const DraftingObject *object = findObject(m_document, objectId);
        if (object == nullptr) {
            return false;
        }
        selectedObjects.push_back(*object);
    }

    const DraftingCalibrationMeasurementResult measurement = measureDraftingCalibrationPattern(
        {std::move(selectedObjects), measuredValue, "manual_ui"});
    if (!measurement.ok) {
        return false;
    }

    DraftingDocument candidate = m_document;
    const std::string note = formatDraftingCalibrationMeasurementNote(measurement.measurement);
    for (const DraftingObjectId &objectId : measurement.measurement.objectIds) {
        const DraftingObject *object = findObject(candidate, objectId);
        if (object == nullptr) {
            return false;
        }
        ObjectMetadata metadata = object->metadata;
        metadata.measurementNote = note;
        const DraftingCommandResult result = applyDraftingCommand(candidate, UpdateMetadataCommand{objectId, metadata});
        if (!result.ok) {
            return false;
        }
    }

    m_document = std::move(candidate);
    m_latestCalibrationMeasurement = measurement.measurement;
    m_pendingCalibrationCorrection = planDraftingCalibrationCorrection(measurement.measurement);
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::applyCalibrationCorrection()
{
    if (!m_pendingCalibrationCorrection || !m_pendingCalibrationCorrection->ok) {
        return false;
    }
    if (!std::isfinite(m_pendingCalibrationCorrection->scaleFactor) || m_pendingCalibrationCorrection->scaleFactor <= 0.0) {
        return false;
    }

    m_plotSettings.calibrationScale = m_pendingCalibrationCorrection->scaleFactor;
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::fitSelectionToDrawableBounds()
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    const DraftingPlotBoundsResult selected = selectedRawPlotOutputBounds(m_document, m_document.selectedObjectIds, grid, m_plotSettings);
    if (!selected.ok
        || !isFinite(drawable)
        || selected.bounds.width > drawable.width
        || selected.bounds.height > drawable.height) {
        return false;
    }

    double dx = 0.0;
    double dy = 0.0;
    if (selected.bounds.x < drawable.x) {
        dx = drawable.x - selected.bounds.x;
    } else if (selected.bounds.x + selected.bounds.width > drawable.x + drawable.width) {
        dx = drawable.x + drawable.width - (selected.bounds.x + selected.bounds.width);
    }
    if (selected.bounds.y < drawable.y) {
        dy = drawable.y - selected.bounds.y;
    } else if (selected.bounds.y + selected.bounds.height > drawable.y + drawable.height) {
        dy = drawable.y + drawable.height - (selected.bounds.y + selected.bounds.height);
    }

    if (std::abs(dx) < 0.0000001 && std::abs(dy) < 0.0000001) {
        return true;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{dx, dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::centerSelectionInDrawable()
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    const DraftingPlotBoundsResult selected = selectedRawPlotOutputBounds(m_document, m_document.selectedObjectIds, grid, m_plotSettings);
    if (!selected.ok || !isFinite(drawable)) {
        return false;
    }

    const double targetX = drawable.x + (drawable.width - selected.bounds.width) / 2.0;
    const double targetY = drawable.y + (drawable.height - selected.bounds.height) / 2.0;
    const double dx = targetX - selected.bounds.x;
    const double dy = targetY - selected.bounds.y;
    if (std::abs(dx) < 0.0000001 && std::abs(dy) < 0.0000001) {
        return true;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{dx, dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::moveSelectionToDrawableOrigin()
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    const DraftingPlotBoundsResult selected = selectedRawPlotOutputBounds(m_document, m_document.selectedObjectIds, grid, m_plotSettings);
    if (!selected.ok || !isFinite(drawable)) {
        return false;
    }

    const double dx = drawable.x - selected.bounds.x;
    const double dy = drawable.y - selected.bounds.y;
    if (std::abs(dx) < 0.0000001 && std::abs(dy) < 0.0000001) {
        return true;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{dx, dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::moveSelectedGuideToDrawableOrigin()
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }
    const auto *guide = std::get_if<GuideGeometry>(&object->geometry);
    if (guide == nullptr) {
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    GuideGeometry next = *guide;
    next.position = guide->orientation == GuideOrientation::Horizontal
        ? grid.drawableBounds.y
        : grid.drawableBounds.x;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, next});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::centerSelectedGuideInDrawable()
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }
    const auto *guide = std::get_if<GuideGeometry>(&object->geometry);
    if (guide == nullptr) {
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    GuideGeometry next = *guide;
    next.position = guide->orientation == GuideOrientation::Horizontal
        ? grid.drawableBounds.y + grid.drawableBounds.height / 2.0
        : grid.drawableBounds.x + grid.drawableBounds.width / 2.0;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, next});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::moveSelectedGuideToDrawableMax()
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }
    const auto *guide = std::get_if<GuideGeometry>(&object->geometry);
    if (guide == nullptr) {
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    GuideGeometry next = *guide;
    next.position = guide->orientation == GuideOrientation::Horizontal
        ? grid.drawableBounds.y + grid.drawableBounds.height
        : grid.drawableBounds.x + grid.drawableBounds.width;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, next});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::offsetSelectedGuide(const QString &direction, const QString &stepMode)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }
    const auto *guide = std::get_if<GuideGeometry>(&object->geometry);
    if (guide == nullptr) {
        return false;
    }

    const double scale = nudgeScaleForMode(stepMode);
    const double stepX = std::max(0.000001, m_snapSettings.gridStepX > 0.0 ? m_snapSettings.gridStepX : m_snapSettings.gridStep);
    const double stepY = std::max(0.000001, m_snapSettings.gridStepY > 0.0 ? m_snapSettings.gridStepY : m_snapSettings.gridStep);
    double delta = 0.0;
    if (direction == QStringLiteral("negative")) {
        delta = -(guide->orientation == GuideOrientation::Horizontal ? stepY : stepX) * scale;
    } else if (direction == QStringLiteral("positive")) {
        delta = (guide->orientation == GuideOrientation::Horizontal ? stepY : stepX) * scale;
    } else {
        return false;
    }

    GuideGeometry next = *guide;
    next.position += delta;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, next});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::fitSelectedConstructionLineToDrawable()
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::ConstructionLine) {
        return false;
    }
    const auto *line = std::get_if<ConstructionLineGeometry>(&object->geometry);
    if (line == nullptr) {
        return false;
    }

    constexpr double epsilon = 0.0000001;
    const bool horizontal = std::abs(line->a.y - line->b.y) < epsilon;
    const bool vertical = std::abs(line->a.x - line->b.x) < epsilon;
    if (!horizontal && !vertical) {
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    ConstructionLineGeometry next = *line;
    if (horizontal) {
        next.a = {grid.drawableBounds.x, line->a.y};
        next.b = {grid.drawableBounds.x + grid.drawableBounds.width, line->a.y};
    } else {
        next.a = {line->a.x, grid.drawableBounds.y};
        next.b = {line->a.x, grid.drawableBounds.y + grid.drawableBounds.height};
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, next});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::createGuideFromSelectedBounds(const QString &placementId)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr
        || source->kind == DraftingShapeKind::Guide
        || source->kind == DraftingShapeKind::ConstructionLine
        || source->kind == DraftingShapeKind::Dimension
        || source->locked
        || objectLayerLocked(m_document, *source)
        || !objectEffectivelyVisible(m_document, *source)
        || !isFinite(source->bounds)) {
        return false;
    }

    GuideGeometry guide;
    const Bounds2D bounds = source->bounds;
    if (placementId == QStringLiteral("left")) {
        guide = {GuideOrientation::Vertical, bounds.x};
    } else if (placementId == QStringLiteral("right")) {
        guide = {GuideOrientation::Vertical, bounds.x + bounds.width};
    } else if (placementId == QStringLiteral("vertical_center")) {
        guide = {GuideOrientation::Vertical, bounds.x + bounds.width / 2.0};
    } else if (placementId == QStringLiteral("top")) {
        guide = {GuideOrientation::Horizontal, bounds.y};
    } else if (placementId == QStringLiteral("bottom")) {
        guide = {GuideOrientation::Horizontal, bounds.y + bounds.height};
    } else if (placementId == QStringLiteral("horizontal_center")) {
        guide = {GuideOrientation::Horizontal, bounds.y + bounds.height / 2.0};
    } else {
        return false;
    }

    if (existingGuideId(m_document, guide)) {
        return true;
    }

    const QString id = nextObjectId(QStringLiteral("guide"), m_nextObjectSerial++);
    auto built = buildDraftingObject(toStdString(id), DraftingShapeKind::Guide, guide);
    if (!built.ok) {
        return false;
    }
    built.object.layerId = source->layerId;
    built.object.metadata.toolProvenance = "bounds_guide";
    const DraftingCommandResult result = applyDraftingCommand(m_document, CreateObjectCommand{built.object});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::createOffsetGuideFromSelectedBounds(const QString &placementId, const QString &stepMode)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr
        || source->kind == DraftingShapeKind::Guide
        || source->kind == DraftingShapeKind::ConstructionLine
        || source->kind == DraftingShapeKind::Dimension
        || source->locked
        || objectLayerLocked(m_document, *source)
        || !objectEffectivelyVisible(m_document, *source)
        || !isFinite(source->bounds)) {
        return false;
    }

    const double scale = nudgeScaleForMode(stepMode);
    const double stepX = std::max(0.000001, m_snapSettings.gridStepX > 0.0 ? m_snapSettings.gridStepX : m_snapSettings.gridStep) * scale;
    const double stepY = std::max(0.000001, m_snapSettings.gridStepY > 0.0 ? m_snapSettings.gridStepY : m_snapSettings.gridStep) * scale;
    const Bounds2D bounds = source->bounds;
    GuideGeometry guide;
    if (placementId == QStringLiteral("left")) {
        guide = {GuideOrientation::Vertical, bounds.x - stepX};
    } else if (placementId == QStringLiteral("right")) {
        guide = {GuideOrientation::Vertical, bounds.x + bounds.width + stepX};
    } else if (placementId == QStringLiteral("center_x_minus")) {
        guide = {GuideOrientation::Vertical, bounds.x + bounds.width / 2.0 - stepX};
    } else if (placementId == QStringLiteral("center_x_plus")) {
        guide = {GuideOrientation::Vertical, bounds.x + bounds.width / 2.0 + stepX};
    } else if (placementId == QStringLiteral("top")) {
        guide = {GuideOrientation::Horizontal, bounds.y - stepY};
    } else if (placementId == QStringLiteral("bottom")) {
        guide = {GuideOrientation::Horizontal, bounds.y + bounds.height + stepY};
    } else if (placementId == QStringLiteral("center_y_minus")) {
        guide = {GuideOrientation::Horizontal, bounds.y + bounds.height / 2.0 - stepY};
    } else if (placementId == QStringLiteral("center_y_plus")) {
        guide = {GuideOrientation::Horizontal, bounds.y + bounds.height / 2.0 + stepY};
    } else {
        return false;
    }

    if (existingGuideId(m_document, guide)) {
        return true;
    }

    const QString id = nextObjectId(QStringLiteral("guide"), m_nextObjectSerial++);
    auto built = buildDraftingObject(toStdString(id), DraftingShapeKind::Guide, guide);
    if (!built.ok) {
        return false;
    }
    built.object.layerId = source->layerId;
    built.object.metadata.toolProvenance = "offset_bounds_guide";
    const DraftingCommandResult result = applyDraftingCommand(m_document, CreateObjectCommand{built.object});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::applyGuidePreset(const QString &presetId)
{
    const DraftingLayer *activeLayer = findLayer(m_document, m_document.activeLayerId);
    if (activeLayer == nullptr || activeLayer->locked) {
        return false;
    }

    struct PresetGuide {
        GuideGeometry geometry;
        std::string label;
        std::string color;
    };

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    if (!isFinite(drawable) || drawable.width <= 0.0 || drawable.height <= 0.0) {
        return false;
    }

    std::vector<PresetGuide> guides;
    const auto addVertical = [&](double x, std::string label, std::string color) {
        guides.push_back({GuideGeometry{GuideOrientation::Vertical, x}, std::move(label), std::move(color)});
    };
    const auto addHorizontal = [&](double y, std::string label, std::string color) {
        guides.push_back({GuideGeometry{GuideOrientation::Horizontal, y}, std::move(label), std::move(color)});
    };

    const double left = drawable.x;
    const double right = drawable.x + drawable.width;
    const double top = drawable.y;
    const double bottom = drawable.y + drawable.height;
    const double centerX = drawable.x + drawable.width / 2.0;
    const double centerY = drawable.y + drawable.height / 2.0;

    if (presetId == QStringLiteral("drawable_bounds")) {
        addVertical(left, "drawable left", "#f6c65b");
        addVertical(right, "drawable right", "#f6c65b");
        addHorizontal(top, "drawable top", "#f6c65b");
        addHorizontal(bottom, "drawable bottom", "#f6c65b");
    } else if (presetId == QStringLiteral("drawable_centerlines")) {
        addVertical(centerX, "center x", "#54d2c6");
        addHorizontal(centerY, "center y", "#54d2c6");
    } else if (presetId == QStringLiteral("thirds")) {
        addVertical(drawable.x + drawable.width / 3.0, "third x 1", "#91c89b");
        addVertical(drawable.x + drawable.width * 2.0 / 3.0, "third x 2", "#91c89b");
        addHorizontal(drawable.y + drawable.height / 3.0, "third y 1", "#91c89b");
        addHorizontal(drawable.y + drawable.height * 2.0 / 3.0, "third y 2", "#91c89b");
    } else if (presetId == QStringLiteral("quarters")) {
        addVertical(drawable.x + drawable.width / 4.0, "quarter x 1", "#83aeca");
        addVertical(centerX, "quarter x 2", "#83aeca");
        addVertical(drawable.x + drawable.width * 3.0 / 4.0, "quarter x 3", "#83aeca");
        addHorizontal(drawable.y + drawable.height / 4.0, "quarter y 1", "#83aeca");
        addHorizontal(centerY, "quarter y 2", "#83aeca");
        addHorizontal(drawable.y + drawable.height * 3.0 / 4.0, "quarter y 3", "#83aeca");
    } else if (presetId == QStringLiteral("margin_safe")) {
        addVertical(left, "safe left", "#d98b8b");
        addVertical(right, "safe right", "#d98b8b");
        addHorizontal(top, "safe top", "#d98b8b");
        addHorizontal(bottom, "safe bottom", "#d98b8b");
        addVertical(centerX, "safe center x", "#d98b8b");
        addHorizontal(centerY, "safe center y", "#d98b8b");
    } else {
        return false;
    }

    DraftingDocument candidate = m_document;
    bool changed = false;
    for (const PresetGuide &guide : guides) {
        if (existingGuideId(candidate, guide.geometry)) {
            continue;
        }

        const QString id = nextObjectId(QStringLiteral("guide"), m_nextObjectSerial++);
        auto built = buildDraftingObject(toStdString(id), DraftingShapeKind::Guide, guide.geometry);
        if (!built.ok) {
            return false;
        }
        built.object.layerId = m_document.activeLayerId;
        built.object.metadata.toolProvenance = "guide_preset";
        built.object.metadata.source = toStdString(presetId);
        built.object.metadata.guideVisual.label = guide.label;
        built.object.metadata.guideVisual.color = guide.color;
        built.object.metadata.guideVisual.dashStyle = "dash";
        built.object.metadata.guideVisual.showLabel = true;

        const DraftingCommandResult result = applyDraftingCommand(candidate, CreateObjectCommand{built.object});
        if (!result.ok) {
            return false;
        }
        changed = true;
    }

    if (changed) {
        m_document = std::move(candidate);
        emit modelChanged();
    }
    return true;
}

bool DrawingDocumentController::alignSelectionToNearestGuide(const QString &modeId)
{
    if (!m_document.activeObjectId || m_document.selectedObjectIds.empty()) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr
        || source->kind == DraftingShapeKind::Guide
        || source->kind == DraftingShapeKind::ConstructionLine
        || source->kind == DraftingShapeKind::Dimension
        || source->locked
        || objectLayerLocked(m_document, *source)
        || !objectEffectivelyVisible(m_document, *source)
        || !isFinite(source->bounds)) {
        return false;
    }

    const Bounds2D bounds = source->bounds;
    GuideOrientation orientation = GuideOrientation::Vertical;
    double target = 0.0;
    if (modeId == QStringLiteral("left")) {
        target = bounds.x;
    } else if (modeId == QStringLiteral("right")) {
        target = bounds.x + bounds.width;
    } else if (modeId == QStringLiteral("center_x")) {
        target = bounds.x + bounds.width / 2.0;
    } else if (modeId == QStringLiteral("top")) {
        orientation = GuideOrientation::Horizontal;
        target = bounds.y;
    } else if (modeId == QStringLiteral("bottom")) {
        orientation = GuideOrientation::Horizontal;
        target = bounds.y + bounds.height;
    } else if (modeId == QStringLiteral("center_y")) {
        orientation = GuideOrientation::Horizontal;
        target = bounds.y + bounds.height / 2.0;
    } else {
        return false;
    }

    const std::optional<double> guidePosition = nearestVisibleGuidePosition(m_document, orientation, target);
    if (!guidePosition) {
        return false;
    }
    const double delta = *guidePosition - target;
    if (std::abs(delta) <= 0.0000001) {
        return true;
    }

    const DraftingCommandResult result = orientation == GuideOrientation::Vertical
        ? applyDraftingCommand(m_document, MoveSelectionCommand{delta, 0.0})
        : applyDraftingCommand(m_document, MoveSelectionCommand{0.0, delta});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::deleteSelectedGuide()
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Guide) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, DeleteObjectCommand{*m_document.activeObjectId});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::deleteAllGuides()
{
    const DraftingCommandResult result = applyDraftingCommand(m_document, DeleteAllGuidesCommand{});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::mergeDuplicateGuides()
{
    const DraftingCommandResult result = applyDraftingCommand(m_document, MergeDuplicateGuidesCommand{});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setAllGuidesVisible(bool visible)
{
    const DraftingCommandResult result = applyDraftingCommand(m_document, SetAllGuidesVisibleCommand{visible});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::setAllGuidesLocked(bool locked)
{
    const DraftingCommandResult result = applyDraftingCommand(m_document, SetAllGuidesLockedCommand{locked});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

void DrawingDocumentController::clickCanvasNormalized(double x, double y)
{
    x = clamp01(x);
    y = clamp01(y);
    const Point2D point = resolveSnap({x, y}, m_document, m_snapSettings).point;
    m_lastGuideDragSnap.clear();

    if (m_selectedToolId == QStringLiteral("select_move")) {
        const DraftingHitTestResult hit = hitTestDocument(m_document, point);
        if (hit.ok) {
            applyDraftingCommand(m_document, SelectObjectCommand{hit.objectId});
        } else {
            clearSelection(m_document);
            ++m_document.revision;
        }
        m_pendingCreation.reset();
        m_previewObject.reset();
        emit modelChanged();
        return;
    }

    const DraftingToolKind kind = toolKind(m_selectedToolId);
    if (kind == DraftingToolKind::Point
        || kind == DraftingToolKind::HorizontalGuide
        || kind == DraftingToolKind::VerticalGuide
        || kind == DraftingToolKind::HorizontalConstructionLine
        || kind == DraftingToolKind::VerticalConstructionLine) {
        const QString id = nextObjectId(objectIdPrefix(kind), m_nextObjectSerial++);
        const auto object = buildDraftingObjectForTool(creationRequest(m_selectedToolId, id, m_document.activeLayerId, point, point));
        if (object.ok) {
            if (object.object.kind == DraftingShapeKind::Guide) {
                const auto *guide = std::get_if<GuideGeometry>(&object.object.geometry);
                const std::optional<DraftingObjectId> existing = guide == nullptr ? std::nullopt : existingGuideId(m_document, *guide);
                if (existing) {
                    applyDraftingCommand(m_document, SelectObjectCommand{*existing});
                    emit modelChanged();
                    return;
                }
            }
            applyDraftingCommand(m_document, CreateObjectCommand{object.object});
            applyDraftingCommand(m_document, SelectObjectCommand{object.object.id});
        }
        emit modelChanged();
        return;
    }

    if (!m_pendingCreation) {
        const QString id = nextObjectId(objectIdPrefix(kind), m_nextObjectSerial++);
        m_pendingCreation = creationRequest(m_selectedToolId, id, m_document.activeLayerId, point, point);
        m_previewObject.reset();
        emit modelChanged();
        return;
    }

    m_pendingCreation->end = point;
    const auto object = buildDraftingObjectForTool(*m_pendingCreation);
    m_pendingCreation.reset();
    m_previewObject.reset();
    if (object.ok) {
        applyDraftingCommand(m_document, CreateObjectCommand{object.object});
        applyDraftingCommand(m_document, SelectObjectCommand{object.object.id});
    }
    emit modelChanged();
}

void DrawingDocumentController::updateCreationPreviewNormalized(double x, double y)
{
    if (!m_pendingCreation) {
        return;
    }

    const Point2D point = resolveSnap({clamp01(x), clamp01(y)}, m_document, m_snapSettings).point;
    DraftingToolCreationRequest preview = *m_pendingCreation;
    preview.end = point;
    const auto object = buildDraftingObjectForTool(preview);
    if (object.ok) {
        m_previewObject = object.object;
    } else {
        m_previewObject.reset();
    }
    emit modelChanged();
}

bool DrawingDocumentController::editSelectedHandleNormalized(const QString &handleId, double x, double y)
{
    if (handleId.isEmpty() || !m_document.activeObjectId) {
        return false;
    }
    m_lastGuideDragSnap.clear();

    const Point2D point = resolveSnap({x, y}, m_document, m_snapSettings).point;
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        EditObjectHandleCommand{*m_document.activeObjectId, handleId.toStdString(), point});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::moveSelectionNormalized(double dx, double dy)
{
    if (m_document.selectedObjectIds.empty() || !std::isfinite(dx) || !std::isfinite(dy)) {
        return false;
    }

    m_lastGuideDragSnap.clear();
    if (m_guideMoveSnapEnabled && m_document.activeObjectId && containsId(m_document.selectedObjectIds, *m_document.activeObjectId)) {
        const DraftingObject *active = findObject(m_document, *m_document.activeObjectId);
        if (active != nullptr
            && active->kind != DraftingShapeKind::Guide
            && isFinite(active->bounds)) {
            MoveSnapChoice best;
            for (const MoveSnapAnchor &anchor : moveSnapAnchorsForObject(*active)) {
                const Point2D intendedAnchor {anchor.point.x + dx, anchor.point.y + dy};
                const DraftingSnapResult snap = resolveSnap(intendedAnchor, m_document, m_snapSettings);
                if (snap.sourceKind != DraftingSnapSourceKind::Guide
                    || containsId(m_document.selectedObjectIds, snap.sourceObjectId)) {
                    continue;
                }
                constexpr double epsilon = 0.000001;
                const bool movesX = std::abs(snap.point.x - intendedAnchor.x) > epsilon;
                const bool movesY = std::abs(snap.point.y - intendedAnchor.y) > epsilon;
                const MoveSnapChoice candidate {
                    true,
                    movesX && movesY,
                    distance(intendedAnchor, snap.point),
                    anchor.rank,
                    dx + snap.point.x - intendedAnchor.x,
                    dy + snap.point.y - intendedAnchor.y,
                    intendedAnchor,
                    snap.point,
                    snap.sourceObjectId,
                    anchor.label,
                };
                if (guideSnapChoiceBetter(candidate, best)) {
                    best = candidate;
                }
            }
            if (best.ok) {
                dx = best.dx;
                dy = best.dy;
                m_lastGuideDragSnap = QVariantMap{
                    {QStringLiteral("kind"), QStringLiteral("guide")},
                    {QStringLiteral("mode"), QStringLiteral("move_selection")},
                    {QStringLiteral("anchor_label"), best.anchorLabel},
                    {QStringLiteral("anchor_rank"), best.anchorRank},
                    {QStringLiteral("raw_anchor"), pointToMap(best.intendedAnchor)},
                    {QStringLiteral("snapped_anchor"), pointToMap(best.snappedAnchor)},
                    {QStringLiteral("source_object_id"), drawing_core::qStringFromStdString(best.sourceObjectId)},
                    {QStringLiteral("intersection"), best.intersection},
                    {QStringLiteral("distance"), best.distance},
                };
            }
        }
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{dx, dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::selectObjectsInBoundsNormalized(double x1, double y1, double x2, double y2)
{
    const double left = std::min(clamp01(x1), clamp01(x2));
    const double top = std::min(clamp01(y1), clamp01(y2));
    const double right = std::max(clamp01(x1), clamp01(x2));
    const double bottom = std::max(clamp01(y1), clamp01(y2));
    const Bounds2D marquee{left, top, right - left, bottom - top};

    std::vector<DraftingObjectId> objectIds;
    for (const DraftingObject &object : m_document.objects) {
        if (objectEffectivelyVisible(m_document, object) && boundsIntersect(object.bounds, marquee)) {
            objectIds.push_back(object.id);
        }
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, SelectObjectsCommand{objectIds});
    if (!result.ok) {
        return false;
    }
    emit modelChanged();
    return true;
}
