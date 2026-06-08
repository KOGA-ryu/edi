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


