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

function expectInBucket(condition, message, bucket) {
    if (!condition) {
        if (bucket) {
            bucket.push(message)
        }
        fail(message)
    }
}

function readJson(filePath) {
    return JSON.parse(fs.readFileSync(filePath, "utf8"))
}

function workflowFixtures(repoRoot) {
    const manifestPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "workflow_manifest.json")
    const manifest = readJson(manifestPath)
    return {
        manifestPath,
        fixtures: Array.isArray(manifest.workflows) ? manifest.workflows : [],
    }
}

function finite(value) {
    return Number.isFinite(Number(value))
}

function near(actual, expected, tolerance) {
    return Math.abs(Number(actual) - Number(expected)) <= Number(tolerance)
}

function parsePrefixedJsonLines(output, prefix) {
    return output.split(/\r?\n/)
        .filter(line => line.indexOf(prefix) >= 0)
        .map(line => JSON.parse(line.slice(line.indexOf(prefix) + prefix.length)))
}

function expectedGeometryFields(object) {
    return [
        "x",
        "y",
        "cx",
        "cy",
        "x1",
        "y1",
        "x2",
        "y2",
        "width",
        "height",
        "radius",
        "sides",
        "rotation_deg",
        "start_angle_deg",
        "end_angle_deg",
    ].filter(field => object[field] !== undefined)
}

function reducedObject(object) {
    return {
        id: String(object && object.id || ""),
        kind: String(object && object.kind || ""),
        x: Number(object && object.x || 0),
        y: Number(object && object.y || 0),
        cx: Number(object && object.cx || 0),
        cy: Number(object && object.cy || 0),
        x1: Number(object && object.x1 || 0),
        y1: Number(object && object.y1 || 0),
        x2: Number(object && object.x2 || 0),
        y2: Number(object && object.y2 || 0),
        width: Number(object && object.width || 0),
        height: Number(object && object.height || 0),
        radius: Number(object && object.radius || 0),
        sides: Number(object && object.sides || 0),
        rotation_deg: Number(object && object.rotation_deg || 0),
        start_angle_deg: Number(object && object.start_angle_deg || 0),
        end_angle_deg: Number(object && object.end_angle_deg || 0),
    }
}

function reducedObjects(summary) {
    return Array.isArray(summary && summary.objects) ? summary.objects.map(reducedObject) : []
}

function reducedMetricRecord(record) {
    return {
        durationMs: Number(record.durationMs || 0),
        pointerMoves: Number(record.pointerMoves || 0),
        controllerMutations: Number(record.controllerMutations || 0),
        renderRequests: Number(record.renderRequests || 0),
        hitTests: Number(record.hitTests || 0),
        snapResolutions: Number(record.snapResolutions || 0),
        handlePlans: Number(record.handlePlans || 0),
        revisionDelta: Number(record.revisionDelta || 0),
    }
}

function ratio(numerator, denominator) {
    return Number(denominator) > 0 ? Number(numerator) / Number(denominator) : 0
}

function maxValue(records, field) {
    return records.reduce((max, record) => Math.max(max, Number(record[field] || 0)), 0)
}

function metricSummary(records) {
    return {
        count: records.length,
        maxDurationMs: maxValue(records, "durationMs"),
        maxPointerMoves: maxValue(records, "pointerMoves"),
        maxControllerMutations: maxValue(records, "controllerMutations"),
        maxRenderRequests: maxValue(records, "renderRequests"),
        maxHitTests: maxValue(records, "hitTests"),
        maxSnapResolutions: maxValue(records, "snapResolutions"),
        maxHandlePlans: maxValue(records, "handlePlans"),
        maxRevisionDelta: maxValue(records, "revisionDelta"),
    }
}

function metricRatios(records) {
    let mutationsPerPointerMove = 0
    let renderRequestsPerPointerMove = 0
    let hitTestsPerPointerMove = 0
    for (const record of records) {
        mutationsPerPointerMove = Math.max(mutationsPerPointerMove, ratio(record.controllerMutations, record.pointerMoves))
        renderRequestsPerPointerMove = Math.max(renderRequestsPerPointerMove, ratio(record.renderRequests, record.pointerMoves))
        hitTestsPerPointerMove = Math.max(hitTestsPerPointerMove, ratio(record.hitTests, record.pointerMoves))
    }
    return {
        mutationsPerPointerMove,
        renderRequestsPerPointerMove,
        hitTestsPerPointerMove,
    }
}

function budgetFailure(label, recordIndex, metric, actual, budget) {
    return {
        script: label.script,
        mode: label.mode,
        recordIndex,
        metric,
        actual,
        budget,
        message: `${label.script}:${label.mode}[${recordIndex}] ${metric} expected <= ${budget}, got ${actual}`,
    }
}

function checkMax(record, budget, budgetField, recordField, label, recordIndex, bucket) {
    if (budget[budgetField] !== undefined) {
        const actual = Number(record[recordField])
        const limit = Number(budget[budgetField])
        if (actual > limit) {
            const failure = budgetFailure(label, recordIndex, recordField, actual, limit)
            bucket.push(failure)
            fail(failure.message)
        }
    }
}

function checkMaxPerPointerMove(record, budget, budgetField, recordField, label, recordIndex, bucket) {
    if (budget[budgetField] !== undefined && Number(record.pointerMoves) > 0) {
        const ratio = Number(record[recordField]) / Number(record.pointerMoves)
        const limit = Number(budget[budgetField])
        if (ratio > limit) {
            const failure = budgetFailure(label, recordIndex, `${recordField}PerPointerMove`, ratio, limit)
            bucket.push(failure)
            fail(failure.message)
        }
    }
}

function checkMetricRecord(record, budget, label, recordIndex, scriptFailures) {
    const budgetFailures = []
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
        expectInBucket(finite(record[field]), `${label.script}:${label.mode}[${recordIndex}] ${field} should be finite`, scriptFailures)
    }
    checkMax(record, budget, "maxDurationMs", "durationMs", label, recordIndex, budgetFailures)
    checkMax(record, budget, "maxPointerMoves", "pointerMoves", label, recordIndex, budgetFailures)
    checkMax(record, budget, "maxControllerMutations", "controllerMutations", label, recordIndex, budgetFailures)
    checkMax(record, budget, "maxRenderRequests", "renderRequests", label, recordIndex, budgetFailures)
    checkMax(record, budget, "maxHitTests", "hitTests", label, recordIndex, budgetFailures)
    checkMax(record, budget, "maxSnapResolutions", "snapResolutions", label, recordIndex, budgetFailures)
    checkMax(record, budget, "maxHandlePlans", "handlePlans", label, recordIndex, budgetFailures)
    checkMaxPerPointerMove(record, budget, "maxMutationsPerPointerMove", "controllerMutations", label, recordIndex, budgetFailures)
    checkMaxPerPointerMove(record, budget, "maxRenderRequestsPerPointerMove", "renderRequests", label, recordIndex, budgetFailures)
    checkMaxPerPointerMove(record, budget, "maxHitTestsPerPointerMove", "hitTests", label, recordIndex, budgetFailures)
    for (const failure of budgetFailures) {
        scriptFailures.push(failure.message)
    }
    return budgetFailures
}

function checkSummary(script, summary, label, scriptFailures) {
    expectInBucket(summary.ok === true, `${label} control script should pass`, scriptFailures)
    expectInBucket(summary.executed > 0, `${label} should execute at least one step`, scriptFailures)
    expectInBucket(finite(summary.revision), `${label} revision should be finite`, scriptFailures)
    const expectedModel = script.expect && script.expect.model ? script.expect.model : {}
    if (expectedModel.objectCount !== undefined) {
        expectInBucket(Number(summary.objectCount) === Number(expectedModel.objectCount),
            `${label} objectCount expected ${expectedModel.objectCount}, got ${summary.objectCount}`, scriptFailures)
    }
    const expectedObjects = Array.isArray(expectedModel.objects) ? expectedModel.objects : []
    for (const expectedObject of expectedObjects) {
        const actualObject = (summary.objects || []).find(object => String(object.kind || "") === String(expectedObject.kind || ""))
        expectInBucket(!!actualObject, `${label} should include object kind ${expectedObject.kind}`, scriptFailures)
        if (!actualObject) {
            continue
        }
        const tolerance = Number(expectedObject.tolerance !== undefined ? expectedObject.tolerance : expectedModel.tolerance !== undefined ? expectedModel.tolerance : 0.0001)
        for (const field of expectedGeometryFields(expectedObject)) {
            expectInBucket(finite(actualObject[field]), `${label} ${expectedObject.kind}.${field} should be finite`, scriptFailures)
            expectInBucket(near(actualObject[field], expectedObject[field], tolerance),
                `${label} ${expectedObject.kind}.${field} expected ${expectedObject[field]}, got ${actualObject[field]}`, scriptFailures)
        }
    }
    const expectedSelection = script.expect && script.expect.selection ? script.expect.selection : {}
    if (expectedSelection.minSelected !== undefined) {
        expectInBucket(Number(summary.selectedCount) >= Number(expectedSelection.minSelected),
            `${label} selectedCount expected >= ${expectedSelection.minSelected}, got ${summary.selectedCount}`, scriptFailures)
    }
    const expectedViewport = script.expect && script.expect.viewport ? script.expect.viewport : {}
    if (expectedViewport.panChanged === true) {
        const viewport = summary.viewport || {}
        expectInBucket(Math.abs(Number(viewport.panX || 0)) > 0 || Math.abs(Number(viewport.panY || 0)) > 0,
            `${label} viewport pan should change`, scriptFailures)
    }
}

function reducedSummary(summary) {
    const viewport = summary && summary.viewport ? summary.viewport : {}
    return {
        ok: summary && summary.ok === true,
        executed: Number(summary && summary.executed || 0),
        objectCount: Number(summary && summary.objectCount || 0),
        selectedCount: Number(summary && summary.selectedCount || 0),
        revision: Number(summary && summary.revision || 0),
        viewport: {
            zoom: Number(viewport.zoom || 1),
            panX: Number(viewport.panX || 0),
            panY: Number(viewport.panY || 0),
        },
        viewportDelta: {
            zoom: Number(viewport.zoom || 1) - 1,
            panX: Number(viewport.panX || 0),
            panY: Number(viewport.panY || 0),
        },
    }
}

function runFixture(repoRoot, fixtureName) {
    const executable = path.join(repoRoot, "build", "qt_qml_region_split")
    const scriptPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", fixtureName)
    const libraryPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "shared_canvas_library.json")
    const script = readJson(scriptPath)
    const library = readJson(libraryPath)
    const scriptFailures = []
    const budgetFailures = []
    const modes = {}
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
        const message = `${fixtureName} launch failed: ${result.error.message}`
        scriptFailures.push(message)
        fail(message)
        return {
            name: String(script.name || fixtureName),
            fixture: fixtureName,
            ok: false,
            failures: scriptFailures,
            budgetFailures,
            modes,
        }
    }
    expectInBucket(result.status === 0, `${fixtureName} app should exit 0, got ${result.status}`, scriptFailures)

    const output = `${result.stdout || ""}\n${result.stderr || ""}`
    const summaries = parsePrefixedJsonLines(output, "drawing_control_script_result ")
    expectInBucket(summaries.length === 1, `${fixtureName} should emit one result summary`, scriptFailures)
    if (summaries.length === 1) {
        checkSummary(script, summaries[0], fixtureName, scriptFailures)
    }

    const metricRecords = parsePrefixedJsonLines(output, "drawing_canvas_interaction_metrics ")
    const telemetryRecords = parsePrefixedJsonLines(output, "drawing_canvas_interaction_events ")
    expectInBucket(metricRecords.length > 0, `${fixtureName} should emit metric records`, scriptFailures)
    expectInBucket(telemetryRecords.length > 0, `${fixtureName} should emit telemetry records`, scriptFailures)

    const expectedModes = script.expect && script.expect.metricsByMode ? script.expect.metricsByMode : {}
    for (const mode of Object.keys(expectedModes)) {
        const matchingRecords = metricRecords.filter(record => String(record.mode || "") === mode)
        expectInBucket(matchingRecords.length > 0, `${fixtureName} should emit ${mode} metrics`, scriptFailures)
        const budgetName = expectedModes[mode]
        const budget = typeof budgetName === "string" ? library.budgets[budgetName] : budgetName
        const reducedRecords = matchingRecords.map(reducedMetricRecord)
        modes[mode] = {
            budgetName: typeof budgetName === "string" ? budgetName : "",
            budget: budget || {},
            records: reducedRecords,
            summary: metricSummary(reducedRecords),
            maxRatios: metricRatios(reducedRecords),
            budgetFailures: [],
        }
        for (let index = 0; index < reducedRecords.length; ++index) {
            const failures = checkMetricRecord(
                reducedRecords[index],
                budget || {},
                { script: fixtureName, mode },
                index,
                scriptFailures)
            modes[mode].budgetFailures = modes[mode].budgetFailures.concat(failures)
            budgetFailures.push(...failures)
        }
    }
    const summary = summaries.length === 1 ? reducedSummary(summaries[0]) : reducedSummary({})
    const objects = summaries.length === 1 ? reducedObjects(summaries[0]) : []
    return {
        name: String(script.name || fixtureName),
        fixture: fixtureName,
        ok: scriptFailures.length === 0,
        failures: scriptFailures,
        budgetFailures,
        summary,
        objects,
        modes,
    }
}

function writeWorkflowReport(repoRoot, manifestPath, scripts) {
    const report = {
        schemaVersion: 1,
        name: "drawing_control_workflows",
        manifest: path.relative(repoRoot, manifestPath),
        ok: scripts.every(script => script.ok),
        scriptCount: scripts.length,
        failureCount: scripts.reduce((sum, script) => sum + script.failures.length, 0),
        budgetFailureCount: scripts.reduce((sum, script) => sum + script.budgetFailures.length, 0),
        scripts,
    }
    const outDir = path.join(repoRoot, "tests", "artifacts", "drawing_metrics")
    fs.mkdirSync(outDir, { recursive: true })
    fs.writeFileSync(path.join(outDir, "control_workflows_summary.json"), `${JSON.stringify(report, null, 2)}\n`)
    return report
}

const repoRoot = path.join(__dirname, "..")
const executable = path.join(repoRoot, "build", "qt_qml_region_split")

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

const scripts = []
const manifest = workflowFixtures(repoRoot)
expect(manifest.fixtures.length > 0, "workflow manifest should include fixtures")
for (const fixtureName of manifest.fixtures) {
    scripts.push(runFixture(repoRoot, fixtureName))
}

writeWorkflowReport(repoRoot, manifest.manifestPath, scripts)

if (process.exitCode) {
    process.exit(process.exitCode)
}
