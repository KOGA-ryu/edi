#!/usr/bin/env node
const fs = require("fs")
const path = require("path")
const WorkflowHarness = require("./drawing_control_workflow_harness.js")
const WorkflowRunner = require("./drawing_control_workflow_runner.js")

function usage() {
    return [
        "Usage:",
        "  node tests/helpers/drawing_control_workflow_report.js --recommend",
        "  node tests/helpers/drawing_control_workflow_report.js --recommend --id <selector>",
        "  node tests/helpers/drawing_control_workflow_report.js --all",
        "  node tests/helpers/drawing_control_workflow_report.js --tag <tag>",
        "  node tests/helpers/drawing_control_workflow_report.js --category <category>",
        "  node tests/helpers/drawing_control_workflow_report.js --fixture <fixture.json>",
        "  node tests/helpers/drawing_control_workflow_report.js --tag line --dry-run",
        "  node tests/helpers/drawing_control_workflow_report.js --tag line --dry-run --compact",
        "  node tests/helpers/drawing_control_workflow_report.js --tag line --compare-baseline",
        "  node tests/helpers/drawing_control_workflow_report.js --tag line --compare-baseline --subsystem rendering",
        "  node tests/helpers/drawing_control_workflow_report.js --tag line --compare-baseline --failures-only",
        "  node tests/helpers/drawing_control_workflow_report.js --all --update-baseline",
        "",
        "Selectors may be repeated or comma-separated. --dry-run prints selected workflows without launching the app; --compact omits workflow lists.",
        "--compare-baseline reports compact metric deltas against workflow_metric_baselines.json.",
        "--update-baseline refreshes selected workflow baselines after a known-good run."
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
        dryRun: false,
        compact: false,
        recommend: false,
        recommendationId: "",
        compareBaseline: false,
        updateBaseline: false,
        subsystem: "",
        failuresOnly: false,
    }
    for (let index = 0; index < argv.length; ++index) {
        const token = argv[index]
        if (token === "--help" || token === "-h") {
            args.help = true
        } else if (token === "--recommend") {
            args.recommend = true
        } else if (token === "--id") {
            args.recommendationId = String(argv[++index] || "")
        } else if (token === "--all") {
            args.all = true
        } else if (token === "--dry-run") {
            args.dryRun = true
        } else if (token === "--compact") {
            args.compact = true
        } else if (token === "--compare-baseline") {
            args.compareBaseline = true
        } else if (token === "--update-baseline") {
            args.updateBaseline = true
        } else if (token === "--subsystem") {
            args.subsystem = String(argv[++index] || "")
        } else if (token === "--failures-only") {
            args.failuresOnly = true
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

function compactBaselineUpdate(repoRoot, report) {
    const existing = WorkflowHarness.workflowMetricBaselines(repoRoot)
    const current = WorkflowHarness.workflowBaselineFromReport(report, existing.policy)
    const merged = WorkflowHarness.mergeWorkflowBaselines(existing, current)
    const baselinePath = WorkflowHarness.writeWorkflowBaselines(repoRoot, merged)
    return {
        ok: true,
        baselinePath,
        updatedWorkflowCount: Object.keys(current.workflows || {}).length,
        baselineWorkflowCount: Object.keys(merged.workflows || {}).length,
    }
}

function filterBaselineComparison(comparison, subsystem) {
    const selectedSubsystem = String(subsystem || "")
    if (selectedSubsystem.length <= 0) {
        return comparison
    }
    const topDeltas = Array.isArray(comparison && comparison.topDeltas) ? comparison.topDeltas : []
    const selectedDeltas = topDeltas.filter(delta => String(delta && delta.subsystem || "") === selectedSubsystem)
    return Object.assign({}, comparison, {
        selectedSubsystem,
        selectedSubsystemDeltaCount: selectedDeltas.length,
        topDeltas: selectedDeltas,
    })
}

function compactBaselineComparison(repoRoot, report, subsystem) {
    const baseline = WorkflowHarness.workflowMetricBaselines(repoRoot)
    const comparison = WorkflowHarness.compareWorkflowBaseline(report, baseline)
    return Object.assign({
        baselinePath: baseline.path,
    }, filterBaselineComparison(comparison, subsystem))
}

function failuresOnlyBaselineComparison(comparison) {
    const hasDeltas = Number(comparison && comparison.failureCount || 0) > 0
        || Number(comparison && comparison.warningCount || 0) > 0
    const result = {
        ok: comparison && comparison.ok === true,
        failureCount: Number(comparison && comparison.failureCount || 0),
        warningCount: Number(comparison && comparison.warningCount || 0),
    }
    if (comparison && comparison.selectedSubsystem !== undefined) {
        result.selectedSubsystem = comparison.selectedSubsystem
        result.selectedSubsystemDeltaCount = Number(comparison.selectedSubsystemDeltaCount || 0)
    }
    if (hasDeltas) {
        result.bySubsystem = comparison.bySubsystem || {}
        result.topDeltas = Array.isArray(comparison.topDeltas) ? comparison.topDeltas : []
    }
    return result
}

function failuresOnlyOutput(output) {
    return {
        ok: output.ok === true,
        baselineComparison: failuresOnlyBaselineComparison(output.baselineComparison || {}),
    }
}

function dryRunOutput(manifest, compact) {
    const output = {
        ok: true,
        dryRun: true,
        selectedWorkflowCount: manifest.selectedWorkflows.length,
        totalWorkflowCount: manifest.workflows.length,
        filters: manifest.filters,
        coverage: WorkflowHarness.workflowCoverage(manifest.selectedWorkflows),
    }
    if (!compact) {
        output.workflows = manifest.selectedWorkflows.map(workflow => ({
            fixture: workflow.fixture,
            kind: workflow.kind,
            category: workflow.category,
            tags: workflow.tags,
        }))
    }
    return output
}

function run() {
    const args = parseArgs(process.argv.slice(2))
    if (args.help) {
        console.log(usage())
        return 0
    }
    const repoRoot = path.join(__dirname, "..", "..")
    if (args.recommend) {
        const output = WorkflowHarness.recommendedSelectorOutput(WorkflowHarness.workflowCoverageExpectations(repoRoot), args.recommendationId)
        console.log(JSON.stringify(output, null, 2))
        return output.ok ? 0 : 1
    }
    if (!hasSelector(args)) {
        throw new Error("one selector or --all is required")
    }
    if (args.subsystem.length > 0 && !args.compareBaseline) {
        throw new Error("--subsystem requires --compare-baseline")
    }
    if (args.failuresOnly && !args.compareBaseline) {
        throw new Error("--failures-only requires --compare-baseline")
    }

    const manifest = WorkflowHarness.workflowFixtures(repoRoot, args.all ? {} : selectorEnv(args))
    if (manifest.selectedWorkflows.length <= 0) {
        throw new Error("workflow selectors did not match any fixtures")
    }
    if (args.dryRun) {
        if (args.compareBaseline || args.updateBaseline) {
            throw new Error("--compare-baseline and --update-baseline require a real workflow run")
        }
        if (args.subsystem.length > 0) {
            throw new Error("--subsystem requires --compare-baseline")
        }
        if (args.failuresOnly) {
            throw new Error("--failures-only requires --compare-baseline")
        }
        console.log(JSON.stringify(dryRunOutput(manifest, args.compact), null, 2))
        return 0
    }

    const executable = path.join(repoRoot, "build", "qt_qml_region_split")
    if (!fs.existsSync(executable)) {
        console.log(JSON.stringify({
            ok: true,
            skipped: true,
            reason: "build/qt_qml_region_split is not built",
        }, null, 2))
        return 0
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
    if (args.updateBaseline) {
        output.baselineUpdate = compactBaselineUpdate(repoRoot, report)
    }
    if (args.compareBaseline) {
        output.baselineComparison = compactBaselineComparison(repoRoot, report, args.subsystem)
        output.ok = output.ok && output.baselineComparison.ok
    }
    if (args.failuresOnly) {
        console.log(JSON.stringify(failuresOnlyOutput(output), null, 2))
        return output.ok ? 0 : 1
    }
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
