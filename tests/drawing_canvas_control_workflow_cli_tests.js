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
