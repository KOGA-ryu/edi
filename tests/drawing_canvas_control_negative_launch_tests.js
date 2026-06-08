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

function parseResultLine(output) {
    const prefix = "drawing_control_script_result "
    const line = output.split(/\r?\n/).find(value => value.indexOf(prefix) >= 0)
    expect(!!line, "negative script should emit result line")
    return line ? JSON.parse(line.slice(line.indexOf(prefix) + prefix.length)) : {}
}

function writeScript(repoRoot, name, script) {
    const dir = path.join(repoRoot, "tests", "artifacts", "drawing_metrics", "negative_control_scripts")
    fs.mkdirSync(dir, { recursive: true })
    const filePath = path.join(dir, `${name}.json`)
    fs.writeFileSync(filePath, `${JSON.stringify(script, null, 2)}\n`)
    return filePath
}

function runNegativeCase(repoRoot, executable, testCase) {
    const scriptPath = writeScript(repoRoot, testCase.name, testCase.script)
    const result = spawnSync(executable, [
        "--project-profile", path.join(repoRoot, "data", "project_profiles", "draftsman_drawing_tool_blank.json"),
        "--drawing-telemetry-log",
        "--drawing-control-script", scriptPath,
        "--drawing-control-library", path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "shared_canvas_library.json"),
        "--drawing-disable-discard-confirmation",
        "--drawing-control-script-exit",
    ], {
        cwd: repoRoot,
        encoding: "utf8",
        timeout: 10000,
    })

    if (result.error) {
        console.error(result.stdout)
        console.error(result.stderr)
        fail(`${testCase.name} launch failed: ${result.error.message}`)
        return
    }

    expect(result.status === 0, `${testCase.name} should exit 0, got ${result.status}`)
    const output = `${result.stdout || ""}\n${result.stderr || ""}`
    const summary = parseResultLine(output)
    expect(summary.ok === false, `${testCase.name} should fail control script`)
    expect(Number(summary.executed || 0) === Number(testCase.executed || 0), `${testCase.name} executed count should be ${testCase.executed || 0}`)
    expect(Number(summary.objectCount || 0) === 0, `${testCase.name} should not leave script objects`)
    expect(Number.isFinite(Number(summary.revision)), `${testCase.name} revision should remain finite`)

    const failures = Array.isArray(summary.failures) ? summary.failures : []
    expect(failures.length > 0, `${testCase.name} should include failures`)
    for (const failure of failures) {
        expect(Number.isFinite(Number(failure.stepIndex)), `${testCase.name} failure should include stepIndex`)
        expect(String(failure.stepType || "").length > 0, `${testCase.name} failure should include stepType`)
        expect(String(failure.message || "").length > 0, `${testCase.name} failure should include message`)
    }
    expect(failures.some(failure => String(failure.message || "").indexOf(testCase.messageIncludes) >= 0),
        `${testCase.name} failure should mention ${testCase.messageIncludes}`)

    const telemetryLines = output.split(/\r?\n/).filter(line => line.indexOf("drawing_canvas_interaction_events ") >= 0)
    expect(telemetryLines.length <= 2, `${testCase.name} should not emit unbounded telemetry`)
}

const repoRoot = path.join(__dirname, "..")
const executable = path.join(repoRoot, "build", "qt_qml_region_split")

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

const cases = [
    {
        name: "unknown_fragment",
        messageIncludes: "unknown fragment",
        script: {
            name: "unknown_fragment",
            steps: [{ use: "missingFragment" }],
        },
    },
    {
        name: "unknown_step_type",
        messageIncludes: "unknown type teleportCanvas",
        script: {
            name: "unknown_step_type",
            steps: [{ type: "teleportCanvas" }],
        },
    },
    {
        name: "missing_point",
        messageIncludes: "clickCanvas requires finite point",
        script: {
            name: "missing_point",
            steps: [{ type: "clickCanvas", point: "missingPoint" }],
        },
    },
    {
        name: "missing_handle_id",
        messageIncludes: "dragHandle requires handleId",
        script: {
            name: "missing_handle_id",
            steps: [{ type: "dragHandle", object: "latest", to: "lineEnd" }],
        },
    },
    {
        name: "invalid_tool_parameter",
        messageIncludes: "invalid circle_arc_mode",
        script: {
            name: "invalid_tool_parameter",
            steps: [{ type: "setToolParameter", parameter: "circle_arc_mode", value: "banana" }],
        },
    },
    {
        name: "drag_handle_before_object",
        messageIncludes: "drag handle target not found",
        script: {
            name: "drag_handle_before_object",
            steps: [
                {
                    type: "dragHandle",
                    object: "latest",
                    handleId: "line_end",
                    from: "lineEnd",
                    to: "lineEndMoved",
                    pointerMoves: 4,
                },
            ],
        },
    },
]

for (const testCase of cases) {
    runNegativeCase(repoRoot, executable, testCase)
}

if (process.exitCode) {
    process.exit(process.exitCode)
}
