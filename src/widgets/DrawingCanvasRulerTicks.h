#pragma once

#include "widgets/DrawingCanvasProjectedGrid.h"

#include <QRectF>
#include <QString>

#include <vector>

namespace drawing_canvas {

// One ruler tick: where it sits along its edge (screen px), what it
// measures (document units: normalized position x grid width/height — the
// same conversion the pointer readout uses), and whether it is a major
// line that deserves a label.
struct DrawingCanvasRulerTick {
    double screenPos = 0.0;
    double unitValue = 0.0;
    bool major = false;
};

// Tick sets for the two measuring side bars: `horizontal` runs along the
// top edge (one tick per vertical grid line), `vertical` down the left
// edge. Pure data out of pure inputs — the painter draws, this measures.
struct DrawingCanvasRulerTicks {
    std::vector<DrawingCanvasRulerTick> horizontal;
    std::vector<DrawingCanvasRulerTick> vertical;
};

// Map the projected grid's lines onto ruler ticks under the current board
// transform (board = the page rectangle in widget px, already zoomed and
// panned). Ticks outside [0, widgetWidth/Height) are clipped — at high zoom
// most of the grid is off-screen and the ruler only labels what is visible.
DrawingCanvasRulerTicks rulerTicksForGrid(
    const DrawingCanvasProjectedGrid &grid,
    const QRectF &board,
    double widgetWidth,
    double widgetHeight,
    double gridUnitWidth,
    double gridUnitHeight);

// Trimmed unit label for a tick ("3", "2.5", "0.75") — fixed precision with
// trailing zeros removed, so dense inch grids stay readable.
QString rulerTickLabel(double unitValue);

} // namespace drawing_canvas
