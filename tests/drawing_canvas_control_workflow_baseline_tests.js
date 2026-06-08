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

function clone(value) {
    return JSON.parse(JSON.stringify(value))
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
    expect(comparison.topDeltas[0].kind === "missing_baseline", "missing workflow failure should classify missing baseline")
    expect(comparison.topDeltas[0].subsystem === "baseline", "missing workflow failure should route to baseline subsystem")
    expect(String(comparison.topDeltas[0].recommendation || "").length > 0, "missing workflow failure should include recommendation")
    expect(comparison.bySubsystem.baseline.count === 1, "missing workflow failure should count baseline subsystem")
}

function runInvariantDeltaContract() {
    const baseline = WorkflowHarness.workflowBaselineFromReport(syntheticReport())
    const comparison = WorkflowHarness.compareWorkflowBaseline(syntheticReport({
        record: {
            renderRequests: 9,
        },
    }), baseline)
    expect(comparison.ok === false, "changed render request max should fail baseline comparison")
    const delta = comparison.topDeltas.find(item => item.path === "modes.dragging_handle.fields.renderRequests.max")
    expect(!!delta,
        "metric max delta should identify exact metric path")
    expect(delta.kind === "metric_regressed", "metric max delta should classify metric regression")
    expect(delta.subsystem === "rendering", "render metric delta should route to rendering subsystem")
    expect(delta.recommendation.indexOf("rendering") >= 0, "render metric delta should recommend rendering inspection")
    expect(comparison.bySubsystem.rendering.topDelta.path === "modes.dragging_handle.fields.renderRequests.max",
        "render metric delta should appear in rendering subsystem summary")
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
    const delta = regression.topDeltas.find(item => item.path === "modes.dragging_handle.fields.durationMs.max")
    expect(!!delta,
        "duration regression should identify duration max path")
    expect(delta.kind === "duration_regressed", "duration delta should classify duration regression")
    expect(delta.subsystem === "workflow_fixture", "duration delta should route to workflow fixture subsystem")
}

function runSummaryDeltaContract() {
    const baseline = WorkflowHarness.workflowBaselineFromReport(syntheticReport())
    const comparison = WorkflowHarness.compareWorkflowBaseline(syntheticReport({
        summary: {
            objectCount: 2,
        },
    }), baseline)
    const delta = comparison.topDeltas.find(item => item.path === "summary.objectCount")
    expect(comparison.ok === false, "summary changes should fail baseline comparison")
    expect(!!delta, "summary delta should identify exact summary path")
    expect(delta.kind === "summary_changed", "summary delta should classify summary change")
    expect(delta.subsystem === "workflow_fixture", "summary delta should route to workflow fixture subsystem")
}

function runModeDeltaContract() {
    const report = syntheticReport()
    const baseline = WorkflowHarness.workflowBaselineFromReport(report)
    const addedModeReport = clone(report)
    addedModeReport.scripts[0].modes.dragging_object = {
        records: [
            {
                durationMs: 1,
                pointerMoves: 1,
                controllerMutations: 1,
                renderRequests: 1,
                hitTests: 0,
                snapResolutions: 0,
                handlePlans: 0,
                revisionDelta: 1,
            },
        ],
    }
    const added = WorkflowHarness.compareWorkflowBaseline(addedModeReport, baseline)
    const addedDelta = added.topDeltas.find(item => item.path === "modes.dragging_object")
    expect(added.ok === false, "added mode should fail baseline comparison")
    expect(!!addedDelta, "added mode should identify mode path")
    expect(addedDelta.kind === "mode_added", "added mode should classify mode addition")
    expect(addedDelta.subsystem === "gesture", "added mode should route to gesture subsystem")
    expect(added.bySubsystem.gesture.count === 1, "added mode should count gesture subsystem")

    const missingBaseline = clone(baseline)
    missingBaseline.workflows["line_drag_end_handle.json"].modes.dragging_object = {
        samples: 1,
        fields: {},
    }
    const missing = WorkflowHarness.compareWorkflowBaseline(report, missingBaseline)
    const missingDelta = missing.topDeltas.find(item => item.path === "modes.dragging_object")
    expect(missing.ok === false, "missing mode should fail baseline comparison")
    expect(!!missingDelta, "missing mode should identify mode path")
    expect(missingDelta.kind === "mode_missing", "missing mode should classify mode removal")
    expect(missingDelta.subsystem === "gesture", "missing mode should route to gesture subsystem")
}

function runMetricSchemaDeltaContract() {
    const report = syntheticReport()
    const baseline = WorkflowHarness.workflowBaselineFromReport(report)
    const metricAddedBaseline = clone(baseline)
    delete metricAddedBaseline.workflows["line_drag_end_handle.json"].modes.dragging_handle.fields.hitTests
    const added = WorkflowHarness.compareWorkflowBaseline(report, metricAddedBaseline)
    const addedDelta = added.topDeltas.find(item => item.path === "modes.dragging_handle.fields.hitTests")
    expect(added.ok === false, "added metric field should fail baseline comparison")
    expect(!!addedDelta, "added metric should identify field path")
    expect(addedDelta.kind === "metric_added", "added metric should classify metric addition")
    expect(addedDelta.subsystem === "metrics", "added metric should route to metrics subsystem")

    const metricMissingBaseline = clone(baseline)
    metricMissingBaseline.workflows["line_drag_end_handle.json"].modes.dragging_handle.fields.extraMetric = {
        max: 1,
        mean: 1,
    }
    const missing = WorkflowHarness.compareWorkflowBaseline(report, metricMissingBaseline)
    const missingDelta = missing.topDeltas.find(item => item.path === "modes.dragging_handle.fields.extraMetric")
    expect(missing.ok === false, "missing metric field should fail baseline comparison")
    expect(!!missingDelta, "missing metric should identify field path")
    expect(missingDelta.kind === "metric_missing", "missing metric should classify metric removal")
    expect(missingDelta.subsystem === "metrics", "missing metric should route to metrics subsystem")
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
runSummaryDeltaContract()
runModeDeltaContract()
runMetricSchemaDeltaContract()
runMergeContract()

if (process.exitCode) {
    process.exit(process.exitCode)
}
