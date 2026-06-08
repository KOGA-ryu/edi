#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

double objectHitScore(const CanvasObjectView &object, double x, double y);
HitResult hitObjectAt(const std::vector<CanvasObjectView> &objects, double x, double y, double tolerance);

} // namespace drawing_canvas
