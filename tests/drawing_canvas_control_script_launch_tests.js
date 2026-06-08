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

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

const result = spawnSync(executable, [
    "--project-profile", path.join(repoRoot, "data", "project_profiles", "draftsman_drawing_tool_blank.json"),
    "--drawing-telemetry-log",
    "--drawing-control-script", path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "line_create_basic.json"),
    "--drawing-control-library", path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "shared_canvas_library.json"),
    "--drawing-control-script-exit",
], {
    cwd: repoRoot,
    encoding: "utf8",
    timeout: 10000,
})

if (result.error) {
    console.error(result.stdout)
    console.error(result.stderr)
    fail(`control script launch failed: ${result.error.message}`)
}

expect(result.status === 0, `control script app should exit 0, got ${result.status}`)
const output = `${result.stdout || ""}\n${result.stderr || ""}`
const resultLine = output.split(/\r?\n/).find(line => line.indexOf("drawing_control_script_result ") >= 0)
expect(!!resultLine, "control script launch should emit result line")

if (resultLine) {
    const payload = resultLine.slice(resultLine.indexOf("drawing_control_script_result ") + "drawing_control_script_result ".length)
    const summary = JSON.parse(payload)
    expect(summary.ok === true, "control script result should pass")
    expect(summary.executed === 3, "line_create_basic should execute three expanded steps")
    expect(summary.objectCount >= 1, "line_create_basic should create at least one drawing object")
    expect(summary.revision > 0, "control script should advance revision")
}

const telemetryLines = output.split(/\r?\n/).filter(line => line.indexOf("drawing_canvas_interaction_events ") >= 0)
expect(telemetryLines.length >= 2, "line_create_basic should emit click telemetry")

if (process.exitCode) {
    process.exit(process.exitCode)
}
