#!/usr/bin/env node
const fs = require("fs")
const path = require("path")
const WorkflowHarness = require("./drawing_control_workflow_harness.js")
const WorkflowRunner = require("./drawing_control_workflow_runner.js")

function usage() {
    return [
        "Usage:",
        "  node tests/helpers/drawing_control_workflow_report.js --all",
        "  node tests/helpers/drawing_control_workflow_report.js --tag <tag>",
        "  node tests/helpers/drawing_control_workflow_report.js --category <category>",
        "  node tests/helpers/drawing_control_workflow_report.js --fixture <fixture.json>",
        "",
        "Selectors may be repeated or comma-separated."
    ].join("\n")
}

function pushValue(target, value) {
    const parts = String(value || "")
        .split(",")
        .map(part => part.trim())
        .filter(part => part.length > 0)
    target.push(...parts)
}

function parseArgs(argv) {
    const args = {
        all: false,
        fixtures: [],
        categories: [],
        tags: [],
    }
    for (let index = 0; index < argv.length; ++index) {
        const token = argv[index]
        if (token === "--help" || token === "-h") {
            args.help = true
        } else if (token === "--all") {
            args.all = true
        } else if (token === "--fixture") {
            pushValue(args.fixtures, argv[++index])
        } else if (token === "--category") {
            pushValue(args.categories, argv[++index])
        } else if (token === "--tag") {
            pushValue(args.tags, argv[++index])
        } else {
            throw new Error(`unknown argument: ${token}`)
        }
    }
    return args
}

function selectorEnv(args) {
    return {
        DRAWING_WORKFLOW_FIXTURE: args.fixtures.join(","),
        DRAWING_WORKFLOW_CATEGORY: args.categories.join(","),
        DRAWING_WORKFLOW_TAG: args.tags.join(","),
    }
}

function hasSelector(args) {
    return args.all || args.fixtures.length > 0 || args.categories.length > 0 || args.tags.length > 0
}

function firstScriptFailure(report) {
    const scripts = Array.isArray(report && report.scripts) ? report.scripts : []
    for (const script of scripts) {
        const failures = Array.isArray(script.failures) ? script.failures : []
        if (failures.length > 0) {
            return {
                script: script.fixture,
                message: failures[0],
            }
        }
    }
    return null
}

function compactOutput(repoRoot, report) {
    const reportPath = path.join(repoRoot, "tests", "artifacts", "drawing_metrics", "control_workflows_summary.json")
    const budgetFailures = report.metricBudgetChecks && Array.isArray(report.metricBudgetChecks.budgetFailuresByGroup)
        ? report.metricBudgetChecks.budgetFailuresByGroup
        : []
    return {
        ok: report.ok === true,
        selectedWorkflowCount: Number(report.selectedWorkflowCount || 0),
        metricSamples: Number(report.metrics && report.metrics.overall && report.metrics.overall.samples || 0),
        budgetFailures: Number(report.metricBudgetChecks && report.metricBudgetChecks.totalFailureCount || 0),
        worstFailure: budgetFailures.length > 0 ? budgetFailures[0] : firstScriptFailure(report),
        reportPath,
    }
}

function run() {
    const args = parseArgs(process.argv.slice(2))
    if (args.help) {
        console.log(usage())
        return 0
    }
    if (!hasSelector(args)) {
        throw new Error("one selector or --all is required")
    }

    const repoRoot = path.join(__dirname, "..", "..")
    const executable = path.join(repoRoot, "build", "qt_qml_region_split")
    if (!fs.existsSync(executable)) {
        console.log(JSON.stringify({
            ok: true,
            skipped: true,
            reason: "build/qt_qml_region_split is not built",
        }, null, 2))
        return 0
    }

    const manifest = WorkflowHarness.workflowFixtures(repoRoot, args.all ? {} : selectorEnv(args))
    if (manifest.selectedWorkflows.length <= 0) {
        throw new Error("workflow selectors did not match any fixtures")
    }

    const metricReducer = WorkflowHarness.loadMetricReducer(repoRoot)
    const budgetPolicy = WorkflowHarness.workflowBudgetPolicy(repoRoot)
    const scripts = WorkflowRunner.runWorkflows(repoRoot, manifest.selectedWorkflows, { executable })
    const report = WorkflowHarness.writeWorkflowReport(
        repoRoot,
        manifest.manifestPath,
        manifest.workflows,
        manifest.filters,
        scripts,
        metricReducer,
        budgetPolicy)
    const output = compactOutput(repoRoot, report)
    console.log(JSON.stringify(output, null, 2))
    return output.ok ? 0 : 1
}

try {
    process.exitCode = run()
} catch (error) {
    console.error(error && error.message ? error.message : String(error))
    console.error(usage())
    process.exitCode = 1
}
