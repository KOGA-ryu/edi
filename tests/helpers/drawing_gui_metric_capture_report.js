#!/usr/bin/env node
const fs = require("fs")
const path = require("path")
const vm = require("vm")

function usage() {
    return [
        "Usage:",
        "  node tests/helpers/drawing_gui_metric_capture_report.js --log <path> --name <name> --out <artifact-root> [--budget <path>] [--baseline <path>]",
        "",
        "The log should contain drawing_canvas_interaction_events [...] console lines."
    ].join("\n")
}

function parseArgs(argv) {
    const args = {
        name: "gui_metric_capture",
        out: path.join("tests", "artifacts", "drawing_metrics"),
    }
    for (let index = 0; index < argv.length; ++index) {
        const token = argv[index]
        if (token === "--help" || token === "-h") {
            args.help = true
        } else if (token === "--log") {
            args.log = argv[++index]
        } else if (token === "--name") {
            args.name = argv[++index]
        } else if (token === "--out") {
            args.out = argv[++index]
        } else if (token === "--budget") {
            args.budget = argv[++index]
        } else if (token === "--baseline") {
            args.baseline = argv[++index]
        } else {
            throw new Error(`unknown argument: ${token}`)
        }
    }
    return args
}

function loadModule(modulePath, context) {
    const source = fs.readFileSync(modulePath, "utf8")
        .replace(".pragma library", "")
        .replace(/^\.import .*$/gm, "")
    vm.runInContext(source, context, { filename: modulePath })
}

function runtimePath(name) {
    return path.join(__dirname, "..", "..", "src", "runtime", name)
}

function loadCaptureModule() {
    const context = { Math, Number, String, Array, Object, JSON }
    vm.createContext(context)

    loadModule(runtimePath("DrawingCanvasInteractionMetrics.js"), context)
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

    loadModule(runtimePath("DrawingCanvasInteractionScript.js"), context)
    context.CanvasInteractionScript = {
        validateFixture: context.validateFixture,
        evaluateFixture: context.evaluateFixture,
        evaluateFixtures: context.evaluateFixtures,
    }

    loadModule(runtimePath("DrawingCanvasInteractionReplay.js"), context)
    context.CanvasInteractionReplay = {
        replayEvents: context.replayEvents,
    }

    loadModule(runtimePath("DrawingCanvasMetricReducer.js"), context)
    context.CanvasMetricReducer = {
        reduceMetrics: context.reduceMetrics,
    }

    loadModule(runtimePath("DrawingCanvasMetricReport.js"), context)
    context.CanvasMetricReport = {
        buildMetricReport: context.buildMetricReport,
        rawRecordsJsonl: context.rawRecordsJsonl,
        parseRawRecordsJsonl: context.parseRawRecordsJsonl,
        summaryReportJson: context.summaryReportJson,
        artifactPaths: context.artifactPaths,
    }

    loadModule(runtimePath("DrawingCanvasGuiMetricCapture.js"), context)
    return context
}

function readJsonIfPresent(filePath) {
    if (!filePath) {
        return null
    }
    return JSON.parse(fs.readFileSync(filePath, "utf8"))
}

function run() {
    const args = parseArgs(process.argv.slice(2))
    if (args.help) {
        console.log(usage())
        return 0
    }
    if (!args.log) {
        throw new Error("--log is required")
    }

    const capture = loadCaptureModule()
    const logText = fs.readFileSync(args.log, "utf8")
    const captured = capture.capturedMetricRecords(logText)
    const report = capture.buildCaptureReport(
        args.name,
        logText,
        readJsonIfPresent(args.budget),
        readJsonIfPresent(args.baseline))
    const paths = capture.CanvasMetricReport.artifactPaths(args.out, args.name)

    fs.mkdirSync(paths.rawDir, { recursive: true })
    fs.mkdirSync(paths.reportsDir, { recursive: true })
    fs.writeFileSync(paths.rawJsonl, capture.CanvasMetricReport.rawRecordsJsonl(captured.records))
    fs.writeFileSync(paths.summaryJson, capture.CanvasMetricReport.summaryReportJson(report))

    console.log(JSON.stringify({
        ok: report.ok,
        samples: report.samples,
        failures: report.failures,
        outliers: report.outliers,
        deltas: report.deltas,
        rawJsonl: paths.rawJsonl,
        summaryJson: paths.summaryJson
    }, null, 2))
    return report.ok ? 0 : 1
}

try {
    process.exitCode = run()
} catch (error) {
    console.error(error && error.message ? error.message : String(error))
    console.error(usage())
    process.exitCode = 1
}
