#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "core/DrawingDocumentProjection.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingArray.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGrid.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingMirror.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingOffset.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <algorithm>
#include <cmath>
#include <optional>
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

DraftingToolCreationRequest creationRequest(const QString &toolId, const QString &objectId, Point2D start, Point2D end)
{
    return {toolKind(toolId), toStdString(objectId), start, end, toStdString(toolId)};
}

bool boundsIntersect(Bounds2D a, Bounds2D b)
{
    return a.x <= b.x + b.width
        && a.x + a.width >= b.x
        && a.y <= b.y + b.height
        && a.y + a.height >= b.y;
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

double nudgeScaleForMode(const QString &stepMode)
{
    if (stepMode == QStringLiteral("fine")) {
        return 0.25;
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

} // namespace

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
    , m_document(makeDraftingDocument("active_drawing", "Active Drawing"))
    , m_gridSettings(defaultDraftingGridSettings())
{
    applyGridToSnap(m_snapSettings, m_gridSettings);
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    QVariantMap model = drawing_core::draftingDocumentToModelProjection(m_document, m_snapSettings, m_previewObject ? &*m_previewObject : nullptr);
    const DraftingGridProjection grid = projectDraftingGrid(m_gridSettings);
    model.insert(QStringLiteral("grid"), gridProjectionToMap(grid));
    if (m_pointerRawPoint) {
        model.insert(QStringLiteral("pointer"), pointerProjectionToMap(*m_pointerRawPoint, m_document, m_snapSettings, grid));
    }

    QVariantList warnings;
    for (const DraftingObject &object : m_document.objects) {
        if (object.visible && boundsOutsideDrawableArea(object.bounds, grid)) {
            warnings.push_back(QVariantMap{
                {QStringLiteral("kind"), QStringLiteral("out_of_drawable_bounds")},
                {QStringLiteral("object_id"), drawing_core::qStringFromStdString(object.id)},
            });
        }
    }
    model.insert(QStringLiteral("warnings"), warnings);
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

void DrawingDocumentController::setSelectedToolId(const QString &toolId)
{
    if (m_selectedToolId == toolId) {
        return;
    }
    m_selectedToolId = toolId;
    m_pendingCreation.reset();
    m_previewObject.reset();
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

bool DrawingDocumentController::nudgeSelection(const QString &direction, const QString &stepMode)
{
    if (m_document.selectedObjectIds.empty()) {
        return false;
    }

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

bool DrawingDocumentController::offsetSelectedObject(const QString &sideId)
{
    if (!m_document.activeObjectId) {
        return false;
    }
    const DraftingObject *source = findObject(m_document, *m_document.activeObjectId);
    if (source == nullptr || source->locked) {
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
    if (source == nullptr || source->locked) {
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
    if (source == nullptr || source->locked) {
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

void DrawingDocumentController::clickCanvasNormalized(double x, double y)
{
    x = clamp01(x);
    y = clamp01(y);
    const Point2D point = resolveSnap({x, y}, m_document, m_snapSettings).point;

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
        const auto object = buildDraftingObjectForTool(creationRequest(m_selectedToolId, id, point, point));
        if (object.ok) {
            applyDraftingCommand(m_document, CreateObjectCommand{object.object});
            applyDraftingCommand(m_document, SelectObjectCommand{object.object.id});
        }
        emit modelChanged();
        return;
    }

    if (!m_pendingCreation) {
        const QString id = nextObjectId(objectIdPrefix(kind), m_nextObjectSerial++);
        m_pendingCreation = creationRequest(m_selectedToolId, id, point, point);
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
        if (object.visible && boundsIntersect(object.bounds, marquee)) {
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
