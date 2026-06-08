#pragma once

#include "DrawingCanvasTypes.h"

namespace drawing_canvas {

QVariantMap initialGestureState();
QVariantMap beginHover(const QVariantMap &state, const CanvasPoint &point, const QVariantMap &target);
QVariantMap beginObjectDrag(const QVariantMap &state, const QString &objectId, const CanvasPoint &point, const QVariantList &selectedIds, const QVariantMap &modifiers);
QVariantMap beginHandleDrag(const QVariantMap &state, const QString &objectId, const QString &handleId, const CanvasPoint &point, const QVariantMap &modifiers);
QVariantMap beginMarquee(const QVariantMap &state, const CanvasPoint &point, const QVariantMap &modifiers);
QVariantMap beginPan(const QVariantMap &state, const ScreenPoint &screenPoint, const QVariantMap &modifiers);
QVariantMap beginDrawingPendingShape(const QVariantMap &state, const CanvasPoint &point, const QVariantMap &modifiers);
QVariantMap updateGesture(const QVariantMap &state, const QVariantMap &payload);
QVariantMap finishGesture(const QVariantMap &state, const QVariantMap &payload);
QVariantMap cancelGesture(const QVariantMap &state);
bool transitionAllowed(const QString &fromMode, const QString &toMode);
QString finishKind(const QVariantMap &state);
QVariantMap finishAction(const QVariantMap &state);
bool isDragging(const QVariantMap &state);
bool isHandleDrag(const QVariantMap &state);
bool isObjectDrag(const QVariantMap &state);
bool isMarquee(const QVariantMap &state);
QString gestureLabel(const QVariantMap &state);

} // namespace drawing_canvas
