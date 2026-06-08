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

function loadModule(modulePath, context) {
    const source = fs.readFileSync(modulePath, "utf8")
        .replace(".pragma library", "")
        .replace(/^\.import .*$/gm, "")
    vm.runInContext(source, context, { filename: modulePath })
}

function loadTelemetryModule() {
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionTelemetry.js"), context)
    return context
}

function loadReplayModule() {
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)

    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionMetrics.js"), context)
    context.CanvasInteractionMetrics = {
        initialMetricsState: context.initialMetricsState,
        beginInteraction: context.beginInteraction,
        recordPointerMove: context.recordPointerMove,
        recordControllerMutation: context.recordControllerMutation,
        recordRenderRequest: context.recordRenderRequest,
        recordHitTest: context.recordHitTest,
        recordSnap: context.recordSnap,
        recordHandlePlan: context.recordHandlePlan,
        finishInteraction: context.finishInteraction,
        cancelInteraction: context.cancelInteraction,
        assertWithinBudget: context.assertWithinBudget,
    }

    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionScript.js"), context)
    context.CanvasInteractionScript = {
        validateFixture: context.validateFixture,
        evaluateFixture: context.evaluateFixture,
        evaluateFixtures: context.evaluateFixtures,
    }

    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionReplay.js"), context)
    return context
}

function runLifecycleContract(telemetry) {
    let state = telemetry.beginInteraction(telemetry.initialTelemetryState(), "dragging_handle", 1000, {
        revision: 20,
        selectedCount: 1,
        visibleObjectCount: 24,
    })
    state = telemetry.recordHitTest(state)
    state = telemetry.recordPointerMove(state)
    state = telemetry.recordSnap(state)
    state = telemetry.recordHandlePlan(state)
    state = telemetry.recordControllerMutation(state, "update_handle_field")
    state = telemetry.recordRenderRequest(state)
    const finished = telemetry.finishInteraction(state, 1120, {
        revision: 21,
        selectedCount: 1,
        visibleObjectCount: 24,
    })

    expect(finished.state.active === false, "finish should clear active telemetry")
    expect(finished.events.length === 8, "finish should include begin, six events, and finish")
    expect(finished.events[0].type === "begin", "first event should be begin")
    expect(finished.events[0].mode === "dragging_handle", "begin should preserve mode")
    expect(finished.events[0].snapshot.revision === 20, "begin should preserve start revision")
    expect(finished.events[5].kind === "update_handle_field", "mutation should preserve kind")
    expect(finished.events[7].type === "finish", "last event should be finish")
    expect(finished.state.completedEvents.length === finished.events.length, "state should keep completed event stream")
}

function runInactiveContract(telemetry) {
    const idle = telemetry.initialTelemetryState()
    const next = telemetry.recordPointerMove(idle)
    expect(next.active === false, "inactive telemetry should stay inactive")
    expect(next.events.length === 0, "inactive telemetry should ignore metric events")
}

function runCancelContract(telemetry) {
    let state = telemetry.beginInteraction(telemetry.initialTelemetryState(), "marquee_select", 50, {
        revision: 3,
        selectedCount: 0,
        visibleObjectCount: 4,
    })
    state = telemetry.recordPointerMove(state, 2)
    const canceled = telemetry.cancelInteraction(state, 75, {
        revision: 3,
        selectedCount: 0,
        visibleObjectCount: 4,
    })
    expect(canceled.events[canceled.events.length - 1].type === "cancel", "cancel should end stream with cancel event")
    expect(canceled.events[1].count === 2, "event count should preserve explicit amount")
}

function runReplayCompatibilityContract(telemetry, replay) {
    let state = telemetry.beginInteraction(telemetry.initialTelemetryState(), "dragging_object", 200, {
        revision: 7,
        selectedCount: 1,
        visibleObjectCount: 4,
    })
    state = telemetry.recordHitTest(state)
    state = telemetry.recordPointerMove(state)
    state = telemetry.recordControllerMutation(state, "move_selected")
    const finished = telemetry.finishInteraction(state, 260, {
        revision: 8,
        selectedCount: 1,
        visibleObjectCount: 4,
    })
    const replayed = replay.replayEvents(finished.events)
    expect(replayed.ok, "telemetry events should replay through metrics runner")
    expect(replayed.record.mode === "dragging_object", "replayed telemetry should preserve mode")
    expect(replayed.record.durationMs === 60, "replayed telemetry should preserve duration")
    expect(replayed.record.revisionDelta === 1, "replayed telemetry should preserve revision delta")
    expect(replayed.record.controllerMutations === 1, "replayed telemetry should preserve mutation count")
}

const telemetry = loadTelemetryModule()
runLifecycleContract(telemetry)
runInactiveContract(telemetry)
runCancelContract(telemetry)
runReplayCompatibilityContract(telemetry, loadReplayModule())

if (process.exitCode) {
    process.exit(process.exitCode)
}
