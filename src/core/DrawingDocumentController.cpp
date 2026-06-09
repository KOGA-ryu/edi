#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "core/DrawingDocumentProjection.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingArray.h"
#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingConstructionOps.h"
#include "drafting/DraftingDimensionOps.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingGuideOps.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingMirror.h"
#include "drafting/DraftingNudgeOps.h"
#include "drafting/DraftingOffset.h"
#include "drafting/DraftingPhysicalEdit.h"
#include "drafting/DraftingPlotBounds.h"
#include "drafting/DraftingPlotJob.h"
#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingQuickMeasure.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <algorithm>
#include <cmath>
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

QString resultCodeName(DraftingResultCode code)
{
    switch (code) {
    case DraftingResultCode::None:
        return QStringLiteral("none");
    case DraftingResultCode::EmptyObjectId:
        return QStringLiteral("empty_object_id");
    case DraftingResultCode::DuplicateObjectId:
        return QStringLiteral("duplicate_object_id");
    case DraftingResultCode::DuplicateLayerId:
        return QStringLiteral("duplicate_layer_id");
    case DraftingResultCode::ObjectNotFound:
        return QStringLiteral("object_not_found");
    case DraftingResultCode::LayerNotFound:
        return QStringLiteral("layer_not_found");
    case DraftingResultCode::KindGeometryMismatch:
        return QStringLiteral("kind_geometry_mismatch");
    case DraftingResultCode::InvalidGeometry:
        return QStringLiteral("invalid_geometry");
    case DraftingResultCode::InvalidSelectionTarget:
        return QStringLiteral("invalid_selection_target");
    case DraftingResultCode::InvalidMetadata:
        return QStringLiteral("invalid_metadata");
    }
    return QStringLiteral("unknown");
}

QVariantMap editStatus(bool ok, const QString &mode, const QString &fieldId, DraftingResultCode code, const QString &message)
{
    return {
        {QStringLiteral("ok"), ok},
        {QStringLiteral("mode"), mode},
        {QStringLiteral("field_id"), fieldId},
        {QStringLiteral("code"), resultCodeName(code)},
        {QStringLiteral("message"), message},
    };
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

QVariantMap quickMeasurementProjectionToMap(const DraftingQuickMeasureResult &measurement)
{
    QVariantMap result {
        {QStringLiteral("ok"), measurement.ok},
        {QStringLiteral("kind"), QString::fromLatin1(draftingQuickMeasureKindName(measurement.kind))},
        {QStringLiteral("unit"), drawing_core::qStringFromStdString(measurement.unitName)},
        {QStringLiteral("unit_label"), drawing_core::qStringFromStdString(measurement.unitLabel)},
    };
    if (!measurement.message.empty()) {
        result.insert(QStringLiteral("message"), drawing_core::qStringFromStdString(measurement.message));
    }
    if (!measurement.ok) {
        if (!measurement.objectId.empty()) {
            result.insert(QStringLiteral("object_id"), drawing_core::qStringFromStdString(measurement.objectId));
            result.insert(QStringLiteral("object_kind"), QString::fromLatin1(shapeKindName(measurement.objectKind)));
            result.insert(QStringLiteral("hit_distance"), measurement.hitDistance);
        }
        return result;
    }

    result.insert(QStringLiteral("object_id"), drawing_core::qStringFromStdString(measurement.objectId));
    result.insert(QStringLiteral("object_kind"), QString::fromLatin1(shapeKindName(measurement.objectKind)));
    result.insert(QStringLiteral("hit_distance"), measurement.hitDistance);
    result.insert(QStringLiteral("label"), drawing_core::qStringFromStdString(measurement.label));

    switch (measurement.kind) {
    case DraftingQuickMeasureKind::Point:
        result.insert(QStringLiteral("x"), measurement.x);
        result.insert(QStringLiteral("y"), measurement.y);
        result.insert(QStringLiteral("physical_x"), measurement.physicalX);
        result.insert(QStringLiteral("physical_y"), measurement.physicalY);
        break;
    case DraftingQuickMeasureKind::Line:
        result.insert(QStringLiteral("length"), measurement.length);
        result.insert(QStringLiteral("angle_deg"), measurement.angleDeg);
        result.insert(QStringLiteral("physical_length"), measurement.physicalLength);
        result.insert(QStringLiteral("physical_angle_deg"), measurement.physicalAngleDeg);
        break;
    case DraftingQuickMeasureKind::Dimension:
        result.insert(QStringLiteral("dimension_kind"), QString::fromLatin1(dimensionKindName(measurement.dimensionKind)));
        result.insert(QStringLiteral("length"), measurement.length);
        result.insert(QStringLiteral("displayed_length"), measurement.displayedLength);
        result.insert(QStringLiteral("angle_deg"), measurement.angleDeg);
        result.insert(QStringLiteral("physical_length"), measurement.physicalLength);
        result.insert(QStringLiteral("physical_displayed_length"), measurement.physicalDisplayedLength);
        result.insert(QStringLiteral("physical_angle_deg"), measurement.physicalAngleDeg);
        result.insert(QStringLiteral("offset"), measurement.offset);
        result.insert(QStringLiteral("physical_offset"), measurement.physicalOffset);
        break;
    case DraftingQuickMeasureKind::Rectangle:
        result.insert(QStringLiteral("width"), measurement.width);
        result.insert(QStringLiteral("height"), measurement.height);
        result.insert(QStringLiteral("area"), measurement.area);
        result.insert(QStringLiteral("physical_width"), measurement.physicalWidth);
        result.insert(QStringLiteral("physical_height"), measurement.physicalHeight);
        result.insert(QStringLiteral("physical_area"), measurement.physicalArea);
        break;
    case DraftingQuickMeasureKind::Circle:
        result.insert(QStringLiteral("radius"), measurement.radius);
        result.insert(QStringLiteral("diameter"), measurement.diameter);
        result.insert(QStringLiteral("physical_radius"), measurement.physicalRadius);
        result.insert(QStringLiteral("physical_diameter"), measurement.physicalDiameter);
        result.insert(QStringLiteral("physical_radius_y"), measurement.physicalRadiusY);
        result.insert(QStringLiteral("physical_diameter_y"), measurement.physicalDiameterY);
        break;
    case DraftingQuickMeasureKind::None:
    case DraftingQuickMeasureKind::Unsupported:
        break;
    }

    return result;
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
        model.insert(QStringLiteral("quick_measurement"), quickMeasurementProjectionToMap(quickMeasureAt(m_document, *m_pointerRawPoint, grid)));
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
    if (!m_lastEditStatus.isEmpty()) {
        model.insert(QStringLiteral("edit_status"), m_lastEditStatus);
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
    m_lastEditStatus.clear();
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
        m_lastEditStatus = editStatus(false, QStringLiteral("normalized"), fieldId, DraftingResultCode::InvalidGeometry, QStringLiteral("geometry edit requires a selected object, field id, and finite value"));
        emit modelChanged();
        return false;
    }

    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        m_lastEditStatus = editStatus(false, QStringLiteral("normalized"), fieldId, DraftingResultCode::ObjectNotFound, QStringLiteral("selected object does not exist"));
        emit modelChanged();
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        NumericGeometryEditCommand{*m_document.activeObjectId, toStdString(fieldId), value});
    if (!result.ok) {
        m_lastEditStatus = editStatus(false, QStringLiteral("normalized"), fieldId, result.code, drawing_core::qStringFromStdString(result.message));
        emit modelChanged();
        return false;
    }

    m_lastEditStatus = editStatus(true, QStringLiteral("normalized"), fieldId, DraftingResultCode::None, {});
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::updateSelectedObjectPhysicalGeometryField(const QString &fieldId, double value)
{
    if (fieldId.isEmpty() || !m_document.activeObjectId || !std::isfinite(value)) {
        m_lastEditStatus = editStatus(false, QStringLiteral("physical"), fieldId, DraftingResultCode::InvalidGeometry, QStringLiteral("physical edit requires a selected object, field id, and finite value"));
        emit modelChanged();
        return false;
    }

    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        m_lastEditStatus = editStatus(false, QStringLiteral("physical"), fieldId, DraftingResultCode::ObjectNotFound, QStringLiteral("selected object does not exist"));
        emit modelChanged();
        return false;
    }

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const DraftingPhysicalGeometryEditPlan plan = planPhysicalGeometryEdit(*object, grid, toStdString(fieldId), value);
    if (!plan.ok || !plan.command) {
        m_lastEditStatus = editStatus(false, QStringLiteral("physical"), fieldId, plan.code, drawing_core::qStringFromStdString(plan.message));
        emit modelChanged();
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, *plan.command);
    if (!result.ok) {
        m_lastEditStatus = editStatus(false, QStringLiteral("physical"), fieldId, result.code, drawing_core::qStringFromStdString(result.message));
        emit modelChanged();
        return false;
    }

    m_lastEditStatus = editStatus(true, QStringLiteral("physical"), fieldId, DraftingResultCode::None, {});
    emit modelChanged();
    return true;
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

bool DrawingDocumentController::setSelectedDimensionKind(const QString &kindId)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const std::optional<DimensionKind> kind = draftingDimensionKindFromId(toStdString(kindId));
    if (!kind) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr || object->kind != DraftingShapeKind::Dimension) {
        return false;
    }
    const auto *dimension = std::get_if<DimensionGeometry>(&object->geometry);
    if (dimension == nullptr) {
        return false;
    }

    const DraftingDimensionPlan plan = planDimensionKindChange(*dimension, *kind);
    if (!plan.ok) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, DraftingGeometry{plan.geometry}});
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

    const LayerPlotStyle plot = layerPlotStyleForPenPreset(layer->plot, toStdString(presetId));
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

    const LayerPlotStyle plot = layerPlotStyleForWidthPreset(layer->plot, toStdString(presetId));
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
    const DraftingLayerCreationPlan plan = planCreateDraftingLayer(m_document);
    if (!plan.ok) {
        return false;
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        CreateLayerCommand{plan.layer, plan.makeActive});
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
    const std::optional<int> delta = layerMoveDeltaFromDirection(toStdString(direction));
    if (!delta) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        MoveLayerCommand{m_document.activeLayerId, *delta});
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

    const DraftingNudgePlan plan = planNudgeDelta(toStdString(direction), m_snapSettings, toStdString(stepMode));
    if (!plan.ok) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{plan.dx, plan.dy});
    if (!result.ok) {
        return false;
    }

    emit modelChanged();
    return true;
}

bool DrawingDocumentController::nudgeSelectionInsideDrawable(const QString &direction, const QString &stepMode)
{
    m_lastGuideDragSnap.clear();
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    const DraftingPlotBoundsResult selected = selectedRawPlotOutputBounds(m_document, m_document.selectedObjectIds, grid, m_plotSettings);
    if (!selected.ok) {
        return false;
    }
    const DraftingNudgePlan plan = planNudgeInsideDrawable(toStdString(direction), m_snapSettings, toStdString(stepMode), selected.bounds, drawable);
    if (!plan.ok) {
        return false;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{plan.dx, plan.dy});
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
    const DraftingOffsetResult offset = offsetDraftingObject(*source, toStdString(id), 0.05, draftingOffsetSideFromId(toStdString(sideId)));
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
    const DraftingMirrorResult mirror = mirrorDraftingObject(*source, toStdString(id), draftingMirrorAxisFromId(toStdString(axisId)));
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
    const std::optional<DraftingAlignmentMode> mode = draftingAlignmentModeFromId(toStdString(modeId));
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
    const std::optional<DraftingAlignmentMode> mode = draftingDistributeModeFromAxisId(toStdString(axisId));
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
    const DraftingGuidePlan plan = moveGuideToDrawable(*guide, grid.drawableBounds, DraftingGuideDrawablePlacement::Origin);
    if (!plan.ok) {
        return false;
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, plan.geometry});
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
    const DraftingGuidePlan plan = moveGuideToDrawable(*guide, grid.drawableBounds, DraftingGuideDrawablePlacement::Center);
    if (!plan.ok) {
        return false;
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, plan.geometry});
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
    const DraftingGuidePlan plan = moveGuideToDrawable(*guide, grid.drawableBounds, DraftingGuideDrawablePlacement::Max);
    if (!plan.ok) {
        return false;
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, plan.geometry});
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

    const double scale = draftingNudgeScaleForMode(toStdString(stepMode));
    const double stepX = effectiveNudgeStepX(m_snapSettings);
    const double stepY = effectiveNudgeStepY(m_snapSettings);
    const DraftingGuidePlan plan = offsetGuide(*guide, toStdString(direction), stepX, stepY, scale);
    if (!plan.ok) {
        return false;
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, plan.geometry});
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

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const DraftingConstructionLinePlan plan = fitConstructionLineToDrawable(*line, grid.drawableBounds);
    if (!plan.ok) {
        return false;
    }
    const DraftingCommandResult result = applyDraftingCommand(
        m_document,
        UpdateGeometryCommand{*m_document.activeObjectId, plan.geometry});
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

    const DraftingGuidePlan plan = guideFromBoundsPlacement(source->bounds, toStdString(placementId));
    if (!plan.ok) {
        return false;
    }
    const GuideGeometry guide = plan.geometry;

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

    const double scale = draftingNudgeScaleForMode(toStdString(stepMode));
    const double stepX = effectiveNudgeStepX(m_snapSettings) * scale;
    const double stepY = effectiveNudgeStepY(m_snapSettings) * scale;
    const DraftingGuidePlan plan = offsetGuideFromBoundsPlacement(source->bounds, toStdString(placementId), stepX, stepY);
    if (!plan.ok) {
        return false;
    }
    const GuideGeometry guide = plan.geometry;

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

    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const DraftingGuidePresetPlan preset = guidePresetForDrawable(toStdString(presetId), grid.drawableBounds);
    if (!preset.ok) {
        return false;
    }

    DraftingDocument candidate = m_document;
    bool changed = false;
    for (const DraftingGuidePresetGuide &guide : preset.guides) {
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

    const DraftingGuideAlignmentPlan plan = alignBoundsToNearestGuide(m_document, source->bounds, toStdString(modeId));
    if (!plan.ok) {
        return false;
    }
    if (std::abs(plan.dx) <= 0.0000001 && std::abs(plan.dy) <= 0.0000001) {
        return true;
    }

    const DraftingCommandResult result = applyDraftingCommand(m_document, MoveSelectionCommand{plan.dx, plan.dy});
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
    m_lastEditStatus.clear();

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
            const DraftingGuideMoveSnapPlan plan = guideMoveSnapPlan(
                m_document,
                *active,
                m_document.selectedObjectIds,
                m_snapSettings,
                dx,
                dy);
            if (plan.ok) {
                dx = plan.dx;
                dy = plan.dy;
                m_lastGuideDragSnap = QVariantMap{
                    {QStringLiteral("kind"), QStringLiteral("guide")},
                    {QStringLiteral("mode"), QStringLiteral("move_selection")},
                    {QStringLiteral("anchor_label"), drawing_core::qStringFromStdString(plan.anchorLabel)},
                    {QStringLiteral("anchor_rank"), plan.anchorRank},
                    {QStringLiteral("raw_anchor"), pointToMap(plan.intendedAnchor)},
                    {QStringLiteral("snapped_anchor"), pointToMap(plan.snappedAnchor)},
                    {QStringLiteral("source_object_id"), drawing_core::qStringFromStdString(plan.sourceObjectId)},
                    {QStringLiteral("intersection"), plan.intersection},
                    {QStringLiteral("distance"), plan.distance},
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
    m_lastEditStatus.clear();
    emit modelChanged();
    return true;
}
