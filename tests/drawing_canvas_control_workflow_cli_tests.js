const fs = require("fs")
const path = require("path")
const { spawnSync } = require("child_process")

function fail(message) {
    console.error(`FAIL: ${message}`)
    process.exitCode = 1
}

function expect(condition, message) {
    if (!condition) {
        fail(message)
    }
}

const repoRoot = path.join(__dirname, "..")
const executable = path.join(repoRoot, "build", "qt_qml_region_split")
const helper = path.join(__dirname, "helpers", "drawing_control_workflow_report.js")

const dryRun = spawnSync(process.execPath, [
    helper,
    "--tag", "line",
    "--dry-run",
], {
    cwd: repoRoot,
    encoding: "utf8",
    timeout: 10000,
})

if (dryRun.status !== 0) {
    console.error(dryRun.stdout)
    console.error(dryRun.stderr)
}
expect(dryRun.status === 0, "workflow report CLI dry-run should pass without launching app")

const dryRunOutput = JSON.parse(dryRun.stdout)
expect(dryRunOutput.ok === true, "dry-run output should report ok")
expect(dryRunOutput.dryRun === true, "dry-run output should identify dry-run mode")
expect(dryRunOutput.selectedWorkflowCount === 4, "dry-run should select line workflows")
expect(dryRunOutput.totalWorkflowCount >= dryRunOutput.selectedWorkflowCount, "dry-run should include total workflow count")
expect(dryRunOutput.filters.tags.indexOf("line") >= 0, "dry-run should preserve tag filter")
expect(dryRunOutput.coverage.byKind.line === 4, "dry-run should count selected workflows by kind")
expect(dryRunOutput.coverage.byCategory.create === 1, "dry-run should count selected workflows by category")
expect(dryRunOutput.coverage.byCategory.edit === 1, "dry-run should include edit category coverage")
expect(dryRunOutput.coverage.byTag.line === 4, "dry-run should count selected workflows by tag")
expect(dryRunOutput.coverage.byTag.geometry === 3, "dry-run should count non-selector tags")
expect(dryRunOutput.workflows.every(workflow => workflow.tags.indexOf("line") >= 0), "dry-run should list selected workflows only")

const compactDryRun = spawnSync(process.execPath, [
    helper,
    "--tag", "line",
    "--dry-run",
    "--compact",
], {
    cwd: repoRoot,
    encoding: "utf8",
    timeout: 10000,
})

if (compactDryRun.status !== 0) {
    console.error(compactDryRun.stdout)
    console.error(compactDryRun.stderr)
}
expect(compactDryRun.status === 0, "workflow report CLI compact dry-run should pass")

const compactDryRunOutput = JSON.parse(compactDryRun.stdout)
expect(compactDryRunOutput.ok === true, "compact dry-run output should report ok")
expect(compactDryRunOutput.selectedWorkflowCount === 4, "compact dry-run should preserve selected workflow count")
expect(compactDryRunOutput.coverage.byKind.line === 4, "compact dry-run should preserve coverage")
expect(compactDryRunOutput.workflows === undefined, "compact dry-run should omit workflow list")

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

const result = spawnSync(process.execPath, [
    helper,
    "--fixture", "arc_create_basic.json",
], {
    cwd: repoRoot,
    encoding: "utf8",
    timeout: 10000,
})

if (result.status !== 0) {
    console.error(result.stdout)
    console.error(result.stderr)
}
expect(result.status === 0, "workflow report CLI should pass selected fixture")

const output = JSON.parse(result.stdout)
expect(output.ok === true, "CLI output should report ok")
expect(output.selectedWorkflowCount === 1, "CLI output should report selected workflow count")
expect(output.metricSamples > 0, "CLI output should report metric samples")
expect(output.budgetFailures === 0, "CLI output should report zero budget failures")
expect(output.worstFailure === null, "CLI output should omit worst failure when passing")
expect(fs.existsSync(output.reportPath), "CLI should write summary report")

const report = JSON.parse(fs.readFileSync(output.reportPath, "utf8"))
expect(report.schemaVersion === 4, "CLI summary report should preserve schema")
expect(report.selectedWorkflowCount === 1, "CLI summary report should preserve fixture filter")
expect(report.filters.fixtures.indexOf("arc_create_basic.json") >= 0, "CLI summary report should preserve selected fixture")

if (process.exitCode) {
    process.exit(process.exitCode)
}
