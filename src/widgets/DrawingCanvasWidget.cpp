#include "widgets/DrawingCanvasWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasGestureState.h"
#include "widgets/DrawingCanvasObjectPainter.h"
#include "widgets/DrawingCanvasProjectedObject.h"

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

QRectF DrawingCanvasWidget::boardRect() const
{
    double aspect = 1.0;
    if (m_controller != nullptr) {
        const QVariantMap grid = m_controller->modelDocument().value(QStringLiteral("grid")).toMap();
        const double gridWidth = grid.value(QStringLiteral("width"), 1.0).toDouble();
        const double gridHeight = grid.value(QStringLiteral("height"), 1.0).toDouble();
        if (std::isfinite(gridWidth) && std::isfinite(gridHeight) && gridWidth > 0.0 && gridHeight > 0.0) {
            aspect = gridWidth / gridHeight;
        }
    }

    const double availableWidth = std::max(1.0, static_cast<double>(width()) - 48.0);
    const double availableHeight = std::max(1.0, static_cast<double>(height()) - 48.0);
    double boardWidth = availableWidth;
    double boardHeight = boardWidth / aspect;
    if (boardHeight > availableHeight) {
        boardHeight = availableHeight;
        boardWidth = boardHeight * aspect;
    }
    return QRectF((width() - boardWidth) * 0.5, (height() - boardHeight) * 0.5, boardWidth, boardHeight);
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

    if (m_controller == nullptr) {
        const QRectF board = boardRect();
        painter.fillRect(board, QColor("#222630"));
        painter.setPen(QPen(QColor("#3d4452"), 1));
        painter.drawRect(board);
        return;
    }
    const QVariantMap model = m_controller->modelDocument();
    const QRectF board = boardRect();
    const drawing_canvas::DrawingCanvasObjectPainterContext objectPainterContext{board, m_controller->selectedObjectId()};
    drawPhysicalGrid(painter, model);

    const QVariantList objects = model.value("drawing_objects").toList();
    for (const QVariant &value : objects) {
        drawing_canvas::drawObject(painter, value.toMap(), objectPainterContext);
    }
    drawing_canvas::drawGuideIntersections(painter, objects, objectPainterContext);
    if (m_plotPreviewVisible) {
        drawPlotPreview(painter, model);
    }
    drawPlotSafetyOverlay(painter, model.value(QStringLiteral("plot_summary")).toMap());
    drawSelectionPlotBounds(painter, model);
    const QVariantMap previewObject = model.value(QStringLiteral("preview_object")).toMap();
    if (!previewObject.isEmpty()) {
        drawing_canvas::drawPreviewObject(painter, previewObject, objectPainterContext);
    }
    drawGuideDragSnapIntent(painter, model);
    drawPointerSnapMarker(painter, model);

    if (drawing_canvas::isMarquee(m_gestureState) && m_gestureState.moved) {
        const QRectF marquee(canvasToScreen(m_gestureState.startPoint.x, m_gestureState.startPoint.y),
            canvasToScreen(m_gestureState.lastPoint.x, m_gestureState.lastPoint.y));
        painter.setPen(QPen(QColor("#75c7ff"), 1, Qt::DashLine));
        painter.setBrush(QColor(117, 199, 255, 32));
        painter.drawRect(marquee.normalized());
    }

    painter.setPen(QColor("#aeb7c7"));
    const QVariantMap grid = model.value(QStringLiteral("grid")).toMap();
    const QVariantMap plot = model.value(QStringLiteral("plot_summary")).toMap();
    const QString plotStatus = plot.value(QStringLiteral("status"), QStringLiteral("blocked")).toString();
    const QString firstWarningKind = plot.value(QStringLiteral("first_warning_kind")).toString();
    painter.drawText(board.adjusted(10, 10, -10, -10), Qt::AlignTop | Qt::AlignLeft,
        QString("Tool: %1\nSelected: %2\nGrid: %3 %4 x %5 %4\nPlot: %6 (%7 warnings)%8")
            .arg(m_controller->selectedToolId(),
                m_controller->selectedObjectId().isEmpty() ? "none" : m_controller->selectedObjectId(),
                QString::number(grid.value(QStringLiteral("width")).toDouble(), 'f', 2),
                grid.value(QStringLiteral("unit_label")).toString(),
                QString::number(grid.value(QStringLiteral("height")).toDouble(), 'f', 2),
                plotStatus,
                QString::number(plot.value(QStringLiteral("warning_count")).toInt()),
                firstWarningKind.isEmpty() ? QString() : QStringLiteral("\n%1").arg(firstWarningKind)));
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
    m_controller->updatePointerNormalized(point.x(), point.y());
    const bool creationTool = m_controller->selectedToolId() == QStringLiteral("line_tool")
        || m_controller->selectedToolId() == QStringLiteral("rectangle_tool")
        || m_controller->selectedToolId() == QStringLiteral("circle_tool")
        || m_controller->selectedToolId() == QStringLiteral("distance_dimension_tool")
        || m_controller->selectedToolId() == QStringLiteral("width_dimension_tool")
        || m_controller->selectedToolId() == QStringLiteral("height_dimension_tool")
        || m_controller->selectedToolId() == QStringLiteral("radius_dimension_tool")
        || m_controller->selectedToolId() == QStringLiteral("diameter_dimension_tool");
    if (creationTool && !(event->buttons() & Qt::LeftButton)) {
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
    if (event->button() == Qt::LeftButton && drawing_canvas::isHandleDrag(m_gestureState)) {
        if (m_controller != nullptr) {
            const QPointF point = screenToCanvas(event->position());
            const QString handleId = m_gestureState.handleId;
            m_controller->editSelectedHandleNormalized(handleId, point.x(), point.y());
        }
        m_gestureState = drawing_canvas::finishGesture(m_gestureState, {.incremental = true}).state;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && drawing_canvas::isObjectDrag(m_gestureState)) {
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

QVariantMap DrawingCanvasWidget::selectedObjectProjection() const
{
    if (m_controller == nullptr || m_controller->selectedObjectId().isEmpty()) {
        return {};
    }
    const QVariantMap model = m_controller->modelDocument();
    for (const QVariant &value : model.value("drawing_objects").toList()) {
        const QVariantMap object = value.toMap();
        if (drawing_canvas::projectedObjectSummary(object).id == m_controller->selectedObjectId()) {
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

    const std::vector<drawing_canvas::DrawingCanvasProjectedHandle> projectedHandles = drawing_canvas::projectedObjectHandles(object);
    if (!projectedHandles.empty()) {
        QString bestId;
        double bestDistance = 999.0;
        for (const drawing_canvas::DrawingCanvasProjectedHandle &handle : projectedHandles) {
            if (!handle.editable) {
                continue;
            }
            const QPointF point = canvasToScreen(handle.x, handle.y);
            const double dx = screenPoint.x() - point.x();
            const double dy = screenPoint.y() - point.y();
            const double distance = std::sqrt(dx * dx + dy * dy);
            if (distance <= handle.hitTolerancePx && distance <= bestDistance) {
                bestDistance = distance;
                bestId = handle.id;
            }
        }
        return bestId;
    }

    return {};
}

void DrawingCanvasWidget::drawPhysicalGrid(QPainter &painter, const QVariantMap &model) const
{
    const QRectF board = boardRect();
    const QVariantMap grid = model.value(QStringLiteral("grid")).toMap();
    painter.fillRect(board, QColor("#222630"));

    const QVariantList lines = grid.value(QStringLiteral("lines")).toList();
    for (const QVariant &lineValue : lines) {
        const QVariantMap line = lineValue.toMap();
        const bool major = line.value(QStringLiteral("major")).toBool();
        painter.setPen(QPen(major ? QColor("#465162") : QColor("#313744"), major ? 1.25 : 1.0));
        const double position = line.value(QStringLiteral("position")).toDouble();
        if (line.value(QStringLiteral("axis")).toString() == QStringLiteral("vertical")) {
            painter.drawLine(canvasToScreen(position, 0.0), canvasToScreen(position, 1.0));
        } else {
            painter.drawLine(canvasToScreen(0.0, position), canvasToScreen(1.0, position));
        }
    }

    const QVariantMap drawable = grid.value(QStringLiteral("drawable_bounds")).toMap();
    const QPointF drawableTopLeft = canvasToScreen(
        drawable.value(QStringLiteral("x")).toDouble(),
        drawable.value(QStringLiteral("y")).toDouble());
    const QPointF drawableBottomRight = canvasToScreen(
        drawable.value(QStringLiteral("x")).toDouble() + drawable.value(QStringLiteral("width")).toDouble(),
        drawable.value(QStringLiteral("y")).toDouble() + drawable.value(QStringLiteral("height")).toDouble());
    painter.setPen(QPen(QColor("#8fb4d8"), 1, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(drawableTopLeft, drawableBottomRight).normalized());

    const QVariantMap origin = grid.value(QStringLiteral("origin")).toMap();
    const QPointF originPoint = canvasToScreen(origin.value(QStringLiteral("x")).toDouble(), origin.value(QStringLiteral("y")).toDouble());
    painter.setPen(QPen(QColor("#d5bb78"), 1.5));
    painter.drawLine(QPointF(originPoint.x() - 8.0, originPoint.y()), QPointF(originPoint.x() + 8.0, originPoint.y()));
    painter.drawLine(QPointF(originPoint.x(), originPoint.y() - 8.0), QPointF(originPoint.x(), originPoint.y() + 8.0));

    painter.setPen(QPen(QColor("#3d4452"), 1));
    painter.drawRect(board);
}

void DrawingCanvasWidget::drawPointerSnapMarker(QPainter &painter, const QVariantMap &model) const
{
    const QVariantMap pointer = model.value(QStringLiteral("pointer")).toMap();
    if (pointer.isEmpty()) {
        return;
    }

    const QVariantMap snapped = pointer.value(QStringLiteral("snapped")).toMap();
    const QPointF point = canvasToScreen(
        snapped.value(QStringLiteral("x")).toDouble(),
        snapped.value(QStringLiteral("y")).toDouble());
    const QString kind = pointer.value(QStringLiteral("kind")).toString();
    const QString source = pointer.value(QStringLiteral("source")).toString();
    const bool inside = pointer.value(QStringLiteral("inside_drawable")).toBool();
    QColor color("#9aa8b6");
    Qt::PenStyle markerStyle = Qt::SolidLine;
    double markerRadius = 5.0;
    if (!inside) {
        color = QColor("#d98b8b");
        markerRadius = 8.0;
    } else if (kind == QStringLiteral("grid")) {
        color = QColor("#8fb4d8");
        markerRadius = 7.0;
    } else if (source == QStringLiteral("guide")) {
        color = QColor("#54d2c6");
        markerStyle = Qt::DashLine;
        markerRadius = 8.0;
    } else if (kind == QStringLiteral("object")) {
        color = QColor("#91c89b");
        markerRadius = 7.0;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, source == QStringLiteral("guide") ? 2.0 : 1.5, markerStyle));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 44));
    painter.drawEllipse(point, markerRadius, markerRadius);
    painter.drawLine(QPointF(point.x() - 10.0, point.y()), QPointF(point.x() + 10.0, point.y()));
    painter.drawLine(QPointF(point.x(), point.y() - 10.0), QPointF(point.x(), point.y() + 10.0));

    QString label = pointer.value(QStringLiteral("label")).toString();
    const QString sourceObjectId = pointer.value(QStringLiteral("source_object_id")).toString();
    if (source == QStringLiteral("guide") && !sourceObjectId.isEmpty()) {
        label = QStringLiteral("guide %1").arg(sourceObjectId);
    }
    painter.setPen(color);
    painter.drawText(point + QPointF(12.0, -10.0), label.isEmpty() ? kind : label);
    painter.restore();
}

void DrawingCanvasWidget::drawGuideDragSnapIntent(QPainter &painter, const QVariantMap &model) const
{
    const QVariantMap snap = model.value(QStringLiteral("guide_drag_snap")).toMap();
    if (snap.isEmpty()) {
        return;
    }

    const QVariantMap rawAnchor = snap.value(QStringLiteral("raw_anchor")).toMap();
    const QVariantMap snappedAnchor = snap.value(QStringLiteral("snapped_anchor")).toMap();
    const QPointF raw = canvasToScreen(
        rawAnchor.value(QStringLiteral("x")).toDouble(),
        rawAnchor.value(QStringLiteral("y")).toDouble());
    const QPointF snapped = canvasToScreen(
        snappedAnchor.value(QStringLiteral("x")).toDouble(),
        snappedAnchor.value(QStringLiteral("y")).toDouble());
    const QString label = snap.value(QStringLiteral("anchor_label"), QStringLiteral("anchor")).toString();
    const QString sourceId = snap.value(QStringLiteral("source_object_id")).toString();
    const bool intersection = snap.value(QStringLiteral("intersection")).toBool();

    painter.save();
    QColor color("#54d2c6");
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, intersection ? 2.0 : 1.5, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(raw, snapped);
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 56));
    painter.drawEllipse(raw, 5.0, 5.0);
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 96));
    painter.drawEllipse(snapped, intersection ? 8.0 : 6.0, intersection ? 8.0 : 6.0);
    painter.setPen(color);
    painter.drawText(snapped + QPointF(10.0, -12.0),
        sourceId.isEmpty()
            ? QStringLiteral("%1 guide").arg(label)
            : QStringLiteral("%1 guide %2").arg(label, sourceId));
    painter.restore();
}

void DrawingCanvasWidget::drawPlotPreview(QPainter &painter, const QVariantMap &model) const
{
    const QVariantMap plot = model.value(QStringLiteral("plot_summary")).toMap();
    const QVariantMap preview = plot.value(QStringLiteral("preview")).toMap();
    const QVariantList travelSegments = preview.value(QStringLiteral("travel_segments")).toList();
    const QVariantList segments = preview.value(QStringLiteral("segments")).toList();
    if (travelSegments.isEmpty() && segments.isEmpty() && !plot.value(QStringLiteral("has_plot_bounds")).toBool()) {
        return;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::NoBrush);

    QPen travelPen(QColor(213, 187, 120, 130), 1.25, Qt::DashLine);
    travelPen.setCapStyle(Qt::RoundCap);
    painter.setPen(travelPen);
    for (const QVariant &segmentValue : travelSegments) {
        const QVariantMap segment = segmentValue.toMap();
        const double x1 = segment.value(QStringLiteral("x1")).toDouble();
        const double y1 = segment.value(QStringLiteral("y1")).toDouble();
        const double x2 = segment.value(QStringLiteral("x2")).toDouble();
        const double y2 = segment.value(QStringLiteral("y2")).toDouble();
        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) {
            continue;
        }
        painter.drawLine(canvasToScreen(x1, y1), canvasToScreen(x2, y2));
    }

    QPen strokePen(QColor(117, 199, 255, 170), 1.75);
    strokePen.setCapStyle(Qt::RoundCap);
    strokePen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(strokePen);
    for (const QVariant &segmentValue : segments) {
        const QVariantMap segment = segmentValue.toMap();
        const double x1 = segment.value(QStringLiteral("x1")).toDouble();
        const double y1 = segment.value(QStringLiteral("y1")).toDouble();
        const double x2 = segment.value(QStringLiteral("x2")).toDouble();
        const double y2 = segment.value(QStringLiteral("y2")).toDouble();
        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) {
            continue;
        }
        painter.drawLine(canvasToScreen(x1, y1), canvasToScreen(x2, y2));
    }

    painter.restore();
}

void DrawingCanvasWidget::drawPlotSafetyOverlay(QPainter &painter, const QVariantMap &plot) const
{
    if (!plot.value(QStringLiteral("has_plot_bounds")).toBool()) {
        return;
    }

    const QVariantMap bounds = plot.value(QStringLiteral("plot_bounds")).toMap();
    const double x = bounds.value(QStringLiteral("x")).toDouble();
    const double y = bounds.value(QStringLiteral("y")).toDouble();
    const double width = bounds.value(QStringLiteral("width")).toDouble();
    const double height = bounds.value(QStringLiteral("height")).toDouble();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height)) {
        return;
    }

    bool calibratedBoundsWarning = false;
    const QVariantList warnings = plot.value(QStringLiteral("warnings")).toList();
    for (const QVariant &warningValue : warnings) {
        const QVariantMap warning = warningValue.toMap();
        if (warning.value(QStringLiteral("kind")).toString() == QStringLiteral("calibrated_plot_out_of_drawable_bounds")) {
            calibratedBoundsWarning = true;
            break;
        }
    }

    const QColor color = calibratedBoundsWarning ? QColor("#d98b8b") : QColor("#91c89b");
    const QPointF topLeft = canvasToScreen(x, y);
    const QPointF bottomRight = canvasToScreen(x + width, y + height);
    const QRectF rect(topLeft, bottomRight);
    QPen pen(color, calibratedBoundsWarning ? 2.0 : 1.5, calibratedBoundsWarning ? Qt::DashLine : Qt::SolidLine);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), calibratedBoundsWarning ? 34 : 18));
    painter.drawRect(rect.normalized());

    const QString warningKind = plot.value(QStringLiteral("first_warning_kind")).toString();
    const QString warningObjectId = plot.value(QStringLiteral("first_warning_object_id")).toString();
    if (!warningKind.isEmpty()) {
        const QString label = warningObjectId.isEmpty()
            ? warningKind
            : QStringLiteral("%1: %2").arg(warningKind, warningObjectId);
        painter.setPen(color);
        painter.drawText(rect.normalized().topLeft() + QPointF(8.0, -8.0), label);
    }
}

void DrawingCanvasWidget::drawSelectionPlotBounds(QPainter &painter, const QVariantMap &model) const
{
    if (!model.value(QStringLiteral("has_selection_plot_bounds")).toBool()) {
        return;
    }

    const QVariantMap bounds = model.value(QStringLiteral("selection_plot_bounds")).toMap();
    const double x = bounds.value(QStringLiteral("x")).toDouble();
    const double y = bounds.value(QStringLiteral("y")).toDouble();
    const double width = bounds.value(QStringLiteral("width")).toDouble();
    const double height = bounds.value(QStringLiteral("height")).toDouble();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height)) {
        return;
    }

    const QColor color = model.value(QStringLiteral("selection_plot_bounds_status")).toString() == QStringLiteral("inside")
        ? QColor("#f6c65b")
        : QColor("#d98b8b");
    QRectF rect(canvasToScreen(x, y), canvasToScreen(x + width, y + height));
    rect = rect.normalized();
    if (rect.width() < 10.0 || rect.height() < 10.0) {
        rect = rect.adjusted(-5.0, -5.0, 5.0, 5.0);
    }

    painter.save();
    painter.setPen(QPen(color, 1.75, Qt::DashLine));
    painter.setBrush(QColor(color.red(), color.green(), color.blue(), 24));
    painter.drawRect(rect);
    painter.restore();
}
