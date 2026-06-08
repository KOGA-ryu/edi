#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include "core/DrawingDocumentProjection.h"
#include "drafting/DraftingCommands.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingSelection.h"
#include "drafting/DraftingSnap.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

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

std::optional<DraftingObject> buildObjectForTool(const QString &toolId, const QString &objectId, Point2D start, Point2D end)
{
    DraftingShapeKind kind = DraftingShapeKind::Point;
    DraftingGeometry geometry = PointGeometry{start};
    if (toolId == QStringLiteral("point_tool")) {
        kind = DraftingShapeKind::Point;
        geometry = PointGeometry{end};
    } else if (toolId == QStringLiteral("line_tool")) {
        kind = DraftingShapeKind::Line;
        geometry = LineGeometry{start, end};
    } else if (toolId == QStringLiteral("rectangle_tool")) {
        const double left = std::min(start.x, end.x);
        const double top = std::min(start.y, end.y);
        const double right = std::max(start.x, end.x);
        const double bottom = std::max(start.y, end.y);
        kind = DraftingShapeKind::Rectangle;
        geometry = RectangleGeometry{{left, top}, right - left, bottom - top};
    } else if (toolId == QStringLiteral("circle_tool")) {
        kind = DraftingShapeKind::Circle;
        geometry = CircleGeometry{start, std::min(1.0, distance(start, end))};
    } else {
        return std::nullopt;
    }

    auto built = buildDraftingObject(toStdString(objectId), kind, std::move(geometry));
    if (!built.ok) {
        return std::nullopt;
    }
    built.object.metadata.toolProvenance = toolId.toStdString();
    return built.object;
}

} // namespace

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent)
    , m_document(makeDraftingDocument("active_drawing", "Active Drawing"))
{
}

QVariantMap DrawingDocumentController::modelDocument() const
{
    return drawing_core::draftingDocumentToModelProjection(m_document, m_snapSettings);
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
    m_hasPendingPoint = false;
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
        m_hasPendingPoint = false;
        emit modelChanged();
        return;
    }

    if (m_selectedToolId == QStringLiteral("point_tool")) {
        const QString id = nextObjectId(QStringLiteral("point"), m_nextObjectSerial++);
        const auto object = buildObjectForTool(m_selectedToolId, id, point, point);
        if (object) {
            applyDraftingCommand(m_document, CreateObjectCommand{*object});
            applyDraftingCommand(m_document, SelectObjectCommand{object->id});
        }
        emit modelChanged();
        return;
    }

    if (!m_hasPendingPoint) {
        m_pendingX = point.x;
        m_pendingY = point.y;
        m_hasPendingPoint = true;
        emit modelChanged();
        return;
    }

    const QString id = nextObjectId(m_selectedToolId.section(QLatin1Char('_'), 0, 0), m_nextObjectSerial++);
    const auto object = buildObjectForTool(m_selectedToolId, id, {m_pendingX, m_pendingY}, point);
    m_hasPendingPoint = false;
    if (object) {
        applyDraftingCommand(m_document, CreateObjectCommand{*object});
        applyDraftingCommand(m_document, SelectObjectCommand{object->id});
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
