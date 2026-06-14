#pragma once

#include "widgets/DrawingCanvasPalette.h"
#include "widgets/DrawingCanvasProjectedObject.h"

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <vector>

class QPainter;

namespace drawing_canvas {

struct DrawingCanvasObjectPainterContext {
    QRectF board;
    QString selectedObjectId;
    DrawingCanvasPalette palette;
    // Plot diagnostics (the red warning BOX + label per blocked object) are
    // plotter chrome, not drafting chrome: drawn only when the plot preview
    // is on. The red stroke recolor stays unconditional as the quiet hint.
    bool plotDiagnostics = false;
};

// A text annotation's projected payload. The position is the TOP-LEFT of the
// glyph box in canvas units; height is cap height in canvas units (default
// 0.04). content empty draws nothing. Plain data, mirroring the other per-kind
// projected structs.
struct DrawingCanvasProjectedTextObject {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
    double height = 0.04;
    QString content;
};

// One object, fully extracted into the typed projection structs the painter
// reads. The QVariantMap path converted maps to these structs PER OBJECT PER
// FRAME; a scene of pre-built items converts once per document mutation and
// paints typed every frame. Per-kind payloads sit side by side (only the one
// matching summary.kind is meaningful) — plain data, variation by kind, no
// variant ceremony for seven small structs.
struct DrawingCanvasSceneItem {
    DrawingCanvasProjectedObjectSummary summary;
    DrawingCanvasProjectedStyle style;
    bool endArrow = false;
    bool startArrow = false;
    DrawingCanvasProjectedGuide guide;
    DrawingCanvasProjectedLine line;
    DrawingCanvasProjectedDimension dimension;
    DrawingCanvasProjectedPointObject point;
    DrawingCanvasProjectedRectangle rectangle;
    DrawingCanvasProjectedCircle circle;
    DrawingCanvasProjectedPolygon polygon;
    DrawingCanvasProjectedTextObject text;
    DrawingCanvasProjectedWall wall;
    // Kept only for the selected object's handle painting (implicitly
    // shared, so retaining it costs a refcount, not a copy).
    QVariantMap source;
};

// The given color with only its alpha replaced.
QColor withAlpha(const QColor &color, int alpha);
// Two axis-aligned lines crossing at point, each extending `extent` px per side.
void drawCrosshair(QPainter &painter, const QPointF &point, double extent);
// Extraction half of the old drawObject: map -> typed item, once per mutation.
DrawingCanvasSceneItem buildCanvasSceneItem(const QVariantMap &object);
// M1.2 corner join (whole-scene pass): where exactly two walls share an
// endpoint, flag one of them to paint the outer-corner miter fill. Mutates the
// wall items' joinA/joinB in place; non-wall items and lone endpoints are
// untouched. Run once after the scene is built, before drawing.
void annotateWallJoins(std::vector<DrawingCanvasSceneItem> &scene);
// Pure geometry for the wall corner miter: given the shared SCREEN point `p`,
// the unit directions `tThis`/`tNbr` pointing from p INTO each wall, and each
// band's SCREEN half-thickness, return the convex polygon (4-pt miter quad, or
// 3-pt bevel when the miter would spike past `miterLimit`*halfThickness) that
// fills the outer notch. Empty when the walls are ~collinear (no real corner).
std::vector<QPointF> wallCornerMiterFill(const QPointF &p, const QPointF &tThis, const QPointF &tNbr,
                                         double hThis, double hNbr, double miterLimit);
// Painting half: typed item -> pixels, every frame.
void drawSceneItem(QPainter &painter, const DrawingCanvasSceneItem &item, const DrawingCanvasObjectPainterContext &context);
void drawGuideIntersections(QPainter &painter, const std::vector<DrawingCanvasSceneItem> &scene, const DrawingCanvasObjectPainterContext &context);
void drawPreviewObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context);

} // namespace drawing_canvas
