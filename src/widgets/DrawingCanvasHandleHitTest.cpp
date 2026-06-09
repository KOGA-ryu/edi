#include "widgets/DrawingCanvasHandleHitTest.h"

#include "widgets/DrawingCanvasProjectedObject.h"
#include "widgets/DrawingCanvasViewport.h"

#include <cmath>
#include <limits>

namespace drawing_canvas {

DrawingCanvasHandleHit hitCanvasHandleAt(const QVariantMap &object, const QPointF &screenPoint, const QRectF &board)
{
    DrawingCanvasHandleHit best;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (const DrawingCanvasProjectedHandle &handle : projectedObjectHandles(object)) {
        if (!handle.editable) {
            continue;
        }
        const QPointF point = canvasToScreen(board, handle.x, handle.y);
        const double dx = screenPoint.x() - point.x();
        const double dy = screenPoint.y() - point.y();
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= handle.hitTolerancePx && distance <= bestDistance) {
            best.hit = true;
            best.handleId = handle.id;
            best.distancePx = distance;
            bestDistance = distance;
        }
    }
    return best;
}

} // namespace drawing_canvas
