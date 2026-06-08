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

function runCoverageContract(repoRoot) {
    const manifest = WorkflowHarness.workflowFixtures(repoRoot, {})
    const expectations = WorkflowHarness.workflowCoverageExpectations(repoRoot)
    const coverage = WorkflowHarness.workflowCoverage(manifest.workflows)
    const result = WorkflowHarness.evaluateWorkflowCoverage(coverage, expectations)

    expect(manifest.workflows.length === 13, "workflow manifest should expose the expected workflow count")
    expect(coverage.byKind.line === 4, "coverage should count line workflows")
    expect(coverage.byCategory.create === 6, "coverage should count create workflows")
    expect(coverage.byTag.geometry === 11, "coverage should count geometry-tagged workflows")
    expect(result.ok, `workflow coverage expectations should pass: ${result.failures.map(failure => failure.message).join(", ")}`)
}

function runFailureContract(repoRoot) {
    const manifest = WorkflowHarness.workflowFixtures(repoRoot, {})
    const coverage = WorkflowHarness.workflowCoverage(manifest.workflows)
    const result = WorkflowHarness.evaluateWorkflowCoverage(coverage, {
        schemaVersion: 1,
        minimums: {
            kinds: {
                line: 999,
            },
        },
    })

    expect(!result.ok, "coverage evaluator should fail missing minimums")
    expect(result.failureCount === 1, "coverage evaluator should report one synthetic failure")
    expect(result.failures[0].group === "kinds", "coverage failure should identify group")
    expect(result.failures[0].key === "line", "coverage failure should identify key")
}

const repoRoot = path.join(__dirname, "..")
runCoverageContract(repoRoot)
runFailureContract(repoRoot)

if (process.exitCode) {
    process.exit(process.exitCode)
}
