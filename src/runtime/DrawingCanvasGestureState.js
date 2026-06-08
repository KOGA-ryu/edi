.pragma library

// Legacy parity reference for the C++ drawing_canvas_core runtime.
// App QML should call drawingCanvasRuntime instead of importing this file.

function finiteNumber(value, fallback) {
    var number = Number(value)
    return Number.isFinite(number) ? number : fallback
}

function asArray(value) {
    if (!value) {
        return []
    }
    if (Array.isArray(value)) {
        return value
    }
    if (typeof value.length === "number") {
        var result = []
        for (var index = 0; index < value.length; ++index) {
            result.push(value[index])
        }
        return result
    }
    return []
}

function normalizePoint(point) {
    return {
        x: finiteNumber(point && point.x, 0),
        y: finiteNumber(point && point.y, 0)
    }
}

function normalizeScreenPoint(point) {
    return {
        x: finiteNumber(point && point.x, 0),
        y: finiteNumber(point && point.y, 0)
    }
}

function normalizeModifiers(modifiers) {
    return {
        shift: modifiers && modifiers.shift === true,
        alt: modifiers && modifiers.alt === true,
        control: modifiers && modifiers.control === true,
        meta: modifiers && modifiers.meta === true
    }
}

function noneIntent() {
    return {
        kind: "none"
    }
}

function initialGestureState() {
    return {
        mode: "idle",
        started: false,
        objectId: "",
        handleId: "",
        selectedIds: [],
        startPoint: { x: 0, y: 0 },
        lastPoint: { x: 0, y: 0 },
        startScreenPoint: { x: 0, y: 0 },
        lastScreenPoint: { x: 0, y: 0 },
        moved: false,
        modifiers: normalizeModifiers({}),
        targetKind: "none",
        targetObjectId: "",
        targetHandleId: "",
        rejected: false
    }
}

function cloneState(state) {
    var source = state || initialGestureState()
    return {
        mode: String(source.mode || "idle"),
        started: source.started === true,
        objectId: String(source.objectId || ""),
        handleId: String(source.handleId || ""),
        selectedIds: asArray(source.selectedIds).map(function(id) { return String(id || "") }),
        startPoint: normalizePoint(source.startPoint),
        lastPoint: normalizePoint(source.lastPoint),
        startScreenPoint: normalizeScreenPoint(source.startScreenPoint),
        lastScreenPoint: normalizeScreenPoint(source.lastScreenPoint),
        moved: source.moved === true,
        modifiers: normalizeModifiers(source.modifiers),
        targetKind: String(source.targetKind || "none"),
        targetObjectId: String(source.targetObjectId || ""),
        targetHandleId: String(source.targetHandleId || ""),
        rejected: source.rejected === true
    }
}

function activeMode(mode) {
    return mode === "drawing_pending_shape"
            || mode === "dragging_object"
            || mode === "dragging_handle"
            || mode === "marquee_select"
            || mode === "panning"
}

function transitionAllowed(fromMode, toMode) {
    var from = String(fromMode || "idle")
    var to = String(toMode || "idle")
    if (from === to) {
        return true
    }
    if (to === "idle") {
        return true
    }
    if (from === "idle") {
        return true
    }
    if (from === "hovering" && !activeMode(to)) {
        return true
    }
    if (from === "hovering" && activeMode(to)) {
        return true
    }
    return false
}

function rejectedState(state) {
    var next = cloneState(state)
    next.rejected = true
    return next
}

function beginGesture(state, mode, payload) {
    var current = cloneState(state)
    var nextMode = String(mode || "idle")
    if (!transitionAllowed(current.mode, nextMode)) {
        return rejectedState(current)
    }
    var point = normalizePoint(payload && payload.point)
    var screenPoint = normalizeScreenPoint(payload && payload.screenPoint)
    return {
        mode: nextMode,
        started: nextMode !== "idle" && nextMode !== "hovering",
        objectId: String(payload && payload.objectId || ""),
        handleId: String(payload && payload.handleId || ""),
        selectedIds: asArray(payload && payload.selectedIds).map(function(id) { return String(id || "") }),
        startPoint: point,
        lastPoint: point,
        startScreenPoint: screenPoint,
        lastScreenPoint: screenPoint,
        moved: false,
        modifiers: normalizeModifiers(payload && payload.modifiers),
        targetKind: String(payload && payload.targetKind || "none"),
        targetObjectId: String(payload && payload.targetObjectId || ""),
        targetHandleId: String(payload && payload.targetHandleId || ""),
        rejected: false
    }
}

function beginHover(state, point, target) {
    return beginGesture(state, "hovering", {
        point: point,
        targetKind: target && target.kind || "none",
        targetObjectId: target && target.objectId || "",
        targetHandleId: target && target.handleId || "",
        modifiers: target && target.modifiers || {}
    })
}

function beginObjectDrag(state, objectId, point, selectedIds, modifiers) {
    return beginGesture(state, "dragging_object", {
        objectId: objectId,
        point: point,
        selectedIds: selectedIds,
        modifiers: modifiers
    })
}

function beginHandleDrag(state, objectId, handleId, point, modifiers) {
    return beginGesture(state, "dragging_handle", {
        objectId: objectId,
        handleId: handleId,
        point: point,
        modifiers: modifiers
    })
}

function beginMarquee(state, point, modifiers) {
    return beginGesture(state, "marquee_select", {
        point: point,
        modifiers: modifiers
    })
}

function beginPan(state, screenPoint, modifiers) {
    return beginGesture(state, "panning", {
        screenPoint: screenPoint,
        modifiers: modifiers
    })
}

function beginDrawingPendingShape(state, point, modifiers) {
    return beginGesture(state, "drawing_pending_shape", {
        point: point,
        modifiers: modifiers
    })
}

function updateGesture(state, payload) {
    var next = cloneState(state)
    next.rejected = false
    next.modifiers = normalizeModifiers(payload && payload.modifiers || next.modifiers)
    if (payload && payload.point) {
        next.lastPoint = normalizePoint(payload.point)
    }
    if (payload && payload.screenPoint) {
        next.lastScreenPoint = normalizeScreenPoint(payload.screenPoint)
    }
    if (payload && payload.targetKind) {
        next.targetKind = String(payload.targetKind || "none")
        next.targetObjectId = String(payload.targetObjectId || "")
        next.targetHandleId = String(payload.targetHandleId || "")
    }
    var dx = next.lastPoint.x - next.startPoint.x
    var dy = next.lastPoint.y - next.startPoint.y
    var sdx = next.lastScreenPoint.x - next.startScreenPoint.x
    var sdy = next.lastScreenPoint.y - next.startScreenPoint.y
    var tolerance = Math.max(0, finiteNumber(payload && payload.moveTolerance, 0.000001))
    var screenTolerance = Math.max(0, finiteNumber(payload && payload.screenMoveTolerancePx, 0))
    next.moved = next.moved
            || Math.abs(dx) > tolerance
            || Math.abs(dy) > tolerance
            || Math.abs(sdx) > screenTolerance
            || Math.abs(sdy) > screenTolerance
    return next
}

function finishIntent(state, payload) {
    var current = cloneState(state)
    if (payload && payload.incremental === true) {
        return noneIntent()
    }
    var finalPoint = payload && payload.point ? normalizePoint(payload.point) : current.lastPoint
    var finalScreenPoint = payload && payload.screenPoint ? normalizeScreenPoint(payload.screenPoint) : current.lastScreenPoint
    if (current.mode === "dragging_handle") {
        return {
            kind: "update_handle",
            objectId: current.objectId,
            handleId: current.handleId,
            point: finalPoint
        }
    }
    if (current.mode === "dragging_object") {
        var dx = finalPoint.x - current.startPoint.x
        var dy = finalPoint.y - current.startPoint.y
        if (current.selectedIds.length > 1) {
            return {
                kind: "move_objects",
                objectIds: current.selectedIds,
                dx: dx,
                dy: dy
            }
        }
        return {
            kind: "move_object",
            objectId: current.objectId,
            dx: dx,
            dy: dy
        }
    }
    if (current.mode === "marquee_select") {
        return {
            kind: "select_objects",
            objectIds: asArray(payload && payload.objectIds).map(function(id) { return String(id || "") }),
            startPoint: current.startPoint,
            endPoint: finalPoint
        }
    }
    if (current.mode === "panning") {
        return {
            kind: "pan",
            dxPx: finalScreenPoint.x - current.startScreenPoint.x,
            dyPx: finalScreenPoint.y - current.startScreenPoint.y
        }
    }
    if (current.mode === "drawing_pending_shape") {
        return {
            kind: "draw_click",
            point: finalPoint
        }
    }
    return noneIntent()
}

function finishGesture(state, payload) {
    return {
        state: initialGestureState(),
        intent: finishIntent(state, payload)
    }
}

function cancelGesture(state) {
    return {
        state: initialGestureState(),
        intent: noneIntent()
    }
}

function isDragging(state) {
    var mode = String(state && state.mode || "")
    return mode === "dragging_object" || mode === "dragging_handle"
}

function isHandleDrag(state) {
    return String(state && state.mode || "") === "dragging_handle"
}

function isObjectDrag(state) {
    return String(state && state.mode || "") === "dragging_object"
}

function isMarquee(state) {
    return String(state && state.mode || "") === "marquee_select"
}

function finishKind(state) {
    var current = cloneState(state)
    if (isHandleDrag(current) || isObjectDrag(current)) {
        return "incremental_drag"
    }
    if (isMarquee(current)) {
        return current.moved ? "marquee_select" : "marquee_click"
    }
    if (current.mode === "panning") {
        return "pan"
    }
    if (current.mode === "drawing_pending_shape") {
        return "draw_click"
    }
    return "none"
}

function finishAction(state) {
    var kind = finishKind(state)
    return {
        kind: kind,
        shouldFinishIncrementalDrag: kind === "incremental_drag",
        shouldSelectMarquee: kind === "marquee_select",
        shouldSuppressClick: kind === "incremental_drag" || kind === "marquee_select",
        shouldEndObjectMove: kind !== "marquee_select",
        shouldResetLifecycle: true
    }
}

function gestureLabel(state) {
    var mode = String(state && state.mode || "idle")
    if (mode === "dragging_handle") {
        return String(state && state.handleId || "") === "rect_rotate" ? "rotate" : "drag handle"
    }
    if (mode === "dragging_object") {
        return asArray(state && state.selectedIds).length > 1 ? "move selection" : "move object"
    }
    if (mode === "marquee_select") {
        return "marquee"
    }
    if (mode === "panning") {
        return "pan"
    }
    if (mode === "drawing_pending_shape") {
        return "draw"
    }
    return mode === "hovering" ? "hover" : "idle"
}
