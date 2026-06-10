#pragma once

#include <QString>
#include <QVariantMap>
#include <QWidget>

#include "widgets/DrawingCanvasGestureState.h"
#include "widgets/DrawingCanvasPalette.h"
#include "widgets/DrawingCanvasViewport.h"

class DrawingDocumentController;

class DrawingCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent = nullptr);
    void setPlotPreviewVisible(bool visible);
    bool plotPreviewVisible() const;
    // Where a canvas-normalized point lands on screen under the current zoom/pan.
    // Public so tests can assert anchor invariance after a zoom gesture.
    QPointF mapCanvasToScreen(double x, double y) const;
    double viewportZoom() const { return m_zoom; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void refresh();

private:
    drawing_canvas::DrawingCanvasViewportInput viewportInput() const;
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
    // Chrome/feedback colors only — object strokes stay document data. Set in
    // the constructor from the default shell theme; a theme-switching phase
    // will pass a custom palette through here.
    drawing_canvas::DrawingCanvasPalette m_palette;
    drawing_canvas::DrawingCanvasGestureState m_gestureState;
    QPointF m_lastDragCanvasPoint;
    bool m_plotPreviewVisible = false;
    double m_zoom = 1.0;
    QPointF m_pan;
    QPointF m_lastPanScreenPoint;
};
