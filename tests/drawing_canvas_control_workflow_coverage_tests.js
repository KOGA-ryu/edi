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

function runSelectorCoverageContract(repoRoot) {
    const expectations = WorkflowHarness.workflowCoverageExpectations(repoRoot)
    const selectors = Array.isArray(expectations.selectors) ? expectations.selectors : []
    expect(selectors.length >= 3, "workflow coverage expectations should include selector contracts")

    for (const selector of selectors) {
        const manifest = WorkflowHarness.workflowFixtures(repoRoot, selector.env || {})
        const coverage = WorkflowHarness.workflowCoverage(manifest.selectedWorkflows)
        const result = WorkflowHarness.evaluateWorkflowCoverage(coverage, {
            schemaVersion: expectations.schemaVersion,
            minimums: selector.minimums || {},
        })
        expect(manifest.selectedWorkflows.length > 0, `${selector.id} should select workflows`)
        expect(result.ok, `${selector.id} coverage expectations should pass: ${result.failures.map(failure => failure.message).join(", ")}`)
    }
}

function selectorEnvFromCommand(command) {
    const result = {}
    const tokens = String(command || "").split(/\s+/).filter(token => token.length > 0)
    for (let index = 0; index < tokens.length; ++index) {
        if (tokens[index] === "--tag") {
            result.DRAWING_WORKFLOW_TAG = tokens[++index] || ""
        } else if (tokens[index] === "--category") {
            result.DRAWING_WORKFLOW_CATEGORY = tokens[++index] || ""
        } else if (tokens[index] === "--fixture") {
            result.DRAWING_WORKFLOW_FIXTURE = tokens[++index] || ""
        }
    }
    return result
}

function runRecommendedSelectorContract(repoRoot) {
    const expectations = WorkflowHarness.workflowCoverageExpectations(repoRoot)
    const selectors = Array.isArray(expectations.selectors) ? expectations.selectors : []
    const selectorIds = new Set(selectors.map(selector => String(selector.id || "")))
    const recommendedSelectors = Array.isArray(expectations.recommendedSelectors) ? expectations.recommendedSelectors : []
    const output = WorkflowHarness.recommendedSelectorOutput(expectations)
    const lineOutput = WorkflowHarness.recommendedSelectorOutput(expectations, "line_system")
    const missingOutput = WorkflowHarness.recommendedSelectorOutput(expectations, "missing_selector")
    expect(recommendedSelectors.length >= 3, "workflow coverage expectations should include recommended selectors")
    expect(output.ok === true, "recommended selector output should report ok")
    expect(output.recommendedSelectors.length === recommendedSelectors.length, "recommended selector output should preserve recommendation count")
    expect(output.recommendedSelectors.every(selector => selector.selectorId === undefined), "recommended selector output should omit internal selector ids")
    expect(lineOutput.ok === true, "recommended selector output should filter by id")
    expect(lineOutput.recommendedSelectors.length === 1, "recommended selector id filter should return one selector")
    expect(lineOutput.recommendedSelectors[0].id === "line_system", "recommended selector id filter should return matching selector")
    expect(missingOutput.ok === false, "recommended selector output should fail missing id")
    expect(missingOutput.recommendedSelectors.length === 0, "missing recommendation id should return no selectors")

    for (const recommendation of recommendedSelectors) {
        const id = String(recommendation.id || "")
        const description = String(recommendation.description || "")
        const command = String(recommendation.command || "")
        const runCommand = String(recommendation.runCommand || "")
        const baselineCommand = String(recommendation.baselineCommand || "")
        expect(id.length > 0, "recommended selector should include id")
        expect(description.length > 0, `${id} should include description`)
        expect(command.length > 0, `${id} should include command`)
        expect(runCommand.length > 0, `${id} should include runCommand`)
        expect(baselineCommand.length > 0, `${id} should include baselineCommand`)
        expect(command.indexOf("--dry-run") >= 0, `${id} command should use dry-run`)
        expect(command.indexOf("--compact") >= 0, `${id} command should use compact output`)
        expect(runCommand.indexOf("--dry-run") < 0, `${id} runCommand should execute real metrics`)
        expect(runCommand.indexOf("--compact") < 0, `${id} runCommand should not use compact dry-run`)
        expect(baselineCommand.indexOf("--compare-baseline") >= 0, `${id} baselineCommand should compare baselines`)
        expect(baselineCommand.indexOf("--dry-run") < 0, `${id} baselineCommand should execute real metrics`)
        expect(baselineCommand.indexOf("--compact") < 0, `${id} baselineCommand should not use compact dry-run`)

        const selectorId = String(recommendation.selectorId || "")
        if (selectorId.length > 0) {
            expect(selectorIds.has(selectorId), `${id} selectorId should reference a selector coverage contract`)
        } else {
            const manifest = WorkflowHarness.workflowFixtures(repoRoot, selectorEnvFromCommand(command))
            expect(manifest.selectedWorkflows.length > 0, `${id} command should select workflows`)
        }
    }
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
runSelectorCoverageContract(repoRoot)
runRecommendedSelectorContract(repoRoot)
runFailureContract(repoRoot)

if (process.exitCode) {
    process.exit(process.exitCode)
}
