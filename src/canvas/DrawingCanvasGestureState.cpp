#include "DrawingCanvasGestureState.h"

#include <cmath>

namespace drawing_canvas {
namespace {

QVariantList asStringListVariant(const QVariant &value) {
    QVariantList result;
    const QVariantList source = value.toList();
    for (const QVariant &entry : source) {
        result.push_back(entry.toString());
    }
    return result;
}

QVariantMap normalizePointMap(const QVariant &value) {
    return pointToVariant(pointFromVariant(value));
}

QVariantMap normalizeScreenPointMap(const QVariant &value) {
    const QVariantMap map = value.toMap();
    return {
        {QStringLiteral("x"), finiteNumber(map.value(QStringLiteral("x")), 0.0)},
        {QStringLiteral("y"), finiteNumber(map.value(QStringLiteral("y")), 0.0)}
    };
}

QVariantMap normalizeModifiers(const QVariant &value) {
    const QVariantMap map = value.toMap();
    return {
        {QStringLiteral("shift"), map.value(QStringLiteral("shift")).toBool()},
        {QStringLiteral("alt"), map.value(QStringLiteral("alt")).toBool()},
        {QStringLiteral("control"), map.value(QStringLiteral("control")).toBool()},
        {QStringLiteral("meta"), map.value(QStringLiteral("meta")).toBool()}
    };
}

QVariantMap noneIntent() {
    return {{QStringLiteral("kind"), QStringLiteral("none")}};
}

QVariantMap cloneState(const QVariantMap &state) {
    const QVariantMap source = state.isEmpty() ? initialGestureState() : state;
    return {
        {QStringLiteral("mode"), source.value(QStringLiteral("mode"), QStringLiteral("idle")).toString()},
        {QStringLiteral("started"), source.value(QStringLiteral("started")).toBool()},
        {QStringLiteral("objectId"), source.value(QStringLiteral("objectId")).toString()},
        {QStringLiteral("handleId"), source.value(QStringLiteral("handleId")).toString()},
        {QStringLiteral("selectedIds"), asStringListVariant(source.value(QStringLiteral("selectedIds")))},
        {QStringLiteral("startPoint"), normalizePointMap(source.value(QStringLiteral("startPoint")))},
        {QStringLiteral("lastPoint"), normalizePointMap(source.value(QStringLiteral("lastPoint")))},
        {QStringLiteral("startScreenPoint"), normalizeScreenPointMap(source.value(QStringLiteral("startScreenPoint")))},
        {QStringLiteral("lastScreenPoint"), normalizeScreenPointMap(source.value(QStringLiteral("lastScreenPoint")))},
        {QStringLiteral("moved"), source.value(QStringLiteral("moved")).toBool()},
        {QStringLiteral("modifiers"), normalizeModifiers(source.value(QStringLiteral("modifiers")))},
        {QStringLiteral("targetKind"), source.value(QStringLiteral("targetKind"), QStringLiteral("none")).toString()},
        {QStringLiteral("targetObjectId"), source.value(QStringLiteral("targetObjectId")).toString()},
        {QStringLiteral("targetHandleId"), source.value(QStringLiteral("targetHandleId")).toString()},
        {QStringLiteral("rejected"), source.value(QStringLiteral("rejected")).toBool()}
    };
}

bool activeMode(const QString &mode) {
    return mode == QStringLiteral("drawing_pending_shape")
        || mode == QStringLiteral("dragging_object")
        || mode == QStringLiteral("dragging_handle")
        || mode == QStringLiteral("marquee_select")
        || mode == QStringLiteral("panning");
}

bool transitionAllowed(const QString &fromMode, const QString &toMode) {
    const QString from = fromMode.isEmpty() ? QStringLiteral("idle") : fromMode;
    const QString to = toMode.isEmpty() ? QStringLiteral("idle") : toMode;
    if (from == to || to == QStringLiteral("idle") || from == QStringLiteral("idle")) {
        return true;
    }
    if (from == QStringLiteral("hovering")) {
        return true;
    }
    return false;
}

QVariantMap rejectedState(const QVariantMap &state) {
    QVariantMap next = cloneState(state);
    next.insert(QStringLiteral("rejected"), true);
    return next;
}

QVariantMap beginGesture(const QVariantMap &state, const QString &mode, const QVariantMap &payload) {
    const QVariantMap current = cloneState(state);
    const QString nextMode = mode.isEmpty() ? QStringLiteral("idle") : mode;
    if (!transitionAllowed(current.value(QStringLiteral("mode")).toString(), nextMode)) {
        return rejectedState(current);
    }
    const QVariantMap point = normalizePointMap(payload.value(QStringLiteral("point")));
    const QVariantMap screenPoint = normalizeScreenPointMap(payload.value(QStringLiteral("screenPoint")));
    return {
        {QStringLiteral("mode"), nextMode},
        {QStringLiteral("started"), nextMode != QStringLiteral("idle") && nextMode != QStringLiteral("hovering")},
        {QStringLiteral("objectId"), payload.value(QStringLiteral("objectId")).toString()},
        {QStringLiteral("handleId"), payload.value(QStringLiteral("handleId")).toString()},
        {QStringLiteral("selectedIds"), asStringListVariant(payload.value(QStringLiteral("selectedIds")))},
        {QStringLiteral("startPoint"), point},
        {QStringLiteral("lastPoint"), point},
        {QStringLiteral("startScreenPoint"), screenPoint},
        {QStringLiteral("lastScreenPoint"), screenPoint},
        {QStringLiteral("moved"), false},
        {QStringLiteral("modifiers"), normalizeModifiers(payload.value(QStringLiteral("modifiers")))},
        {QStringLiteral("targetKind"), payload.value(QStringLiteral("targetKind"), QStringLiteral("none")).toString()},
        {QStringLiteral("targetObjectId"), payload.value(QStringLiteral("targetObjectId")).toString()},
        {QStringLiteral("targetHandleId"), payload.value(QStringLiteral("targetHandleId")).toString()},
        {QStringLiteral("rejected"), false}
    };
}

QVariantMap finishIntent(const QVariantMap &state, const QVariantMap &payload) {
    const QVariantMap current = cloneState(state);
    if (payload.value(QStringLiteral("incremental")).toBool()) {
        return noneIntent();
    }
    const QVariantMap finalPoint = payload.contains(QStringLiteral("point"))
        ? normalizePointMap(payload.value(QStringLiteral("point")))
        : current.value(QStringLiteral("lastPoint")).toMap();
    const QVariantMap finalScreenPoint = payload.contains(QStringLiteral("screenPoint"))
        ? normalizeScreenPointMap(payload.value(QStringLiteral("screenPoint")))
        : current.value(QStringLiteral("lastScreenPoint")).toMap();
    const QString mode = current.value(QStringLiteral("mode")).toString();
    if (mode == QStringLiteral("dragging_handle")) {
        return {
            {QStringLiteral("kind"), QStringLiteral("update_handle")},
            {QStringLiteral("objectId"), current.value(QStringLiteral("objectId")).toString()},
            {QStringLiteral("handleId"), current.value(QStringLiteral("handleId")).toString()},
            {QStringLiteral("point"), finalPoint}
        };
    }
    if (mode == QStringLiteral("dragging_object")) {
        const QVariantMap startPoint = current.value(QStringLiteral("startPoint")).toMap();
        const double dx = finiteNumber(finalPoint.value(QStringLiteral("x")), 0.0) - finiteNumber(startPoint.value(QStringLiteral("x")), 0.0);
        const double dy = finiteNumber(finalPoint.value(QStringLiteral("y")), 0.0) - finiteNumber(startPoint.value(QStringLiteral("y")), 0.0);
        const QVariantList selectedIds = current.value(QStringLiteral("selectedIds")).toList();
        if (selectedIds.size() > 1) {
            return {{QStringLiteral("kind"), QStringLiteral("move_objects")}, {QStringLiteral("objectIds"), selectedIds}, {QStringLiteral("dx"), dx}, {QStringLiteral("dy"), dy}};
        }
        return {{QStringLiteral("kind"), QStringLiteral("move_object")}, {QStringLiteral("objectId"), current.value(QStringLiteral("objectId")).toString()}, {QStringLiteral("dx"), dx}, {QStringLiteral("dy"), dy}};
    }
    if (mode == QStringLiteral("marquee_select")) {
        return {
            {QStringLiteral("kind"), QStringLiteral("select_objects")},
            {QStringLiteral("objectIds"), asStringListVariant(payload.value(QStringLiteral("objectIds")))},
            {QStringLiteral("startPoint"), current.value(QStringLiteral("startPoint")).toMap()},
            {QStringLiteral("endPoint"), finalPoint}
        };
    }
    if (mode == QStringLiteral("panning")) {
        const QVariantMap startScreenPoint = current.value(QStringLiteral("startScreenPoint")).toMap();
        return {
            {QStringLiteral("kind"), QStringLiteral("pan")},
            {QStringLiteral("dxPx"), finiteNumber(finalScreenPoint.value(QStringLiteral("x")), 0.0) - finiteNumber(startScreenPoint.value(QStringLiteral("x")), 0.0)},
            {QStringLiteral("dyPx"), finiteNumber(finalScreenPoint.value(QStringLiteral("y")), 0.0) - finiteNumber(startScreenPoint.value(QStringLiteral("y")), 0.0)}
        };
    }
    if (mode == QStringLiteral("drawing_pending_shape")) {
        return {{QStringLiteral("kind"), QStringLiteral("draw_click")}, {QStringLiteral("point"), finalPoint}};
    }
    return noneIntent();
}

QString finishKind(const QVariantMap &state) {
    const QVariantMap current = cloneState(state);
    if (isHandleDrag(current) || isObjectDrag(current)) {
        return QStringLiteral("incremental_drag");
    }
    if (isMarquee(current)) {
        return current.value(QStringLiteral("moved")).toBool() ? QStringLiteral("marquee_select") : QStringLiteral("marquee_click");
    }
    const QString mode = current.value(QStringLiteral("mode")).toString();
    if (mode == QStringLiteral("panning")) {
        return QStringLiteral("pan");
    }
    if (mode == QStringLiteral("drawing_pending_shape")) {
        return QStringLiteral("draw_click");
    }
    return QStringLiteral("none");
}

} // namespace

QVariantMap initialGestureState() {
    return {
        {QStringLiteral("mode"), QStringLiteral("idle")},
        {QStringLiteral("started"), false},
        {QStringLiteral("objectId"), QString()},
        {QStringLiteral("handleId"), QString()},
        {QStringLiteral("selectedIds"), QVariantList()},
        {QStringLiteral("startPoint"), pointToVariant({0.0, 0.0})},
        {QStringLiteral("lastPoint"), pointToVariant({0.0, 0.0})},
        {QStringLiteral("startScreenPoint"), pointToVariant({0.0, 0.0})},
        {QStringLiteral("lastScreenPoint"), pointToVariant({0.0, 0.0})},
        {QStringLiteral("moved"), false},
        {QStringLiteral("modifiers"), normalizeModifiers({})},
        {QStringLiteral("targetKind"), QStringLiteral("none")},
        {QStringLiteral("targetObjectId"), QString()},
        {QStringLiteral("targetHandleId"), QString()},
        {QStringLiteral("rejected"), false}
    };
}

QVariantMap beginHover(const QVariantMap &state, const CanvasPoint &point, const QVariantMap &target) {
    return beginGesture(state, QStringLiteral("hovering"), {
        {QStringLiteral("point"), pointToVariant(point)},
        {QStringLiteral("targetKind"), target.value(QStringLiteral("kind"), QStringLiteral("none")).toString()},
        {QStringLiteral("targetObjectId"), target.value(QStringLiteral("objectId")).toString()},
        {QStringLiteral("targetHandleId"), target.value(QStringLiteral("handleId")).toString()},
        {QStringLiteral("modifiers"), target.value(QStringLiteral("modifiers")).toMap()}
    });
}

QVariantMap beginObjectDrag(const QVariantMap &state, const QString &objectId, const CanvasPoint &point, const QVariantList &selectedIds, const QVariantMap &modifiers) {
    return beginGesture(state, QStringLiteral("dragging_object"), {{QStringLiteral("objectId"), objectId}, {QStringLiteral("point"), pointToVariant(point)}, {QStringLiteral("selectedIds"), selectedIds}, {QStringLiteral("modifiers"), modifiers}});
}

QVariantMap beginHandleDrag(const QVariantMap &state, const QString &objectId, const QString &handleId, const CanvasPoint &point, const QVariantMap &modifiers) {
    return beginGesture(state, QStringLiteral("dragging_handle"), {{QStringLiteral("objectId"), objectId}, {QStringLiteral("handleId"), handleId}, {QStringLiteral("point"), pointToVariant(point)}, {QStringLiteral("modifiers"), modifiers}});
}

QVariantMap beginMarquee(const QVariantMap &state, const CanvasPoint &point, const QVariantMap &modifiers) {
    return beginGesture(state, QStringLiteral("marquee_select"), {{QStringLiteral("point"), pointToVariant(point)}, {QStringLiteral("modifiers"), modifiers}});
}

QVariantMap beginPan(const QVariantMap &state, const ScreenPoint &screenPoint, const QVariantMap &modifiers) {
    return beginGesture(state, QStringLiteral("panning"), {{QStringLiteral("screenPoint"), QVariantMap{{QStringLiteral("x"), screenPoint.x}, {QStringLiteral("y"), screenPoint.y}}}, {QStringLiteral("modifiers"), modifiers}});
}

QVariantMap beginDrawingPendingShape(const QVariantMap &state, const CanvasPoint &point, const QVariantMap &modifiers) {
    return beginGesture(state, QStringLiteral("drawing_pending_shape"), {{QStringLiteral("point"), pointToVariant(point)}, {QStringLiteral("modifiers"), modifiers}});
}

QVariantMap updateGesture(const QVariantMap &state, const QVariantMap &payload) {
    QVariantMap next = cloneState(state);
    next.insert(QStringLiteral("rejected"), false);
    next.insert(QStringLiteral("modifiers"), normalizeModifiers(payload.contains(QStringLiteral("modifiers")) ? payload.value(QStringLiteral("modifiers")) : next.value(QStringLiteral("modifiers"))));
    if (payload.contains(QStringLiteral("point"))) {
        next.insert(QStringLiteral("lastPoint"), normalizePointMap(payload.value(QStringLiteral("point"))));
    }
    if (payload.contains(QStringLiteral("screenPoint"))) {
        next.insert(QStringLiteral("lastScreenPoint"), normalizeScreenPointMap(payload.value(QStringLiteral("screenPoint"))));
    }
    if (payload.contains(QStringLiteral("targetKind"))) {
        next.insert(QStringLiteral("targetKind"), payload.value(QStringLiteral("targetKind"), QStringLiteral("none")).toString());
        next.insert(QStringLiteral("targetObjectId"), payload.value(QStringLiteral("targetObjectId")).toString());
        next.insert(QStringLiteral("targetHandleId"), payload.value(QStringLiteral("targetHandleId")).toString());
    }
    const QVariantMap lastPoint = next.value(QStringLiteral("lastPoint")).toMap();
    const QVariantMap startPoint = next.value(QStringLiteral("startPoint")).toMap();
    const QVariantMap lastScreenPoint = next.value(QStringLiteral("lastScreenPoint")).toMap();
    const QVariantMap startScreenPoint = next.value(QStringLiteral("startScreenPoint")).toMap();
    const double dx = finiteNumber(lastPoint.value(QStringLiteral("x")), 0.0) - finiteNumber(startPoint.value(QStringLiteral("x")), 0.0);
    const double dy = finiteNumber(lastPoint.value(QStringLiteral("y")), 0.0) - finiteNumber(startPoint.value(QStringLiteral("y")), 0.0);
    const double sdx = finiteNumber(lastScreenPoint.value(QStringLiteral("x")), 0.0) - finiteNumber(startScreenPoint.value(QStringLiteral("x")), 0.0);
    const double sdy = finiteNumber(lastScreenPoint.value(QStringLiteral("y")), 0.0) - finiteNumber(startScreenPoint.value(QStringLiteral("y")), 0.0);
    const double tolerance = std::max(0.0, finiteNumber(payload.value(QStringLiteral("moveTolerance")), 0.000001));
    const double screenTolerance = std::max(0.0, finiteNumber(payload.value(QStringLiteral("screenMoveTolerancePx")), 0.0));
    next.insert(QStringLiteral("moved"), next.value(QStringLiteral("moved")).toBool()
        || std::abs(dx) > tolerance
        || std::abs(dy) > tolerance
        || std::abs(sdx) > screenTolerance
        || std::abs(sdy) > screenTolerance);
    return next;
}

QVariantMap finishGesture(const QVariantMap &state, const QVariantMap &payload) {
    return {
        {QStringLiteral("state"), initialGestureState()},
        {QStringLiteral("intent"), finishIntent(state, payload)}
    };
}

QVariantMap cancelGesture(const QVariantMap &) {
    return {
        {QStringLiteral("state"), initialGestureState()},
        {QStringLiteral("intent"), noneIntent()}
    };
}

QVariantMap finishAction(const QVariantMap &state) {
    const QString kind = finishKind(state);
    return {
        {QStringLiteral("kind"), kind},
        {QStringLiteral("shouldFinishIncrementalDrag"), kind == QStringLiteral("incremental_drag")},
        {QStringLiteral("shouldSelectMarquee"), kind == QStringLiteral("marquee_select")},
        {QStringLiteral("shouldSuppressClick"), kind == QStringLiteral("incremental_drag") || kind == QStringLiteral("marquee_select")},
        {QStringLiteral("shouldEndObjectMove"), kind != QStringLiteral("marquee_select")},
        {QStringLiteral("shouldResetLifecycle"), true}
    };
}

bool isDragging(const QVariantMap &state) {
    const QString mode = state.value(QStringLiteral("mode")).toString();
    return mode == QStringLiteral("dragging_object") || mode == QStringLiteral("dragging_handle");
}

bool isHandleDrag(const QVariantMap &state) {
    return state.value(QStringLiteral("mode")).toString() == QStringLiteral("dragging_handle");
}

bool isObjectDrag(const QVariantMap &state) {
    return state.value(QStringLiteral("mode")).toString() == QStringLiteral("dragging_object");
}

bool isMarquee(const QVariantMap &state) {
    return state.value(QStringLiteral("mode")).toString() == QStringLiteral("marquee_select");
}

QString gestureLabel(const QVariantMap &state) {
    const QString mode = state.value(QStringLiteral("mode"), QStringLiteral("idle")).toString();
    if (mode == QStringLiteral("dragging_handle")) {
        return state.value(QStringLiteral("handleId")).toString() == QStringLiteral("rect_rotate") ? QStringLiteral("rotate") : QStringLiteral("drag handle");
    }
    if (mode == QStringLiteral("dragging_object")) {
        return state.value(QStringLiteral("selectedIds")).toList().size() > 1 ? QStringLiteral("move selection") : QStringLiteral("move object");
    }
    if (mode == QStringLiteral("marquee_select")) {
        return QStringLiteral("marquee");
    }
    if (mode == QStringLiteral("panning")) {
        return QStringLiteral("pan");
    }
    if (mode == QStringLiteral("drawing_pending_shape")) {
        return QStringLiteral("draw");
    }
    return mode == QStringLiteral("hovering") ? QStringLiteral("hover") : QStringLiteral("idle");
}

} // namespace drawing_canvas
