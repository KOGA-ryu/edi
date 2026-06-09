#pragma once

#include <QVariantMap>

#include <vector>

namespace drawing_canvas {

struct DrawingCanvasProjectedPoint {
    double x = 0.0;
    double y = 0.0;
};

std::vector<DrawingCanvasProjectedPoint> projectedObjectPoints(const QVariantMap &object);

} // namespace drawing_canvas
