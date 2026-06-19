#pragma once

#include <QPointF>
#include <QRectF>
#include <QVariantMap>

namespace drawing_canvas {

// Interactive zoom limits — the floor/ceiling for wheel + pinch zoom. The FIT
// path uses a separate, lower floor (kViewportFitMinZoom) so a giant dungeon can
// be framed whole below the interactive minimum without clipping.
inline constexpr double kViewportMinZoom = 0.2;
inline constexpr double kViewportMaxZoom = 16.0;

// SCALE-POLICY (invariant 0): every dimension is NAMED DATA, never a magic
// literal in logic. These three knobs own the auto-fit look; the user owns the
// exact values.
//
// Breathing-room margin the auto-fit leaves around the framed content, as a
// FRACTION of the viewport's shorter side, PER SIDE. 0.05/side = ~10% total
// breathing room (the SCALE-POLICY ask). Deriving from the viewport (instead of
// a fixed pixel count) keeps the margin proportional at any canvas size.
inline constexpr double kViewportFitPaddingFraction = 0.05;
// Fit-only zoom floor: computeFitView may zoom out THIS far to frame a whole
// dungeon, well below the interactive kViewportMinZoom. A ~5x dungeon needs
// ~0.07; 0.02 leaves headroom for ~17x before the fit itself starts to clip.
inline constexpr double kViewportFitMinZoom = 0.02;
// Fallback pixel padding when paddingPx is left unset/non-finite. Kept as the
// historical default so callers that don't derive a fraction still behave.
inline constexpr double kViewportFitPaddingFallbackPx = 48.0;

struct DrawingCanvasViewportInput {
    double widgetWidth = 1.0;
    double widgetHeight = 1.0;
    double gridWidth = 1.0;
    double gridHeight = 1.0;
    double paddingPx = kViewportFitPaddingFallbackPx;
    double zoom = 1.0;
    double panXPx = 0.0;
    double panYPx = 0.0;
    // Overlay occlusion: how many pixels of the canvas widget are covered by the
    // shell's floating right/bottom panels (and, for symmetry, left/top). The
    // auto-fit frames content into the UN-occluded sub-rectangle so a dungeon
    // never lands under a panel. Default 0 = no overlays = fit fills the widget.
    double insetLeftPx = 0.0;
    double insetTopPx = 0.0;
    double insetRightPx = 0.0;
    double insetBottomPx = 0.0;
};

// Fit-path zoom clamp: same upper bound as interactive zoom, but a far lower
// floor so a huge dungeon frames whole. Interactive zoom keeps clampViewportZoom.
double clampFitZoom(double zoom);

double viewportAspect(double gridWidth, double gridHeight);
double clampViewportZoom(double zoom);
DrawingCanvasViewportInput viewportInputFromModel(const QVariantMap &model, double widgetWidth, double widgetHeight);
// Base fit-rect at zoom 1 / pan 0 (centered, aspect-correct).
QRectF viewportFitRect(const DrawingCanvasViewportInput &input);
// Fit-rect scaled by the clamped zoom and offset by pan.
QRectF viewportBoardRect(const DrawingCanvasViewportInput &input);
// Returns a new input whose zoom is scaled by factor (clamped) and whose pan is
// solved so the canvas point currently under anchorPx stays under anchorPx.
DrawingCanvasViewportInput zoomViewportAtPoint(const DrawingCanvasViewportInput &input, double factor, const QPointF &anchorPx);
// DM-01 view-auto-fit: given a content box in NORMALIZED canvas coords ([0,1] over
// the grid — the same space documentObjectsBounds() reports), return a new input
// whose zoom + pan FRAME that box centered in the viewport with `paddingPx` of
// margin on every side. A degenerate (zero-area) box leaves zoom/pan untouched —
// there is nothing to frame, so the caller's view is preserved. The zoom is
// clamped to the viewport limits; a box larger than the limits allow simply
// renders as large as the clamp permits, still centered.
DrawingCanvasViewportInput computeFitView(const DrawingCanvasViewportInput &input,
                                          double contentX, double contentY,
                                          double contentWidth, double contentHeight);
QPointF canvasToScreen(const QRectF &board, double x, double y);
QRectF boundsToScreenRect(const QRectF &board, double x, double y, double width, double height);
QPointF screenToCanvas(const QRectF &board, const QPointF &point);

} // namespace drawing_canvas
