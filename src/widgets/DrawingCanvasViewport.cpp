#include "widgets/DrawingCanvasViewport.h"

#include <algorithm>
#include <cmath>

namespace drawing_canvas {
namespace {

double positiveFiniteNumber(const QVariant &value, double fallback)
{
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !std::isfinite(number) || number <= 0.0) {
        return fallback;
    }
    return number;
}

} // namespace

double viewportAspect(double gridWidth, double gridHeight)
{
    if (!std::isfinite(gridWidth) || !std::isfinite(gridHeight) || gridWidth <= 0.0 || gridHeight <= 0.0) {
        return 1.0;
    }
    return gridWidth / gridHeight;
}

double clampViewportZoom(double zoom)
{
    if (!std::isfinite(zoom) || zoom <= 0.0) {
        return 1.0;
    }
    return std::clamp(zoom, kViewportMinZoom, kViewportMaxZoom);
}

DrawingCanvasViewportInput viewportInputFromModel(const QVariantMap &model, double widgetWidth, double widgetHeight)
{
    const QVariantMap grid = model.value(QStringLiteral("grid")).toMap();

    DrawingCanvasViewportInput input;
    input.widgetWidth = widgetWidth;
    input.widgetHeight = widgetHeight;
    input.gridWidth = positiveFiniteNumber(grid.value(QStringLiteral("width")), 1.0);
    input.gridHeight = positiveFiniteNumber(grid.value(QStringLiteral("height")), 1.0);
    return input;
}

QRectF viewportFitRect(const DrawingCanvasViewportInput &input)
{
    const double aspect = viewportAspect(input.gridWidth, input.gridHeight);
    const double padding = std::max(0.0, std::isfinite(input.paddingPx) ? input.paddingPx : 48.0);
    const double widgetWidth = std::max(1.0, std::isfinite(input.widgetWidth) ? input.widgetWidth : 1.0);
    const double widgetHeight = std::max(1.0, std::isfinite(input.widgetHeight) ? input.widgetHeight : 1.0);
    const double availableWidth = std::max(1.0, widgetWidth - padding);
    const double availableHeight = std::max(1.0, widgetHeight - padding);

    double boardWidth = availableWidth;
    double boardHeight = boardWidth / aspect;
    if (boardHeight > availableHeight) {
        boardHeight = availableHeight;
        boardWidth = boardHeight * aspect;
    }
    return QRectF((widgetWidth - boardWidth) * 0.5, (widgetHeight - boardHeight) * 0.5, boardWidth, boardHeight);
}

QRectF viewportBoardRect(const DrawingCanvasViewportInput &input)
{
    const QRectF fit = viewportFitRect(input);
    const double zoom = clampViewportZoom(input.zoom);
    const double panX = std::isfinite(input.panXPx) ? input.panXPx : 0.0;
    const double panY = std::isfinite(input.panYPx) ? input.panYPx : 0.0;

    const double boardWidth = fit.width() * zoom;
    const double boardHeight = fit.height() * zoom;
    // Scale about the fit-rect's centre, then translate by the pan offset.
    const double left = fit.center().x() - boardWidth * 0.5 + panX;
    const double top = fit.center().y() - boardHeight * 0.5 + panY;
    return QRectF(left, top, boardWidth, boardHeight);
}

DrawingCanvasViewportInput zoomViewportAtPoint(const DrawingCanvasViewportInput &input, double factor, const QPointF &anchorPx)
{
    DrawingCanvasViewportInput result = input;
    const double safeFactor = (std::isfinite(factor) && factor > 0.0) ? factor : 1.0;
    const double newZoom = clampViewportZoom(clampViewportZoom(input.zoom) * safeFactor);

    const QRectF board = viewportBoardRect(input);
    const double boardWidth = std::max(1.0, board.width());
    const double boardHeight = std::max(1.0, board.height());
    const double anchorX = std::isfinite(anchorPx.x()) ? anchorPx.x() : board.center().x();
    const double anchorY = std::isfinite(anchorPx.y()) ? anchorPx.y() : board.center().y();
    // Canvas-space fraction under the anchor, unclamped so off-board anchors stay exact.
    const double cx = (anchorX - board.left()) / boardWidth;
    const double cy = (anchorY - board.top()) / boardHeight;

    const QRectF fit = viewportFitRect(input);
    const double newWidth = fit.width() * newZoom;
    const double newHeight = fit.height() * newZoom;
    // Solve pan so canvasToScreen(board', c) == anchor (see header).
    result.zoom = newZoom;
    result.panXPx = anchorX - fit.center().x() + newWidth * (0.5 - cx);
    result.panYPx = anchorY - fit.center().y() + newHeight * (0.5 - cy);
    return result;
}

QPointF canvasToScreen(const QRectF &board, double x, double y)
{
    const double safeX = std::isfinite(x) ? x : 0.0;
    const double safeY = std::isfinite(y) ? y : 0.0;
    return QPointF(board.left() + safeX * board.width(), board.top() + safeY * board.height());
}

QRectF boundsToScreenRect(const QRectF &board, double x, double y, double width, double height)
{
    return QRectF(canvasToScreen(board, x, y), canvasToScreen(board, x + width, y + height)).normalized();
}

QPointF screenToCanvas(const QRectF &board, const QPointF &point)
{
    const double width = std::max(1.0, std::isfinite(board.width()) ? board.width() : 1.0);
    const double height = std::max(1.0, std::isfinite(board.height()) ? board.height() : 1.0);
    const double x = std::isfinite(point.x()) ? point.x() : board.left();
    const double y = std::isfinite(point.y()) ? point.y() : board.top();
    return QPointF(
        std::clamp((x - board.left()) / width, 0.0, 1.0),
        std::clamp((y - board.top()) / height, 0.0, 1.0));
}

} // namespace drawing_canvas
