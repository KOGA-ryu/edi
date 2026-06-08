const fs = require("fs")
const path = require("path")
const WorkflowHarness = require("./helpers/drawing_control_workflow_harness.js")
const WorkflowRunner = require("./helpers/drawing_control_workflow_runner.js")

function fail(message) {
    console.error(`FAIL: ${message}`)
    process.exitCode = 1
}

function expect(condition, message) {
    if (!condition) {
        fail(message)
    }
}

function finite(value) {
    return Number.isFinite(Number(value))
}

function runWorkflowFilterContract(manifest) {
    expect(manifest.workflows.length >= 13, "workflow manifest should include all control workflows")
    expect(manifest.workflows.every(workflow => workflow.kind.length > 0 && workflow.category.length > 0 && workflow.tags.length > 0),
        "workflow manifest entries should include kind, category, and tags")

    const lineWorkflows = WorkflowHarness.selectWorkflows(manifest.workflows, {
        fixtures: [],
        categories: [],
        tags: ["line"],
    })
    expect(lineWorkflows.length > 0, "line tag filter should select workflows")
    expect(lineWorkflows.length < manifest.workflows.length, "line tag filter should not select every workflow")
    expect(lineWorkflows.every(workflow => workflow.tags.indexOf("line") >= 0), "line tag filter should only select line-tagged workflows")

    const editWorkflows = WorkflowHarness.selectWorkflows(manifest.workflows, {
        fixtures: [],
        categories: ["edit"],
        tags: [],
    })
    expect(editWorkflows.length > 0, "edit category filter should select workflows")
    expect(editWorkflows.every(workflow => workflow.category === "edit"), "edit category filter should only select edit workflows")

    const arcWorkflows = WorkflowHarness.selectWorkflows(manifest.workflows, {
        fixtures: ["arc_create_basic.json"],
        categories: [],
        tags: [],
    })
    expect(arcWorkflows.length === 1, "fixture filter should select one named workflow")
    expect(arcWorkflows[0].fixture === "arc_create_basic.json", "fixture filter should preserve the selected fixture")
}

function runWorkflowReportContract(report) {
    expect(report.schemaVersion === 4, "workflow report should use schema version 4")
    expect(report.ok === true, "workflow report should pass script and group metric budgets")
    expect(report.metrics.overall.samples > 0, "workflow report should include metric samples")
    expect(finite(report.metrics.overall.fields.durationMs.p95), "workflow report should include duration p95")
    expect(finite(report.metrics.overall.fields.renderRequests.max), "workflow report should include render request max")
    expect(Object.keys(report.metrics.byMode).length > 0, "workflow report should group metrics by mode")
    expect(Object.keys(report.metrics.byKind).length > 0, "workflow report should group metrics by kind")
    expect(Object.keys(report.metrics.byCategory).length > 0, "workflow report should group metrics by category")
    expect(report.metricBudgetChecks.policy.budgetCount > 0, "workflow report should include metric budget policy")
    expect(report.metricBudgetChecks.ok === true, "workflow metric budget checks should pass")
    expect(report.metricBudgetChecks.budgetFailuresByGroup.length === 0, "passing workflow metric budgets should not report group failures")
}

function runWorkflowBudgetContract(metricReducer) {
    const records = [
        {
            sampleId: "slow_drag",
            script: "line_drag_end_handle.json",
            kind: "line",
            category: "edit",
            tags: ["line", "handle"],
            mode: "dragging_handle",
            durationMs: 10,
            pointerMoves: 2,
            controllerMutations: 2,
            renderRequests: 6,
            hitTests: 0,
            snapResolutions: 0,
            handlePlans: 0,
            revisionDelta: 2,
            mutationsPerPointerMove: 1,
            renderRequestsPerPointerMove: 3,
            hitTestsPerPointerMove: 0,
            snapResolutionsPerPointerMove: 0,
            handlePlansPerPointerMove: 0,
        },
    ]
    const checks = WorkflowHarness.evaluateWorkflowMetricBudgets(records, {
        schemaVersion: 1,
        path: path.join(__dirname, "synthetic_workflow_metric_budgets.json"),
        policy: { maxFailures: 1 },
        budgets: [
            {
                id: "synthetic_drag_budget",
                match: { kind: "line", category: "edit", mode: "dragging_handle" },
                limits: { maxRenderRequestsPerPointerMove: 1.25 },
            },
        ],
    }, metricReducer, repoRoot)

    expect(!checks.ok, "synthetic expensive workflow should fail group budget")
    expect(checks.budgetFailuresByGroup.length === 1, "group budget failures should be capped and compact")
    expect(checks.budgetFailuresByGroup[0].metric === "renderRequestsPerPointerMove", "group budget should identify failing metric")
}

const repoRoot = path.join(__dirname, "..")
const executable = path.join(repoRoot, "build", "qt_qml_region_split")

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

const scripts = []
const manifest = WorkflowHarness.workflowFixtures(repoRoot, process.env)
const metricReducer = WorkflowHarness.loadMetricReducer(repoRoot)
const budgetPolicy = WorkflowHarness.workflowBudgetPolicy(repoRoot)
runWorkflowFilterContract(manifest)
runWorkflowBudgetContract(metricReducer)
expect(manifest.selectedWorkflows.length > 0, "workflow filters should select at least one fixture")
for (const workflow of manifest.selectedWorkflows) {
    scripts.push(WorkflowRunner.runFixture(repoRoot, workflow))
}

const report = WorkflowHarness.writeWorkflowReport(repoRoot, manifest.manifestPath, manifest.workflows, manifest.filters, scripts, metricReducer, budgetPolicy)
runWorkflowReportContract(report)

if (process.exitCode) {
    process.exit(process.exitCode)
}
