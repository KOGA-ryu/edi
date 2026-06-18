#pragma once

#include <QMarginsF>
#include <QPixmap>
#include <QString>
#include <QVariantMap>
#include <QWidget>

#include "widgets/DrawingCanvasGestureState.h"
#include "widgets/DrawingCanvasObjectPainter.h"
#include "widgets/DrawingCanvasPalette.h"
#include "widgets/DrawingCanvasViewport.h"

#include <vector>

class DrawingDocumentController;

class DrawingCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit DrawingCanvasWidget(DrawingDocumentController *controller, QWidget *parent = nullptr);
    void setPlotPreviewVisible(bool visible);
    void zoomAtCenter(double factor);
    void resetView(); // zoom 1, pan 0 — which IS fit-to-board by layout
    // DM-01: frame the whole document (every object's union bounds) in the
    // viewport. A view-only transform — it reads computeDocumentBounds() and
    // applies a pan/zoom, never touching the model, so it cannot loop modelChanged.
    // An empty document (nullopt bounds) is a no-op, leaving the view as-is.
    void fitDocumentInView();
    // Overlay occlusion the shell reports: the right/bottom feature panels float
    // ON TOP of the canvas, so the auto-fit must frame content into the visible
    // sub-rectangle (widget minus these insets) rather than under a panel. Pure
    // DATA — the shell owns panel geometry and pushes it here; the canvas stays
    // ignorant of which panels exist. STORE-ONLY: setting insets does NOT re-fit
    // (re-fitting on every overlay open/close/resize would yank a user's panned/
    // zoomed view); it just records the occlusion + repaints. The deliberate fit
    // (fitDocumentInView, on load/reset) reads the current insets when it runs.
    void setViewInsets(const QMarginsF &insets);
    QMarginsF viewInsets() const { return m_viewInsets; }
    bool plotPreviewVisible() const;
    // Re-theme the canvas chrome (board, grid, snap markers...). Object
    // stroke colors are document data and are untouched by this.
    void setCanvasPalette(const drawing_canvas::DrawingCanvasPalette &palette);
    // Where a canvas-normalized point lands on screen under the current zoom/pan.
    // Public so tests can assert anchor invariance after a zoom gesture.
    QPointF mapCanvasToScreen(double x, double y) const;
    double viewportZoom() const { return m_zoom; }

signals:
    // Zoom changed (wheel, keys, pinch, reset). Pan deliberately excluded:
    // consumers today show only the percentage.
    void zoomChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override; // macOS pinch (QNativeGestureEvent)

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
    void drawPhysicalGrid(QPainter &painter, const QRectF &board, const QVariantMap &model) const;
    // The measuring side bars (top + left), in document units, tracking
    // zoom/pan through the board transform. Static-layer content; the
    // pointer caret on them is painted dynamically.
    void drawRulers(QPainter &painter, const QRectF &board, const QVariantMap &model) const;
    void drawRulerPointerCaret(QPainter &painter, const QRectF &board, const QVariantMap &model) const;
    // The typed scene: objects pre-extracted into painter structs, rebuilt
    // only when the controller's generation moves. Steady-state frames
    // (mouse tracking, zoom, pan) paint typed items and never touch
    // QVariantMap — the map-unpack-per-object-per-frame cost is gone.
    const std::vector<drawing_canvas::DrawingCanvasSceneItem> &sceneItems(const QVariantMap &model) const;
    void drawPointerSnapMarker(QPainter &painter, const QRectF &board, const QVariantMap &model) const;
    void drawGuideDragSnapIntent(QPainter &painter, const QRectF &board, const QVariantMap &model) const;
    void drawPlotPreview(QPainter &painter, const QRectF &board, const QVariantMap &plotSummary) const;
    void drawPlotSafetyOverlay(QPainter &painter, const QRectF &board, const QVariantMap &plot) const;
    void drawSelectionPlotBounds(QPainter &painter, const QRectF &board, const QVariantMap &model) const;

    DrawingDocumentController *m_controller = nullptr;
    // Chrome/feedback colors only — object strokes stay document data. Set in
    // the constructor from the default shell theme; a theme-switching phase
    // will pass a custom palette through here.
    drawing_canvas::DrawingCanvasPalette m_palette;
    drawing_canvas::DrawingCanvasGestureState m_gestureState;
    mutable std::vector<drawing_canvas::DrawingCanvasSceneItem> m_sceneCache;
    mutable quint64 m_sceneGeneration = 0; // 0 = never built
    // The static layer: backdrop + grid + every document object, rendered
    // once per (mutation, viewport) and blitted per frame. Sampling showed
    // steady frames were 97% software AA line rasterization — the only way
    // to not pay it per mouse move is to not rasterize per mouse move.
    mutable QPixmap m_staticLayer;
    mutable quint64 m_staticGeneration = 0;
    mutable QRectF m_staticBoard;
    mutable QSize m_staticSize;
    QPointF m_lastDragCanvasPoint;
    bool m_plotPreviewVisible = false;
    double m_zoom = 1.0;
    QPointF m_pan;
    QPointF m_lastPanScreenPoint;
    // Pixels of the canvas occluded by the shell's floating overlay panels, by
    // edge. Default 0 = no overlays = fit fills the whole widget.
    QMarginsF m_viewInsets;
};
