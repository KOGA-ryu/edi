const fs = require("fs")
const path = require("path")
const vm = require("vm")

function fail(message) {
    console.error(`FAIL: ${message}`)
    process.exitCode = 1
}

function expect(condition, message) {
    if (!condition) {
        fail(message)
    }
}

function expectNear(actual, expected, message) {
    if (Math.abs(actual - expected) >= 0.0001) {
        fail(`${message}; expected ${expected}, got ${actual}`)
    }
}

function loadGestureModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasGestureState.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function runInitialStateContract(gesture) {
    const state = gesture.initialGestureState()
    expect(state.mode === "idle", "initial mode should be idle")
    expect(!state.started, "initial state should not be started")
    expect(state.objectId === "", "initial state should not select an object id")
    expect(state.handleId === "", "initial state should not select a handle id")
    expect(Number.isFinite(state.startPoint.x) && Number.isFinite(state.startPoint.y), "initial points should be finite")
    JSON.stringify(state)
}

function runOneActiveGestureContract(gesture) {
    const idle = gesture.initialGestureState()
    const handle = gesture.beginHandleDrag(idle, "script_rect_01", "rect_se", { x: 0.25, y: 0.25 }, { shift: false })
    expect(handle.mode === "dragging_handle", "beginHandleDrag from idle should enter dragging_handle")
    expect(handle.objectId === "script_rect_01", "handle drag should record object id")
    expect(handle.handleId === "rect_se", "handle drag should record handle id")

    const rejected = gesture.beginObjectDrag(handle, "script_rect_01", { x: 0.5, y: 0.5 }, ["script_rect_01"], {})
    expect(rejected.mode === "dragging_handle", "new active gesture should not replace current gesture implicitly")
    expect(rejected.rejected === true, "rejected transition should be explicit")
    expect(gesture.transitionAllowed("dragging_handle", "dragging_object") === false, "active drag to active drag should be rejected")
    expect(gesture.transitionAllowed("dragging_handle", "idle") === true, "active drag can transition to idle")
}

function runCancelContract(gesture) {
    const state = gesture.beginObjectDrag(gesture.initialGestureState(), "script_line_01", { x: 0.1, y: 0.1 }, ["script_line_01"], {})
    const result = gesture.cancelGesture(state)
    expect(result.state.mode === "idle", "cancel should clear to idle")
    expect(result.intent.kind === "none", "cancel should emit no mutation intent")
}

function runHoverContract(gesture) {
    const state = gesture.beginHover(gesture.initialGestureState(), { x: 0.3, y: 0.4 }, { kind: "object", objectId: "script_line_01" })
    expect(state.mode === "hovering", "hover should enter hovering mode")
    expect(state.targetKind === "object", "hover should record target kind")
    expect(state.targetObjectId === "script_line_01", "hover should record target object id")
    const finished = gesture.finishGesture(state, {})
    expect(finished.intent.kind === "none", "hover finish should emit no mutation intent")
}

function runObjectDragContract(gesture) {
    const started = gesture.beginObjectDrag(
        gesture.initialGestureState(),
        "script_line_01",
        { x: 0.2, y: 0.25 },
        ["script_line_01", "script_line_02"],
        { shift: true }
    )
    const updated = gesture.updateGesture(started, { point: { x: 0.35, y: 0.45 }, modifiers: { shift: true } })
    expect(updated.moved, "object drag update should mark moved")
    expectNear(updated.lastPoint.x, 0.35, "object drag should update last x")
    expectNear(updated.lastPoint.y, 0.45, "object drag should update last y")

    const final = gesture.finishGesture(updated, { point: { x: 0.35, y: 0.45 } })
    expect(final.state.mode === "idle", "object drag finish should clear state")
    expect(final.intent.kind === "move_objects", "multi-selection drag should emit move_objects intent")
    expect(final.intent.objectIds.length === 2, "move_objects intent should keep selected ids")
    expectNear(final.intent.dx, 0.15, "object drag intent should compute dx")
    expectNear(final.intent.dy, 0.2, "object drag intent should compute dy")

    const incremental = gesture.finishGesture(updated, { point: { x: 0.35, y: 0.45 }, incremental: true })
    expect(incremental.intent.kind === "none", "incremental object drag finish should emit no final mutation")
}

function runHandleDragContract(gesture) {
    const started = gesture.beginHandleDrag(gesture.initialGestureState(), "script_rect_01", "rect_se", { x: 0.2, y: 0.2 }, {})
    const updated = gesture.updateGesture(started, { point: { x: 0.4, y: 0.45 } })
    expect(gesture.isHandleDrag(updated), "handle drag helper should identify handle drag")
    expect(updated.moved, "handle drag update should mark moved")

    const final = gesture.finishGesture(updated, { point: { x: 0.4, y: 0.45 } })
    expect(final.intent.kind === "update_handle", "handle drag finish should emit update_handle intent")
    expect(final.intent.objectId === "script_rect_01", "handle intent should keep object id")
    expect(final.intent.handleId === "rect_se", "handle intent should keep handle id")
    expectNear(final.intent.point.x, 0.4, "handle intent should include final point")

    const incremental = gesture.finishGesture(updated, { point: { x: 0.4, y: 0.45 }, incremental: true })
    expect(incremental.intent.kind === "none", "incremental handle drag finish should emit no final mutation")
}

function runMarqueeContract(gesture) {
    const started = gesture.beginMarquee(gesture.initialGestureState(), { x: 0.1, y: 0.2 }, {})
    const updated = gesture.updateGesture(started, { point: { x: 0.6, y: 0.7 } })
    expect(gesture.isMarquee(updated), "marquee helper should identify marquee state")
    expect(updated.moved, "marquee update should mark moved")
    const final = gesture.finishGesture(updated, { point: { x: 0.6, y: 0.7 }, objectIds: ["script_a", "script_b"] })
    expect(final.intent.kind === "select_objects", "marquee should emit selection intent")
    expect(final.intent.objectIds.join(",") === "script_a,script_b", "marquee should carry selected ids")
    expectNear(final.intent.startPoint.x, 0.1, "marquee should preserve start point")
    expectNear(final.intent.endPoint.y, 0.7, "marquee should include end point")
}

function runPanContract(gesture) {
    const started = gesture.beginPan(gesture.initialGestureState(), { x: 100, y: 200 }, { meta: true })
    const updated = gesture.updateGesture(started, { screenPoint: { x: 124, y: 176 } })
    expect(updated.mode === "panning", "pan should remain panning during update")
    expect(updated.moved, "pan update should mark moved")
    const final = gesture.finishGesture(updated, { screenPoint: { x: 124, y: 176 } })
    expect(final.intent.kind === "pan", "pan finish should emit viewport intent")
    expectNear(final.intent.dxPx, 24, "pan should compute dx pixels")
    expectNear(final.intent.dyPx, -24, "pan should compute dy pixels")
}

function runFinishKindContract(gesture) {
    expect(gesture.finishKind(gesture.initialGestureState()) === "none", "idle finish kind should be none")

    const objectDrag = gesture.beginObjectDrag(
        gesture.initialGestureState(),
        "script_line_01",
        { x: 0.1, y: 0.1 },
        ["script_line_01"],
        {}
    )
    expect(gesture.finishKind(objectDrag) === "incremental_drag", "object drag finish kind should be incremental drag")

    const handleDrag = gesture.beginHandleDrag(gesture.initialGestureState(), "script_rect_01", "rect_se", { x: 0.2, y: 0.2 }, {})
    expect(gesture.finishKind(handleDrag) === "incremental_drag", "handle drag finish kind should be incremental drag")

    const marqueeClick = gesture.beginMarquee(gesture.initialGestureState(), { x: 0.2, y: 0.2 }, {})
    expect(gesture.finishKind(marqueeClick) === "marquee_click", "unmoved marquee finish kind should be marquee click")

    const marqueeSelect = gesture.updateGesture(marqueeClick, { point: { x: 0.4, y: 0.4 } })
    expect(gesture.finishKind(marqueeSelect) === "marquee_select", "moved marquee finish kind should be marquee select")

    const pan = gesture.beginPan(gesture.initialGestureState(), { x: 10, y: 20 }, {})
    expect(gesture.finishKind(pan) === "pan", "pan finish kind should be pan")

    const draw = gesture.beginDrawingPendingShape(gesture.initialGestureState(), { x: 0.3, y: 0.4 }, {})
    expect(gesture.finishKind(draw) === "draw_click", "pending draw finish kind should be draw click")
}

const gesture = loadGestureModule()
runInitialStateContract(gesture)
runOneActiveGestureContract(gesture)
runCancelContract(gesture)
runHoverContract(gesture)
runObjectDragContract(gesture)
runHandleDragContract(gesture)
runMarqueeContract(gesture)
runPanContract(gesture)
runFinishKindContract(gesture)

if (process.exitCode) {
    process.exit(process.exitCode)
}
