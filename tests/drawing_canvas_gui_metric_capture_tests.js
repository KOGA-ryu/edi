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

function loadCaptureModule() {
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
    context.CanvasInteractionReplay = {
        replayEvents: context.replayEvents,
    }

    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasMetricReducer.js"), context)
    context.CanvasMetricReducer = {
        reduceMetrics: context.reduceMetrics,
    }

    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasMetricReport.js"), context)
    context.CanvasMetricReport = {
        buildMetricReport: context.buildMetricReport,
        rawRecordsJsonl: context.rawRecordsJsonl,
        parseRawRecordsJsonl: context.parseRawRecordsJsonl,
        summaryReportJson: context.summaryReportJson,
        artifactPaths: context.artifactPaths,
    }

    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasGuiMetricCapture.js"), context)
    return context
}

function sampleEvents(startMs, endMs) {
    return [
        {
            type: "begin",
            mode: "dragging_handle",
            timestampMs: startMs,
            snapshot: { revision: 20, selectedCount: 1, visibleObjectCount: 24 },
        },
        { type: "hitTest" },
        { type: "pointerMove" },
        { type: "snap" },
        { type: "handlePlan" },
        { type: "controllerMutation", kind: "update_handle_field" },
        { type: "renderRequest" },
        { type: "pointerMove" },
        { type: "snap" },
        { type: "handlePlan" },
        { type: "controllerMutation", kind: "update_handle_field" },
        { type: "renderRequest" },
        {
            type: "finish",
            timestampMs: endMs,
            snapshot: { revision: 21, selectedCount: 1, visibleObjectCount: 24 },
        },
    ]
}

function consoleLine(events) {
    return `qml: drawing_canvas_interaction_events ${JSON.stringify(events)}`
}

function runLineParserContract(capture) {
    const ignored = capture.parseTelemetryLine("qml: unrelated log")
    expect(ignored.ok, "unrelated console lines should not fail")
    expect(!ignored.found, "unrelated console lines should not be captured")

    const parsed = capture.parseTelemetryLine(consoleLine(sampleEvents(1000, 1120)))
    expect(parsed.ok, "telemetry line should parse")
    expect(parsed.found, "telemetry line should be found")
    expect(parsed.events.length === 13, "telemetry line should return event array")

    const invalid = capture.parseTelemetryLine("drawing_canvas_interaction_events nope")
    expect(!invalid.ok, "invalid JSON payload should fail")
    expect(invalid.failures[0].indexOf("JSON parse failed") >= 0, "invalid JSON should explain parse failure")
}

function runCaptureReplayContract(capture) {
    const log = [
        "qml: startup noise",
        consoleLine(sampleEvents(1000, 1120)),
        "qml: middle noise",
        consoleLine(sampleEvents(2000, 2110)),
    ].join("\n")

    const parsed = capture.parseTelemetryLines(log)
    expect(parsed.ok, "valid captured log should parse")
    expect(parsed.streams.length === 2, "parser should extract two telemetry streams")
    expect(parsed.streams[0].sampleId === "capture_1", "parser should assign stable sample ids")

    const replayed = capture.replayCapturedStreams(parsed.streams)
    expect(replayed.ok, "captured streams should replay")
    expect(replayed.records.length === 2, "replay should produce one record per stream")
    expect(replayed.records[0].mode === "dragging_handle", "record should preserve mode")
    expect(replayed.records[0].pointerMoves === 2, "record should preserve pointer move count")
    expect(replayed.records[0].controllerMutations === 2, "record should preserve mutation count")
    expect(replayed.records[0].revisionDelta === 1, "record should preserve revision delta")
}

function runCaptureFailureContract(capture) {
    const badJson = capture.capturedMetricRecords("qml: drawing_canvas_interaction_events []\nqml: drawing_canvas_interaction_events nope")
    expect(!badJson.ok, "invalid capture payload should fail")
    expect(badJson.failures.length === 1, "capture should report invalid telemetry line")

    const badLifecycle = capture.capturedMetricRecords(consoleLine([{ type: "pointerMove" }]))
    expect(!badLifecycle.ok, "bad interaction lifecycle should fail")
    expect(badLifecycle.failures[0].failures.some(message => message.indexOf("requires active interaction") >= 0), "bad lifecycle should explain replay failure")
}

function runCaptureReportContract(capture) {
    const log = [
        consoleLine(sampleEvents(1000, 1120)),
        consoleLine(sampleEvents(2000, 2110)),
    ].join("\n")
    const report = capture.buildCaptureReport("Drag Line Endpoint GUI", log, {
        mode: "dragging_handle",
        maxDurationMs: 150,
        maxPointerMoves: 2,
        maxControllerMutations: 2,
        maxRenderRequests: 2,
        maxHitTests: 1,
        maxSnapResolutions: 2,
        maxHandlePlans: 2,
        revisionDelta: 1,
    })

    expect(report.ok, "captured report should pass matching budgets")
    expect(report.samples === 2, "captured report should include sample count")
    expect(report.summary.durationMs.p95 === 120, "captured report should summarize durations")
    expect(report.records === undefined, "captured report should not include raw records")
}

function runArtifactWriterContract(capture) {
    const log = [
        consoleLine(sampleEvents(1000, 1120)),
        consoleLine(sampleEvents(2000, 2110)),
    ].join("\n")
    const captured = capture.capturedMetricRecords(log)
    const report = capture.buildCaptureReport("Drag Line Endpoint GUI", log, {
        mode: "dragging_handle",
        maxControllerMutations: 2,
        revisionDelta: 1,
    })
    const paths = capture.CanvasMetricReport.artifactPaths(path.join(__dirname, "artifacts", "drawing_metrics"), "Drag Line Endpoint GUI")
    fs.mkdirSync(paths.rawDir, { recursive: true })
    fs.mkdirSync(paths.reportsDir, { recursive: true })
    fs.writeFileSync(paths.rawJsonl, capture.CanvasMetricReport.rawRecordsJsonl(captured.records))
    fs.writeFileSync(paths.summaryJson, capture.CanvasMetricReport.summaryReportJson(report))

    const records = capture.CanvasMetricReport.parseRawRecordsJsonl(fs.readFileSync(paths.rawJsonl, "utf8"))
    const summary = JSON.parse(fs.readFileSync(paths.summaryJson, "utf8"))
    expect(records.length === 2, "capture raw artifact should include replayed records")
    expect(summary.samples === 2, "capture summary artifact should include sample count")
    expect(summary.records === undefined, "capture summary artifact should not include raw records")
}

const capture = loadCaptureModule()
runLineParserContract(capture)
runCaptureReplayContract(capture)
runCaptureFailureContract(capture)
runCaptureReportContract(capture)
runArtifactWriterContract(capture)

if (process.exitCode) {
    process.exit(process.exitCode)
}
