#pragma once

#include "widgets/DrawingCanvasPalette.h"

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QPainter;

namespace drawing_canvas {

struct DrawingCanvasObjectPainterContext {
    QRectF board;
    QString selectedObjectId;
    DrawingCanvasPalette palette;
};

// The given color with only its alpha replaced.
QColor withAlpha(const QColor &color, int alpha);
// Two axis-aligned lines crossing at point, each extending `extent` px per side.
void drawCrosshair(QPainter &painter, const QPointF &point, double extent);
void drawGuideIntersections(QPainter &painter, const QVariantList &objects, const DrawingCanvasObjectPainterContext &context);
void drawObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context);
void drawPreviewObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context);

} // namespace drawing_canvas
