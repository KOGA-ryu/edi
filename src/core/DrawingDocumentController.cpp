#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "core/DrawingDocumentProjection.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingSnap.h"
#include "drafting/DraftingToolCreation.h"

#include <algorithm>
#include <cmath>
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

} // namespace

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
    , m_document(makeDraftingDocument("active_drawing", "Active Drawing"))
{
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    return drawing_core::draftingDocumentToModelProjection(m_document, m_snapSettings, m_previewObject ? &*m_previewObject : nullptr);
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
    if (kind == DraftingToolKind::Point) {
        const QString id = nextObjectId(QStringLiteral("point"), m_nextObjectSerial++);
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
