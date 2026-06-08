#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

BoardBounds boardBounds(double viewWidth, double viewHeight, double zoom, double panX, double panY);
double canvasToScreenX(const BoardBounds &bounds, double x);
double canvasToScreenY(const BoardBounds &bounds, double y);
ScreenPoint canvasToScreen(const BoardBounds &bounds, const CanvasPoint &point);
CanvasPoint screenToCanvas(const BoardBounds &bounds, const ScreenPoint &point);
BoardBounds boardBoundsFromVariant(const QVariantMap &bounds);

} // namespace drawing_canvas
