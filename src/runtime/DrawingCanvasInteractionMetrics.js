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

function initialMetricsState() {
    return {
        active: false,
        mode: "idle",
        startedAtMs: 0,
        pointerMoves: 0,
        controllerMutations: 0,
        renderRequests: 0,
        hitTests: 0,
        snapResolutions: 0,
        handlePlans: 0,
        revisionStart: 0,
        revisionEnd: 0,
        selectedCountStart: 0,
        selectedCountEnd: 0,
        visibleObjectCountStart: 0,
        visibleObjectCountEnd: 0,
        events: []
    }
}

function snapshotValue(snapshot, name) {
    return Math.max(0, finiteNumber(snapshot && snapshot[name], 0))
}

function cloneState(state) {
    var source = state || initialMetricsState()
    return {
        active: source.active === true,
        mode: String(source.mode || "idle"),
        startedAtMs: finiteNumber(source.startedAtMs, 0),
        pointerMoves: snapshotValue(source, "pointerMoves"),
        controllerMutations: snapshotValue(source, "controllerMutations"),
        renderRequests: snapshotValue(source, "renderRequests"),
        hitTests: snapshotValue(source, "hitTests"),
        snapResolutions: snapshotValue(source, "snapResolutions"),
        handlePlans: snapshotValue(source, "handlePlans"),
        revisionStart: snapshotValue(source, "revisionStart"),
        revisionEnd: snapshotValue(source, "revisionEnd"),
        selectedCountStart: snapshotValue(source, "selectedCountStart"),
        selectedCountEnd: snapshotValue(source, "selectedCountEnd"),
        visibleObjectCountStart: snapshotValue(source, "visibleObjectCountStart"),
        visibleObjectCountEnd: snapshotValue(source, "visibleObjectCountEnd"),
        events: asArray(source.events).map(function(event) {
            return {
                kind: String(event && event.kind || ""),
                count: Math.max(0, finiteNumber(event && event.count, 0))
            }
        })
    }
}

function beginInteraction(state, mode, timestampMs, snapshot) {
    var next = initialMetricsState()
    next.active = true
    next.mode = String(mode || "idle")
    next.startedAtMs = finiteNumber(timestampMs, 0)
    next.revisionStart = snapshotValue(snapshot, "revision")
    next.revisionEnd = next.revisionStart
    next.selectedCountStart = snapshotValue(snapshot, "selectedCount")
    next.selectedCountEnd = next.selectedCountStart
    next.visibleObjectCountStart = snapshotValue(snapshot, "visibleObjectCount")
    next.visibleObjectCountEnd = next.visibleObjectCountStart
    return next
}

function increment(state, field, amount, eventKind) {
    var next = cloneState(state)
    if (!next.active) {
        return next
    }
    next[field] = Math.max(0, finiteNumber(next[field], 0) + Math.max(1, finiteNumber(amount, 1)))
    if (eventKind) {
        next.events.push({
            kind: String(eventKind),
            count: Math.max(1, finiteNumber(amount, 1))
        })
    }
    return next
}

function recordPointerMove(state, count) {
    return increment(state, "pointerMoves", count, "pointer_move")
}

function recordControllerMutation(state, kind, count) {
    return increment(state, "controllerMutations", count, String(kind || "controller_mutation"))
}

function recordRenderRequest(state, count) {
    return increment(state, "renderRequests", count, "render_request")
}

function recordHitTest(state, count) {
    return increment(state, "hitTests", count, "hit_test")
}

function recordSnap(state, count) {
    return increment(state, "snapResolutions", count, "snap_resolution")
}

function recordHandlePlan(state, count) {
    return increment(state, "handlePlans", count, "handle_plan")
}

function finishInteraction(state, timestampMs, snapshot) {
    var current = cloneState(state)
    current.revisionEnd = snapshotValue(snapshot, "revision")
    current.selectedCountEnd = snapshotValue(snapshot, "selectedCount")
    current.visibleObjectCountEnd = snapshotValue(snapshot, "visibleObjectCount")
    var finishedAtMs = finiteNumber(timestampMs, current.startedAtMs)
    var record = {
        mode: current.mode,
        durationMs: Math.max(0, finishedAtMs - current.startedAtMs),
        pointerMoves: current.pointerMoves,
        controllerMutations: current.controllerMutations,
        renderRequests: current.renderRequests,
        hitTests: current.hitTests,
        snapResolutions: current.snapResolutions,
        handlePlans: current.handlePlans,
        revisionStart: current.revisionStart,
        revisionEnd: current.revisionEnd,
        revisionDelta: current.revisionEnd - current.revisionStart,
        selectedCountStart: current.selectedCountStart,
        selectedCountEnd: current.selectedCountEnd,
        visibleObjectCountStart: current.visibleObjectCountStart,
        visibleObjectCountEnd: current.visibleObjectCountEnd,
        events: current.events
    }
    return {
        state: initialMetricsState(),
        record: record
    }
}

function cancelInteraction(state, timestampMs, snapshot) {
    var finished = finishInteraction(state, timestampMs, snapshot)
    finished.record.canceled = true
    return finished
}

function checkMetricMax(record, budget, budgetField, recordField, failures) {
    if (budget && budget[budgetField] !== undefined && finiteNumber(record && record[recordField], 0) > finiteNumber(budget[budgetField], 0)) {
        failures.push(recordField + " expected <= " + String(budget[budgetField]) + ", got " + String(record[recordField]))
    }
}

function checkMetricEqual(record, budget, field, failures) {
    if (budget && budget[field] !== undefined && finiteNumber(record && record[field], 0) !== finiteNumber(budget[field], 0)) {
        failures.push(field + " expected " + String(budget[field]) + ", got " + String(record[field]))
    }
}

function assertWithinBudget(record, budget) {
    var failures = []
    if (budget && budget.mode !== undefined && String(record && record.mode || "") !== String(budget.mode || "")) {
        failures.push("mode expected " + String(budget.mode) + ", got " + String(record && record.mode || ""))
    }
    checkMetricMax(record, budget, "maxDurationMs", "durationMs", failures)
    checkMetricMax(record, budget, "maxPointerMoves", "pointerMoves", failures)
    checkMetricMax(record, budget, "maxControllerMutations", "controllerMutations", failures)
    checkMetricMax(record, budget, "maxRenderRequests", "renderRequests", failures)
    checkMetricMax(record, budget, "maxHitTests", "hitTests", failures)
    checkMetricMax(record, budget, "maxSnapResolutions", "snapResolutions", failures)
    checkMetricMax(record, budget, "maxHandlePlans", "handlePlans", failures)
    checkMetricEqual(record, budget, "revisionDelta", failures)
    return {
        ok: failures.length === 0,
        failures: failures
    }
}
