.pragma library
.import "DrawingCanvasInteractionMetrics.js" as CanvasInteractionMetrics
.import "DrawingCanvasInteractionScript.js" as CanvasInteractionScript

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

function safeClone(value) {
    if (value === undefined || value === null) {
        return value
    }
    return JSON.parse(JSON.stringify(value))
}

function eventCount(event) {
    var count = Number(event && event.count)
    return Number.isFinite(count) && count > 0 ? count : 1
}

function failure(message, index) {
    if (index === undefined) {
        return message
    }
    return "events[" + String(index) + "] " + message
}

function replayEvents(events) {
    var list = asArray(events)
    var failures = []
    var state = CanvasInteractionMetrics.initialMetricsState()
    var record = null

    for (var index = 0; index < list.length; ++index) {
        var event = list[index] || ({})
        var type = String(event.type || "")

        if (type === "begin") {
            if (state.active) {
                failures.push(failure("begin received while another interaction is active", index))
                continue
            }
            if (record) {
                failures.push(failure("begin received after interaction already completed", index))
                continue
            }
            state = CanvasInteractionMetrics.beginInteraction(state, event.mode, event.timestampMs, event.snapshot)
        } else if (type === "pointerMove") {
            if (!state.active) {
                failures.push(failure("pointerMove requires active interaction", index))
                continue
            }
            state = CanvasInteractionMetrics.recordPointerMove(state, eventCount(event))
        } else if (type === "snap") {
            if (!state.active) {
                failures.push(failure("snap requires active interaction", index))
                continue
            }
            state = CanvasInteractionMetrics.recordSnap(state, eventCount(event))
        } else if (type === "hitTest") {
            if (!state.active) {
                failures.push(failure("hitTest requires active interaction", index))
                continue
            }
            state = CanvasInteractionMetrics.recordHitTest(state, eventCount(event))
        } else if (type === "handlePlan") {
            if (!state.active) {
                failures.push(failure("handlePlan requires active interaction", index))
                continue
            }
            state = CanvasInteractionMetrics.recordHandlePlan(state, eventCount(event))
        } else if (type === "renderRequest") {
            if (!state.active) {
                failures.push(failure("renderRequest requires active interaction", index))
                continue
            }
            state = CanvasInteractionMetrics.recordRenderRequest(state, eventCount(event))
        } else if (type === "controllerMutation") {
            if (!state.active) {
                failures.push(failure("controllerMutation requires active interaction", index))
                continue
            }
            state = CanvasInteractionMetrics.recordControllerMutation(state, event.kind, eventCount(event))
        } else if (type === "finish") {
            if (!state.active) {
                failures.push(failure("finish requires active interaction", index))
                continue
            }
            var finished = CanvasInteractionMetrics.finishInteraction(state, event.timestampMs, event.snapshot)
            state = finished.state
            record = finished.record
        } else if (type === "cancel") {
            if (!state.active) {
                failures.push(failure("cancel requires active interaction", index))
                continue
            }
            var canceled = CanvasInteractionMetrics.cancelInteraction(state, event.timestampMs, event.snapshot)
            state = canceled.state
            record = canceled.record
        } else {
            failures.push(failure("unknown event type " + String(type || "<empty>"), index))
        }
    }

    if (state.active) {
        failures.push("events ended with active interaction; finish or cancel is required")
    } else if (!record) {
        failures.push("events did not complete an interaction")
    }

    return {
        ok: failures.length === 0,
        record: record || ({}),
        failures: failures
    }
}

function fixtureWithReplayMetrics(fixture, record) {
    var derived = safeClone(fixture) || ({})
    derived.actual = derived.actual || ({})
    derived.actual.metrics = safeClone(record) || ({})
    return derived
}

function evaluateReplayFixture(fixture) {
    var validation = CanvasInteractionScript.validateFixture
            ? CanvasInteractionScript.validateFixture(fixture)
            : ({ ok: true, failures: [] })
    if (!validation.ok) {
        return {
            ok: false,
            record: {},
            failures: validation.failures
        }
    }

    var replay = replayEvents(fixture && fixture.events)
    if (!replay.ok) {
        return {
            ok: false,
            record: replay.record,
            failures: replay.failures
        }
    }

    var result = CanvasInteractionScript.evaluateFixture(fixtureWithReplayMetrics(fixture, replay.record))
    return {
        ok: result.ok,
        record: replay.record,
        failures: result.failures
    }
}

function evaluateReplayFixtures(fixtures) {
    var list = asArray(fixtures)
    var failures = []
    for (var index = 0; index < list.length; ++index) {
        var result = evaluateReplayFixture(list[index])
        if (!result.ok) {
            failures.push({
                name: String(list[index] && list[index].name || "fixture_" + String(index)),
                failures: result.failures
            })
        }
    }
    return {
        ok: failures.length === 0,
        failures: failures
    }
}
