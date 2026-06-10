#include "widgets/DrawingCanvasGestureState.h"

#include "widgets/DrawingCanvasValues.h"

#include <cmath>

namespace drawing_canvas {
namespace {

DrawingCanvasGestureIntent noneIntent()
{
    return {};
}

bool transitionAllowedInternal(DrawingCanvasGestureMode fromMode, DrawingCanvasGestureMode toMode)
{
    if (fromMode == toMode || toMode == DrawingCanvasGestureMode::Idle || fromMode == DrawingCanvasGestureMode::Idle) {
        return true;
    }
    return fromMode == DrawingCanvasGestureMode::Hovering;
}

DrawingCanvasGestureState rejectedState(const DrawingCanvasGestureState &state)
{
    DrawingCanvasGestureState next = state;
    next.rejected = true;
    return next;
}

DrawingCanvasGestureState beginGesture(
    const DrawingCanvasGestureState &state,
    DrawingCanvasGestureMode mode,
    CanvasPoint point,
    ScreenPoint screenPoint,
    const QString &objectId,
    const QString &handleId,
    const QStringList &selectedIds,
    const QString &targetKind,
    const QString &targetObjectId,
    const QString &targetHandleId,
    DrawingCanvasModifiers modifiers)
{
    if (!transitionAllowedInternal(state.mode, mode)) {
        return rejectedState(state);
    }
    DrawingCanvasGestureState next;
    next.mode = mode;
    next.started = mode != DrawingCanvasGestureMode::Idle && mode != DrawingCanvasGestureMode::Hovering;
    next.objectId = objectId;
    next.handleId = handleId;
    next.selectedIds = selectedIds;
    next.startPoint = point;
    next.lastPoint = point;
    next.startScreenPoint = screenPoint;
    next.lastScreenPoint = screenPoint;
    next.modifiers = modifiers;
    next.targetKind = targetKind.isEmpty() ? QStringLiteral("none") : targetKind;
    next.targetObjectId = targetObjectId;
    next.targetHandleId = targetHandleId;
    return next;
}

DrawingCanvasGestureIntent finishIntent(const DrawingCanvasGestureState &state, const DrawingCanvasFinishOptions &options)
{
    if (options.incremental) {
        return noneIntent();
    }
    const CanvasPoint finalPoint = options.hasPoint ? options.point : state.lastPoint;
    const ScreenPoint finalScreenPoint = options.hasScreenPoint ? options.screenPoint : state.lastScreenPoint;

    if (state.mode == DrawingCanvasGestureMode::DraggingHandle) {
        DrawingCanvasGestureIntent intent;
        intent.kind = DrawingCanvasGestureIntentKind::UpdateHandle;
        intent.objectId = state.objectId;
        intent.handleId = state.handleId;
        intent.point = finalPoint;
        return intent;
    }
    if (state.mode == DrawingCanvasGestureMode::DraggingObject) {
        DrawingCanvasGestureIntent intent;
        intent.kind = state.selectedIds.size() > 1 ? DrawingCanvasGestureIntentKind::MoveObjects : DrawingCanvasGestureIntentKind::MoveObject;
        intent.objectId = state.objectId;
        intent.objectIds = state.selectedIds;
        intent.dx = finalPoint.x - state.startPoint.x;
        intent.dy = finalPoint.y - state.startPoint.y;
        return intent;
    }
    if (state.mode == DrawingCanvasGestureMode::MarqueeSelect) {
        DrawingCanvasGestureIntent intent;
        intent.kind = DrawingCanvasGestureIntentKind::SelectObjects;
        intent.objectIds = options.objectIds;
        intent.startPoint = state.startPoint;
        intent.endPoint = finalPoint;
        return intent;
    }
    if (state.mode == DrawingCanvasGestureMode::Panning) {
        DrawingCanvasGestureIntent intent;
        intent.kind = DrawingCanvasGestureIntentKind::Pan;
        intent.dxPx = finalScreenPoint.x - state.startScreenPoint.x;
        intent.dyPx = finalScreenPoint.y - state.startScreenPoint.y;
        return intent;
    }
    if (state.mode == DrawingCanvasGestureMode::DrawingPendingShape) {
        DrawingCanvasGestureIntent intent;
        intent.kind = DrawingCanvasGestureIntentKind::DrawClick;
        intent.point = finalPoint;
        return intent;
    }
    return noneIntent();
}

} // namespace

DrawingCanvasGestureState initialGestureState()
{
    return {};
}

DrawingCanvasGestureState beginHover(
    const DrawingCanvasGestureState &state,
    CanvasPoint point,
    const QString &targetKind,
    const QString &targetObjectId,
    const QString &targetHandleId,
    DrawingCanvasModifiers modifiers)
{
    return beginGesture(
        state,
        DrawingCanvasGestureMode::Hovering,
        point,
        {},
        {},
        {},
        {},
        targetKind,
        targetObjectId,
        targetHandleId,
        modifiers);
}

DrawingCanvasGestureState beginObjectDrag(const DrawingCanvasGestureState &state, const QString &objectId, CanvasPoint point, const QStringList &selectedIds, DrawingCanvasModifiers modifiers)
{
    return beginGesture(state, DrawingCanvasGestureMode::DraggingObject, point, {}, objectId, {}, selectedIds, QStringLiteral("none"), {}, {}, modifiers);
}

DrawingCanvasGestureState beginHandleDrag(const DrawingCanvasGestureState &state, const QString &objectId, const QString &handleId, CanvasPoint point, DrawingCanvasModifiers modifiers)
{
    return beginGesture(state, DrawingCanvasGestureMode::DraggingHandle, point, {}, objectId, handleId, {}, QStringLiteral("none"), {}, {}, modifiers);
}

DrawingCanvasGestureState beginMarquee(const DrawingCanvasGestureState &state, CanvasPoint point, DrawingCanvasModifiers modifiers)
{
    return beginGesture(state, DrawingCanvasGestureMode::MarqueeSelect, point, {}, {}, {}, {}, QStringLiteral("none"), {}, {}, modifiers);
}

DrawingCanvasGestureState beginPan(const DrawingCanvasGestureState &state, ScreenPoint screenPoint, DrawingCanvasModifiers modifiers)
{
    return beginGesture(state, DrawingCanvasGestureMode::Panning, {}, screenPoint, {}, {}, {}, QStringLiteral("none"), {}, {}, modifiers);
}

DrawingCanvasGestureState beginDrawingPendingShape(const DrawingCanvasGestureState &state, CanvasPoint point, DrawingCanvasModifiers modifiers)
{
    return beginGesture(state, DrawingCanvasGestureMode::DrawingPendingShape, point, {}, {}, {}, {}, QStringLiteral("none"), {}, {}, modifiers);
}

DrawingCanvasGestureState updateGesture(const DrawingCanvasGestureState &state, CanvasPoint point, DrawingCanvasModifiers modifiers, double moveTolerance)
{
    DrawingCanvasGestureState next = state;
    next.rejected = false;
    next.modifiers = modifiers;
    next.lastPoint = point;
    const double dx = next.lastPoint.x - next.startPoint.x;
    const double dy = next.lastPoint.y - next.startPoint.y;
    const double tolerance = std::max(0.0, finiteNumber(moveTolerance, 0.000001));
    next.moved = next.moved || std::abs(dx) > tolerance || std::abs(dy) > tolerance;
    return next;
}

DrawingCanvasGestureState updateGestureScreenPoint(const DrawingCanvasGestureState &state, ScreenPoint screenPoint, DrawingCanvasModifiers modifiers, double screenMoveTolerancePx)
{
    DrawingCanvasGestureState next = state;
    next.rejected = false;
    next.modifiers = modifiers;
    next.lastScreenPoint = screenPoint;
    const double dx = next.lastScreenPoint.x - next.startScreenPoint.x;
    const double dy = next.lastScreenPoint.y - next.startScreenPoint.y;
    const double tolerance = std::max(0.0, finiteNumber(screenMoveTolerancePx, 0.0));
    next.moved = next.moved || std::abs(dx) > tolerance || std::abs(dy) > tolerance;
    return next;
}

DrawingCanvasFinishResult finishGesture(const DrawingCanvasGestureState &state, DrawingCanvasFinishOptions options)
{
    return {initialGestureState(), finishIntent(state, options)};
}

DrawingCanvasFinishResult cancelGesture()
{
    return {initialGestureState(), noneIntent()};
}

bool transitionAllowed(DrawingCanvasGestureMode fromMode, DrawingCanvasGestureMode toMode)
{
    return transitionAllowedInternal(fromMode, toMode);
}

QString finishKind(const DrawingCanvasGestureState &state)
{
    if (isHandleDrag(state) || isObjectDrag(state)) {
        return QStringLiteral("incremental_drag");
    }
    if (isMarquee(state)) {
        return state.moved ? QStringLiteral("marquee_select") : QStringLiteral("marquee_click");
    }
    if (state.mode == DrawingCanvasGestureMode::Panning) {
        return QStringLiteral("pan");
    }
    if (state.mode == DrawingCanvasGestureMode::DrawingPendingShape) {
        return QStringLiteral("draw_click");
    }
    return QStringLiteral("none");
}

bool isDragging(const DrawingCanvasGestureState &state)
{
    return state.mode == DrawingCanvasGestureMode::DraggingObject || state.mode == DrawingCanvasGestureMode::DraggingHandle;
}

bool isHandleDrag(const DrawingCanvasGestureState &state)
{
    return state.mode == DrawingCanvasGestureMode::DraggingHandle;
}

bool isObjectDrag(const DrawingCanvasGestureState &state)
{
    return state.mode == DrawingCanvasGestureMode::DraggingObject;
}

bool isMarquee(const DrawingCanvasGestureState &state)
{
    return state.mode == DrawingCanvasGestureMode::MarqueeSelect;
}

QString gestureLabel(const DrawingCanvasGestureState &state)
{
    if (state.mode == DrawingCanvasGestureMode::DraggingHandle) {
        return state.handleId == QStringLiteral("rect_rotate") ? QStringLiteral("rotate") : QStringLiteral("drag handle");
    }
    if (state.mode == DrawingCanvasGestureMode::DraggingObject) {
        return state.selectedIds.size() > 1 ? QStringLiteral("move selection") : QStringLiteral("move object");
    }
    if (state.mode == DrawingCanvasGestureMode::MarqueeSelect) {
        return QStringLiteral("marquee");
    }
    if (state.mode == DrawingCanvasGestureMode::Panning) {
        return QStringLiteral("pan");
    }
    if (state.mode == DrawingCanvasGestureMode::DrawingPendingShape) {
        return QStringLiteral("draw");
    }
    return state.mode == DrawingCanvasGestureMode::Hovering ? QStringLiteral("hover") : QStringLiteral("idle");
}

} // namespace drawing_canvas
