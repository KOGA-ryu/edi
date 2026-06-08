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

function loadMetricsModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionMetrics.js")
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

function runLifecycleContract(metrics) {
    let state = metrics.beginInteraction(metrics.initialMetricsState(), "dragging_object", 1000, {
        revision: 20,
        selectedCount: 1,
        visibleObjectCount: 24,
    })
    state = metrics.recordPointerMove(state)
    state = metrics.recordPointerMove(state)
    state = metrics.recordSnap(state, 2)
    state = metrics.recordHitTest(state)
    state = metrics.recordControllerMutation(state, "move_selected")
    state = metrics.recordRenderRequest(state)
    const finished = metrics.finishInteraction(state, 1425, {
        revision: 21,
        selectedCount: 1,
        visibleObjectCount: 24,
    })

    expect(finished.state.active === false, "finish should clear active metrics state")
    expect(finished.record.mode === "dragging_object", "record should preserve interaction mode")
    expectNear(finished.record.durationMs, 425, "record should compute duration")
    expect(finished.record.pointerMoves === 2, "record should count pointer moves")
    expect(finished.record.snapResolutions === 2, "record should count snap resolutions")
    expect(finished.record.hitTests === 1, "record should count hit tests")
    expect(finished.record.controllerMutations === 1, "record should count controller mutations")
    expect(finished.record.renderRequests === 1, "record should count render requests")
    expect(finished.record.revisionDelta === 1, "record should compute revision delta")
    expect(finished.record.events.length === 6, "record should retain event trace")
}

function runInactiveContract(metrics) {
    const idle = metrics.initialMetricsState()
    const next = metrics.recordPointerMove(idle)
    expect(next.pointerMoves === 0, "inactive metrics should ignore pointer move records")
    expect(next.active === false, "inactive metrics should remain inactive")
}

function runCancelContract(metrics) {
    let state = metrics.beginInteraction(metrics.initialMetricsState(), "dragging_handle", 50, {
        revision: 3,
        selectedCount: 1,
        visibleObjectCount: 4,
    })
    state = metrics.recordHandlePlan(state)
    const canceled = metrics.cancelInteraction(state, 75, {
        revision: 3,
        selectedCount: 1,
        visibleObjectCount: 4,
    })
    expect(canceled.record.canceled === true, "cancel should mark the finished record")
    expect(canceled.record.handlePlans === 1, "cancel should preserve observed counts")
    expect(canceled.record.revisionDelta === 0, "cancel should preserve revision delta")
}

function runBudgetContract(metrics) {
    const record = {
        mode: "dragging_handle",
        durationMs: 120,
        pointerMoves: 4,
        controllerMutations: 2,
        renderRequests: 3,
        hitTests: 1,
        snapResolutions: 4,
        handlePlans: 4,
        revisionDelta: 1,
    }
    const ok = metrics.assertWithinBudget(record, {
        mode: "dragging_handle",
        maxDurationMs: 200,
        maxPointerMoves: 5,
        maxControllerMutations: 4,
        maxRenderRequests: 4,
        maxHitTests: 2,
        maxSnapResolutions: 5,
        maxHandlePlans: 5,
        revisionDelta: 1,
    })
    expect(ok.ok, "record should pass matching budget")

    const bad = metrics.assertWithinBudget(record, {
        mode: "dragging_object",
        maxPointerMoves: 2,
        maxControllerMutations: 1,
        revisionDelta: 0,
    })
    expect(!bad.ok, "record should fail exceeded budget")
    expect(bad.failures.length === 4, "budget failures should include mode, pointer, mutation, revision")
}

const metrics = loadMetricsModule()
runLifecycleContract(metrics)
runInactiveContract(metrics)
runCancelContract(metrics)
runBudgetContract(metrics)

if (process.exitCode) {
    process.exit(process.exitCode)
}
