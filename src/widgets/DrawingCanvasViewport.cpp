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

DrawingCanvasViewportInput computeFitView(const DrawingCanvasViewportInput &input,
                                          double contentX, double contentY,
                                          double contentWidth, double contentHeight)
{
    DrawingCanvasViewportInput result = input;
    // A degenerate box has no extent to frame — leave the view exactly as the
    // caller had it (the empty-document no-op the spec asks for falls out here,
    // since documentObjectsBounds() returns nullopt and the caller skips us).
    if (!std::isfinite(contentWidth) || !std::isfinite(contentHeight) || contentWidth <= 0.0 || contentHeight <= 0.0
        || !std::isfinite(contentX) || !std::isfinite(contentY)) {
        return result;
    }

    // The board maps normalized [0,1] canvas coords onto a rect of size
    // fit.width()*zoom x fit.height()*zoom (see viewportBoardRect). So a content
    // box of normalized size (bw,bh) paints at screen size (bw*fit.w*zoom,
    // bh*fit.h*zoom). Solve the zoom that makes that just fit the padded viewport,
    // taking the tighter of the two axes so nothing clips.
    const QRectF fit = viewportFitRect(input);
    const double fitWidth = std::max(1.0, fit.width());
    const double fitHeight = std::max(1.0, fit.height());
    const double padding = std::max(0.0, std::isfinite(input.paddingPx) ? input.paddingPx : 48.0);
    const double widgetWidth = std::max(1.0, std::isfinite(input.widgetWidth) ? input.widgetWidth : 1.0);
    const double widgetHeight = std::max(1.0, std::isfinite(input.widgetHeight) ? input.widgetHeight : 1.0);
    // Pad on BOTH sides of each axis (the box is centered), hence 2*padding.
    const double availWidth = std::max(1.0, widgetWidth - 2.0 * padding);
    const double availHeight = std::max(1.0, widgetHeight - 2.0 * padding);

    const double zoomForWidth = availWidth / (contentWidth * fitWidth);
    const double zoomForHeight = availHeight / (contentHeight * fitHeight);
    const double zoom = clampViewportZoom(std::min(zoomForWidth, zoomForHeight));

    // Pan so the content-box CENTER lands at the viewport center. From
    // viewportBoardRect: left = fit.center.x - boardW/2 + panX, and the box center
    // sits at left + (bx + bw/2)*boardW. Set that equal to widgetWidth/2 and solve.
    const double boardWidth = fitWidth * zoom;
    const double boardHeight = fitHeight * zoom;
    const double boxCenterFracX = contentX + contentWidth * 0.5;
    const double boxCenterFracY = contentY + contentHeight * 0.5;
    result.zoom = zoom;
    result.panXPx = widgetWidth * 0.5 - fit.center().x() + boardWidth * (0.5 - boxCenterFracX);
    result.panYPx = widgetHeight * 0.5 - fit.center().y() + boardHeight * (0.5 - boxCenterFracY);
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
