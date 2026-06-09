#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVariantMap>

namespace drawing_canvas {

struct DrawingCanvasHandleHit {
    bool hit = false;
    QString handleId;
    double distancePx = 0.0;
};

DrawingCanvasHandleHit hitCanvasHandleAt(const QVariantMap &object, const QPointF &screenPoint, const QRectF &board);

} // namespace drawing_canvas
