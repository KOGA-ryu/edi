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

function readFixture(name) {
    return JSON.parse(fs.readFileSync(path.join(__dirname, "fixtures", "drawing_interactions", name), "utf8"))
}

function runReplayContract(replay) {
    const result = replay.replayEvents([
        {
            type: "begin",
            mode: "dragging_object",
            timestampMs: 200,
            snapshot: { revision: 7, selectedCount: 1, visibleObjectCount: 4 },
        },
        { type: "hitTest", count: 2 },
        { type: "pointerMove" },
        { type: "snap" },
        { type: "controllerMutation", kind: "move_selected", count: 3 },
        { type: "renderRequest", count: 2 },
        {
            type: "finish",
            timestampMs: 260,
            snapshot: { revision: 8, selectedCount: 1, visibleObjectCount: 4 },
        },
    ])

    expect(result.ok, "valid event stream should replay")
    expect(result.record.mode === "dragging_object", "replay should preserve mode")
    expect(result.record.durationMs === 60, "replay should compute duration")
    expect(result.record.pointerMoves === 1, "replay should count pointer moves")
    expect(result.record.hitTests === 2, "replay should count hit tests")
    expect(result.record.snapResolutions === 1, "replay should count snap resolutions")
    expect(result.record.controllerMutations === 3, "replay should count controller mutations")
    expect(result.record.renderRequests === 2, "replay should count render requests")
    expect(result.record.revisionDelta === 1, "replay should compute revision delta")
}

function runReplayValidationContract(replay) {
    const unknown = replay.replayEvents([
        { type: "begin", mode: "dragging_object", timestampMs: 1 },
        { type: "wildcard" },
        { type: "finish", timestampMs: 2 },
    ])
    expect(!unknown.ok, "unknown events should fail replay")
    expect(unknown.failures.some(message => message.indexOf("unknown event type") >= 0), "unknown event failure should be explicit")

    const beforeBegin = replay.replayEvents([{ type: "pointerMove" }])
    expect(!beforeBegin.ok, "metric event before begin should fail")
    expect(beforeBegin.failures.some(message => message.indexOf("requires active interaction") >= 0), "before-begin failure should mention active interaction")

    const doubleBegin = replay.replayEvents([
        { type: "begin", mode: "dragging_object", timestampMs: 1 },
        { type: "begin", mode: "dragging_handle", timestampMs: 2 },
        { type: "finish", timestampMs: 3 },
    ])
    expect(!doubleBegin.ok, "double begin should fail")
    expect(doubleBegin.failures.some(message => message.indexOf("another interaction is active") >= 0), "double begin failure should be explicit")

    const missingFinish = replay.replayEvents([
        { type: "begin", mode: "dragging_object", timestampMs: 1 },
        { type: "pointerMove" },
    ])
    expect(!missingFinish.ok, "missing finish should fail")
    expect(missingFinish.failures.some(message => message.indexOf("finish or cancel is required") >= 0), "missing finish failure should be explicit")
}

function runFixtureReplayContract(replay) {
    const fixture = readFixture("drag_line_endpoint.events.json")
    const result = replay.evaluateReplayFixture(fixture)
    expect(result.ok, "event fixture should pass after replay")
    expect(result.record.pointerMoves === 2, "fixture replay should produce pointer move count")
    expect(result.record.controllerMutations === 2, "fixture replay should produce mutation count")

    fixture.events.splice(fixture.events.length - 1, 0, { type: "controllerMutation", kind: "extra" })
    fixture.events.splice(fixture.events.length - 1, 0, { type: "controllerMutation", kind: "extra" })
    fixture.events.splice(fixture.events.length - 1, 0, { type: "controllerMutation", kind: "extra" })
    const failed = replay.evaluateReplayFixture(fixture)
    expect(!failed.ok, "fixture should fail when replayed metrics exceed budget")
    expect(
        failed.failures.some(message => message.indexOf("controllerMutations expected <=") >= 0),
        "fixture replay failure should explain exceeded budget"
    )
}

function runBatchReplayContract(replay) {
    const good = readFixture("drag_line_endpoint.events.json")
    const bad = readFixture("drag_line_endpoint.events.json")
    bad.name = "bad_drag_line_endpoint"
    bad.actual.state.selectedObjectId = "other"

    const result = replay.evaluateReplayFixtures([good, bad])
    expect(!result.ok, "batch replay should fail when any fixture fails")
    expect(result.failures.length === 1, "batch replay should report only failing fixture")
    expect(result.failures[0].name === "bad_drag_line_endpoint", "batch replay should keep failing fixture name")
}

const replay = loadReplayModule()
runReplayContract(replay)
runReplayValidationContract(replay)
runFixtureReplayContract(replay)
runBatchReplayContract(replay)

if (process.exitCode) {
    process.exit(process.exitCode)
}
