#include "widgets/DrawingCanvasWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QVariantList>
#include <QVariantMap>
#include <QWheelEvent>

#include <cmath>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasGestureState.h"
#include "widgets/DrawingCanvasHandleHitTest.h"
#include "widgets/DrawingCanvasObjectPainter.h"
#include "widgets/DrawingCanvasProjectedDocument.h"
#include "widgets/DrawingCanvasProjectedGrid.h"
#include "widgets/DrawingCanvasProjectedObject.h"
#include "widgets/DrawingCanvasProjectedPlot.h"
#include "widgets/DrawingCanvasProjectedPointer.h"
#include "widgets/DrawingCanvasProjectedStatus.h"
#include "widgets/DrawingCanvasViewport.h"
#include "widgets/ShellTheme.h"

namespace {

using drawing_canvas::withAlpha;

// Tools that create an object across two clicks and show a live preview between them.
bool isTwoClickCreationTool(const QString &toolId)
{
    static const QStringList tools{
        QStringLiteral("line_tool"),
        QStringLiteral("rectangle_tool"),
        QStringLiteral("circle_tool"),
        QStringLiteral("arc_tool"),
        QStringLiteral("regular_polygon_tool"),
        QStringLiteral("angled_construction_line_tool"),
        QStringLiteral("distance_dimension_tool"),
        QStringLiteral("width_dimension_tool"),
        QStringLiteral("height_dimension_tool"),
        QStringLiteral("radius_dimension_tool"),
        QStringLiteral("diameter_dimension_tool"),
    };
    return tools.contains(toolId);
}

} // namespace

DrawingCanvasWidget::DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_palette(drawing_canvas::deriveCanvasPalette(edi::shell::deriveShellTheme(edi::shell::ShellThemeInputs{})))
    , m_gestureState(drawing_canvas::initialGestureState())
{
    setMinimumSize(480, 360);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    if (m_controller != nullptr) {
        connect(m_controller, &DrawingDocumentController::modelChanged, this, &DrawingCanvasWidget::refresh);
    }
}

void DrawingCanvasWidget::refresh()
{
    update();
}

void DrawingCanvasWidget::setCanvasPalette(const drawing_canvas::DrawingCanvasPalette &palette)
{
    m_palette = palette;
    update();
}

void DrawingCanvasWidget::setPlotPreviewVisible(bool visible)
{
    if (m_plotPreviewVisible == visible) {
        return;
    }
    m_plotPreviewVisible = visible;
    update();
}

bool DrawingCanvasWidget::plotPreviewVisible() const
{
    return m_plotPreviewVisible;
}

drawing_canvas::DrawingCanvasViewportInput DrawingCanvasWidget::viewportInput() const
{
    const QVariantMap model = m_controller == nullptr ? QVariantMap{} : m_controller->modelDocument();
    drawing_canvas::DrawingCanvasViewportInput input =
        drawing_canvas::viewportInputFromModel(model, width(), height());
    input.zoom = m_zoom;
    input.panXPx = m_pan.x();
    input.panYPx = m_pan.y();
    return input;
}

QRectF DrawingCanvasWidget::boardRect() const
{
    return drawing_canvas::viewportBoardRect(viewportInput());
}

QPointF DrawingCanvasWidget::mapCanvasToScreen(double x, double y) const
{
    return drawing_canvas::canvasToScreen(boardRect(), x, y);
}

QPointF DrawingCanvasWidget::canvasToScreen(double x, double y) const
{
    return drawing_canvas::canvasToScreen(boardRect(), x, y);
}

QRectF DrawingCanvasWidget::boundsToScreenRect(double x, double y, double width, double height) const
{
    return drawing_canvas::boundsToScreenRect(boardRect(), x, y, width, height);
}

QPointF DrawingCanvasWidget::screenToCanvas(const QPointF &point) const
{
    return drawing_canvas::screenToCanvas(boardRect(), point);
}

void DrawingCanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), m_palette.backdrop);

    if (m_controller == nullptr) {
        const QRectF board = boardRect();
        painter.fillRect(board, m_palette.boardFill);
        painter.setPen(QPen(m_palette.boardOutline, 1));
        painter.drawRect(board);
        return;
    }
    const QVariantMap model = m_controller->modelDocument();
    const QRectF board = boardRect();
    const drawing_canvas::DrawingCanvasProjectedDocumentSurface document = drawing_canvas::projectedDocumentSurface(model);
    const drawing_canvas::DrawingCanvasObjectPainterContext objectPainterContext{board, m_controller->selectedObjectId(), m_palette};
    drawPhysicalGrid(painter, model);

    for (const QVariant &value : document.drawingObjects) {
        drawing_canvas::drawObject(painter, value.toMap(), objectPainterContext);
    }
    drawing_canvas::drawGuideIntersections(painter, document.drawingObjects, objectPainterContext);
    if (m_plotPreviewVisible) {
        drawPlotPreview(painter, document.plotSummary);
    }
    drawPlotSafetyOverlay(painter, document.plotSummary);
    drawSelectionPlotBounds(painter, model);
    if (!document.previewObject.isEmpty()) {
        drawing_canvas::drawPreviewObject(painter, document.previewObject, objectPainterContext);
    }
    drawGuideDragSnapIntent(painter, model);
    drawPointerSnapMarker(painter, model);

    if (drawing_canvas::isMarquee(m_gestureState) && m_gestureState.moved) {
        const QRectF marquee(canvasToScreen(m_gestureState.startPoint.x, m_gestureState.startPoint.y),
            canvasToScreen(m_gestureState.lastPoint.x, m_gestureState.lastPoint.y));
        painter.setPen(QPen(m_palette.preview, 1, Qt::DashLine));
        painter.setBrush(withAlpha(m_palette.preview, 32));
        painter.drawRect(marquee.normalized());
    }

    // No status text painted on the document: the grid speaks for itself
    // (user direction). Tool/selection live in the chrome status; grid and
    // plot detail live in the inspector (projectedCanvasStatus still serves
    // those consumers).
}

void DrawingCanvasWidget::mousePressEvent(QMouseEvent *event)
{
    // Middle-button drag pans the view (navigation, not a document edit).
    if (event->button() == Qt::MiddleButton) {
        m_gestureState = drawing_canvas::beginPan(
            m_gestureState, {event->position().x(), event->position().y()}, {});
        m_lastPanScreenPoint = event->position();
        setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }

    if (m_controller == nullptr || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (m_controller->selectedToolId() == QStringLiteral("select_move")) {
        const QString handleId = hitSelectedHandle(event->position());
        if (!handleId.isEmpty()) {
            const QPointF point = screenToCanvas(event->position());
            // Coalesce the whole handle drag (press + moves + release) into one
            // undo step.
            m_controller->beginInteractiveEdit();
            m_gestureState = drawing_canvas::beginHandleDrag(
                m_gestureState,
                m_controller->selectedObjectId(),
                handleId,
                {point.x(), point.y()},
                {});
            m_controller->editSelectedHandleNormalized(handleId, point.x(), point.y());
            event->accept();
            return;
        }
    }

    const QPointF point = screenToCanvas(event->position());
    m_controller->updatePointerNormalized(point.x(), point.y());
    m_controller->clickCanvasNormalized(point.x(), point.y());
    if (m_controller->selectedToolId() == QStringLiteral("select_move") && !m_controller->selectedObjectId().isEmpty()) {
        QStringList selectedIds;
        selectedIds.push_back(m_controller->selectedObjectId());
        m_gestureState = drawing_canvas::beginObjectDrag(
            m_gestureState,
            m_controller->selectedObjectId(),
            {point.x(), point.y()},
            selectedIds,
            {});
        m_lastDragCanvasPoint = point;
        // Coalesce the whole object drag into one undo step.
        m_controller->beginInteractiveEdit();
    } else if (m_controller->selectedToolId() == QStringLiteral("select_move")) {
        m_gestureState = drawing_canvas::beginMarquee(m_gestureState, {point.x(), point.y()}, {});
    }
}

void DrawingCanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    // While panning, translate the view by the raw screen delta.
    if (m_gestureState.mode == drawing_canvas::DrawingCanvasGestureMode::Panning
        && (event->buttons() & Qt::MiddleButton)) {
        m_pan += event->position() - m_lastPanScreenPoint;
        m_lastPanScreenPoint = event->position();
        m_gestureState = drawing_canvas::updateGestureScreenPoint(
            m_gestureState, {event->position().x(), event->position().y()});
        update();
        event->accept();
        return;
    }

    if (m_controller == nullptr) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF point = screenToCanvas(event->position());
    m_controller->updatePointerNormalized(point.x(), point.y());
    if (isTwoClickCreationTool(m_controller->selectedToolId()) && !(event->buttons() & Qt::LeftButton)) {
        m_controller->updateCreationPreviewNormalized(point.x(), point.y());
        event->accept();
        return;
    }

    if (!(event->buttons() & Qt::LeftButton)) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    m_gestureState = drawing_canvas::updateGesture(m_gestureState, {point.x(), point.y()});

    if (drawing_canvas::isHandleDrag(m_gestureState)) {
        const QString handleId = m_gestureState.handleId;
        m_controller->editSelectedHandleNormalized(handleId, point.x(), point.y());
        event->accept();
        return;
    }

    if (drawing_canvas::isObjectDrag(m_gestureState)) {
        const double dx = point.x() - m_lastDragCanvasPoint.x();
        const double dy = point.y() - m_lastDragCanvasPoint.y();
        if (m_controller->moveSelectionNormalized(dx, dy)) {
            m_lastDragCanvasPoint = point;
        }
        event->accept();
        return;
    }

    if (drawing_canvas::isMarquee(m_gestureState)) {
        update();
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void DrawingCanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton
        && m_gestureState.mode == drawing_canvas::DrawingCanvasGestureMode::Panning) {
        m_gestureState = drawing_canvas::finishGesture(m_gestureState).state;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && drawing_canvas::isHandleDrag(m_gestureState)) {
        if (m_controller != nullptr) {
            const QPointF point = screenToCanvas(event->position());
            const QString handleId = m_gestureState.handleId;
            m_controller->editSelectedHandleNormalized(handleId, point.x(), point.y());
            m_controller->endInteractiveEdit();
        }
        m_gestureState = drawing_canvas::finishGesture(m_gestureState, {.incremental = true}).state;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && drawing_canvas::isObjectDrag(m_gestureState)) {
        if (m_controller != nullptr) {
            m_controller->endInteractiveEdit();
        }
        m_gestureState = drawing_canvas::finishGesture(m_gestureState, {.incremental = true}).state;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && drawing_canvas::isMarquee(m_gestureState)) {
        if (m_gestureState.moved && m_controller != nullptr) {
            m_controller->selectObjectsInBoundsNormalized(
                m_gestureState.startPoint.x,
                m_gestureState.startPoint.y,
                m_gestureState.lastPoint.x,
                m_gestureState.lastPoint.y);
        }
        m_gestureState = drawing_canvas::finishGesture(m_gestureState).state;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void DrawingCanvasWidget::wheelEvent(QWheelEvent *event)
{
    if (m_controller == nullptr) {
        QWidget::wheelEvent(event);
        return;
    }

    // Ctrl/Cmd + scroll zooms exponentially at the cursor (legacy semantics).
    if (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) {
        const double delta = event->angleDelta().y();
        const double factor = std::pow(1.0015, delta);
        const drawing_canvas::DrawingCanvasViewportInput zoomed =
            drawing_canvas::zoomViewportAtPoint(viewportInput(), factor, event->position());
        m_zoom = zoomed.zoom;
        m_pan = QPointF(zoomed.panXPx, zoomed.panYPx);
        update();
        event->accept();
        return;
    }

    // Plain scroll pans: prefer high-resolution pixel deltas, fall back to the
    // coarser angle delta (1/8 degree steps) scaled to pixels.
    QPointF delta(event->pixelDelta());
    if (delta.isNull()) {
        delta = QPointF(event->angleDelta()) / 2.0;
    }
    m_pan += delta;
    update();
    event->accept();
}

void DrawingCanvasWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_controller == nullptr || event->button() != Qt::LeftButton) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    // Qt delivers press->release->dblclick: the first click of the pair has
    // already anchored a vertex, so finishing here closes the trail at the
    // point the user double-clicked — the standard polyline ending gesture.
    if (m_controller->finishPendingPolyline()) {
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void DrawingCanvasWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_controller == nullptr) {
        QWidget::keyPressEvent(event);
        return;
    }

    const Qt::KeyboardModifiers mods = event->modifiers();

    if (event->key() == Qt::Key_Escape) {
        m_controller->endInteractiveEdit(); // close any in-flight drag transaction
        m_controller->cancelPendingCreation();
        m_gestureState = drawing_canvas::cancelGesture().state;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_controller->finishPendingPolyline()) {
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        m_controller->deleteSelectedObject();
        event->accept();
        return;
    }
    if ((mods & Qt::ControlModifier) && event->key() == Qt::Key_D) {
        m_controller->duplicateSelectedObject();
        event->accept();
        return;
    }

    QString direction;
    switch (event->key()) {
    case Qt::Key_Left:
        direction = QStringLiteral("left");
        break;
    case Qt::Key_Right:
        direction = QStringLiteral("right");
        break;
    case Qt::Key_Up:
        direction = QStringLiteral("up");
        break;
    case Qt::Key_Down:
        direction = QStringLiteral("down");
        break;
    default:
        break;
    }
    if (!direction.isEmpty()) {
        // Plain = grid step, Alt = fine, Shift = coarse (4x).
        QString stepMode = QStringLiteral("grid");
        if (mods & Qt::AltModifier) {
            stepMode = QStringLiteral("fine");
        } else if (mods & Qt::ShiftModifier) {
            stepMode = QStringLiteral("coarse");
        }
        m_controller->nudgeSelection(direction, stepMode);
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

QVariantMap DrawingCanvasWidget::selectedObjectProjection() const
{
    if (m_controller == nullptr || m_controller->selectedObjectId().isEmpty()) {
        return {};
    }
    return drawing_canvas::projectedObjectById(m_controller->modelDocument(), m_controller->selectedObjectId());
}

QString DrawingCanvasWidget::hitSelectedHandle(const QPointF &screenPoint) const
{
    const QVariantMap object = selectedObjectProjection();
    if (object.isEmpty()) {
        return {};
    }
    return drawing_canvas::hitCanvasHandleAt(object, screenPoint, boardRect()).handleId;
}

void DrawingCanvasWidget::drawPhysicalGrid(QPainter &painter, const QVariantMap &model) const
{
    const QRectF board = boardRect();
    const drawing_canvas::DrawingCanvasProjectedGrid grid = drawing_canvas::projectedGrid(model);
    painter.fillRect(board, m_palette.boardFill);

    for (const drawing_canvas::DrawingCanvasProjectedGridLine &line : grid.lines) {
        painter.setPen(QPen(line.major ? m_palette.gridMajor : m_palette.gridMinor, line.major ? 1.25 : 1.0));
        if (line.axis == QStringLiteral("vertical")) {
            painter.drawLine(canvasToScreen(line.position, 0.0), canvasToScreen(line.position, 1.0));
        } else {
            painter.drawLine(canvasToScreen(0.0, line.position), canvasToScreen(1.0, line.position));
        }
    }

    if (grid.drawableBounds.visible) {
        painter.setPen(QPen(m_palette.drawableBounds, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(boundsToScreenRect(
            grid.drawableBounds.x, grid.drawableBounds.y, grid.drawableBounds.width, grid.drawableBounds.height));
    }

    if (grid.origin.visible) {
        const QPointF originPoint = canvasToScreen(grid.origin.x, grid.origin.y);
        painter.setPen(QPen(m_palette.originMarker, 1.5));
        drawing_canvas::drawCrosshair(painter, originPoint, 8.0);
    }

    painter.setPen(QPen(m_palette.boardOutline, 1));
    painter.drawRect(board);
}

void DrawingCanvasWidget::drawPointerSnapMarker(QPainter &painter, const QVariantMap &model) const
{
    const drawing_canvas::DrawingCanvasProjectedPointer pointer = drawing_canvas::projectedPointer(model);
    if (!pointer.visible) {
        return;
    }

    const QPointF point = canvasToScreen(pointer.snappedX, pointer.snappedY);
    QColor color = m_palette.snapFree;
    Qt::PenStyle markerStyle = Qt::SolidLine;
    double markerRadius = 5.0;
    if (!pointer.insideDrawable) {
        color = m_palette.snapOutside;
        markerRadius = 8.0;
    } else if (pointer.kind == QStringLiteral("grid")) {
        color = m_palette.snapGrid;
        markerRadius = 7.0;
    } else if (pointer.source == QStringLiteral("guide")) {
        color = m_palette.snapGuide;
        markerStyle = Qt::DashLine;
        markerRadius = 8.0;
    } else if (pointer.kind == QStringLiteral("object")) {
        color = m_palette.snapObject;
        markerRadius = 7.0;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, pointer.source == QStringLiteral("guide") ? 2.0 : 1.5, markerStyle));
    painter.setBrush(withAlpha(color, 44));
    painter.drawEllipse(point, markerRadius, markerRadius);
    drawing_canvas::drawCrosshair(painter, point, 10.0);

    QString label = pointer.label;
    if (pointer.source == QStringLiteral("guide") && !pointer.sourceObjectId.isEmpty()) {
        label = QStringLiteral("guide %1").arg(pointer.sourceObjectId);
    }
    painter.setPen(color);
    painter.drawText(point + QPointF(12.0, -10.0), label.isEmpty() ? pointer.kind : label);
    painter.restore();
}

void DrawingCanvasWidget::drawGuideDragSnapIntent(QPainter &painter, const QVariantMap &model) const
{
    const drawing_canvas::DrawingCanvasProjectedGuideDragSnapIntent snap =
        drawing_canvas::projectedGuideDragSnapIntent(model);
    if (!snap.visible) {
        return;
    }

    const QPointF raw = canvasToScreen(snap.rawX, snap.rawY);
    const QPointF snapped = canvasToScreen(snap.snappedX, snap.snappedY);

    painter.save();
    QColor color = m_palette.snapGuide;
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, snap.intersection ? 2.0 : 1.5, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(raw, snapped);
    painter.setBrush(withAlpha(color, 56));
    painter.drawEllipse(raw, 5.0, 5.0);
    painter.setBrush(withAlpha(color, 96));
    painter.drawEllipse(snapped, snap.intersection ? 8.0 : 6.0, snap.intersection ? 8.0 : 6.0);
    painter.setPen(color);
    painter.drawText(snapped + QPointF(10.0, -12.0),
        snap.sourceObjectId.isEmpty()
            ? QStringLiteral("%1 guide").arg(snap.label)
            : QStringLiteral("%1 guide %2").arg(snap.label, snap.sourceObjectId));
    painter.restore();
}

void DrawingCanvasWidget::drawPlotPreview(QPainter &painter, const QVariantMap &plotSummary) const
{
    const drawing_canvas::DrawingCanvasProjectedPlotPreview preview = drawing_canvas::projectedPlotPreview(plotSummary);
    if (preview.travelSegments.empty() && preview.strokeSegments.empty() && !preview.hasPlotBounds) {
        return;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);

    QPen travelPen(withAlpha(m_palette.plotTravel, 130), 1.25, Qt::DashLine);
    travelPen.setCapStyle(Qt::RoundCap);
    painter.setPen(travelPen);
    for (const drawing_canvas::DrawingCanvasProjectedSegment &segment : preview.travelSegments) {
        painter.drawLine(canvasToScreen(segment.x1, segment.y1), canvasToScreen(segment.x2, segment.y2));
    }

    QPen strokePen(withAlpha(m_palette.preview, 170), 1.75);
    strokePen.setCapStyle(Qt::RoundCap);
    strokePen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(strokePen);
    for (const drawing_canvas::DrawingCanvasProjectedSegment &segment : preview.strokeSegments) {
        painter.drawLine(canvasToScreen(segment.x1, segment.y1), canvasToScreen(segment.x2, segment.y2));
    }

    painter.restore();
}

void DrawingCanvasWidget::drawPlotSafetyOverlay(QPainter &painter, const QVariantMap &plot) const
{
    const drawing_canvas::DrawingCanvasProjectedBoundsOverlay overlay = drawing_canvas::projectedPlotBoundsOverlay(plot);
    if (!overlay.visible) {
        return;
    }

    const QColor color = overlay.calibratedBoundsWarning ? m_palette.safetyWarning : m_palette.safetyOk;
    const QRectF rect = boundsToScreenRect(overlay.bounds.x, overlay.bounds.y, overlay.bounds.width, overlay.bounds.height);
    QPen pen(color, overlay.calibratedBoundsWarning ? 2.0 : 1.5, overlay.calibratedBoundsWarning ? Qt::DashLine : Qt::SolidLine);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(withAlpha(color, overlay.calibratedBoundsWarning ? 34 : 18));
    painter.drawRect(rect);

    if (!overlay.warningKind.isEmpty()) {
        const QString label = overlay.warningObjectId.isEmpty()
            ? overlay.warningKind
            : QStringLiteral("%1: %2").arg(overlay.warningKind, overlay.warningObjectId);
        painter.setPen(color);
        painter.drawText(rect.topLeft() + QPointF(8.0, -8.0), label);
    }
}

void DrawingCanvasWidget::drawSelectionPlotBounds(QPainter &painter, const QVariantMap &model) const
{
    const drawing_canvas::DrawingCanvasProjectedSelectionBoundsOverlay overlay = drawing_canvas::projectedSelectionBoundsOverlay(model);
    if (!overlay.visible) {
        return;
    }

    const QColor color = overlay.status == QStringLiteral("inside")
        ? m_palette.selection
        : m_palette.safetyWarning;
    QRectF rect = boundsToScreenRect(overlay.bounds.x, overlay.bounds.y, overlay.bounds.width, overlay.bounds.height);
    if (rect.width() < 10.0 || rect.height() < 10.0) {
        rect = rect.adjusted(-5.0, -5.0, 5.0, 5.0);
    }

    painter.save();
    painter.setPen(QPen(color, 1.75, Qt::DashLine));
    painter.setBrush(withAlpha(color, 24));
    painter.drawRect(rect);
    painter.restore();
}
