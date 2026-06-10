#pragma once

#include <QPointF>
#include <QRectF>
#include <QVariantMap>

namespace drawing_canvas {

struct DrawingCanvasViewportInput {
    double widgetWidth = 1.0;
    double widgetHeight = 1.0;
    double gridWidth = 1.0;
    double gridHeight = 1.0;
    double paddingPx = 48.0;
};

double viewportAspect(double gridWidth, double gridHeight);
DrawingCanvasViewportInput viewportInputFromModel(const QVariantMap &model, double widgetWidth, double widgetHeight);
QRectF viewportBoardRect(const DrawingCanvasViewportInput &input);
QPointF canvasToScreen(const QRectF &board, double x, double y);
QRectF boundsToScreenRect(const QRectF &board, double x, double y, double width, double height);
QPointF screenToCanvas(const QRectF &board, const QPointF &point);

} // namespace drawing_canvas
