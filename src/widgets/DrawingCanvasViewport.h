#pragma once

#include <QPointF>
#include <QRectF>

namespace drawing_canvas {

struct DrawingCanvasViewportInput {
    double widgetWidth = 1.0;
    double widgetHeight = 1.0;
    double gridWidth = 1.0;
    double gridHeight = 1.0;
    double paddingPx = 48.0;
};

double viewportAspect(double gridWidth, double gridHeight);
QRectF viewportBoardRect(const DrawingCanvasViewportInput &input);
QPointF canvasToScreen(const QRectF &board, double x, double y);
QPointF screenToCanvas(const QRectF &board, const QPointF &point);

} // namespace drawing_canvas
