#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "core/DrawingDocumentProjection.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingArray.h"
#include "drafting/DraftingCalibration.h"
#include "drafting/DraftingClipboard.h"
#include "drafting/DraftingConstructionOps.h"
#include "drafting/DraftingDimensionOps.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingGuideOps.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingMetadata.h"
#include "drafting/DraftingMirror.h"
#include "drafting/DraftingModify.h"
#include "drafting/DraftingNudgeOps.h"
#include "drafting/DraftingOffset.h"
#include "drafting/DraftingPhysicalEdit.h"
#include "drafting/DraftingPlotBounds.h"
#include "drafting/DraftingPlotJob.h"
#include "drafting/DraftingPlotPlan.h"
#include "drafting/DraftingQuickMeasure.h"
#include "drafting/DraftingGcodeOut.h"
#include "drafting/DraftingHpglOut.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingSerialize.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingSvgOut.h"
#include "drafting/DraftingToolCreation.h"

#include <QByteArray>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace {

using namespace edi::drafting;

QString nextObjectId(const QString &kind, int serial)
{
    return drawing_core::qStringFromStdString(draftingObjectIdForSerial(kind.toStdString(), serial));
}

// Object ids are "<prefix>_NNNN"; recover the trailing numeric suffix so a
// freshly opened document keeps minting unique ids above what it already holds.
int objectIdTrailingSerial(const std::string &id)
{
    std::size_t end = id.size();
    std::size_t begin = end;
    while (begin > 0 && std::isdigit(static_cast<unsigned char>(id[begin - 1]))) {
        --begin;
    }
    if (begin == end) {
        return 0;
    }
    try {
        return std::stoi(id.substr(begin, end - begin));
    } catch (...) {
        return 0;
    }
}

int highestObjectSerial(const DraftingDocument &document)
{
    int highest = 0;
    for (const auto &object : document.objects) {
        highest = std::max(highest, objectIdTrailingSerial(object.id));
    }
    return highest;
}

constexpr std::size_t kUndoStackCap = 100;

// True when `before` and `after` are identical apart from selection state.
// Pure-selection changes still bump revision, so undo uses this to exclude them
// (matching most CAD). The comparison reuses the document serializer, masking
// the selection/revision fields, so it covers every other field exhaustively.
bool documentsDifferOnlyBySelection(const DraftingDocument &before, const DraftingDocument &after)
{
    DraftingDocument a = before;
    DraftingDocument b = after;
    a.selectedObjectIds.clear();
    a.activeObjectId.reset();
    a.revision = 0;
    b.selectedObjectIds.clear();
    b.activeObjectId.reset();
    b.revision = 0;
    return encodeDraftingDocument(a) == encodeDraftingDocument(b);
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
        {QStringLiteral("inside_drawable"), boundsContainsPoint(grid.drawableBounds, snap.point)},
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
    // Bucket once, look up per object: the inner scan over plan.warnings was
    // O(objects x warnings) — quadratic exactly in the bulk regime.
    QHash<QString, QVariantList> warningsByObject;
    for (const DraftingPlotWarning &warning : plan.warnings) {
        warningsByObject[drawing_core::qStringFromStdString(warning.objectId)].push_back(QVariantMap{
            {QStringLiteral("kind"), drawing_core::qStringFromStdString(warning.kind)},
            {QStringLiteral("message"), drawing_core::qStringFromStdString(warning.message)},
        });
    }

    QVariantList objects = model.value(QStringLiteral("drawing_objects")).toList();
    for (QVariant &objectValue : objects) {
        QVariantMap object = objectValue.toMap();
        const QString objectId = object.value(QStringLiteral("id")).toString();
        const QVariantList objectWarnings = warningsByObject.value(objectId);
        bool rawOutsideDrawable = false;
        bool calibratedOutsideDrawable = false;
        for (const QVariant &warningValue : objectWarnings) {
            const QString kind = warningValue.toMap().value(QStringLiteral("kind")).toString();
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
    applyDraftingGridToSnapSettings(m_snapSettings, m_gridSettings);
    // Fresh document: current and saved epochs both 0 -> clean.

    // One funnel invalidates the projection cache: every modelChanged
    // emission, wherever it comes from. Connecting to our own signal beats
    // sprinkling ++generation at thirty emit sites — an emit site added
    // tomorrow invalidates correctly by construction.
    connect(this, &DrawingDocumentController::modelChanged, this, [this]() { ++m_modelGeneration; });
}

bool DrawingDocumentController::isDocumentDirty() const
{
    // O(1) and never false-clean: epochs are monotonic, so equality means the
    // current state IS the saved state (not merely a revision-number collision).
    return m_documentEpoch != m_savedEpoch;
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    // Two-part projection: the DOCUMENT-shaped model is expensive (full
    // object projection, plot plan, safety annotation, selection bounds)
    // and changes only with modelChanged — so it is cached against the
    // generation counter. The VOLATILE keys (pointer, preview ghost, quick
    // measure, drag intent, calibration, edit status) ride per-call on a
    // shallow copy: QVariantMap is implicitly shared, so the copy is cheap
    // and inserting overlay keys never mutates the cache.
    if (m_cachedModelGeneration != m_modelGeneration) {
        const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
        QVariantMap model = drawing_core::draftingDocumentToModelProjection(m_document, m_snapSettings, &grid, nullptr);
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
        model.insert(QStringLiteral("warnings"), plotWarningsToList(plotPlan.warnings));
        m_cachedDocumentModel = model;
        m_cachedGrid = grid;
        m_cachedModelGeneration = m_modelGeneration;
    }

    QVariantMap model = m_cachedDocumentModel;
    if (m_previewObject) {
        // The ghost tracks the cursor; it joins per-call so a creation drag
        // never invalidates the document cache.
        model.insert(QStringLiteral("preview_object"),
                     drawing_core::draftingObjectToCanvasProjection(*m_previewObject, &m_cachedGrid, false));
    }
    if (m_pointerRawPoint) {
        model.insert(QStringLiteral("pointer"), pointerProjectionToMap(*m_pointerRawPoint, m_document, m_snapSettings, m_cachedGrid));
        model.insert(QStringLiteral("quick_measurement"), quickMeasurementProjectionToMap(quickMeasureAt(m_document, *m_pointerRawPoint, m_cachedGrid)));
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
    if (m_pointCapture) {
        // Volatile view state, like the pointer/preview: a prompt the canvas
        // shows while it waits for the capture click. Rides per-call so arming
        // a pick never invalidates the document cache.
        model.insert(QStringLiteral("awaiting_point_capture"), true);
        model.insert(QStringLiteral("point_capture_prompt"), m_pointCapture->prompt);
    }
    return model;
}

bool DrawingDocumentController::saveDocument(const QUrl &url)
{
    const edi::formats::ByteBuffer bytes = encodeDraftingDocument(m_document);
    const QByteArray payload(reinterpret_cast<const char *>(bytes.data()),
                             static_cast<int>(bytes.size()));
    const QVariantMap result = m_store.save(url, payload);
    const bool ok = result.value(QStringLiteral("ok")).toBool();
    if (ok) {
        m_savedEpoch = m_documentEpoch; // the written state is now the clean one
    }
    return ok;
}

bool DrawingDocumentController::openDocument(const QUrl &url)
{
    const QVariantMap result = m_store.open(url);
    if (!result.value(QStringLiteral("ok")).toBool()) {
        return false;
    }
    const QByteArray payload = result.value(QStringLiteral("bytes")).toByteArray();
    const edi::formats::ByteBuffer bytes(payload.begin(), payload.end());
    auto decoded = decodeDraftingDocument(bytes, url.toString().toStdString());
    if (!decoded.ok || !decoded.value) {
        return false;
    }

    m_document = std::move(*decoded.value);
    // Resume id minting above the highest suffix already present so newly
    // created objects never collide with loaded ones.
    m_nextObjectSerial = highestObjectSerial(m_document) + 1;
    // A freshly loaded document has no in-flight creation, preview, or status,
    // and starts a fresh undo history.
    m_pendingCreation.reset();
    m_previewObject.reset();
    m_lastGuideDragSnap.clear();
    m_lastEditStatus.clear();
    // Opening replaces the whole document; abandon any in-flight drag bracket.
    m_interactiveEditActive = false;
    m_undoStack.clear();
    m_redoStack.clear();
    // A freshly loaded document is the clean baseline: reset the epoch space so
    // current == saved, and future edits mint epochs above it.
    m_documentEpoch = 0;
    m_savedEpoch = 0;
    m_nextEpoch = 1;
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::exportSvgDocument(const QUrl &url)
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const DraftingPlotJob job = buildDraftingPlotJob(m_document, grid, m_plotSettings);
    const std::string svg = svgFromPlotJob(job, grid);
    const QVariantMap result = m_store.exportText(url, drawing_core::qStringFromStdString(svg));
    return result.value(QStringLiteral("ok")).toBool();
}

bool DrawingDocumentController::exportHpglDocument(const QUrl &url)
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const DraftingPlotJob job = buildDraftingPlotJob(m_document, grid, m_plotSettings);
    const std::string hpgl = hpglFromPlotJob(job, grid);
    const QVariantMap result = m_store.exportText(url, drawing_core::qStringFromStdString(hpgl));
    return result.value(QStringLiteral("ok")).toBool();
}

bool DrawingDocumentController::exportGcodeDocument(const QUrl &url)
{
    // Same plot-job pipeline as HPGL/SVG, different emitter — the plot job is
    // the one source of truth for what the machine draws.
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const DraftingPlotJob job = buildDraftingPlotJob(m_document, grid, m_plotSettings);
    const std::string gcode = gcodeFromPlotJob(job, grid);
    const QVariantMap result = m_store.exportText(url, drawing_core::qStringFromStdString(gcode));
    return result.value(QStringLiteral("ok")).toBool();
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

bool DrawingDocumentController::intersectionSnapEnabled() const
{
    return m_snapSettings.intersectionEnabled;
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
    return QString::fromLatin1(draftingSnapTolerancePresetId(m_snapSettings.objectTolerance));
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
    m_pointCapture.reset(); // switching tools cancels an armed point pick
    m_lastGuideDragSnap.clear();
    m_lastEditStatus.clear();
    emit modelChanged();
}

void DrawingDocumentController::setPolygonSides(int sides)
{
    m_polygonSides = std::clamp(sides, 3, 24);
}

int DrawingDocumentController::polygonSides() const
{
    return m_polygonSides;
}

void DrawingDocumentController::setRectCornerRadius(double radius)
{
    // Clamp non-finite/negative to 0 (a box); the geometry validator would
    // reject anything else, but normalizing here keeps the tool option clean.
    m_rectCornerRadius = std::isfinite(radius) && radius > 0.0 ? radius : 0.0;
}

double DrawingDocumentController::rectCornerRadius() const
{
    return m_rectCornerRadius;
}

void DrawingDocumentController::setRectInset(double inset)
{
    m_rectInset = std::isfinite(inset) && inset > 0.0 ? inset : 0.0;
}

double DrawingDocumentController::rectInset() const
{
    return m_rectInset;
}

void DrawingDocumentController::setFixedRadius(double radius)
{
    // Same normalization as the rectangle options: invalid means "off"
    // (gesture-sized), never a rejected build later. Clamped to the unit
    // document space so the stored state always equals the radius the build
    // actually stamps (resolveToolRadius clamps the same way).
    m_fixedRadius = std::isfinite(radius) && radius > 0.0 ? std::min(radius, 1.0) : 0.0;
}

double DrawingDocumentController::fixedRadius() const
{
    return m_fixedRadius;
}

void DrawingDocumentController::setFilletRadius(double radius)
{
    // Unlike fixedRadius, 0 is not a meaningful fillet (a zero arc), so an
    // invalid value leaves the current radius untouched rather than disabling.
    if (std::isfinite(radius) && radius > 0.0) {
        m_filletRadius = std::min(radius, 1.0);
    }
}

double DrawingDocumentController::filletRadius() const
{
    return m_filletRadius;
}

void DrawingDocumentController::setArrayCount(int count)
{
    m_arrayCount = std::clamp(count, 1, 99);
}

int DrawingDocumentController::arrayCount() const
{
    return m_arrayCount;
}

void DrawingDocumentController::setArraySpacingX(double spacing)
{
    // Negative spacing is legal (the array marches left/up); non-finite
    // input is normalized away (std::clamp on NaN would be UB), and the
    // magnitude clamps to the unit document space — the same range the
    // spins can represent, so state and UI cannot disagree.
    m_arraySpacingX = std::isfinite(spacing) ? std::clamp(spacing, -1.0, 1.0) : 0.0;
}

double DrawingDocumentController::arraySpacingX() const
{
    return m_arraySpacingX;
}

void DrawingDocumentController::setArraySpacingY(double spacing)
{
    m_arraySpacingY = std::isfinite(spacing) ? std::clamp(spacing, -1.0, 1.0) : 0.0;
}

double DrawingDocumentController::arraySpacingY() const
{
    return m_arraySpacingY;
}

void DrawingDocumentController::setAspectLockEnabled(bool enabled)
{
    m_aspectLockEnabled = enabled;
}

bool DrawingDocumentController::aspectLockEnabled() const
{
    return m_aspectLockEnabled;
}

void DrawingDocumentController::setSnapFlag(bool DraftingSnapSettings::*flag, bool enabled)
{
    if (m_snapSettings.*flag == enabled) {
        return;
    }
    m_snapSettings.*flag = enabled;
    emit modelChanged();
}

void DrawingDocumentController::setGridSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::gridEnabled, enabled);
}

void DrawingDocumentController::setObjectSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::objectSnapEnabled, enabled);
}

void DrawingDocumentController::setEndpointSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::endpointEnabled, enabled);
}

void DrawingDocumentController::setVertexSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::vertexEnabled, enabled);
}

void DrawingDocumentController::setMidpointSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::midpointEnabled, enabled);
}

void DrawingDocumentController::setCenterSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::centerEnabled, enabled);
}

void DrawingDocumentController::setIntersectionSnapEnabled(bool enabled)
{
    setSnapFlag(&DraftingSnapSettings::intersectionEnabled, enabled);
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
    setSnapFlag(&DraftingSnapSettings::objectPriorityBeforeGrid, enabled);
}

void DrawingDocumentController::setObjectSnapTolerancePreset(QString presetId)
{
    const double tolerance = draftingSnapToleranceForPreset(toStdString(presetId));
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
    commitGridSettings(draftingGridPresetSettings(preset));
}

void DrawingDocumentController::commitGridSettings(DraftingGridSettings settings)
{
    m_gridSettings = sanitizeDraftingGridSettings(settings);
    applyDraftingGridToSnapSettings(m_snapSettings, m_gridSettings);
    emit modelChanged();
}

void DrawingDocumentController::commitCustomGridSettings(DraftingGridSettings settings)
{
    settings.preset = DraftingGridPreset::Custom;
    commitGridSettings(settings);
}

void DrawingDocumentController::setGridUnitId(const QString &unitId)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.unit = draftingGridUnitFromName(toStdString(unitId));
    commitCustomGridSettings(settings);
}

void DrawingDocumentController::setGridSize(double width, double height)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.width = width;
    settings.height = height;
    commitCustomGridSettings(settings);
}

void DrawingDocumentController::setGridMargins(double left, double top, double right, double bottom)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.marginLeft = left;
    settings.marginTop = top;
    settings.marginRight = right;
    settings.marginBottom = bottom;
    commitCustomGridSettings(settings);
}

void DrawingDocumentController::setGridStep(double minorStep)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.minorStep = minorStep;
    commitCustomGridSettings(settings);
}

void DrawingDocumentController::setGridMajorLineEvery(int majorLineEvery)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.majorLineEvery = majorLineEvery;
    commitCustomGridSettings(settings);
}

void DrawingDocumentController::setGridVisible(bool visible)
{
    DraftingGridSettings settings = m_gridSettings;
    settings.visible = visible;
    commitCustomGridSettings(settings);
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
    const Point2D point = normalizeDraftingPoint({x, y});
    if (m_pointerRawPoint && m_pointerRawPoint->x == point.x && m_pointerRawPoint->y == point.y) {
        return;
    }
    m_pointerRawPoint = point;
    emit pointerChanged(); // movement, not mutation
}

bool DrawingDocumentController::finishEdit(const QString &mode, const QString &fieldId, bool ok,
                                           DraftingResultCode code, const QString &message)
{
    m_lastEditStatus = editStatus(ok, mode, fieldId, code, message);
    emit modelChanged();
    return ok;
}

bool DrawingDocumentController::applyFieldEdit(
    const QString &mode,
    const QString &invalidMessage,
    const QString &fieldId,
    double value,
    const std::function<DraftingPhysicalGeometryEditPlan(const DraftingObject &)> &planEdit)
{
    if (fieldId.isEmpty() || !m_document.activeObjectId || !std::isfinite(value)) {
        return finishEdit(mode, fieldId, false, DraftingResultCode::InvalidGeometry, invalidMessage);
    }

    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return finishEdit(mode, fieldId, false, DraftingResultCode::ObjectNotFound, QStringLiteral("selected object does not exist"));
    }

    const DraftingPhysicalGeometryEditPlan plan = planEdit(*object);
    if (!plan.ok || !plan.command) {
        return finishEdit(mode, fieldId, false, plan.code, drawing_core::qStringFromStdString(plan.message));
    }

    beginEdit();
    const DraftingCommandResult result = applyDraftingCommand(m_document, *plan.command);
    if (!result.ok) {
        return finishEdit(mode, fieldId, false, result.code, drawing_core::qStringFromStdString(result.message));
    }

    commitEdit();
    return finishEdit(mode, fieldId, true, DraftingResultCode::None, {});
}

bool DrawingDocumentController::updateSelectedObjectGeometryField(const QString &fieldId, double value)
{
    return applyFieldEdit(
        QStringLiteral("normalized"),
        QStringLiteral("geometry edit requires a selected object, field id, and finite value"),
        fieldId,
        value,
        [&](const DraftingObject &) {
            return DraftingPhysicalGeometryEditPlan::accepted(
                NumericGeometryEditCommand{*m_document.activeObjectId, toStdString(fieldId), value});
        });
}

bool DrawingDocumentController::updateSelectedObjectPhysicalGeometryField(const QString &fieldId, double value)
{
    return applyFieldEdit(
        QStringLiteral("physical"),
        QStringLiteral("physical edit requires a selected object, field id, and finite value"),
        fieldId,
        value,
        [&](const DraftingObject &object) {
            const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
            return planPhysicalGeometryEdit(object, grid, toStdString(fieldId), value);
        });
}

bool DrawingDocumentController::setSelectedObjectStrokeColor(const QString &color)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    const QString trimmed = color.trimmed();
    // Free-string color, validated at the gate with the SAME check the plot
    // readiness uses (#rrggbb): empty = inherit; junk would otherwise
    // persist, paint black, and emit invalid SVG with no diagnostic.
    if (!trimmed.isEmpty() && !draftingStrokeColorIsValid(trimmed.toStdString())) {
        return false;
    }
    StrokeStyle stroke = object->stroke;
    stroke.color = trimmed.toStdString();
    return applyCommandAndEmit(UpdateStrokeStyleCommand{*m_document.activeObjectId, stroke});
}

bool DrawingDocumentController::setSelectedObjectStrokeWidth(double width)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    StrokeStyle stroke = object->stroke;
    stroke.width = width;
    return applyCommandAndEmit(UpdateStrokeStyleCommand{*m_document.activeObjectId, stroke});
}

bool DrawingDocumentController::setSelectedObjectStrokeOpacity(double opacity)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    if (!std::isfinite(opacity)) {
        return false;
    }
    StrokeStyle stroke = object->stroke;
    // Unlike width, 0 is NOT an inherit sentinel here — a fully transparent
    // stroke is a legal style (layers have no opacity to inherit from).
    stroke.opacity = std::clamp(opacity, 0.0, 1.0);
    return applyCommandAndEmit(UpdateStrokeStyleCommand{*m_document.activeObjectId, stroke});
}

bool DrawingDocumentController::setSelectedObjectFillColor(const QString &color)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    const QString trimmed = color.trimmed();
    // Same #rrggbb gate as stroke: empty is allowed (no fill colour chosen);
    // junk would persist and paint an undefined brush.
    if (!trimmed.isEmpty() && !draftingStrokeColorIsValid(trimmed.toStdString())) {
        return false;
    }
    FillStyle fill = object->fill;
    fill.color = trimmed.toStdString();
    return applyCommandAndEmit(UpdateFillStyleCommand{*m_document.activeObjectId, fill});
}

bool DrawingDocumentController::setSelectedObjectTextContent(const QString &content)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    const auto *text = std::get_if<TextAnnotationGeometry>(&object->geometry);
    if (text == nullptr) {
        return false; // content edits only apply to a text annotation
    }
    // Content is geometry, so it rides UpdateGeometryCommand (the whole-geometry
    // replace) rather than a style command — one undo step, one choke point.
    TextAnnotationGeometry updated = *text;
    updated.content = content.toStdString();
    return applyCommandAndEmit(UpdateGeometryCommand{*m_document.activeObjectId, DraftingGeometry{updated}});
}

bool DrawingDocumentController::setSelectedObjectFillOpacity(double opacity)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    if (!std::isfinite(opacity)) {
        return false;
    }
    FillStyle fill = object->fill;
    // 0 = transparent (no fill) is the legal default; this is the only fill axis
    // exposed today, the colour rides setSelectedObjectFillColor.
    fill.opacity = std::clamp(opacity, 0.0, 1.0);
    return applyCommandAndEmit(UpdateFillStyleCommand{*m_document.activeObjectId, fill});
}

bool DrawingDocumentController::setSelectedObjectLineStyle(const QString &lineStyle)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *object = findObject(m_document, *m_document.activeObjectId);
    if (object == nullptr) {
        return false;
    }
    StrokeStyle stroke = object->stroke;
    stroke.lineStyle = lineStyle.toStdString();
    return applyCommandAndEmit(UpdateStrokeStyleCommand{*m_document.activeObjectId, stroke});
}

bool DrawingDocumentController::setSelectedObjectLocked(bool locked)
{
    const DraftingObject *object = activeObject(m_document);
    if (object == nullptr) {
        return false;
    }

    return applyCommandAndEmit(UpdateObjectFlagsCommand{*m_document.activeObjectId, locked, object->visible});
}

bool DrawingDocumentController::setSelectedObjectVisible(bool visible)
{
    const DraftingObject *object = activeObject(m_document);
    if (object == nullptr) {
        return false;
    }

    return applyCommandAndEmit(UpdateObjectFlagsCommand{*m_document.activeObjectId, object->locked, visible});
}

bool DrawingDocumentController::applyCommandAndEmit(const DraftingCommand &command)
{
    beginEdit();
    const DraftingCommandResult result = applyDraftingCommand(m_document, command);
    if (!result.ok) {
        return false;
    }

    // The variant IS the classification: no serialize-and-diff needed to
    // know whether this was a selection command.
    const bool selectionOnly = std::holds_alternative<SelectObjectCommand>(command)
        || std::holds_alternative<SelectObjectsCommand>(command);
    commitEdit(selectionOnly);
    emit modelChanged();
    return true;
}

void DrawingDocumentController::pushUndoState(const DraftingDocument &before, std::uint64_t epochBefore)
{
    m_undoStack.push_back({before, epochBefore});
    if (m_undoStack.size() > kUndoStackCap) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear();
    // The post-mutation state is genuinely new: give it a never-reused epoch so
    // dirty tracking can't alias it against an older state of the same revision.
    m_documentEpoch = m_nextEpoch++;
}

void DrawingDocumentController::beginEdit()
{
    m_editBefore = m_document;
    m_editEpochBefore = m_documentEpoch;
    m_editCommitted = false;
}

void DrawingDocumentController::commitEdit(std::optional<bool> selectionOnly)
{
    if (m_editCommitted) {
        return;
    }
    m_editCommitted = true;
    if (m_interactiveEditActive) {
        return; // inside a gesture: one undo step is pushed by endInteractiveEdit
    }
    if (m_editBefore.revision == m_document.revision) {
        return; // no document mutation
    }
    // Pure selection is not undoable and must not clear the redo stack.
    // When the caller classified the command, trust it; only unclassified
    // brackets pay the encode-and-compare (two full serializations).
    if (selectionOnly.value_or(false)
        || (!selectionOnly.has_value() && documentsDifferOnlyBySelection(m_editBefore, m_document))) {
        return;
    }
    pushUndoState(m_editBefore, m_editEpochBefore);
}

void DrawingDocumentController::beginInteractiveEdit()
{
    // Always (re)capture the baseline. If a previous bracket leaked — a gesture
    // whose mouse-release never arrived, or one interrupted by undo/redo/open —
    // the stale m_interactiveBefore must not survive into this fresh gesture, so
    // we overwrite it rather than early-returning on the active flag.
    m_interactiveBefore = m_document;
    m_interactiveEpochBefore = m_documentEpoch;
    m_interactiveEditActive = true;
}

void DrawingDocumentController::endInteractiveEdit()
{
    if (!m_interactiveEditActive) {
        return;
    }
    m_interactiveEditActive = false;
    // Push the whole gesture as a single undo step, if it changed geometry.
    if (m_interactiveBefore.revision == m_document.revision) {
        return;
    }
    if (documentsDifferOnlyBySelection(m_interactiveBefore, m_document)) {
        return;
    }
    pushUndoState(m_interactiveBefore, m_interactiveEpochBefore);
}

bool DrawingDocumentController::canUndo() const
{
    return !m_undoStack.empty();
}

bool DrawingDocumentController::canRedo() const
{
    return !m_redoStack.empty();
}

bool DrawingDocumentController::undo()
{
    if (m_undoStack.empty()) {
        return false;
    }
    DocumentSnapshot restored = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    m_redoStack.push_back({m_document, m_documentEpoch});
    m_document = std::move(restored.document);
    m_documentEpoch = restored.epoch; // restore the state's own epoch, not a new one
    m_pendingCreation.reset();
    m_previewObject.reset();
    m_lastGuideDragSnap.clear();
    m_lastEditStatus.clear();
    // Undo replaces the whole document; any in-flight drag bracket is now stale.
    m_interactiveEditActive = false;
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::redo()
{
    if (m_redoStack.empty()) {
        return false;
    }
    DocumentSnapshot restored = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    m_undoStack.push_back({m_document, m_documentEpoch});
    m_document = std::move(restored.document);
    m_documentEpoch = restored.epoch;
    m_pendingCreation.reset();
    m_previewObject.reset();
    m_lastGuideDragSnap.clear();
    m_lastEditStatus.clear();
    // Redo replaces the whole document; any in-flight drag bracket is now stale.
    m_interactiveEditActive = false;
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::applyActiveObjectMetadataUpdate(
    DraftingShapeKind kind,
    const std::function<DraftingMetadataUpdatePlan(const ObjectMetadata &)> &planMetadata)
{
    const DraftingObject *object = activeObjectOfKind(m_document, kind);
    if (object == nullptr) {
        return false;
    }

    const DraftingMetadataUpdatePlan plan = planMetadata(object->metadata);
    if (!plan.ok) {
        return false;
    }
    return applyCommandAndEmit(UpdateMetadataCommand{*m_document.activeObjectId, plan.metadata});
}

bool DrawingDocumentController::applyActiveObjectMetadataUpdate(
    const std::function<DraftingMetadataUpdatePlan(const ObjectMetadata &)> &planMetadata)
{
    // Kind-agnostic twin of the above: resolves the active object regardless
    // of shape, for the object-wide metadata fields.
    const DraftingObject *object = activeObject(m_document);
    if (object == nullptr) {
        return false;
    }
    const DraftingMetadataUpdatePlan plan = planMetadata(object->metadata);
    if (!plan.ok) {
        return false;
    }
    return applyCommandAndEmit(UpdateMetadataCommand{*m_document.activeObjectId, plan.metadata});
}

bool DrawingDocumentController::setSelectedObjectRole(const QString &roleId)
{
    return applyActiveObjectMetadataUpdate([&](const ObjectMetadata &metadata) {
        return planObjectRoleUpdate(metadata, objectRoleFromName(toStdString(roleId)));
    });
}

bool DrawingDocumentController::setSelectedObjectMaterial(const QString &material)
{
    return applyActiveObjectMetadataUpdate([&](const ObjectMetadata &metadata) {
        return planObjectMaterialUpdate(metadata, toStdString(material));
    });
}

bool DrawingDocumentController::setSelectedObjectExportGroup(const QString &exportGroup)
{
    return applyActiveObjectMetadataUpdate([&](const ObjectMetadata &metadata) {
        return planObjectExportGroupUpdate(metadata, toStdString(exportGroup));
    });
}

bool DrawingDocumentController::setSelectedObjectTags(const QStringList &tags)
{
    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(tags.size()));
    for (const QString &tag : tags) {
        const QString trimmed = tag.trimmed();
        if (!trimmed.isEmpty()) { // a stray comma must not mint an empty tag
            values.push_back(toStdString(trimmed));
        }
    }
    return applyActiveObjectMetadataUpdate([&](const ObjectMetadata &metadata) {
        return planObjectTagsUpdate(metadata, values);
    });
}

bool DrawingDocumentController::applyActiveObjectGeometryUpdate(
    DraftingShapeKind kind,
    const std::function<std::optional<DraftingGeometry>(const DraftingObject &)> &planGeometry)
{
    const DraftingObject *object = activeObjectOfKind(m_document, kind);
    if (object == nullptr) {
        return false;
    }

    const std::optional<DraftingGeometry> geometry = planGeometry(*object);
    if (!geometry) {
        return false;
    }
    return applyCommandAndEmit(UpdateGeometryCommand{*m_document.activeObjectId, *geometry});
}

template <typename Geometry>
bool DrawingDocumentController::applyActiveGeometryPlan(
    const std::function<std::optional<DraftingGeometry>(const Geometry &)> &planGeometry)
{
    return applyActiveObjectGeometryUpdate(shapeKindOf<Geometry>(), [&](const DraftingObject &object) -> std::optional<DraftingGeometry> {
        const auto *geometry = std::get_if<Geometry>(&object.geometry);
        if (geometry == nullptr) {
            return std::nullopt;
        }
        return planGeometry(*geometry);
    });
}

bool DrawingDocumentController::setSelectedGuideLabel(const QString &label)
{
    return applyActiveObjectMetadataUpdate(DraftingShapeKind::Guide, [&](const ObjectMetadata &metadata) {
        return planGuideVisualLabelUpdate(metadata, toStdString(label));
    });
}

bool DrawingDocumentController::setSelectedGuideColor(const QString &color)
{
    return applyActiveObjectMetadataUpdate(DraftingShapeKind::Guide, [&](const ObjectMetadata &metadata) {
        return planGuideVisualColorUpdate(metadata, toStdString(color));
    });
}

bool DrawingDocumentController::setSelectedGuideDashStyle(const QString &dashStyle)
{
    return applyActiveObjectMetadataUpdate(DraftingShapeKind::Guide, [&](const ObjectMetadata &metadata) {
        return planGuideVisualDashStyleUpdate(metadata, toStdString(dashStyle));
    });
}

bool DrawingDocumentController::setSelectedGuideLabelVisible(bool visible)
{
    return applyActiveObjectMetadataUpdate(DraftingShapeKind::Guide, [&](const ObjectMetadata &metadata) {
        return planGuideVisualLabelVisibleUpdate(metadata, visible);
    });
}

bool DrawingDocumentController::setSelectedDimensionKind(const QString &kindId)
{
    const std::optional<DimensionKind> kind = draftingDimensionKindFromId(toStdString(kindId));
    if (!kind) {
        return false;
    }
    return applyActiveGeometryPlan<DimensionGeometry>([&](const DimensionGeometry &dimension) -> std::optional<DraftingGeometry> {
        const DraftingDimensionPlan plan = planDimensionKindChange(dimension, *kind);
        if (!plan.ok) {
            return std::nullopt;
        }
        return DraftingGeometry{plan.geometry};
    });
}

bool DrawingDocumentController::setSelectedDimensionLabelVisible(bool visible)
{
    return applyActiveObjectMetadataUpdate(DraftingShapeKind::Dimension, [&](const ObjectMetadata &metadata) {
        return planDimensionVisualLabelVisibleUpdate(metadata, visible);
    });
}

bool DrawingDocumentController::applyLayerFlagsUpdate(
    const LayerId &layerId,
    DraftingLayerFlagsPlan (*planFlags)(const DraftingLayer &, bool),
    bool value)
{
    const DraftingLayer *layer = findLayer(m_document, layerId);
    if (layer == nullptr) {
        return false;
    }
    const DraftingLayerFlagsPlan plan = planFlags(*layer, value);
    if (!plan.ok) {
        return false;
    }

    return applyCommandAndEmit(UpdateLayerFlagsCommand{plan.layerId, plan.locked, plan.visible});
}

bool DrawingDocumentController::applyActiveLayerPlotStyleUpdate(
    const std::function<LayerPlotStyle(const DraftingLayer &)> &planPlot)
{
    const DraftingLayer *layer = findLayer(m_document, m_document.activeLayerId);
    if (layer == nullptr) {
        return false;
    }

    return applyCommandAndEmit(UpdateLayerPlotStyleCommand{layer->id, planPlot(*layer)});
}

bool DrawingDocumentController::setDefaultLayerLocked(bool locked)
{
    return applyLayerFlagsUpdate("default", planLayerLockedUpdate, locked);
}

bool DrawingDocumentController::setDefaultLayerVisible(bool visible)
{
    return applyLayerFlagsUpdate("default", planLayerVisibleUpdate, visible);
}

bool DrawingDocumentController::setActiveLayerLocked(bool locked)
{
    return applyLayerFlagsUpdate(m_document.activeLayerId, planLayerLockedUpdate, locked);
}

bool DrawingDocumentController::setActiveLayerVisible(bool visible)
{
    return applyLayerFlagsUpdate(m_document.activeLayerId, planLayerVisibleUpdate, visible);
}

bool DrawingDocumentController::setActiveLayerPlotEnabled(bool enabled)
{
    return applyActiveLayerPlotStyleUpdate([&](const DraftingLayer &layer) {
        LayerPlotStyle plot = layer.plot;
        plot.plotEnabled = enabled;
        return plot;
    });
}

bool DrawingDocumentController::setActiveLayerPenPreset(const QString &presetId)
{
    return applyActiveLayerPlotStyleUpdate([&](const DraftingLayer &layer) {
        return layerPlotStyleForPenPreset(layer.plot, toStdString(presetId));
    });
}

bool DrawingDocumentController::setActiveLayerStrokeWidthPreset(const QString &presetId)
{
    return applyActiveLayerPlotStyleUpdate([&](const DraftingLayer &layer) {
        return layerPlotStyleForWidthPreset(layer.plot, toStdString(presetId));
    });
}

bool DrawingDocumentController::createLayer()
{
    const DraftingLayerCreationPlan plan = planCreateDraftingLayer(m_document);
    if (!plan.ok) {
        return false;
    }
    return applyCommandAndEmit(CreateLayerCommand{plan.layer, plan.makeActive});
}

bool DrawingDocumentController::renameActiveLayer(const QString &name)
{
    return applyCommandAndEmit(RenameLayerCommand{m_document.activeLayerId, toStdString(name)});
}

bool DrawingDocumentController::setActiveLayerId(const QString &layerId)
{
    return applyCommandAndEmit(SetActiveLayerCommand{toStdString(layerId)});
}

bool DrawingDocumentController::moveActiveLayer(const QString &direction)
{
    const std::optional<int> delta = layerMoveDeltaFromDirection(toStdString(direction));
    if (!delta) {
        return false;
    }

    return applyCommandAndEmit(MoveLayerCommand{m_document.activeLayerId, *delta});
}

bool DrawingDocumentController::moveSelectedObjectToLayer(const QString &layerId)
{
    if (!m_document.activeObjectId) {
        return false;
    }

    return applyCommandAndEmit(MoveObjectToLayerCommand{*m_document.activeObjectId, toStdString(layerId)});
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

    return applyCommandAndEmit(MoveSelectionCommand{plan.dx, plan.dy});
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

    return applyCommandAndEmit(MoveSelectionCommand{plan.dx, plan.dy});
}

bool DrawingDocumentController::createTransformedActiveObject(
    const QString &idPrefix,
    const std::function<std::optional<DraftingObject>(const DraftingObject &source, const std::string &newId)> &transform)
{
    const DraftingObject *source = activeObject(m_document);
    if (source == nullptr || !draftingObjectEffectivelyEditable(m_document, *source)) {
        return false;
    }

    const QString id = nextObjectId(idPrefix, m_nextObjectSerial++);
    const std::optional<DraftingObject> object = transform(*source, toStdString(id));
    if (!object) {
        return false;
    }

    beginEdit();
    const DraftingCommandResult create = applyDraftingCommand(m_document, CreateObjectCommand{*object});
    if (!create.ok) {
        return false;
    }
    applyDraftingCommand(m_document, SelectObjectCommand{object->id});
    commitEdit();
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::offsetSelectedObject(const QString &sideId)
{
    return createTransformedActiveObject(QStringLiteral("offset"), [&](const DraftingObject &source, const std::string &newId) -> std::optional<DraftingObject> {
        const DraftingOffsetResult offset = offsetDraftingObject(source, newId, defaultDraftingOffsetDistance(), draftingOffsetSideFromId(toStdString(sideId)));
        if (!offset.ok) {
            return std::nullopt;
        }
        return offset.object;
    });
}

bool DrawingDocumentController::mirrorSelectedObject(const QString &axisId)
{
    return createTransformedActiveObject(QStringLiteral("mirror"), [&](const DraftingObject &source, const std::string &newId) -> std::optional<DraftingObject> {
        const DraftingMirrorResult mirror = mirrorDraftingObject(source, newId, draftingMirrorAxisFromId(toStdString(axisId)));
        if (!mirror.ok) {
            return std::nullopt;
        }
        return mirror.object;
    });
}

bool DrawingDocumentController::repeatSelectedObject(const QString &axisId)
{
    const std::optional<DraftingArrayRepeatSettings> settings =
        draftingArrayRepeatSettingsFromAxisId(toStdString(axisId), m_arrayCount, m_arraySpacingX, m_arraySpacingY);
    if (!settings) {
        return false;
    }
    return createArrayFromActiveObject(QStringLiteral("repeat"), settings->copyCount,
        [&settings](const DraftingObject &source, const std::vector<DraftingObjectId> &newObjectIds) {
            return repeatDraftingObject(source, newObjectIds, settings->spacingX, settings->spacingY);
        });
}

bool DrawingDocumentController::gridArraySelectedObject()
{
    // One count spin drives both axes: a grid of count x count cells, the
    // source occupying the first. count copies-per-axis squared minus the
    // source is the planner's id budget.
    const int cells = m_arrayCount * m_arrayCount;
    return createArrayFromActiveObject(QStringLiteral("grid"), cells - 1,
        [this](const DraftingObject &source, const std::vector<DraftingObjectId> &newObjectIds) {
            return gridArrayDraftingObject(source, newObjectIds, m_arrayCount, m_arrayCount, m_arraySpacingX, m_arraySpacingY);
        });
}

bool DrawingDocumentController::beginRadialArrayCenterPick()
{
    // Arm a pick-a-point capture for the ring centre. Require a usable source
    // up front: a "click a centre" prompt with nothing to array would be a
    // dead end, worse than an immediate, honest refusal. The captured click
    // (resolvePointCapture) supplies the centre and runs the array.
    const DraftingObject *source = activeObject(m_document);
    if (source == nullptr || !draftingObjectEffectivelyEditable(m_document, *source)) {
        return false;
    }
    m_pointCapture = PendingPointCapture{PointCaptureIntent::RadialArrayCenter,
                                         QStringLiteral("Click to set the radial array centre")};
    emit pointerChanged(); // view state: the prompt appears, the document is untouched
    return true;
}

bool DrawingDocumentController::runRadialArrayAtCenter(Point2D center)
{
    // The centre is now PICKED, not the drawable centre — the user can ring
    // copies around any point (and even around the source itself, which the
    // planner still rejects as a zero arm). One operation, one undo step.
    return createArrayFromActiveObject(QStringLiteral("radial"), m_arrayCount,
        [center](const DraftingObject &source, const std::vector<DraftingObjectId> &newObjectIds) {
            return radialArrayDraftingObject(source, newObjectIds, center);
        });
}

QString DrawingDocumentController::pointCapturePrompt() const
{
    return m_pointCapture ? m_pointCapture->prompt : QString();
}

bool DrawingDocumentController::beginTrimSelectedLine()
{
    // Trim needs a LINE to act on. Arm only when one is selected and editable;
    // the captured click then chooses the part to remove (applyTrimAtPoint).
    const DraftingObject *source = activeObjectOfKind(m_document, DraftingShapeKind::Line);
    if (source == nullptr || !draftingObjectEffectivelyEditable(m_document, *source)) {
        return false;
    }
    m_pointCapture = PendingPointCapture{PointCaptureIntent::TrimPoint,
                                         QStringLiteral("Click the part of the line to trim away")};
    emit pointerChanged();
    return true;
}

void DrawingDocumentController::applyTrimAtPoint(Point2D point)
{
    const DraftingObject *target = activeObjectOfKind(m_document, DraftingShapeKind::Line);
    if (target == nullptr) {
        return; // the selection changed out from under the armed capture
    }
    const auto *targetLine = std::get_if<LineGeometry>(&target->geometry);
    if (targetLine == nullptr) {
        return;
    }
    // Every OTHER line is a candidate cutting boundary. The pure op picks the
    // crossing nearest the click and trims the line back to it.
    std::vector<LineGeometry> boundaries;
    for (const DraftingObject &object : m_document.objects) {
        if (object.id == target->id) {
            continue;
        }
        if (const auto *line = std::get_if<LineGeometry>(&object.geometry)) {
            boundaries.push_back(*line);
        }
    }
    const DraftingTrimResult result = trimLineAtPoint(*targetLine, boundaries, point);
    if (!result.ok) {
        // A dead trim click (no crossing line, or a collapse) must say why,
        // not silently no-op — the same discipline as the array rejections.
        finishEdit(QStringLiteral("trim"), drawing_core::qStringFromStdString(target->id), false,
                   result.code, drawing_core::qStringFromStdString(result.message));
        return;
    }
    applyCommandAndEmit(UpdateGeometryCommand{*m_document.activeObjectId, DraftingGeometry{result.geometry}});
}

bool DrawingDocumentController::beginFilletSelectedLine()
{
    const DraftingObject *source = activeObjectOfKind(m_document, DraftingShapeKind::Line);
    if (source == nullptr || !draftingObjectEffectivelyEditable(m_document, *source)) {
        return false;
    }
    m_pointCapture = PendingPointCapture{PointCaptureIntent::FilletSecondLine,
                                         QStringLiteral("Click the other line near the corner to round")};
    emit pointerChanged();
    return true;
}

void DrawingDocumentController::applyFilletAtPoint(Point2D point)
{
    const DraftingObject *target = activeObjectOfKind(m_document, DraftingShapeKind::Line);
    if (target == nullptr) {
        return;
    }
    const auto *targetLine = std::get_if<LineGeometry>(&target->geometry);
    if (targetLine == nullptr) {
        return;
    }
    // The click picks the OTHER line: the nearest line to it. (The pick also
    // disambiguates which corner — filletLines reads the corner off the point.)
    const DraftingObject *other = nullptr;
    const LineGeometry *otherLine = nullptr;
    double bestDistance = 0.0;
    for (const DraftingObject &object : m_document.objects) {
        if (object.id == target->id) {
            continue;
        }
        const auto *line = std::get_if<LineGeometry>(&object.geometry);
        if (line == nullptr) {
            continue;
        }
        const double distanceToLine = hitDistance(object.geometry, point);
        if (other == nullptr || distanceToLine < bestDistance) {
            other = &object;
            otherLine = line;
            bestDistance = distanceToLine;
        }
    }
    if (otherLine == nullptr) {
        finishEdit(QStringLiteral("fillet"), drawing_core::qStringFromStdString(target->id), false,
                   DraftingResultCode::InvalidSelectionTarget, QStringLiteral("fillet needs a second line"));
        return;
    }
    const DraftingFilletResult result = filletLines(*targetLine, *otherLine, m_filletRadius, point);
    if (!result.ok) {
        finishEdit(QStringLiteral("fillet"), drawing_core::qStringFromStdString(target->id), false,
                   result.code, drawing_core::qStringFromStdString(result.message));
        return;
    }
    // Atomic: trim BOTH lines and create the rounding arc as ONE undo step (the
    // same begin/commit bracket cutSelection uses for its multi-delete). Capture
    // the ids and layer up front — the pointers into m_document.objects must not
    // be read after the first command mutates the vector.
    const DraftingObjectId targetId = target->id;
    const DraftingObjectId otherId = other->id;
    const QString arcId = nextObjectId(QStringLiteral("fillet"), m_nextObjectSerial++);
    DraftingObject arc = makeDraftingObject(toStdString(arcId), DraftingShapeKind::Arc, DraftingGeometry{result.arc});
    arc.layerId = target->layerId;
    arc.bounds = computeBounds(arc.geometry);
    arc.metadata.toolProvenance = "fillet";

    beginEdit();
    applyDraftingCommand(m_document, UpdateGeometryCommand{targetId, DraftingGeometry{result.line1}});
    applyDraftingCommand(m_document, UpdateGeometryCommand{otherId, DraftingGeometry{result.line2}});
    applyDraftingCommand(m_document, CreateObjectCommand{arc});
    applyDraftingCommand(m_document, SelectObjectCommand{arc.id});
    commitEdit();
    emit modelChanged();
}

void DrawingDocumentController::resolvePointCapture(Point2D point)
{
    const PointCaptureIntent intent = m_pointCapture->intent;
    m_pointCapture.reset();
    switch (intent) {
    case PointCaptureIntent::RadialArrayCenter:
        runRadialArrayAtCenter(point);
        break;
    case PointCaptureIntent::TrimPoint:
        applyTrimAtPoint(point);
        break;
    case PointCaptureIntent::FilletSecondLine:
        applyFilletAtPoint(point);
        break;
    }
    // The consumers emit modelChanged on success and on a surfaced failure, but
    // a silent early-out (null/locked source) would not — so refresh here to
    // guarantee the now-cleared prompt leaves the view.
    emit pointerChanged();
}

bool DrawingDocumentController::createArrayFromActiveObject(
    const QString &idPrefix,
    int copyCount,
    const std::function<DraftingArrayResult(
        const DraftingObject &source,
        const std::vector<DraftingObjectId> &newObjectIds)> &plan)
{
    if (copyCount < 1) {
        // Reachable from the UI (grid with count 1 has zero copy cells), so
        // it must say something — a silent dead button is indistinguishable
        // from a broken one.
        return finishEdit(QStringLiteral("array"), idPrefix, false,
                          DraftingResultCode::InvalidGeometry,
                          QStringLiteral("array count must create at least one copy"));
    }
    const DraftingObject *source = activeObject(m_document);
    if (source == nullptr || !draftingObjectEffectivelyEditable(m_document, *source)) {
        return false;
    }

    const int firstSerial = m_nextObjectSerial;
    std::vector<DraftingObjectId> objectIds;
    objectIds.reserve(static_cast<std::size_t>(copyCount));
    for (int index = 0; index < copyCount; ++index) {
        objectIds.push_back(toStdString(nextObjectId(idPrefix, m_nextObjectSerial++)));
    }

    DraftingArrayResult planned = plan(*source, objectIds);
    if (!planned.ok) {
        // No objects were created: reclaim the minted serials (a failed
        // 99x99 grid would otherwise burn 9800 ids), and surface the
        // planner's message through the existing edit-status channel —
        // these rejections (zero spacing, guide source, zero ring arm) are
        // user-reachable via the spins.
        m_nextObjectSerial = firstSerial;
        return finishEdit(QStringLiteral("array"), idPrefix, false,
                          planned.code, QString::fromStdString(planned.message));
    }
    // A stale rejection from an earlier failed click must not outlive a
    // success; createObjectsAndSelect emits modelChanged, which republishes
    // the (now empty) status.
    m_lastEditStatus.clear();
    return createObjectsAndSelect(std::move(planned.objects));
}

bool DrawingDocumentController::createObjectsAndSelect(std::vector<DraftingObject> objects)
{
    // The classified commit below relies on a non-empty batch: an empty one
    // would degrade to a pure selection clear, which must not push undo.
    if (objects.empty()) {
        return false;
    }
    beginEdit();
    // Ids are gathered BEFORE the command consumes the vector — the batch is
    // taken by value and moved into the variant so an N-object array is not
    // copied a second time on its way into the command layer.
    std::vector<DraftingObjectId> selectedIds;
    selectedIds.reserve(objects.size());
    for (const DraftingObject &object : objects) {
        selectedIds.push_back(object.id);
    }
    // One atomic command for the whole batch: either every object lands or
    // none does, so the partial-commit bookkeeping the per-object loop needed
    // (commit what landed, then bail) is gone — and so is its O(N^2) id
    // re-scan. On rejection the abandoned edit bracket is harmless: the next
    // beginEdit re-captures.
    const DraftingCommandResult create = applyDraftingCommand(m_document, CreateObjectsCommand{std::move(objects)});
    if (!create.ok) {
        return false;
    }
    applyDraftingCommand(m_document, SelectObjectsCommand{selectedIds});
    // A successful non-empty batch create is never selection-only: classify
    // the commit so it skips the encode-and-compare (two full document
    // serializations — real money at 9800 objects).
    commitEdit(false);
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::alignSelection(const QString &modeId)
{
    const std::optional<DraftingAlignmentMode> mode = draftingAlignmentModeFromId(toStdString(modeId));
    if (!mode) {
        return false;
    }

    return applyCommandAndEmit(AlignSelectionCommand{*mode});
}

bool DrawingDocumentController::distributeSelection(const QString &axisId)
{
    const std::optional<DraftingAlignmentMode> mode = draftingDistributeModeFromAxisId(toStdString(axisId));
    if (!mode) {
        return false;
    }

    return applyCommandAndEmit(DistributeSelectionCommand{*mode});
}

bool DrawingDocumentController::createCalibrationPattern(const QString &patternId)
{
    if (!activeDraftingLayerAcceptsNewObjects(m_document)) {
        return false;
    }

    const DraftingCalibrationPatternRequest request = defaultDraftingCalibrationPatternRequest(
        draftingCalibrationPatternKindFromId(toStdString(patternId)),
        toStdString(nextObjectId(QStringLiteral("calibration"), m_nextObjectSerial++)),
        m_document.activeLayerId);

    DraftingCalibrationPatternResult pattern = buildDraftingCalibrationPattern(request);
    if (!pattern.ok || pattern.objects.empty()) {
        return false;
    }

    return createObjectsAndSelect(std::move(pattern.objects));
}

bool DrawingDocumentController::recordCalibrationMeasurement(double measuredValue)
{
    const DraftingCalibrationMeasurementResult measurement = measureSelectedDraftingCalibrationPattern(
        m_document,
        m_document.selectedObjectIds,
        measuredValue,
        "manual_ui");
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
        const DraftingMetadataUpdatePlan plan = planMeasurementNoteUpdate(object->metadata, note);
        if (!plan.ok) {
            return false;
        }
        const DraftingCommandResult result = applyDraftingCommand(candidate, UpdateMetadataCommand{objectId, plan.metadata});
        if (!result.ok) {
            return false;
        }
    }

    beginEdit();
    m_document = std::move(candidate);
    m_latestCalibrationMeasurement = measurement.measurement;
    m_pendingCalibrationCorrection = planDraftingCalibrationCorrection(measurement.measurement);
    commitEdit();
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

bool DrawingDocumentController::applySelectionDrawablePlacement(DraftingSelectionDrawablePlacement placement)
{
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    const Bounds2D drawable = grid.drawableBounds;
    const DraftingPlotBoundsResult selected = selectedRawPlotOutputBounds(m_document, m_document.selectedObjectIds, grid, m_plotSettings);
    if (!selected.ok) {
        return false;
    }

    const DraftingNudgePlan plan = planSelectionDrawableMove(selected.bounds, drawable, placement);
    if (!plan.ok) {
        return false;
    }
    if (!translationHasEffect(plan.dx, plan.dy)) {
        return true;
    }

    return applyCommandAndEmit(MoveSelectionCommand{plan.dx, plan.dy});
}

bool DrawingDocumentController::fitSelectionToDrawableBounds()
{
    return applySelectionDrawablePlacement(DraftingSelectionDrawablePlacement::FitInside);
}

bool DrawingDocumentController::centerSelectionInDrawable()
{
    return applySelectionDrawablePlacement(DraftingSelectionDrawablePlacement::Center);
}

bool DrawingDocumentController::moveSelectionToDrawableOrigin()
{
    return applySelectionDrawablePlacement(DraftingSelectionDrawablePlacement::Origin);
}

bool DrawingDocumentController::applyGuideDrawablePlacement(DraftingGuideDrawablePlacement placement)
{
    return applyActiveGeometryPlan<GuideGeometry>([&](const GuideGeometry &guide) -> std::optional<DraftingGeometry> {
        const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
        const DraftingGuidePlan plan = moveGuideToDrawable(guide, grid.drawableBounds, placement);
        if (!plan.ok) {
            return std::nullopt;
        }
        return DraftingGeometry{plan.geometry};
    });
}

bool DrawingDocumentController::moveSelectedGuideToDrawableOrigin()
{
    return applyGuideDrawablePlacement(DraftingGuideDrawablePlacement::Origin);
}

bool DrawingDocumentController::centerSelectedGuideInDrawable()
{
    return applyGuideDrawablePlacement(DraftingGuideDrawablePlacement::Center);
}

bool DrawingDocumentController::moveSelectedGuideToDrawableMax()
{
    return applyGuideDrawablePlacement(DraftingGuideDrawablePlacement::Max);
}

bool DrawingDocumentController::offsetSelectedGuide(const QString &direction, const QString &stepMode)
{
    return applyActiveGeometryPlan<GuideGeometry>([&](const GuideGeometry &guide) -> std::optional<DraftingGeometry> {
        const double scale = draftingNudgeScaleForMode(toStdString(stepMode));
        const double stepX = effectiveNudgeStepX(m_snapSettings);
        const double stepY = effectiveNudgeStepY(m_snapSettings);
        const DraftingGuidePlan plan = offsetGuide(guide, toStdString(direction), stepX, stepY, scale);
        if (!plan.ok) {
            return std::nullopt;
        }
        return DraftingGeometry{plan.geometry};
    });
}

bool DrawingDocumentController::fitSelectedConstructionLineToDrawable()
{
    return applyActiveGeometryPlan<ConstructionLineGeometry>([&](const ConstructionLineGeometry &line) -> std::optional<DraftingGeometry> {
        const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
        const DraftingConstructionLinePlan plan = fitConstructionLineToDrawable(line, grid.drawableBounds);
        if (!plan.ok) {
            return std::nullopt;
        }
        return DraftingGeometry{plan.geometry};
    });
}

bool DrawingDocumentController::createGuideFromActiveBounds(
    const char *sourceTag,
    const std::function<DraftingGuidePlan(const Bounds2D &bounds)> &planGuide)
{
    const DraftingObject *source = activeObject(m_document);
    if (source == nullptr || !draftingObjectUsableAsBoundsSource(m_document, *source)) {
        return false;
    }

    const DraftingGuidePlan plan = planGuide(source->bounds);
    if (!plan.ok) {
        return false;
    }
    const GuideGeometry guide = plan.geometry;

    if (existingGuideId(m_document, guide)) {
        return true;
    }

    const QString id = nextObjectId(QStringLiteral("guide"), m_nextObjectSerial++);
    auto built = buildDraftingGuideObject(toStdString(id), guide, source->layerId, sourceTag);
    if (!built.ok) {
        return false;
    }
    return applyCommandAndEmit(CreateObjectCommand{built.object});
}

bool DrawingDocumentController::createGuideFromSelectedBounds(const QString &placementId)
{
    return createGuideFromActiveBounds("bounds_guide", [&](const Bounds2D &bounds) {
        return guideFromBoundsPlacement(bounds, toStdString(placementId));
    });
}

bool DrawingDocumentController::createOffsetGuideFromSelectedBounds(const QString &placementId, const QString &stepMode)
{
    return createGuideFromActiveBounds("offset_bounds_guide", [&](const Bounds2D &bounds) {
        const double scale = draftingNudgeScaleForMode(toStdString(stepMode));
        const double stepX = effectiveNudgeStepX(m_snapSettings) * scale;
        const double stepY = effectiveNudgeStepY(m_snapSettings) * scale;
        return offsetGuideFromBoundsPlacement(bounds, toStdString(placementId), stepX, stepY);
    });
}

bool DrawingDocumentController::applyGuidePreset(const QString &presetId)
{
    if (!activeDraftingLayerAcceptsNewObjects(m_document)) {
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
        auto built = buildDraftingGuideObject(
            toStdString(id),
            guide.geometry,
            m_document.activeLayerId,
            "guide_preset",
            toStdString(presetId),
            guidePresetVisualMetadata(guide));
        if (!built.ok) {
            return false;
        }

        const DraftingCommandResult result = applyDraftingCommand(candidate, CreateObjectCommand{built.object});
        if (!result.ok) {
            return false;
        }
        changed = true;
    }

    if (changed) {
        beginEdit();
        m_document = std::move(candidate);
        commitEdit();
        emit modelChanged();
    }
    return true;
}

bool DrawingDocumentController::alignSelectionToNearestGuide(const QString &modeId)
{
    if (m_document.selectedObjectIds.empty()) {
        return false;
    }
    const DraftingObject *source = activeObject(m_document);
    if (source == nullptr || !draftingObjectUsableAsBoundsSource(m_document, *source)) {
        return false;
    }

    const DraftingGuideAlignmentPlan plan = alignBoundsToNearestGuide(m_document, source->bounds, toStdString(modeId));
    if (!plan.ok) {
        return false;
    }
    if (!translationHasEffect(plan.dx, plan.dy)) {
        return true;
    }

    return applyCommandAndEmit(MoveSelectionCommand{plan.dx, plan.dy});
}

bool DrawingDocumentController::deleteSelectedGuide()
{
    const DraftingObject *object = activeObjectOfKind(m_document, DraftingShapeKind::Guide);
    if (object == nullptr) {
        return false;
    }

    return applyCommandAndEmit(DeleteObjectCommand{*m_document.activeObjectId});
}

bool DrawingDocumentController::deleteAllGuides()
{
    return applyCommandAndEmit(DeleteAllGuidesCommand{});
}

bool DrawingDocumentController::mergeDuplicateGuides()
{
    return applyCommandAndEmit(MergeDuplicateGuidesCommand{});
}

bool DrawingDocumentController::setAllGuidesVisible(bool visible)
{
    return applyCommandAndEmit(SetAllGuidesVisibleCommand{visible});
}

bool DrawingDocumentController::setAllGuidesLocked(bool locked)
{
    return applyCommandAndEmit(SetAllGuidesLockedCommand{locked});
}

void DrawingDocumentController::clickCanvasNormalized(double x, double y)
{
    const Point2D normalized = normalizeDraftingPoint({x, y});
    const Point2D point = resolveSnap(normalized, m_document, m_snapSettings).point;

    // A pick-a-point capture intercepts the click BEFORE any tool dispatch: the
    // click feeds a point to a waiting consumer (the radial-array centre), not
    // a select/create. It never touches selection, so the source object stays
    // active for the consumer, and it runs its own edit transaction.
    if (m_pointCapture) {
        resolvePointCapture(point);
        return;
    }

    m_lastGuideDragSnap.clear();
    const bool clearedEditStatus = !m_lastEditStatus.isEmpty();
    m_lastEditStatus.clear();
    beginEdit();

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
        commitEdit();
        emit modelChanged();
        return;
    }

    const DraftingToolKind kind = toolKind(m_selectedToolId);
    if (kind == DraftingToolKind::Point
        || kind == DraftingToolKind::HorizontalGuide
        || kind == DraftingToolKind::VerticalGuide
        || kind == DraftingToolKind::HorizontalConstructionLine
        || kind == DraftingToolKind::VerticalConstructionLine
        || kind == DraftingToolKind::TextAnnotation) {
        const QString id = nextObjectId(objectIdPrefix(kind), m_nextObjectSerial++);
        const auto object = buildDraftingObjectForTool(creationRequest(m_selectedToolId, id, m_document.activeLayerId, point, point));
        if (object.ok) {
            if (object.object.kind == DraftingShapeKind::Guide) {
                const auto *guide = std::get_if<GuideGeometry>(&object.object.geometry);
                const std::optional<DraftingObjectId> existing = guide == nullptr ? std::nullopt : existingGuideId(m_document, *guide);
                if (existing) {
                    applyDraftingCommand(m_document, SelectObjectCommand{*existing});
                    commitEdit();
                    emit modelChanged();
                    return;
                }
            }
            applyDraftingCommand(m_document, CreateObjectCommand{object.object});
            applyDraftingCommand(m_document, SelectObjectCommand{object.object.id});
        }
        commitEdit();
        emit modelChanged();
        return;
    }

    if (kind == DraftingToolKind::Polyline || kind == DraftingToolKind::Spline) {
        // Multi-click: every click anchors a point; the trail rides in the
        // pending request until finishPendingMultiClick (double-click/Enter)
        // commits it or cancelPendingCreation (Escape) drops it. Polyline and
        // spline share this exact gesture — they differ only in geometry kind.
        if (!m_pendingCreation) {
            const QString id = nextObjectId(objectIdPrefix(kind), m_nextObjectSerial++);
            m_pendingCreation = creationRequest(m_selectedToolId, id, m_document.activeLayerId, point, point);
            m_pendingCreation->vertices = {point};
        } else {
            m_pendingCreation->vertices.push_back(point);
        }
        m_previewObject.reset();
        commitEdit();
        // Pending state is transient — pointer-class. EXCEPT when this click
        // also wiped a live edit-rejected status: that label lives on the
        // modelChanged path, and the review caught it staying stale here.
        if (clearedEditStatus) {
            emit modelChanged();
        } else {
            emit pointerChanged();
        }
        return;
    }

    if (!m_pendingCreation) {
        const QString id = nextObjectId(objectIdPrefix(kind), m_nextObjectSerial++);
        m_pendingCreation = creationRequest(m_selectedToolId, id, m_document.activeLayerId, point, point);
        // Carry tool-option state into the pending request so the second click
        // and the live preview build with the chosen polygon side count and
        // rectangle variant parameters.
        m_pendingCreation->polygonSides = m_polygonSides;
        m_pendingCreation->rectCornerRadius = m_rectCornerRadius;
        m_pendingCreation->rectInset = m_rectInset;
        m_pendingCreation->fixedRadius = m_fixedRadius;
        m_previewObject.reset();
        commitEdit();
        if (clearedEditStatus) {
            emit modelChanged(); // the wiped edit-status label must refresh
        } else {
            emit pointerChanged();
        }
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
    commitEdit();
    emit modelChanged();
}

bool DrawingDocumentController::finishPendingMultiClick()
{
    // Both multi-click tools (polyline, spline) end the same way — a shared
    // finish path, not one method per tool. A scaffolder adding a third
    // multi-click tool extends this guard, it does not clone the method.
    if (!m_pendingCreation
        || (m_pendingCreation->tool != DraftingToolKind::Polyline
            && m_pendingCreation->tool != DraftingToolKind::Spline)) {
        return false;
    }
    beginEdit();
    const auto object = buildDraftingObjectForTool(*m_pendingCreation);
    m_pendingCreation.reset();
    m_previewObject.reset();
    if (object.ok) {
        applyDraftingCommand(m_document, CreateObjectCommand{object.object});
        applyDraftingCommand(m_document, SelectObjectCommand{object.object.id});
    }
    // A one-point trail simply dissolves (same outcome as Escape): there is
    // no curve/polyline to make, and committing nothing keeps undo clean.
    commitEdit();
    emit modelChanged();
    return object.ok;
}

void DrawingDocumentController::cancelPendingCreation()
{
    // View state only: cancel an in-flight two-click creation, its preview, or
    // an armed pick-a-point capture (Escape routes here).
    if (!m_pendingCreation && !m_previewObject && !m_pointCapture) {
        return;
    }
    m_pendingCreation.reset();
    m_previewObject.reset();
    m_pointCapture.reset();
    emit pointerChanged(); // view state, not a mutation
}

bool DrawingDocumentController::deleteSelectedObject()
{
    if (!m_document.activeObjectId) {
        return false;
    }
    return applyCommandAndEmit(DeleteObjectCommand{*m_document.activeObjectId});
}

bool DrawingDocumentController::duplicateSelectedObject()
{
    return createTransformedActiveObject(QStringLiteral("copy"),
        [](const DraftingObject &source, const std::string &newId) -> std::optional<DraftingObject> {
            DraftingObject copy = source;
            copy.id = newId;
            copy.geometry = translateGeometry(source.geometry, 0.02, 0.02);
            copy.bounds = computeBounds(copy.geometry);
            return copy;
        });
}

bool DrawingDocumentController::copySelection()
{
    // Snapshot in DOCUMENT order, not selection order: paste then recreates
    // objects in the same z-order they had, so a copied group keeps its
    // stacking. Copy never mutates the document — no beginEdit, no signal,
    // no undo entry (copying is not an edit).
    std::vector<DraftingObject> snapshot;
    for (const DraftingObject &object : m_document.objects) {
        if (std::find(m_document.selectedObjectIds.begin(), m_document.selectedObjectIds.end(), object.id)
            != m_document.selectedObjectIds.end()) {
            snapshot.push_back(object);
        }
    }
    if (snapshot.empty()) {
        return false; // nothing selected: leave the clipboard untouched
    }
    m_clipboard = std::move(snapshot);
    return true;
}

bool DrawingDocumentController::cutSelection()
{
    // Copy first (fills the clipboard), then delete the same ids as ONE undo
    // step. Capture the ids up front: deleting mutates selectedObjectIds, so
    // iterating it live would skip objects.
    if (!copySelection()) {
        return false;
    }
    const std::vector<DraftingObjectId> ids = m_document.selectedObjectIds;
    beginEdit();
    for (const DraftingObjectId &id : ids) {
        applyDraftingCommand(m_document, DeleteObjectCommand{id});
    }
    commitEdit();
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::paste()
{
    if (m_clipboard.empty()) {
        return false;
    }
    // The pure planner mints the ids and offsets the geometry; the controller
    // only applies the result and threads its serial counter through.
    const int serialBefore = m_nextObjectSerial;
    DraftingPasteResult plan = planDraftingPaste(
        m_clipboard, "paste", m_nextObjectSerial, 0.02, 0.02);
    m_nextObjectSerial = plan.nextSerial;

    // Atomic (user decision 2026-06-11): a paste lands whole or not at all,
    // selected as one unit, one undo step — same contract as arrays and
    // calibration patterns. The old per-object loop pasted the valid subset
    // when e.g. the clipboard's layer had been locked since the copy; that
    // best-effort path half-pasted copied groups and is deliberately gone.
    // A stale rejection from an earlier failed action must not outlive a
    // success (same discipline as createArrayFromActiveObject).
    m_lastEditStatus.clear();
    if (!createObjectsAndSelect(std::move(plan.objects))) {
        m_nextObjectSerial = serialBefore; // nothing landed: reclaim the ids
        // Cmd+V over a locked layer must say so: the only caller discards
        // the bool, and canPaste() still reports true, so without a status
        // a refused paste is indistinguishable from a broken shortcut.
        return finishEdit(QStringLiteral("paste"), QStringLiteral("clipboard"), false,
                          DraftingResultCode::InvalidSelectionTarget,
                          QStringLiteral("paste rejected: clipboard target layer is locked or missing"));
    }
    return true;
}

bool DrawingDocumentController::canPaste() const
{
    return !m_clipboard.empty();
}

void DrawingDocumentController::updateCreationPreviewNormalized(double x, double y)
{
    if (!m_pendingCreation) {
        return;
    }

    const Point2D point = resolveSnap(normalizeDraftingPoint({x, y}), m_document, m_snapSettings).point;
    DraftingToolCreationRequest preview = *m_pendingCreation;
    preview.end = point;
    if (preview.tool == DraftingToolKind::Polyline || preview.tool == DraftingToolKind::Spline) {
        // The pointer is a provisional last point: one anchored click plus the
        // cursor already previews as a valid two-point polyline/spline.
        preview.vertices.push_back(point);
    }
    const auto object = buildDraftingObjectForTool(preview);
    if (object.ok) {
        m_previewObject = object.object;
    } else {
        m_previewObject.reset();
    }
    emit pointerChanged(); // the preview ghost tracks the cursor — still not a mutation
}

bool DrawingDocumentController::editSelectedHandleNormalized(const QString &handleId, double x, double y)
{
    if (handleId.isEmpty() || !m_document.activeObjectId) {
        return false;
    }
    m_lastGuideDragSnap.clear();

    const Point2D point = resolveSnap({x, y}, m_document, m_snapSettings).point;
    EditObjectHandleCommand command{*m_document.activeObjectId, handleId.toStdString(), point};
    command.preserveAspect = m_aspectLockEnabled; // N4: rectangle corners honor the toggle
    return applyCommandAndEmit(command);
}

bool DrawingDocumentController::moveSelectionNormalized(double dx, double dy)
{
    if (m_document.selectedObjectIds.empty() || !std::isfinite(dx) || !std::isfinite(dy)) {
        return false;
    }

    m_lastGuideDragSnap.clear();
    if (m_guideMoveSnapEnabled && m_document.activeObjectId && isSelected(m_document, *m_document.activeObjectId)) {
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

    return applyCommandAndEmit(MoveSelectionCommand{dx, dy});
}

bool DrawingDocumentController::selectObjectById(const QString &id)
{
    const std::string objectId = toStdString(id);
    if (findObject(m_document, objectId) == nullptr) {
        return false;
    }
    beginEdit();
    const DraftingCommandResult result = applyDraftingCommand(m_document, SelectObjectCommand{objectId});
    if (!result.ok) {
        return false;
    }
    m_lastEditStatus.clear();
    commitEdit(); // selection-only: never an undo step (same rule as marquee)
    emit modelChanged();
    return true;
}

bool DrawingDocumentController::selectObjectsInBoundsNormalized(double x1, double y1, double x2, double y2)
{
    const Point2D a = normalizeDraftingPoint({x1, y1});
    const Point2D b = normalizeDraftingPoint({x2, y2});
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.x, b.x);
    const double bottom = std::max(a.y, b.y);
    const Bounds2D marquee{left, top, right - left, bottom - top};

    const std::vector<DraftingObjectId> objectIds = selectableObjectsInBounds(m_document, marquee);

    beginEdit();
    const DraftingCommandResult result = applyDraftingCommand(m_document, SelectObjectsCommand{objectIds});
    if (!result.ok) {
        return false;
    }
    m_lastEditStatus.clear();
    commitEdit(); // marquee selection is selection-only: never an undo step
    emit modelChanged();
    return true;
}
