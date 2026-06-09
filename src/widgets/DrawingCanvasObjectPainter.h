#pragma once

#include <QRectF>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QPainter;

namespace drawing_canvas {

struct DrawingCanvasObjectPainterContext {
    QRectF board;
    QString selectedObjectId;
};

void drawGuideIntersections(QPainter &painter, const QVariantList &objects, const DrawingCanvasObjectPainterContext &context);
void drawObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context);
void drawPreviewObject(QPainter &painter, const QVariantMap &object, const DrawingCanvasObjectPainterContext &context);

} // namespace drawing_canvas
