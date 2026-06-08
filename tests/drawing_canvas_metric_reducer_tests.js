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

function loadReducerModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasMetricReducer.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function sampleRecords() {
    return [
        { sampleId: "run_01", mode: "dragging_handle", durationMs: 100, pointerMoves: 2, controllerMutations: 2, renderRequests: 2, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 },
        { sampleId: "run_02", mode: "dragging_handle", durationMs: 110, pointerMoves: 2, controllerMutations: 2, renderRequests: 3, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 },
        { sampleId: "run_03", mode: "dragging_handle", durationMs: 120, pointerMoves: 2, controllerMutations: 2, renderRequests: 3, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 },
        { sampleId: "run_04", mode: "dragging_handle", durationMs: 130, pointerMoves: 2, controllerMutations: 2, renderRequests: 4, hitTests: 2, snapResolutions: 3, handlePlans: 2, revisionDelta: 1 },
    ]
}

function runSummaryContract(reducer) {
    const summary = reducer.summarizeDistributions(sampleRecords())
    expect(summary.durationMs.count === 4, "summary should count samples")
    expect(summary.durationMs.p50 === 110, "summary should compute nearest-rank p50")
    expect(summary.durationMs.p95 === 130, "summary should compute nearest-rank p95")
    expect(summary.renderRequests.max === 4, "summary should compute max")
    expect(summary.durationMs.mean === 115, "summary should compute rounded mean")
}

function runReductionContract(reducer) {
    const report = reducer.reduceMetrics(sampleRecords(), {
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

    expect(report.ok, "matching samples should pass budgets")
    expect(report.samples === 4, "report should include sample count")
    expect(report.failures.length === 0, "passing report should not include failures")
    expect(report.summary.durationMs.p95 === 130, "report should include compact distributions")
    expect(report.records === undefined, "report should not carry raw records")
}

function runFailureContract(reducer) {
    const records = sampleRecords()
    records.push({ sampleId: "run_05", mode: "dragging_handle", durationMs: 140, pointerMoves: 2, controllerMutations: 7, renderRequests: 8, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 })
    const report = reducer.reduceMetrics(records, {
        mode: "dragging_handle",
        maxControllerMutations: 2,
        maxRenderRequests: 4,
    }, null, {
        maxFailures: 2,
    })

    expect(!report.ok, "budget violations should fail report")
    expect(report.failures.length === 2, "report should cap failure samples")
    expect(report.failures[0].sampleId === "run_05", "failure should retain sample id")
    expect(report.failures[0].field === "controllerMutations", "failure should identify field")
    expect(report.failures[0].actual === 7, "failure should include actual value")
}

function runOutlierContract(reducer) {
    const records = sampleRecords()
    records.push({ sampleId: "slow_run", mode: "dragging_handle", durationMs: 480, pointerMoves: 2, controllerMutations: 2, renderRequests: 3, hitTests: 1, snapResolutions: 2, handlePlans: 2, revisionDelta: 1 })
    const outliers = reducer.findOutliers(records, ["durationMs"], {
        outlierMedianMultiplier: 3,
        maxOutliers: 2,
    })

    expect(outliers.length === 1, "outlier detector should report slow run")
    expect(outliers[0].sampleId === "slow_run", "outlier should retain sample id")
    expect(outliers[0].field === "durationMs", "outlier should identify field")
}

function runBaselineDeltaContract(reducer) {
    const report = reducer.reduceMetrics(sampleRecords(), {}, {
        summary: {
            durationMs: { p95: 100 },
            renderRequests: { p95: 4 },
        },
    }, {
        baselineDeltaThreshold: 10,
    })

    expect(report.deltas.length === 1, "baseline comparison should only include meaningful regressions")
    expect(report.deltas[0].field === "durationMs", "delta should identify regressed field")
    expect(report.deltas[0].baseline === 100, "delta should include baseline")
    expect(report.deltas[0].current === 130, "delta should include current summary")
}

const reducer = loadReducerModule()
runSummaryContract(reducer)
runReductionContract(reducer)
runFailureContract(reducer)
runOutlierContract(reducer)
runBaselineDeltaContract(reducer)

if (process.exitCode) {
    process.exit(process.exitCode)
}
