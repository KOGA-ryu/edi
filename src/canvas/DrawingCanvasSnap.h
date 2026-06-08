#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

double effectiveGridStepPx(const QVariantMap &settings);
SnapResult noneSnap(const CanvasPoint &point, const QVariantMap &settings);
SnapResult gridSnap(const CanvasPoint &point, const QVariantMap &settings);
SnapResult resolveSnap(const CanvasPoint &rawPoint, const std::vector<CanvasObjectView> &objects, const QVariantMap &settings);

} // namespace drawing_canvas
