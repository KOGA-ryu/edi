#pragma once

#include <QPointF>
#include <QRectF>
#include <QVariantMap>

namespace drawing_canvas {

inline constexpr double kViewportMinZoom = 0.2;
inline constexpr double kViewportMaxZoom = 16.0;

struct DrawingCanvasViewportInput {
    double widgetWidth = 1.0;
    double widgetHeight = 1.0;
    double gridWidth = 1.0;
    double gridHeight = 1.0;
    double paddingPx = 48.0;
    double zoom = 1.0;
    double panXPx = 0.0;
    double panYPx = 0.0;
};

double viewportAspect(double gridWidth, double gridHeight);
double clampViewportZoom(double zoom);
DrawingCanvasViewportInput viewportInputFromModel(const QVariantMap &model, double widgetWidth, double widgetHeight);
// Base fit-rect at zoom 1 / pan 0 (centered, aspect-correct).
QRectF viewportFitRect(const DrawingCanvasViewportInput &input);
// Fit-rect scaled by the clamped zoom and offset by pan.
QRectF viewportBoardRect(const DrawingCanvasViewportInput &input);
// Returns a new input whose zoom is scaled by factor (clamped) and whose pan is
// solved so the canvas point currently under anchorPx stays under anchorPx.
DrawingCanvasViewportInput zoomViewportAtPoint(const DrawingCanvasViewportInput &input, double factor, const QPointF &anchorPx);
// DM-01 view-auto-fit: given a content box in NORMALIZED canvas coords ([0,1] over
// the grid — the same space documentObjectsBounds() reports), return a new input
// whose zoom + pan FRAME that box centered in the viewport with `paddingPx` of
// margin on every side. A degenerate (zero-area) box leaves zoom/pan untouched —
// there is nothing to frame, so the caller's view is preserved. The zoom is
// clamped to the viewport limits; a box larger than the limits allow simply
// renders as large as the clamp permits, still centered.
DrawingCanvasViewportInput computeFitView(const DrawingCanvasViewportInput &input,
                                          double contentX, double contentY,
                                          double contentWidth, double contentHeight);
QPointF canvasToScreen(const QRectF &board, double x, double y);
QRectF boundsToScreenRect(const QRectF &board, double x, double y, double width, double height);
QPointF screenToCanvas(const QRectF &board, const QPointF &point);

} // namespace drawing_canvas
