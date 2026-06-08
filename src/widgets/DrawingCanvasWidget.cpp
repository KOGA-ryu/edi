#include "widgets/DrawingCanvasWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

#include "canvas/DrawingCanvasGestureState.h"
#include "canvas/DrawingCanvasHandles.h"
#include "core/DrawingCore.h"

DrawingCanvasWidget::DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
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

QRectF DrawingCanvasWidget::boardRect() const
{
    const double side = std::max(1.0, std::min(width(), height()) - 40.0);
    return QRectF((width() - side) * 0.5, (height() - side) * 0.5, side, side);
}

QPointF DrawingCanvasWidget::canvasToScreen(double x, double y) const
{
    const QRectF board = boardRect();
    return QPointF(board.left() + x * board.width(), board.top() + y * board.height());
}

QPointF DrawingCanvasWidget::screenToCanvas(const QPointF &point) const
{
    const QRectF board = boardRect();
    const double x = std::clamp((point.x() - board.left()) / board.width(), 0.0, 1.0);
    const double y = std::clamp((point.y() - board.top()) / board.height(), 0.0, 1.0);
    return QPointF(x, y);
}

void DrawingCanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#17191f"));

    const QRectF board = boardRect();
    painter.fillRect(board, QColor("#222630"));
    painter.setPen(QPen(QColor("#3d4452"), 1));
    painter.drawRect(board);

    painter.setPen(QPen(QColor("#313744"), 1));
    for (int i = 1; i < 16; ++i) {
        const double t = static_cast<double>(i) / 16.0;
        painter.drawLine(canvasToScreen(t, 0), canvasToScreen(t, 1));
        painter.drawLine(canvasToScreen(0, t), canvasToScreen(1, t));
    }

    if (m_controller == nullptr) {
        return;
    }
    const QVariantMap model = m_controller->modelDocument();
    for (const QVariant &value : model.value("drawing_objects").toList()) {
        drawObject(painter, value.toMap());
    }
    const QVariantMap previewObject = model.value(QStringLiteral("preview_object")).toMap();
    if (!previewObject.isEmpty()) {
        drawPreviewObject(painter, previewObject);
    }

    if (drawing_canvas::isMarquee(m_gestureState) && m_gestureState.value(QStringLiteral("moved")).toBool()) {
        const QVariantMap start = m_gestureState.value(QStringLiteral("startPoint")).toMap();
        const QVariantMap last = m_gestureState.value(QStringLiteral("lastPoint")).toMap();
        const QRectF marquee(canvasToScreen(start.value(QStringLiteral("x")).toDouble(), start.value(QStringLiteral("y")).toDouble()),
            canvasToScreen(last.value(QStringLiteral("x")).toDouble(), last.value(QStringLiteral("y")).toDouble()));
        painter.setPen(QPen(QColor("#75c7ff"), 1, Qt::DashLine));
        painter.setBrush(QColor(117, 199, 255, 32));
        painter.drawRect(marquee.normalized());
    }

    painter.setPen(QColor("#aeb7c7"));
    painter.drawText(board.adjusted(10, 10, -10, -10), Qt::AlignTop | Qt::AlignLeft,
        QString("Tool: %1\nSelected: %2")
            .arg(m_controller->selectedToolId(), m_controller->selectedObjectId().isEmpty() ? "none" : m_controller->selectedObjectId()));
}

void DrawingCanvasWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_controller == nullptr || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (m_controller->selectedToolId() == QStringLiteral("select_move")) {
        const QString handleId = hitSelectedHandle(event->position());
        if (!handleId.isEmpty()) {
            const QPointF point = screenToCanvas(event->position());
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
    m_controller->clickCanvasNormalized(point.x(), point.y());
    if (m_controller->selectedToolId() == QStringLiteral("select_move") && !m_controller->selectedObjectId().isEmpty()) {
        QVariantList selectedIds;
        selectedIds.push_back(m_controller->selectedObjectId());
        m_gestureState = drawing_canvas::beginObjectDrag(
            m_gestureState,
            m_controller->selectedObjectId(),
            {point.x(), point.y()},
            selectedIds,
            {});
        m_lastDragCanvasPoint = point;
    } else if (m_controller->selectedToolId() == QStringLiteral("select_move")) {
        m_gestureState = drawing_canvas::beginMarquee(m_gestureState, {point.x(), point.y()}, {});
    }
}

void DrawingCanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_controller == nullptr) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF point = screenToCanvas(event->position());
    const bool creationTool = m_controller->selectedToolId() == QStringLiteral("line_tool")
        || m_controller->selectedToolId() == QStringLiteral("rectangle_tool")
        || m_controller->selectedToolId() == QStringLiteral("circle_tool");
    if (creationTool && !(event->buttons() & Qt::LeftButton)) {
        m_controller->updateCreationPreviewNormalized(point.x(), point.y());
        event->accept();
        return;
    }

    if (!(event->buttons() & Qt::LeftButton)) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    m_gestureState = drawing_canvas::updateGesture(m_gestureState, {
        {QStringLiteral("point"), QVariantMap{{QStringLiteral("x"), point.x()}, {QStringLiteral("y"), point.y()}}},
    });

    if (drawing_canvas::isHandleDrag(m_gestureState)) {
        const QString handleId = m_gestureState.value(QStringLiteral("handleId")).toString();
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
    if (event->button() == Qt::LeftButton && drawing_canvas::isHandleDrag(m_gestureState)) {
        if (m_controller != nullptr) {
            const QPointF point = screenToCanvas(event->position());
            const QString handleId = m_gestureState.value(QStringLiteral("handleId")).toString();
            m_controller->editSelectedHandleNormalized(handleId, point.x(), point.y());
        }
        m_gestureState = drawing_canvas::finishGesture(m_gestureState, {{QStringLiteral("incremental"), true}}).value(QStringLiteral("state")).toMap();
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && drawing_canvas::isObjectDrag(m_gestureState)) {
        m_gestureState = drawing_canvas::finishGesture(m_gestureState, {{QStringLiteral("incremental"), true}}).value(QStringLiteral("state")).toMap();
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && drawing_canvas::isMarquee(m_gestureState)) {
        const QVariantMap start = m_gestureState.value(QStringLiteral("startPoint")).toMap();
        const QVariantMap last = m_gestureState.value(QStringLiteral("lastPoint")).toMap();
        if (m_gestureState.value(QStringLiteral("moved")).toBool() && m_controller != nullptr) {
            m_controller->selectObjectsInBoundsNormalized(
                start.value(QStringLiteral("x")).toDouble(),
                start.value(QStringLiteral("y")).toDouble(),
                last.value(QStringLiteral("x")).toDouble(),
                last.value(QStringLiteral("y")).toDouble());
        }
        m_gestureState = drawing_canvas::finishGesture(m_gestureState, {}).value(QStringLiteral("state")).toMap();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

QVariantMap DrawingCanvasWidget::selectedObjectProjection() const
{
    if (m_controller == nullptr || m_controller->selectedObjectId().isEmpty()) {
        return {};
    }
    const QVariantMap model = m_controller->modelDocument();
    for (const QVariant &value : model.value("drawing_objects").toList()) {
        const QVariantMap object = value.toMap();
        if (object.value(QStringLiteral("id")).toString() == m_controller->selectedObjectId()) {
            return object;
        }
    }
    return {};
}

QString DrawingCanvasWidget::hitSelectedHandle(const QPointF &screenPoint) const
{
    const QVariantMap object = selectedObjectProjection();
    if (object.isEmpty()) {
        return {};
    }

    const QRectF board = boardRect();
    QVariantMap settings;
    settings.insert(QStringLiteral("canvasSizePx"), 512.0);
    settings.insert(QStringLiteral("rotateHandleOffsetPx"), 28.0);
    settings.insert(QStringLiteral("handleHitTolerancePx"), 14.0);
    settings.insert(QStringLiteral("rotateHandleHitTolerancePx"), 18.0);

    const drawing_canvas::HitResult hit = drawing_canvas::hitHandleAt(
        drawing_canvas::CanvasObjectView{object},
        screenPoint.x(),
        screenPoint.y(),
        drawing_canvas::BoardBounds{board.x(), board.y(), board.width()},
        settings);
    return hit.ok && hit.kind == QStringLiteral("handle") ? hit.objectId : QString();
}

void DrawingCanvasWidget::drawObject(QPainter &painter, const QVariantMap &object) const
{
    const QString kind = object.value(QStringLiteral("kind")).toString();
    const QString id = object.value(QStringLiteral("id")).toString();
    const bool selected = m_controller != nullptr && id == m_controller->selectedObjectId();

    QPen pen(selected ? QColor("#f6c65b") : QColor("#d7dde8"), selected ? 3 : 2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    if (kind == QStringLiteral("point")) {
        const QPointF point = canvasToScreen(object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble());
        painter.setBrush(selected ? QColor("#f6c65b") : QColor("#d7dde8"));
        painter.drawEllipse(point, 4.0, 4.0);
    } else if (kind == QStringLiteral("line")) {
        painter.drawLine(
            canvasToScreen(object.value(QStringLiteral("x1")).toDouble(), object.value(QStringLiteral("y1")).toDouble()),
            canvasToScreen(object.value(QStringLiteral("x2")).toDouble(), object.value(QStringLiteral("y2")).toDouble()));
    } else if (kind == QStringLiteral("rectangle")) {
        const QPointF origin = canvasToScreen(object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble());
        const QPointF extent = canvasToScreen(
            object.value(QStringLiteral("x")).toDouble() + object.value(QStringLiteral("width")).toDouble(),
            object.value(QStringLiteral("y")).toDouble() + object.value(QStringLiteral("height")).toDouble());
        QRectF rect(origin, extent);
        const double rotation = object.value(QStringLiteral("rotation_deg")).toDouble();
        if (std::abs(rotation) > 0.000001) {
            painter.save();
            painter.translate(rect.center());
            painter.rotate(rotation);
            painter.translate(-rect.center());
            painter.drawRect(rect);
            painter.restore();
        } else {
            painter.drawRect(rect.normalized());
        }
    } else if (kind == QStringLiteral("circle")) {
        const QPointF center = canvasToScreen(object.value(QStringLiteral("cx")).toDouble(), object.value(QStringLiteral("cy")).toDouble());
        const double radius = object.value(QStringLiteral("radius")).toDouble() * boardRect().width();
        painter.drawEllipse(center, radius, radius);
    } else if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon")) {
        QPolygonF polygon;
        for (const drawing_canvas::CanvasPoint &point : drawing_canvas::CanvasObjectView{object}.points()) {
            polygon.push_back(canvasToScreen(point.x, point.y));
        }
        if (kind == QStringLiteral("polygon")) {
            painter.drawPolygon(polygon);
        } else {
            painter.drawPolyline(polygon);
        }
    }

    if (selected) {
        drawSelectedHandles(painter, object);
    }
}

void DrawingCanvasWidget::drawPreviewObject(QPainter &painter, const QVariantMap &object) const
{
    painter.save();
    QPen pen(QColor("#75c7ff"), 2, Qt::DashLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("line")) {
        painter.drawLine(
            canvasToScreen(object.value(QStringLiteral("x1")).toDouble(), object.value(QStringLiteral("y1")).toDouble()),
            canvasToScreen(object.value(QStringLiteral("x2")).toDouble(), object.value(QStringLiteral("y2")).toDouble()));
    } else if (kind == QStringLiteral("rectangle")) {
        const QPointF origin = canvasToScreen(object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble());
        const QPointF extent = canvasToScreen(
            object.value(QStringLiteral("x")).toDouble() + object.value(QStringLiteral("width")).toDouble(),
            object.value(QStringLiteral("y")).toDouble() + object.value(QStringLiteral("height")).toDouble());
        painter.drawRect(QRectF(origin, extent).normalized());
    } else if (kind == QStringLiteral("circle")) {
        const QPointF center = canvasToScreen(object.value(QStringLiteral("cx")).toDouble(), object.value(QStringLiteral("cy")).toDouble());
        const double radius = object.value(QStringLiteral("radius")).toDouble() * boardRect().width();
        painter.drawEllipse(center, radius, radius);
    }
    painter.restore();
}

void DrawingCanvasWidget::drawSelectedHandles(QPainter &painter, const QVariantMap &object) const
{
    QVariantMap settings;
    settings.insert(QStringLiteral("canvasSizePx"), 512.0);
    settings.insert(QStringLiteral("rotateHandleOffsetPx"), 28.0);
    const drawing_canvas::CanvasObjectView objectView{object};
    painter.setPen(QPen(QColor("#1d1f26"), 2));
    painter.setBrush(QColor("#f6c65b"));
    for (const drawing_canvas::HandleDescriptor &handle : drawing_canvas::visibleHandlesForObject(objectView, settings)) {
        const QPointF point = canvasToScreen(handle.x, handle.y);
        if (handle.hasAnchor) {
            painter.drawLine(canvasToScreen(handle.anchorX, handle.anchorY), point);
        }
        const double size = handle.role == QStringLiteral("rotate") ? 10.0 : 8.0;
        painter.drawEllipse(QRectF(point.x() - size * 0.5, point.y() - size * 0.5, size, size));
    }
}
