#include "DrawingCanvasViewport.h"

#include <algorithm>
#include <cmath>

namespace drawing_canvas {

BoardBounds boardBounds(double viewWidth, double viewHeight, double zoom, double panX, double panY) {
    const double width = std::max(0.0, finiteNumber(viewWidth, 0.0));
    const double height = std::max(0.0, finiteNumber(viewHeight, 0.0));
    const double safeZoom = std::max(0.0001, finiteNumber(zoom, 1.0));
    const double safePanX = finiteNumber(panX, 0.0);
    const double safePanY = finiteNumber(panY, 0.0);
    const double board = std::max(32.0, std::min(width, height) - 16.0) * safeZoom;
    return {
        std::round((width - board) / 2.0 + safePanX),
        std::round((height - board) / 2.0 + safePanY),
        board
    };
}

double canvasToScreenX(const BoardBounds &bounds, double x) {
    return finiteNumber(bounds.x, 0.0) + finiteNumber(x, 0.0) * finiteNumber(bounds.size, 1.0);
}

double canvasToScreenY(const BoardBounds &bounds, double y) {
    return finiteNumber(bounds.y, 0.0) + finiteNumber(y, 0.0) * finiteNumber(bounds.size, 1.0);
}

ScreenPoint canvasToScreen(const BoardBounds &bounds, const CanvasPoint &point) {
    return {canvasToScreenX(bounds, point.x), canvasToScreenY(bounds, point.y)};
}

CanvasPoint screenToCanvas(const BoardBounds &bounds, const ScreenPoint &point) {
    const double size = std::max(0.000001, finiteNumber(bounds.size, 1.0));
    return {
        (finiteNumber(point.x, 0.0) - finiteNumber(bounds.x, 0.0)) / size,
        (finiteNumber(point.y, 0.0) - finiteNumber(bounds.y, 0.0)) / size
    };
}

BoardBounds boardBoundsFromVariant(const QVariantMap &bounds) {
    return {
        finiteNumber(bounds.value(QStringLiteral("x")), 0.0),
        finiteNumber(bounds.value(QStringLiteral("y")), 0.0),
        std::max(0.000001, finiteNumber(bounds.value(QStringLiteral("size")), 1.0))
    };
}

} // namespace drawing_canvas
