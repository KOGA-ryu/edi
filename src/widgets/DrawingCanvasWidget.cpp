#include "widgets/DrawingCanvasWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>

#include "core/DrawingCore.h"

DrawingCanvasWidget::DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
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
    for (const QVariant &value : model.value("generated_objects").toList()) {
        drawObject(painter, value.toMap());
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
    const QPointF point = screenToCanvas(event->position());
    m_controller->clickCanvasNormalized(point.x(), point.y());
}

void DrawingCanvasWidget::drawObject(QPainter &painter, const QVariantMap &object) const
{
    const QString kind = object.value("kind").toString();
    const bool selected = object.value("selected").toBool()
        || object.value("id").toString() == (m_controller != nullptr ? m_controller->selectedObjectId() : QString());
    painter.setPen(QPen(selected ? QColor("#f4d46f") : QColor("#79b8ff"), selected ? 2.4 : 1.6));
    painter.setBrush(Qt::NoBrush);

    if (kind == "point" || kind == "tone_probe") {
        const QVariantList point = object.value("point").toList();
        const QPointF screen = canvasToScreen(point.value(0).toDouble(), point.value(1).toDouble());
        painter.setBrush(selected ? QColor("#f4d46f") : QColor("#79b8ff"));
        painter.drawEllipse(screen, 4, 4);
        return;
    }
    if (kind == "line" || kind == "glyph_baseline") {
        const QPointF from = canvasToScreen(object.value("x1").toDouble(), object.value("y1").toDouble());
        const QPointF to = canvasToScreen(object.value("x2").toDouble(), object.value("y2").toDouble());
        painter.drawLine(from, to);
        return;
    }
    if (kind == "circle" || kind == "arc" || kind == "polygon") {
        const QPointF center = canvasToScreen(object.value("cx").toDouble(), object.value("cy").toDouble());
        const double radius = object.value("radius").toDouble() * boardRect().width();
        painter.drawEllipse(center, radius, radius);
        return;
    }
    if (kind == "rectangle" || kind == "image_reference_frame" || kind == "ascii_crop_frame" || kind == "ascii_cell_region") {
        const QPointF topLeft = canvasToScreen(object.value("x").toDouble(), object.value("y").toDouble());
        const QSizeF size(object.value("width").toDouble() * boardRect().width(), object.value("height").toDouble() * boardRect().height());
        painter.drawRect(QRectF(topLeft, size));
    }
}
