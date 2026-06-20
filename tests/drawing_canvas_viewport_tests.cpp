#include "widgets/DrawingCanvasViewport.h"

#include <QVariantMap>

#include "EdiAssert.h"
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
    EDI_CHECK(near(square.width(), 352.0));
    EDI_CHECK(near(square.height(), 352.0));
    EDI_CHECK(near(square.left(), 74.0));
    EDI_CHECK(near(square.top(), 24.0));

    const QRectF wide = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = 2.0,
        .gridHeight = 1.0,
    });
    EDI_CHECK(near(wide.width(), 452.0));
    EDI_CHECK(near(wide.height(), 226.0));
    EDI_CHECK(near(wide.left(), 24.0));
    EDI_CHECK(near(wide.top(), 87.0));

    const QRectF tall = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = 1.0,
        .gridHeight = 2.0,
    });
    EDI_CHECK(near(tall.width(), 176.0));
    EDI_CHECK(near(tall.height(), 352.0));
    EDI_CHECK(near(tall.left(), 162.0));
    EDI_CHECK(near(tall.top(), 24.0));

    const QPointF screen = canvasToScreen(wide, 0.25, 0.5);
    const QPointF roundTrip = screenToCanvas(wide, screen);
    EDI_CHECK(near(roundTrip.x(), 0.25));
    EDI_CHECK(near(roundTrip.y(), 0.5));

    const QPointF lowClamp = screenToCanvas(wide, QPointF(wide.left() - 100.0, wide.top() - 100.0));
    EDI_CHECK(near(lowClamp.x(), 0.0));
    EDI_CHECK(near(lowClamp.y(), 0.0));

    const QPointF highClamp = screenToCanvas(wide, QPointF(wide.right() + 100.0, wide.bottom() + 100.0));
    EDI_CHECK(near(highClamp.x(), 1.0));
    EDI_CHECK(near(highClamp.y(), 1.0));

    const QRectF badGrid = viewportBoardRect({
        .widgetWidth = 500.0,
        .widgetHeight = 400.0,
        .gridWidth = std::numeric_limits<double>::quiet_NaN(),
        .gridHeight = 0.0,
    });
    EDI_CHECK(near(badGrid.width(), square.width()));
    EDI_CHECK(near(badGrid.height(), square.height()));
    EDI_CHECK(near(viewportAspect(-10.0, 2.0), 1.0));

    const QRectF badWidget = viewportBoardRect({
        .widgetWidth = std::numeric_limits<double>::quiet_NaN(),
        .widgetHeight = -20.0,
        .gridWidth = 1.0,
        .gridHeight = 1.0,
    });
    EDI_CHECK(std::isfinite(badWidget.width()));
    EDI_CHECK(std::isfinite(badWidget.height()));
    EDI_CHECK(badWidget.width() > 0.0);
    EDI_CHECK(badWidget.height() > 0.0);

    const QPointF badInput = canvasToScreen(wide, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    EDI_CHECK(near(badInput.x(), wide.left()));
    EDI_CHECK(near(badInput.y(), wide.top()));

    const DrawingCanvasViewportInput projected = viewportInputFromModel(QVariantMap{
        {QStringLiteral("grid"), QVariantMap{
            {QStringLiteral("width"), 11.0},
            {QStringLiteral("height"), 8.5},
        }},
    },
        600.0,
        400.0);
    EDI_CHECK(near(projected.widgetWidth, 600.0));
    EDI_CHECK(near(projected.widgetHeight, 400.0));
    EDI_CHECK(near(projected.gridWidth, 11.0));
    EDI_CHECK(near(projected.gridHeight, 8.5));

    const DrawingCanvasViewportInput badProjected = viewportInputFromModel(QVariantMap{
        {QStringLiteral("grid"), QVariantMap{
            {QStringLiteral("width"), QStringLiteral("bad")},
            {QStringLiteral("height"), -8.5},
        }},
    },
        600.0,
        400.0);
    EDI_CHECK(near(badProjected.gridWidth, 1.0));
    EDI_CHECK(near(badProjected.gridHeight, 1.0));

    const QRectF board(100.0, 50.0, 200.0, 100.0);
    const QRectF screenBounds = boundsToScreenRect(board, 0.25, 0.25, 0.5, 0.5);
    EDI_CHECK(near(screenBounds.left(), 150.0));
    EDI_CHECK(near(screenBounds.top(), 75.0));
    EDI_CHECK(near(screenBounds.width(), 100.0));
    EDI_CHECK(near(screenBounds.height(), 50.0));

    // Negative extents normalize.
    const QRectF inverted = boundsToScreenRect(board, 0.75, 0.75, -0.5, -0.5);
    EDI_CHECK(near(inverted.left(), 150.0));
    EDI_CHECK(near(inverted.top(), 75.0));
    EDI_CHECK(near(inverted.width(), 100.0));
    EDI_CHECK(near(inverted.height(), 50.0));

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
    EDI_CHECK(near(identityBoard.width(), fit.width()));
    EDI_CHECK(near(identityBoard.left(), fit.left()));

    // Zoom scales about the fit centre.
    DrawingCanvasViewportInput zoomed = baseInput;
    zoomed.zoom = 2.0;
    const QRectF zoomedBoard = viewportBoardRect(zoomed);
    EDI_CHECK(near(zoomedBoard.width(), fit.width() * 2.0));
    EDI_CHECK(near(zoomedBoard.height(), fit.height() * 2.0));
    EDI_CHECK(near(zoomedBoard.center().x(), fit.center().x()));
    EDI_CHECK(near(zoomedBoard.center().y(), fit.center().y()));

    // Zoom clamps to [0.2, 16].
    EDI_CHECK(near(clampViewportZoom(1000.0), kViewportMaxZoom));
    EDI_CHECK(near(clampViewportZoom(0.0001), kViewportMinZoom));
    EDI_CHECK(near(clampViewportZoom(std::numeric_limits<double>::quiet_NaN()), 1.0));
    DrawingCanvasViewportInput overZoom = baseInput;
    overZoom.zoom = 1000.0;
    EDI_CHECK(near(viewportBoardRect(overZoom).width(), fit.width() * kViewportMaxZoom));

    // Pan offsets the board by the pixel deltas.
    DrawingCanvasViewportInput panned = baseInput;
    panned.panXPx = 12.0;
    panned.panYPx = -7.0;
    const QRectF pannedBoard = viewportBoardRect(panned);
    EDI_CHECK(near(pannedBoard.left(), fit.left() + 12.0));
    EDI_CHECK(near(pannedBoard.top(), fit.top() - 7.0));

    // Anchor invariance: the canvas point under the cursor is unmoved by zoom.
    {
        const QRectF before = viewportBoardRect(baseInput);
        const QPointF anchor(200.0, 150.0);
        const double cx = (anchor.x() - before.left()) / before.width();
        const double cy = (anchor.y() - before.top()) / before.height();

        const DrawingCanvasViewportInput zoomedIn = zoomViewportAtPoint(baseInput, 2.0, anchor);
        EDI_CHECK(near(clampViewportZoom(zoomedIn.zoom), 2.0));
        const QRectF after = viewportBoardRect(zoomedIn);
        const QPointF mapped = canvasToScreen(after, cx, cy);
        EDI_CHECK(near(mapped.x(), anchor.x()));
        EDI_CHECK(near(mapped.y(), anchor.y()));

        // Zooming out then keeps the same anchor fixed too.
        const DrawingCanvasViewportInput zoomedOut = zoomViewportAtPoint(zoomedIn, 0.5, anchor);
        const QRectF after2 = viewportBoardRect(zoomedOut);
        const QPointF mapped2 = canvasToScreen(after2, cx, cy);
        EDI_CHECK(near(mapped2.x(), anchor.x()));
        EDI_CHECK(near(mapped2.y(), anchor.y()));

        // Anchor invariance holds even at the zoom clamp.
        const DrawingCanvasViewportInput clampedZoom = zoomViewportAtPoint(baseInput, 10000.0, anchor);
        EDI_CHECK(near(clampViewportZoom(clampedZoom.zoom), kViewportMaxZoom));
        const QPointF mappedClamped = canvasToScreen(viewportBoardRect(clampedZoom), cx, cy);
        EDI_CHECK(near(mappedClamped.x(), anchor.x()));
        EDI_CHECK(near(mappedClamped.y(), anchor.y()));
    }

    // DM-01 / SCALE-POLICY computeFitView: a content box frames centered with a
    // PROPORTIONAL breathing-room margin on every side. The fit margin is NO LONGER
    // input.paddingPx (that stays the board's at-rest inset); it is a NAMED fraction
    // of the visible shorter side — kViewportFitPaddingFraction (0.05/side). Square
    // grid + square widget so the math is clean.
    {
        const DrawingCanvasViewportInput fitInput{
            .widgetWidth = 400.0,
            .widgetHeight = 400.0,
            .gridWidth = 1.0,
            .gridHeight = 1.0,
            .paddingPx = 40.0, // the AT-REST board inset; the fit derives its own margin
        };
        // Fit margin = 0.05 * min(400,400) = 20 per side. availWidth = 400 - 2*20 = 360;
        // the fit-rect (at zoom 1) is 400 - paddingPx(40) = 360 wide, so zoom = 360/360 = 1.
        const double fitPad = kViewportFitPaddingFraction * 400.0; // 20
        const DrawingCanvasViewportInput framed = computeFitView(fitInput, 0.0, 0.0, 1.0, 1.0);
        const QRectF board = viewportBoardRect(framed);
        // Box center (0.5,0.5) lands at the viewport center.
        const QPointF center = canvasToScreen(board, 0.5, 0.5);
        EDI_CHECK(near(center.x(), 200.0));
        EDI_CHECK(near(center.y(), 200.0));
        // The framed box spans exactly the padded width (top-left at the breathing-room
        // margin, bottom-right at widget - margin).
        const QPointF topLeft = canvasToScreen(board, 0.0, 0.0);
        const QPointF bottomRight = canvasToScreen(board, 1.0, 1.0);
        EDI_CHECK(near(topLeft.x(), fitPad));
        EDI_CHECK(near(topLeft.y(), fitPad));
        EDI_CHECK(near(bottomRight.x(), 400.0 - fitPad));
        EDI_CHECK(near(bottomRight.y(), 400.0 - fitPad));

        // A sub-region (the right half) frames just that half, still centered.
        const DrawingCanvasViewportInput half = computeFitView(fitInput, 0.5, 0.0, 0.5, 1.0);
        const QRectF halfBoard = viewportBoardRect(half);
        const QPointF halfCenter = canvasToScreen(halfBoard, 0.75, 0.5);
        EDI_CHECK(near(halfCenter.x(), 200.0));
        EDI_CHECK(near(halfCenter.y(), 200.0));

        // A degenerate (zero-area) box is a no-op: zoom + pan are untouched.
        DrawingCanvasViewportInput preset = fitInput;
        preset.zoom = 3.0;
        preset.panXPx = 11.0;
        preset.panYPx = -5.0;
        const DrawingCanvasViewportInput unchanged = computeFitView(preset, 0.2, 0.2, 0.0, 0.0);
        EDI_CHECK(near(unchanged.zoom, 3.0));
        EDI_CHECK(near(unchanged.panXPx, 11.0));
        EDI_CHECK(near(unchanged.panYPx, -5.0));
    }

    // SCALE-POLICY overlay insets: the shell's floating right/bottom panels cover
    // the canvas edges, so the fit must frame into the VISIBLE sub-rectangle, not
    // the full widget — otherwise a dungeon lands under a panel (the map-workspace
    // clipping bug). A right inset shifts the framed center LEFT and shrinks zoom.
    {
        DrawingCanvasViewportInput insetInput{
            .widgetWidth = 400.0,
            .widgetHeight = 400.0,
            .gridWidth = 1.0,
            .gridHeight = 1.0,
            .paddingPx = 40.0,
        };
        insetInput.insetRightPx = 200.0; // the right half is occluded by a panel
        // Visible rect is [0,200] x [0,400]; its center is x=100. The whole [0,1]
        // box must center THERE, not at x=200 — the proof the dungeon clears the panel.
        const DrawingCanvasViewportInput framed = computeFitView(insetInput, 0.0, 0.0, 1.0, 1.0);
        const QRectF board = viewportBoardRect(framed);
        const QPointF center = canvasToScreen(board, 0.5, 0.5);
        EDI_CHECK(near(center.x(), 100.0));  // visible-rect center, not widget center
        EDI_CHECK(near(center.y(), 200.0));
        // And the box's right edge stays clear of the occluded region (x < 200).
        const QPointF boxRight = canvasToScreen(board, 1.0, 0.5);
        EDI_CHECK(boxRight.x() < 200.0);
    }

    // SCALE-POLICY fit floor: a dungeon far larger than the grid needs the fit to
    // zoom out BELOW the interactive kViewportMinZoom to frame whole. computeFitView
    // uses kViewportFitMinZoom (much lower), and the render transform honors it —
    // while interactive zoom (zoomViewportAtPoint) still refuses to go below the
    // interactive floor.
    {
        const DrawingCanvasViewportInput bigInput{
            .widgetWidth = 400.0,
            .widgetHeight = 400.0,
            .gridWidth = 1.0,
            .gridHeight = 1.0,
            .paddingPx = 40.0,
        };
        // A box 20x the grid: fitting it needs zoom ~0.05 (20x smaller), well below
        // the interactive floor of 0.2 — yet the whole box must still frame.
        const DrawingCanvasViewportInput framed = computeFitView(bigInput, 0.0, 0.0, 20.0, 20.0);
        EDI_CHECK(framed.zoom < kViewportMinZoom);   // below the interactive floor
        EDI_CHECK(framed.zoom >= kViewportFitMinZoom); // but not below the fit floor
        // The render transform (viewportBoardRect) must HONOR the sub-floor zoom,
        // so the whole 20-unit box actually fits inside the widget.
        const QRectF board = viewportBoardRect(framed);
        const QPointF tl = canvasToScreen(board, 0.0, 0.0);
        const QPointF br = canvasToScreen(board, 20.0, 20.0);
        EDI_CHECK(tl.x() >= -1.0 && tl.y() >= -1.0);
        EDI_CHECK(br.x() <= 401.0 && br.y() <= 401.0);
        // Interactive zoom is unaffected: a wheel-out from base never drops below
        // the interactive floor (the fit floor is a fit-only concession).
        DrawingCanvasViewportInput base = bigInput;
        const DrawingCanvasViewportInput wheeled = zoomViewportAtPoint(base, 0.0001, QPointF(200.0, 200.0));
        EDI_CHECK(near(clampViewportZoom(wheeled.zoom), kViewportMinZoom));
    }

    return 0;
}
