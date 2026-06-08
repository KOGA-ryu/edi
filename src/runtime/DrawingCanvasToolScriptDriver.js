.pragma library

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

function cloneValue(value) {
    if (value === undefined || value === null) {
        return value
    }
    return JSON.parse(JSON.stringify(value))
}

function point(value, fallback) {
    var source = value || fallback || ({})
    return {
        x: finiteNumber(source.x, 0),
        y: finiteNumber(source.y, 0)
    }
}

function pointerOp(op, pointValue) {
    var p = point(pointValue)
    return {
        op: op,
        x: p.x,
        y: p.y
    }
}

function pointerDown(pointValue) {
    var op = pointerOp("pointerDown", pointValue)
    op.button = "left"
    return op
}

function pointerUp(pointValue) {
    var op = pointerOp("pointerUp", pointValue)
    op.button = "left"
    return op
}

function interpolatedPoint(from, to, index, count) {
    var start = point(from)
    var end = point(to, start)
    var total = Math.max(1, Math.round(finiteNumber(count, 1)))
    var t = Math.max(0, Math.min(1, index / total))
    return {
        x: Math.round((start.x + (end.x - start.x) * t) * 10000) / 10000,
        y: Math.round((start.y + (end.y - start.y) * t) * 10000) / 10000
    }
}

function dragOps(fromValue, toValue, pointerMoves) {
    var from = point(fromValue, toValue)
    var to = point(toValue, from)
    var moves = Math.max(1, Math.round(finiteNumber(pointerMoves, 8)))
    var ops = [
        pointerOp("movePointer", from),
        pointerDown(from)
    ]
    for (var index = 1; index <= moves; ++index) {
        ops.push(pointerOp("pointerMove", interpolatedPoint(from, to, index, moves)))
    }
    ops.push(pointerUp(to))
    return ops
}

function stepTelemetryMode(step) {
    var type = String(step && step.type || "")
    if (type === "clickCanvas") {
        return "draw_click"
    }
    if (type === "dragHandle") {
        return "dragging_handle"
    }
    if (type === "dragObject") {
        return "dragging_object"
    }
    if (type === "marqueeSelect") {
        return "marquee_select"
    }
    if (type === "panCanvas") {
        return "panning"
    }
    if (type === "zoomCanvas") {
        return "zooming"
    }
    return ""
}

function appendUnique(list, value) {
    var text = String(value || "")
    if (text.length > 0 && list.indexOf(text) < 0) {
        list.push(text)
    }
}

function stepDriverOps(step) {
    var type = String(step && step.type || "")
    if (type === "selectTool") {
        return [{
            op: "setTool",
            toolId: String(step.toolId || "")
        }]
    }
    if (type === "clickCanvas") {
        var p = point(step.point)
        return [
            pointerOp("movePointer", p),
            pointerDown(p),
            pointerUp(p)
        ]
    }
    if (type === "dragHandle") {
        var handleOps = dragOps(step.from || step.to, step.to, step.pointerMoves)
        handleOps.unshift({
            op: "targetHandle",
            object: String(step.object || ""),
            handleId: String(step.handleId || "")
        })
        return handleOps
    }
    if (type === "dragObject") {
        var from = step.from || ({ x: 0, y: 0 })
        var to = step.to || ({
            x: finiteNumber(from.x, 0) + finiteNumber(step.dx, 0),
            y: finiteNumber(from.y, 0) + finiteNumber(step.dy, 0)
        })
        var objectOps = dragOps(from, to, step.pointerMoves)
        objectOps.unshift({
            op: "targetObject",
            object: String(step.object || "")
        })
        return objectOps
    }
    if (type === "marqueeSelect") {
        return dragOps(step.from, step.to, step.pointerMoves)
    }
    if (type === "panCanvas") {
        return [{
            op: "wheelPan",
            dx: finiteNumber(step.dx, 0),
            dy: finiteNumber(step.dy, 0)
        }]
    }
    if (type === "zoomCanvas") {
        var zoom = {
            op: "wheelZoom",
            factor: finiteNumber(step.factor, 1)
        }
        if (step.point) {
            var pz = point(step.point)
            zoom.x = pz.x
            zoom.y = pz.y
        }
        return [zoom]
    }
    return [{
        op: "unsupportedStep",
        type: type
    }]
}

function driverPlan(executionPlan) {
    var source = executionPlan && executionPlan.plan ? executionPlan.plan : executionPlan
    var steps = asArray(source && source.steps)
    var ops = []
    var telemetryModes = []
    var stepMap = []
    for (var index = 0; index < steps.length; ++index) {
        var before = ops.length
        var stepOps = stepDriverOps(steps[index])
        for (var opIndex = 0; opIndex < stepOps.length; ++opIndex) {
            ops.push(stepOps[opIndex])
        }
        appendUnique(telemetryModes, stepTelemetryMode(steps[index]))
        stepMap.push({
            stepIndex: index,
            stepType: String(steps[index] && steps[index].type || ""),
            opStart: before,
            opCount: stepOps.length,
            telemetryMode: stepTelemetryMode(steps[index])
        })
    }
    return {
        ok: true,
        failures: [],
        plan: {
            name: String(source && source.name || ""),
            profile: String(source && source.profile || ""),
            ops: ops,
            expectedTelemetryModes: telemetryModes,
            metricsByMode: cloneValue(source && source.metricsByMode || ({})) || ({}),
            expect: cloneValue(source && source.expect || ({})) || ({}),
            stepMap: stepMap
        }
    }
}

function driverOps(executionPlan) {
    return driverPlan(executionPlan).plan.ops
}
