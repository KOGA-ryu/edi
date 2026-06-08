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

const helper = path.join(__dirname, "helpers", "drawing_gui_metric_capture_report.js")
const logPath = path.join(__dirname, "fixtures", "drawing_interactions", "gui_capture_sample.log")
const budgetPath = path.join(__dirname, "fixtures", "drawing_interactions", "gui_capture_budget.json")
const outRoot = path.join(__dirname, "artifacts", "drawing_metrics_cli")

fs.rmSync(outRoot, { recursive: true, force: true })

const result = spawnSync(process.execPath, [
    helper,
    "--log", logPath,
    "--name", "Drag Line Endpoint GUI CLI",
    "--budget", budgetPath,
    "--out", outRoot,
], {
    cwd: path.join(__dirname, ".."),
    encoding: "utf8",
})

if (result.status !== 0) {
    console.error(result.stdout)
    console.error(result.stderr)
}
expect(result.status === 0, "capture CLI should pass matching fixture budget")

const output = JSON.parse(result.stdout)
expect(output.ok === true, "CLI output should report ok")
expect(output.samples === 2, "CLI output should report sample count")
expect(fs.existsSync(output.rawJsonl), "CLI should write raw JSONL artifact")
expect(fs.existsSync(output.summaryJson), "CLI should write summary JSON artifact")

const rawLines = fs.readFileSync(output.rawJsonl, "utf8").trim().split(/\r?\n/)
const summary = JSON.parse(fs.readFileSync(output.summaryJson, "utf8"))
expect(rawLines.length === 2, "raw artifact should contain one metric record per captured stream")
expect(summary.samples === 2, "summary artifact should preserve sample count")
expect(summary.records === undefined, "summary artifact should not include raw records")

if (process.exitCode) {
    process.exit(process.exitCode)
}
