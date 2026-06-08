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

function loadReportModule() {
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasMetricReducer.js"), context)
    context.CanvasMetricReducer = {
        reduceMetrics: context.reduceMetrics,
        reduceMetricsByMode: context.reduceMetricsByMode,
    }
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasMetricReport.js"), context)
    return context
}

function sampleRecords() {
    return [
        { sampleId: "run_01", mode: "dragging_handle", durationMs: 100, pointerMoves: 2, controllerMutations: 2, renderRequests: 2, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 },
        { sampleId: "run_02", mode: "dragging_handle", durationMs: 110, pointerMoves: 2, controllerMutations: 2, renderRequests: 3, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 },
        { sampleId: "run_03", mode: "dragging_handle", durationMs: 120, pointerMoves: 2, controllerMutations: 2, renderRequests: 4, hitTests: 2, snapResolutions: 3, handlePlans: 2, revisionDelta: 1 },
    ]
}

function runReportShapeContract(report) {
    const result = report.buildMetricReport("Drag Line Endpoint", sampleRecords(), {
        mode: "dragging_handle",
        maxDurationMs: 150,
        maxPointerMoves: 2,
        maxControllerMutations: 2,
        maxRenderRequests: 4,
        maxHitTests: 2,
        maxSnapResolutions: 3,
        maxHandlePlans: 2,
        revisionDelta: 1,
    })

    expect(result.name === "Drag Line Endpoint", "report should preserve display name")
    expect(result.ok, "report should pass matching budgets")
    expect(result.samples === 3, "report should include sample count")
    expect(result.summary.durationMs.p95 === 120, "report should include reduced summary")
    expect(result.summary.renderRequestsPerPointerMove.max === 2, "report should include ratio summary")
    expect(result.modes.dragging_handle.samples === 3, "report should include per-mode summary")
    expect(result.failures.length === 0, "report should include capped failures")
    expect(result.records === undefined, "report should not include raw records")
}

function runModeReportContract(report) {
    const result = report.buildMetricModeReport("Mixed GUI", [
        { sampleId: "click_01", mode: "draw_click", durationMs: 4, pointerMoves: 0, controllerMutations: 1, renderRequests: 1, hitTests: 0, snapResolutions: 1, handlePlans: 0, revisionDelta: 1 },
        { sampleId: "drag_01", mode: "dragging_handle", durationMs: 650, pointerMoves: 36, controllerMutations: 36, renderRequests: 72, hitTests: 70, snapResolutions: 36, handlePlans: 36, revisionDelta: 36 },
    ], {
        draw_click: {
            maxRenderRequests: 1,
            revisionDelta: 1,
        },
        dragging_handle: {
            maxRenderRequestsPerPointerMove: 1.25,
        },
    })

    expect(!result.ok, "mode report should fail expensive mode budget")
    expect(result.samples === 2, "mode report should include total samples")
    expect(result.modes.draw_click.ok, "mode report should preserve passing mode")
    expect(!result.modes.dragging_handle.ok, "mode report should preserve failing mode")
    expect(result.modes.dragging_handle.summary.renderRequestsPerPointerMove.max === 2, "mode report should include ratio summary")
    expect(result.summary === undefined, "mode report should not duplicate all-mode summary")
}

function runArtifactNamingContract(report) {
    expect(report.safeArtifactName("Drag Line Endpoint!") === "drag_line_endpoint", "safe artifact names should normalize punctuation")
    const files = report.reportFileNames("Drag Line Endpoint!")
    expect(files.rawJsonl === "drag_line_endpoint.jsonl", "raw filename should use jsonl")
    expect(files.summaryJson === "drag_line_endpoint.summary.json", "summary filename should be explicit")

    const paths = report.artifactPaths("tests/artifacts/drawing_metrics", "Drag Line Endpoint!")
    expect(paths.rawDir === "tests/artifacts/drawing_metrics/raw", "paths should include raw dir")
    expect(paths.reportsDir === "tests/artifacts/drawing_metrics/reports", "paths should include reports dir")
    expect(paths.rawJsonl === "tests/artifacts/drawing_metrics/raw/drag_line_endpoint.jsonl", "paths should include raw artifact")
    expect(paths.summaryJson === "tests/artifacts/drawing_metrics/reports/drag_line_endpoint.summary.json", "paths should include summary artifact")
}

function runJsonlContract(report) {
    const records = sampleRecords()
    const jsonl = report.rawRecordsJsonl(records)
    const parsed = report.parseRawRecordsJsonl(jsonl)
    expect(jsonl.endsWith("\n"), "jsonl should end with newline for append-friendly artifacts")
    expect(parsed.length === records.length, "jsonl should parse all records")
    expect(parsed[1].sampleId === "run_02", "jsonl should preserve record fields")
}

function runArtifactRoundTripContract(report) {
    const artifactRoot = path.join(__dirname, "artifacts", "drawing_metrics")
    const paths = report.artifactPaths(artifactRoot, "Drag Line Endpoint")
    fs.mkdirSync(paths.rawDir, { recursive: true })
    fs.mkdirSync(paths.reportsDir, { recursive: true })

    const records = sampleRecords()
    const summary = report.buildMetricReport("Drag Line Endpoint", records, {
        mode: "dragging_handle",
        maxControllerMutations: 2,
        revisionDelta: 1,
    })

    fs.writeFileSync(paths.rawJsonl, report.rawRecordsJsonl(records))
    fs.writeFileSync(paths.summaryJson, report.summaryReportJson(summary))

    const readRecords = report.parseRawRecordsJsonl(fs.readFileSync(paths.rawJsonl, "utf8"))
    const readSummary = JSON.parse(fs.readFileSync(paths.summaryJson, "utf8"))
    expect(readRecords.length === 3, "raw artifact should round-trip records")
    expect(readSummary.name === "Drag Line Endpoint", "summary artifact should round-trip report")
    expect(readSummary.records === undefined, "summary artifact should not include raw records")
}

const report = loadReportModule()
runReportShapeContract(report)
runModeReportContract(report)
runArtifactNamingContract(report)
runJsonlContract(report)
runArtifactRoundTripContract(report)

if (process.exitCode) {
    process.exit(process.exitCode)
}
