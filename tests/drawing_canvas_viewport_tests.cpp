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

    const QRectF board(100.0, 50.0, 200.0, 100.0);
    const QRectF screenBounds = boundsToScreenRect(board, 0.25, 0.25, 0.5, 0.5);
    assert(near(screenBounds.left(), 150.0));
    assert(near(screenBounds.top(), 75.0));
    assert(near(screenBounds.width(), 100.0));
    assert(near(screenBounds.height(), 50.0));

    // Negative extents normalize.
    const QRectF inverted = boundsToScreenRect(board, 0.75, 0.75, -0.5, -0.5);
    assert(near(inverted.left(), 150.0));
    assert(near(inverted.top(), 75.0));
    assert(near(inverted.width(), 100.0));
    assert(near(inverted.height(), 50.0));

    // --- zoom and pan -------------------------------------------------------
    const DrawingCanvasViewportInput baseInput{
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = 1.0,
        .gridHeight = 1.0,
    };
    const QRectF fit = viewportFitRect(baseInput);
    // Default zoom/pan reproduces the bare fit-rect.
    const QRectF identityBoard = viewportBoardRect(baseInput);
    assert(near(identityBoard.width(), fit.width()));
    assert(near(identityBoard.left(), fit.left()));

    // Zoom scales about the fit centre.
    DrawingCanvasViewportInput zoomed = baseInput;
    zoomed.zoom = 2.0;
    const QRectF zoomedBoard = viewportBoardRect(zoomed);
    assert(near(zoomedBoard.width(), fit.width() * 2.0));
    assert(near(zoomedBoard.height(), fit.height() * 2.0));
    assert(near(zoomedBoard.center().x(), fit.center().x()));
    assert(near(zoomedBoard.center().y(), fit.center().y()));

    // Zoom clamps to [0.2, 16].
    assert(near(clampViewportZoom(1000.0), kViewportMaxZoom));
    assert(near(clampViewportZoom(0.0001), kViewportMinZoom));
    assert(near(clampViewportZoom(std::numeric_limits<double>::quiet_NaN()), 1.0));
    DrawingCanvasViewportInput overZoom = baseInput;
    overZoom.zoom = 1000.0;
    assert(near(viewportBoardRect(overZoom).width(), fit.width() * kViewportMaxZoom));

    // Pan offsets the board by the pixel deltas.
    DrawingCanvasViewportInput panned = baseInput;
    panned.panXPx = 12.0;
    panned.panYPx = -7.0;
    const QRectF pannedBoard = viewportBoardRect(panned);
    assert(near(pannedBoard.left(), fit.left() + 12.0));
    assert(near(pannedBoard.top(), fit.top() - 7.0));

    // Anchor invariance: the canvas point under the cursor is unmoved by zoom.
    {
        const QRectF before = viewportBoardRect(baseInput);
        const QPointF anchor(200.0, 150.0);
        const double cx = (anchor.x() - before.left()) / before.width();
        const double cy = (anchor.y() - before.top()) / before.height();

        const DrawingCanvasViewportInput zoomedIn = zoomViewportAtPoint(baseInput, 2.0, anchor);
        assert(near(clampViewportZoom(zoomedIn.zoom), 2.0));
        const QRectF after = viewportBoardRect(zoomedIn);
        const QPointF mapped = canvasToScreen(after, cx, cy);
        assert(near(mapped.x(), anchor.x()));
        assert(near(mapped.y(), anchor.y()));

        // Zooming out then keeps the same anchor fixed too.
        const DrawingCanvasViewportInput zoomedOut = zoomViewportAtPoint(zoomedIn, 0.5, anchor);
        const QRectF after2 = viewportBoardRect(zoomedOut);
        const QPointF mapped2 = canvasToScreen(after2, cx, cy);
        assert(near(mapped2.x(), anchor.x()));
        assert(near(mapped2.y(), anchor.y()));

        // Anchor invariance holds even at the zoom clamp.
        const DrawingCanvasViewportInput clampedZoom = zoomViewportAtPoint(baseInput, 10000.0, anchor);
        assert(near(clampViewportZoom(clampedZoom.zoom), kViewportMaxZoom));
        const QPointF mappedClamped = canvasToScreen(viewportBoardRect(clampedZoom), cx, cy);
        assert(near(mappedClamped.x(), anchor.x()));
        assert(near(mappedClamped.y(), anchor.y()));
    }

    return 0;
}
