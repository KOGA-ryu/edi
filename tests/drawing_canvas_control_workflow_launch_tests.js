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

function readJson(filePath) {
    return JSON.parse(fs.readFileSync(filePath, "utf8"))
}

function finite(value) {
    return Number.isFinite(Number(value))
}

function parsePrefixedJsonLines(output, prefix) {
    return output.split(/\r?\n/)
        .filter(line => line.indexOf(prefix) >= 0)
        .map(line => JSON.parse(line.slice(line.indexOf(prefix) + prefix.length)))
}

function checkMax(record, budget, budgetField, recordField, label) {
    if (budget[budgetField] !== undefined) {
        expect(Number(record[recordField]) <= Number(budget[budgetField]),
            `${label} ${recordField} expected <= ${budget[budgetField]}, got ${record[recordField]}`)
    }
}

function checkMaxPerPointerMove(record, budget, budgetField, recordField, label) {
    if (budget[budgetField] !== undefined && Number(record.pointerMoves) > 0) {
        const ratio = Number(record[recordField]) / Number(record.pointerMoves)
        expect(ratio <= Number(budget[budgetField]),
            `${label} ${recordField}/pointerMoves expected <= ${budget[budgetField]}, got ${ratio}`)
    }
}

function checkMetricRecord(record, budget, label) {
    const numericFields = [
        "durationMs",
        "pointerMoves",
        "controllerMutations",
        "renderRequests",
        "hitTests",
        "snapResolutions",
        "handlePlans",
        "revisionDelta",
    ]
    for (const field of numericFields) {
        expect(finite(record[field]), `${label} ${field} should be finite`)
    }
    checkMax(record, budget, "maxDurationMs", "durationMs", label)
    checkMax(record, budget, "maxPointerMoves", "pointerMoves", label)
    checkMax(record, budget, "maxControllerMutations", "controllerMutations", label)
    checkMax(record, budget, "maxRenderRequests", "renderRequests", label)
    checkMax(record, budget, "maxHitTests", "hitTests", label)
    checkMax(record, budget, "maxSnapResolutions", "snapResolutions", label)
    checkMax(record, budget, "maxHandlePlans", "handlePlans", label)
    checkMaxPerPointerMove(record, budget, "maxMutationsPerPointerMove", "controllerMutations", label)
    checkMaxPerPointerMove(record, budget, "maxRenderRequestsPerPointerMove", "renderRequests", label)
    checkMaxPerPointerMove(record, budget, "maxHitTestsPerPointerMove", "hitTests", label)
}

function checkSummary(script, summary, label) {
    expect(summary.ok === true, `${label} control script should pass`)
    expect(summary.executed > 0, `${label} should execute at least one step`)
    expect(finite(summary.revision), `${label} revision should be finite`)
    const expectedModel = script.expect && script.expect.model ? script.expect.model : {}
    if (expectedModel.objectCount !== undefined) {
        expect(Number(summary.objectCount) === Number(expectedModel.objectCount),
            `${label} objectCount expected ${expectedModel.objectCount}, got ${summary.objectCount}`)
    }
    const expectedObjects = Array.isArray(expectedModel.objects) ? expectedModel.objects : []
    for (const expectedObject of expectedObjects) {
        expect((summary.objects || []).some(object => String(object.kind || "") === String(expectedObject.kind || "")),
            `${label} should include object kind ${expectedObject.kind}`)
    }
    const expectedSelection = script.expect && script.expect.selection ? script.expect.selection : {}
    if (expectedSelection.minSelected !== undefined) {
        expect(Number(summary.selectedCount) >= Number(expectedSelection.minSelected),
            `${label} selectedCount expected >= ${expectedSelection.minSelected}, got ${summary.selectedCount}`)
    }
    const expectedViewport = script.expect && script.expect.viewport ? script.expect.viewport : {}
    if (expectedViewport.panChanged === true) {
        const viewport = summary.viewport || {}
        expect(Math.abs(Number(viewport.panX || 0)) > 0 || Math.abs(Number(viewport.panY || 0)) > 0,
            `${label} viewport pan should change`)
    }
}

function runFixture(repoRoot, fixtureName) {
    const executable = path.join(repoRoot, "build", "qt_qml_region_split")
    const scriptPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", fixtureName)
    const libraryPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "shared_canvas_library.json")
    const script = readJson(scriptPath)
    const library = readJson(libraryPath)
    const result = spawnSync(executable, [
        "--project-profile", path.join(repoRoot, script.profile || "data/project_profiles/draftsman_drawing_tool_blank.json"),
        "--drawing-telemetry-log",
        "--drawing-metrics-log",
        "--drawing-control-script", scriptPath,
        "--drawing-control-library", libraryPath,
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
        fail(`${fixtureName} launch failed: ${result.error.message}`)
        return
    }
    expect(result.status === 0, `${fixtureName} app should exit 0, got ${result.status}`)

    const output = `${result.stdout || ""}\n${result.stderr || ""}`
    const summaries = parsePrefixedJsonLines(output, "drawing_control_script_result ")
    expect(summaries.length === 1, `${fixtureName} should emit one result summary`)
    if (summaries.length === 1) {
        checkSummary(script, summaries[0], fixtureName)
    }

    const metricRecords = parsePrefixedJsonLines(output, "drawing_canvas_interaction_metrics ")
    const telemetryRecords = parsePrefixedJsonLines(output, "drawing_canvas_interaction_events ")
    expect(metricRecords.length > 0, `${fixtureName} should emit metric records`)
    expect(telemetryRecords.length > 0, `${fixtureName} should emit telemetry records`)

    const expectedModes = script.expect && script.expect.metricsByMode ? script.expect.metricsByMode : {}
    for (const mode of Object.keys(expectedModes)) {
        const matchingRecords = metricRecords.filter(record => String(record.mode || "") === mode)
        expect(matchingRecords.length > 0, `${fixtureName} should emit ${mode} metrics`)
        const budgetName = expectedModes[mode]
        const budget = typeof budgetName === "string" ? library.budgets[budgetName] : budgetName
        for (const record of matchingRecords) {
            checkMetricRecord(record, budget || {}, `${fixtureName}:${mode}`)
        }
    }
}

const repoRoot = path.join(__dirname, "..")
const executable = path.join(repoRoot, "build", "qt_qml_region_split")

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

for (const fixtureName of [
    "line_create_basic.json",
    "line_drag_end_handle.json",
    "line_move_object.json",
    "marquee_select_lines.json",
    "pan_canvas_basic.json",
]) {
    runFixture(repoRoot, fixtureName)
}

if (process.exitCode) {
    process.exit(process.exitCode)
}
