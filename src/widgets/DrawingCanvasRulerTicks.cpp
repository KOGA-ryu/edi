#include "widgets/DrawingCanvasRulerTicks.h"

#include "widgets/DrawingCanvasViewport.h"

namespace drawing_canvas {

DrawingCanvasRulerTicks rulerTicksForGrid(
    const DrawingCanvasProjectedGrid &grid,
    const QRectF &board,
    double widgetWidth,
    double widgetHeight,
    double gridUnitWidth,
    double gridUnitHeight)
{
    DrawingCanvasRulerTicks ticks;
    for (const DrawingCanvasProjectedGridLine &line : grid.lines) {
        if (line.axis == QStringLiteral("vertical")) {
            const double x = canvasToScreen(board, line.position, 0.0).x();
            if (x >= 0.0 && x < widgetWidth) {
                ticks.horizontal.push_back({x, line.position * gridUnitWidth, line.major});
            }
        } else {
            const double y = canvasToScreen(board, 0.0, line.position).y();
            if (y >= 0.0 && y < widgetHeight) {
                ticks.vertical.push_back({y, line.position * gridUnitHeight, line.major});
            }
        }
    }
    return ticks;
}

QString rulerTickLabel(double unitValue)
{
    QString label = QString::number(unitValue, 'f', 2);
    while (label.endsWith(QLatin1Char('0'))) {
        label.chop(1);
    }
    if (label.endsWith(QLatin1Char('.'))) {
        label.chop(1);
    }
    // Avoid the "-0" oddity a tiny negative rounding could print.
    return label == QStringLiteral("-0") ? QStringLiteral("0") : label;
}

} // namespace drawing_canvas
