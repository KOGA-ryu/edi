#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

double canvasSizePx(const QVariantMap &settings);
double rotateHandleOffsetPx(const QVariantMap &settings);
double normalizedToPx(double value, const QVariantMap &settings);
double rawNormalizedToPx(double value, const QVariantMap &settings);
double roundDegrees(double value);

CanvasPoint rotatedRectCenter(const CanvasObjectView &object);
std::vector<HandleDescriptor> rotatedRectCorners(const CanvasObjectView &object);
CanvasPoint rotatedRectTopMidpoint(const CanvasObjectView &object);
HandleDescriptor rotatedRectRotationHandle(const CanvasObjectView &object, const QVariantMap &settings);
CanvasPoint unrotatePointForRect(const CanvasObjectView &object, double x, double y);

std::vector<HandleDescriptor> handlesForObject(const CanvasObjectView &object, const QVariantMap &settings);
std::vector<HandleDescriptor> visibleHandlesForObject(const CanvasObjectView &object, const QVariantMap &settings);
HandleDescriptor handleById(const CanvasObjectView &object, const QString &handleId, const QVariantMap &settings);
HitResult hitHandleAt(const CanvasObjectView &object, double screenX, double screenY, const BoardBounds &bounds, const QVariantMap &settings);
HandleUpdatePlan handleUpdatePlan(const CanvasObjectView &object, const QString &handleId, const CanvasPoint &point, const QVariantMap &settings);

} // namespace drawing_canvas
