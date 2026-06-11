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
    DrawingCanvasProjectedGuide guide;
    DrawingCanvasProjectedLine line;
    DrawingCanvasProjectedDimension dimension;
    DrawingCanvasProjectedPointObject point;
    DrawingCanvasProjectedRectangle rectangle;
    DrawingCanvasProjectedCircle circle;
    DrawingCanvasProjectedPolygon polygon;
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
// Painting half: typed item -> pixels, every frame.
void drawSceneItem(QPainter &painter, const DrawingCanvasSceneItem &item, const DrawingCanvasObjectPainterContext &context);
void drawGuideIntersections(QPainter &painter, const std::vector<DrawingCanvasSceneItem> &scene, const DrawingCanvasObjectPainterContext &context);
void drawPreviewObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context);

} // namespace drawing_canvas
