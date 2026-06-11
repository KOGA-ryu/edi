#include "widgets/DrawingCanvasRulerTicks.h"

#include <cassert>

using drawing_canvas::DrawingCanvasProjectedGrid;
using drawing_canvas::DrawingCanvasProjectedGridLine;
using drawing_canvas::DrawingCanvasRulerTicks;
using drawing_canvas::rulerTickLabel;
using drawing_canvas::rulerTicksForGrid;

namespace {

DrawingCanvasProjectedGridLine line(const char *axis, double position, bool major)
{
    DrawingCanvasProjectedGridLine result;
    result.axis = QString::fromLatin1(axis);
    result.position = position;
    result.major = major;
    return result;
}

bool near(double a, double b)
{
    return std::abs(a - b) < 0.0001;
}

} // namespace

int main()
{
    // A 10x8-unit page on a board at (100,50) sized 400x300 in a 600x450
    // widget: vertical lines land on the top ruler, horizontal on the left.
    DrawingCanvasProjectedGrid grid;
    grid.lines = {
        line("vertical", 0.0, true),
        line("vertical", 0.5, false),
        line("vertical", 1.0, true),
        line("horizontal", 0.25, false),
        line("horizontal", 1.0, true),
    };
    const QRectF board(100.0, 50.0, 400.0, 300.0);
    const DrawingCanvasRulerTicks ticks = rulerTicksForGrid(grid, board, 600.0, 450.0, 10.0, 8.0);

    assert(ticks.horizontal.size() == 3);
    assert(near(ticks.horizontal[0].screenPos, 100.0) && near(ticks.horizontal[0].unitValue, 0.0));
    assert(ticks.horizontal[0].major);
    assert(near(ticks.horizontal[1].screenPos, 300.0) && near(ticks.horizontal[1].unitValue, 5.0));
    assert(!ticks.horizontal[1].major);
    assert(near(ticks.horizontal[2].screenPos, 500.0) && near(ticks.horizontal[2].unitValue, 10.0));

    assert(ticks.vertical.size() == 2);
    assert(near(ticks.vertical[0].screenPos, 125.0) && near(ticks.vertical[0].unitValue, 2.0));
    assert(near(ticks.vertical[1].screenPos, 350.0) && near(ticks.vertical[1].unitValue, 8.0));

    // Zoom/pan clipping: pan the board far left — off-widget lines drop, so
    // the ruler labels only what is visible.
    const QRectF panned(-250.0, 50.0, 400.0, 300.0);
    const DrawingCanvasRulerTicks clipped = rulerTicksForGrid(grid, panned, 600.0, 450.0, 10.0, 8.0);
    assert(clipped.horizontal.size() == 1); // only the 1.0 line (x=150) remains
    assert(near(clipped.horizontal[0].unitValue, 10.0));

    // Labels trim like a human would write them.
    assert(rulerTickLabel(3.0) == QStringLiteral("3"));
    assert(rulerTickLabel(2.5) == QStringLiteral("2.5"));
    assert(rulerTickLabel(0.75) == QStringLiteral("0.75"));
    assert(rulerTickLabel(-0.0001) == QStringLiteral("0"));

    return 0;
}
