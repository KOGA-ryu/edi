#pragma once

#include <QString>
#include <QVariantMap>
#include <QWidget>

#include "widgets/DrawingCanvasGestureState.h"

class DrawingDocumentController;

class DrawingCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent = nullptr);
    void setPlotPreviewVisible(bool visible);
    bool plotPreviewVisible() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void refresh();

private:
    QRectF boardRect() const;
    QPointF canvasToScreen(double x, double y) const;
    QRectF boundsToScreenRect(double x, double y, double width, double height) const;
    QPointF screenToCanvas(const QPointF &point) const;
    QVariantMap selectedObjectProjection() const;
    QString hitSelectedHandle(const QPointF &screenPoint) const;
    void drawPhysicalGrid(QPainter &painter, const QVariantMap &model) const;
    void drawPointerSnapMarker(QPainter &painter, const QVariantMap &model) const;
    void drawGuideDragSnapIntent(QPainter &painter, const QVariantMap &model) const;
    void drawPlotPreview(QPainter &painter, const QVariantMap &plotSummary) const;
    void drawPlotSafetyOverlay(QPainter &painter, const QVariantMap &plot) const;
    void drawSelectionPlotBounds(QPainter &painter, const QVariantMap &model) const;

    DrawingDocumentController *m_controller = nullptr;
    drawing_canvas::DrawingCanvasGestureState m_gestureState;
    QPointF m_lastDragCanvasPoint;
    bool m_plotPreviewVisible = false;
};
