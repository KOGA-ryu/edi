const path = require("path")
const WorkflowHarness = require("./helpers/drawing_control_workflow_harness.js")

function fail(message) {
    console.error(`FAIL: ${message}`)
    process.exitCode = 1
}

function expect(condition, message) {
    if (!condition) {
        fail(message)
    }
}

function syntheticReport(overrides) {
    const record = Object.assign({
        durationMs: 10,
        pointerMoves: 4,
        controllerMutations: 4,
        renderRequests: 5,
        hitTests: 1,
        snapResolutions: 4,
        handlePlans: 0,
        revisionDelta: 4,
    }, overrides && overrides.record ? overrides.record : {})
    const summary = Object.assign({
        executed: 3,
        objectCount: 1,
        selectedCount: 1,
    }, overrides && overrides.summary ? overrides.summary : {})
    const fixture = overrides && overrides.fixture ? overrides.fixture : "line_drag_end_handle.json"
    return {
        schemaVersion: 4,
        selectedWorkflowCount: 1,
        metrics: {
            overall: {
                samples: 1,
            },
        },
        scripts: [
            {
                fixture,
                metadata: {
                    kind: "line",
                    category: "edit",
                    tags: ["line", "handle"],
                },
                summary,
                modes: {
                    dragging_handle: {
                        records: [record],
                    },
                },
            },
        ],
    }
}

function runBaselineProjectionContract() {
    const baseline = WorkflowHarness.workflowBaselineFromReport(syntheticReport())
    const workflow = baseline.workflows["line_drag_end_handle.json"]
    expect(baseline.schemaVersion === 1, "baseline projection should use schema version 1")
    expect(baseline.workflowCount === 1, "baseline projection should count workflows")
    expect(workflow.kind === "line", "baseline should preserve workflow kind")
    expect(workflow.category === "edit", "baseline should preserve workflow category")
    expect(workflow.summary.objectCount === 1, "baseline should preserve object count")
    expect(workflow.modes.dragging_handle.samples === 1, "baseline should preserve mode sample count")
    expect(workflow.modes.dragging_handle.fields.renderRequests.max === 5, "baseline should preserve metric max")
    expect(workflow.modes.dragging_handle.fields.renderRequestsPerPointerMove.max === 1.25, "baseline should derive ratio metrics")
}

function runBaselineComparisonContract() {
    const report = syntheticReport()
    const baseline = WorkflowHarness.workflowBaselineFromReport(report)
    const comparison = WorkflowHarness.compareWorkflowBaseline(report, baseline)
    expect(comparison.ok === true, "identical workflow baseline comparison should pass")
    expect(comparison.comparedWorkflowCount === 1, "comparison should count selected workflows")
    expect(comparison.failureCount === 0, "passing comparison should have no failures")
}

function runMissingBaselineContract() {
    const comparison = WorkflowHarness.compareWorkflowBaseline(syntheticReport(), {
        schemaVersion: 1,
        policy: WorkflowHarness.defaultWorkflowBaselinePolicy(),
        workflows: {},
    })
    expect(comparison.ok === false, "missing workflow baseline should fail")
    expect(comparison.topDeltas[0].path === "workflow", "missing workflow failure should identify workflow path")
}

function runInvariantDeltaContract() {
    const baseline = WorkflowHarness.workflowBaselineFromReport(syntheticReport())
    const comparison = WorkflowHarness.compareWorkflowBaseline(syntheticReport({
        record: {
            renderRequests: 9,
        },
    }), baseline)
    expect(comparison.ok === false, "changed render request max should fail baseline comparison")
    expect(comparison.topDeltas.some(delta => delta.path === "modes.dragging_handle.fields.renderRequests.max"),
        "metric max delta should identify exact metric path")
}

function runDurationThresholdContract() {
    const baseline = WorkflowHarness.workflowBaselineFromReport(syntheticReport())
    const smallJitter = WorkflowHarness.compareWorkflowBaseline(syntheticReport({
        record: {
            durationMs: 35,
        },
    }), baseline)
    expect(smallJitter.ok === true, "small duration jitter should not fail baseline comparison")

    const regression = WorkflowHarness.compareWorkflowBaseline(syntheticReport({
        record: {
            durationMs: 200,
        },
    }), baseline)
    expect(regression.ok === false, "large duration regression should fail baseline comparison")
    expect(regression.topDeltas.some(delta => delta.path === "modes.dragging_handle.fields.durationMs.max"),
        "duration regression should identify duration max path")
}

function runMergeContract() {
    const first = WorkflowHarness.workflowBaselineFromReport(syntheticReport())
    const second = WorkflowHarness.workflowBaselineFromReport(syntheticReport({
        fixture: "point_drag_handle.json",
    }))
    const merged = WorkflowHarness.mergeWorkflowBaselines(first, second)
    expect(merged.workflowCount === 2, "baseline merge should preserve existing workflows")
    expect(!!merged.workflows["line_drag_end_handle.json"], "baseline merge should keep old workflow")
    expect(!!merged.workflows["point_drag_handle.json"], "baseline merge should add new workflow")
}

function runPolicyMergeContract() {
    const policy = WorkflowHarness.workflowBaselinePolicy({
        duration: {
            maxAbsoluteRegressionMs: 12,
        },
    })
    expect(policy.duration.maxAbsoluteRegressionMs === 12, "policy merge should apply duration override")
    expect(policy.duration.maxRegressionRatio === 2.5, "policy merge should preserve nested duration defaults")
    expect(policy.exactMetricMaxFields.indexOf("renderRequests") >= 0, "policy merge should preserve exact metric defaults")
}

const repoRoot = path.join(__dirname, "..")
expect(WorkflowHarness.workflowBaselinePath(repoRoot).endsWith("workflow_metric_baselines.json"),
    "baseline path should target workflow fixture contract")
runPolicyMergeContract()
runBaselineProjectionContract()
runBaselineComparisonContract()
runMissingBaselineContract()
runInvariantDeltaContract()
runDurationThresholdContract()
runMergeContract()

if (process.exitCode) {
    process.exit(process.exitCode)
}
