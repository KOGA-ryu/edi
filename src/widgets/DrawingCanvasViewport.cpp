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

QRectF viewportBoardRect(const DrawingCanvasViewportInput &input)
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

QPointF canvasToScreen(const QRectF &board, double x, double y)
{
    const double safeX = std::isfinite(x) ? x : 0.0;
    const double safeY = std::isfinite(y) ? y : 0.0;
    return QPointF(board.left() + safeX * board.width(), board.top() + safeY * board.height());
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
