#include "widgets/DrawingCanvasViewport.h"

#include <QVariantMap>

#include <cassert>
#include <cmath>
#include <limits>

using namespace drawing_canvas;

namespace {

bool near(double a, double b, double epsilon = 0.000001)
{
    return std::abs(a - b) <= epsilon;
}

} // namespace

int main()
{
    const QRectF square = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = 1.0,
        .gridHeight = 1.0,
    });
    assert(near(square.width(), 352.0));
    assert(near(square.height(), 352.0));
    assert(near(square.left(), 74.0));
    assert(near(square.top(), 24.0));

    const QRectF wide = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = 2.0,
        .gridHeight = 1.0,
    });
    assert(near(wide.width(), 452.0));
    assert(near(wide.height(), 226.0));
    assert(near(wide.left(), 24.0));
    assert(near(wide.top(), 87.0));

    const QRectF tall = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = 1.0,
        .gridHeight = 2.0,
    });
    assert(near(tall.width(), 176.0));
    assert(near(tall.height(), 352.0));
    assert(near(tall.left(), 162.0));
    assert(near(tall.top(), 24.0));

    const QPointF screen = canvasToScreen(wide, 0.25, 0.5);
    const QPointF roundTrip = screenToCanvas(wide, screen);
    assert(near(roundTrip.x(), 0.25));
    assert(near(roundTrip.y(), 0.5));

    const QPointF lowClamp = screenToCanvas(wide, QPointF(wide.left() - 100.0, wide.top() - 100.0));
    assert(near(lowClamp.x(), 0.0));
    assert(near(lowClamp.y(), 0.0));

    const QPointF highClamp = screenToCanvas(wide, QPointF(wide.right() + 100.0, wide.bottom() + 100.0));
    assert(near(highClamp.x(), 1.0));
    assert(near(highClamp.y(), 1.0));

    const QRectF badGrid = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = std::numeric_limits<double>::quiet_NaN(),
        .gridHeight = 0.0,
    });
    assert(near(badGrid.width(), square.width()));
    assert(near(badGrid.height(), square.height()));
    assert(near(viewportAspect(-10.0, 2.0), 1.0));

    const QRectF badWidget = viewportBoardRect({
        .widgetWidth = std::numeric_limits<double>::quiet_NaN(),
        .widgetHeight = -20.0,
        .gridWidth = 1.0,
        .gridHeight = 1.0,
    });
    assert(std::isfinite(badWidget.width()));
    assert(std::isfinite(badWidget.height()));
    assert(badWidget.width() > 0.0);
    assert(badWidget.height() > 0.0);

    const QPointF badInput = canvasToScreen(wide, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    assert(near(badInput.x(), wide.left()));
    assert(near(badInput.y(), wide.top()));

    const DrawingCanvasViewportInput projected = viewportInputFromModel(QVariantMap{
        {QStringLiteral("grid"), QVariantMap{
            {QStringLiteral("width"), 11.0},
            {QStringLiteral("height"), 8.5},
        }},
    },
        600.0,
        400.0);
    assert(near(projected.widgetWidth, 600.0));
    assert(near(projected.widgetHeight, 400.0));
    assert(near(projected.gridWidth, 11.0));
    assert(near(projected.gridHeight, 8.5));

    const DrawingCanvasViewportInput badProjected = viewportInputFromModel(QVariantMap{
        {QStringLiteral("grid"), QVariantMap{
            {QStringLiteral("width"), QStringLiteral("bad")},
            {QStringLiteral("height"), -8.5},
        }},
    },
        600.0,
        400.0);
    assert(near(badProjected.gridWidth, 1.0));
    assert(near(badProjected.gridHeight, 1.0));

    return 0;
}
