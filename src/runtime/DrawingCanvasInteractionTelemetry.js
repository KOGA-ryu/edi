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

function eventCount(count) {
    var value = finiteNumber(count, 1)
    return value > 0 ? value : 1
}

function normalizedSnapshot(snapshot) {
    return {
        revision: Math.max(0, finiteNumber(snapshot && snapshot.revision, 0)),
        selectedCount: Math.max(0, finiteNumber(snapshot && snapshot.selectedCount, 0)),
        visibleObjectCount: Math.max(0, finiteNumber(snapshot && snapshot.visibleObjectCount, 0))
    }
}

function initialTelemetryState() {
    return {
        active: false,
        mode: "idle",
        events: [],
        completedEvents: []
    }
}

function cloneState(state) {
    var source = state || initialTelemetryState()
    return {
        active: source.active === true,
        mode: String(source.mode || "idle"),
        events: asArray(source.events).map(cloneValue),
        completedEvents: asArray(source.completedEvents).map(cloneValue)
    }
}

function appendEvent(state, event) {
    var next = cloneState(state)
    if (!next.active) {
        return next
    }
    next.events.push(event)
    return next
}

function beginInteraction(state, mode, timestampMs, snapshot) {
    var next = initialTelemetryState()
    next.active = true
    next.mode = String(mode || "idle")
    next.completedEvents = asArray(state && state.completedEvents).map(cloneValue)
    next.events.push({
        type: "begin",
        mode: next.mode,
        timestampMs: finiteNumber(timestampMs, 0),
        snapshot: normalizedSnapshot(snapshot)
    })
    return next
}

function recordPointerMove(state, count) {
    return appendEvent(state, {
        type: "pointerMove",
        count: eventCount(count)
    })
}

function recordControllerMutation(state, kind, count) {
    return appendEvent(state, {
        type: "controllerMutation",
        kind: String(kind || "controller_mutation"),
        count: eventCount(count)
    })
}

function recordRenderRequest(state, count) {
    return appendEvent(state, {
        type: "renderRequest",
        count: eventCount(count)
    })
}

function recordHitTest(state, count) {
    return appendEvent(state, {
        type: "hitTest",
        count: eventCount(count)
    })
}

function recordSnap(state, count) {
    return appendEvent(state, {
        type: "snap",
        count: eventCount(count)
    })
}

function recordHandlePlan(state, count) {
    return appendEvent(state, {
        type: "handlePlan",
        count: eventCount(count)
    })
}

function finishWithType(state, type, timestampMs, snapshot) {
    var current = cloneState(state)
    if (!current.active) {
        return {
            state: current,
            events: []
        }
    }
    current.events.push({
        type: type,
        timestampMs: finiteNumber(timestampMs, 0),
        snapshot: normalizedSnapshot(snapshot)
    })
    var completed = current.events.map(cloneValue)
    return {
        state: {
            active: false,
            mode: "idle",
            events: [],
            completedEvents: completed
        },
        events: completed
    }
}

function finishInteraction(state, timestampMs, snapshot) {
    return finishWithType(state, "finish", timestampMs, snapshot)
}

function cancelInteraction(state, timestampMs, snapshot) {
    return finishWithType(state, "cancel", timestampMs, snapshot)
}
