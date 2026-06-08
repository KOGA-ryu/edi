const fs = require("fs")
const path = require("path")
const { spawnSync } = require("child_process")
const vm = require("vm")

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

function loadMetricReducer(repoRoot) {
    const modulePath = path.join(repoRoot, "src", "runtime", "DrawingCanvasMetricReducer.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function normalizeWorkflowEntry(entry) {
    if (typeof entry === "string") {
        return {
            fixture: entry,
            kind: "unknown",
            category: "unknown",
            tags: [],
        }
    }
    return {
        fixture: String(entry && entry.fixture || ""),
        kind: String(entry && entry.kind || "unknown"),
        category: String(entry && entry.category || "unknown"),
        tags: Array.isArray(entry && entry.tags) ? entry.tags.map(tag => String(tag)) : [],
    }
}

function valueSet(value) {
    return String(value || "")
        .split(",")
        .map(part => part.trim())
        .filter(part => part.length > 0)
}

function workflowFilters(env) {
    return {
        fixtures: valueSet(env && env.DRAWING_WORKFLOW_FIXTURE),
        categories: valueSet(env && env.DRAWING_WORKFLOW_CATEGORY),
        tags: valueSet(env && env.DRAWING_WORKFLOW_TAG),
    }
}

function workflowMatchesFilters(workflow, filters) {
    const fixture = String(workflow && workflow.fixture || "")
    const category = String(workflow && workflow.category || "")
    const tags = Array.isArray(workflow && workflow.tags) ? workflow.tags.map(tag => String(tag)) : []
    if (filters.fixtures.length > 0 && filters.fixtures.indexOf(fixture) < 0) {
        return false
    }
    if (filters.categories.length > 0 && filters.categories.indexOf(category) < 0) {
        return false
    }
    if (filters.tags.length > 0 && !filters.tags.some(tag => tags.indexOf(tag) >= 0)) {
        return false
    }
    return true
}

function selectWorkflows(workflows, filters) {
    return workflows.filter(workflow => workflowMatchesFilters(workflow, filters))
}

function workflowFixtures(repoRoot, env) {
    const manifestPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "workflow_manifest.json")
    const manifest = readJson(manifestPath)
    const filters = workflowFilters(env || {})
    const workflows = (Array.isArray(manifest.workflows) ? manifest.workflows : [])
        .map(normalizeWorkflowEntry)
        .filter(workflow => workflow.fixture.length > 0)
    return {
        manifestPath,
        workflows,
        selectedWorkflows: selectWorkflows(workflows, filters),
        filters,
    }
}

function workflowBudgetPolicy(repoRoot) {
    const policyPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "workflow_metric_budgets.json")
    const policy = readJson(policyPath)
    const budgets = (Array.isArray(policy.budgets) ? policy.budgets : [])
        .map((budget, index) => ({
            id: String(budget && budget.id || `budget_${index}`),
            match: budget && budget.match ? budget.match : {},
            limits: budget && budget.limits ? budget.limits : {},
        }))
    return {
        path: policyPath,
        schemaVersion: Number(policy.schemaVersion || 1),
        policy: policy.policy || {},
        budgets,
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

function runFixture(repoRoot, workflow) {
    const fixtureName = String(workflow && workflow.fixture || "")
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
            metadata: {
                kind: String(workflow && workflow.kind || "unknown"),
                category: String(workflow && workflow.category || "unknown"),
                tags: Array.isArray(workflow && workflow.tags) ? workflow.tags.map(tag => String(tag)) : [],
            },
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
        metadata: {
            kind: String(workflow && workflow.kind || "unknown"),
            category: String(workflow && workflow.category || "unknown"),
            tags: Array.isArray(workflow && workflow.tags) ? workflow.tags.map(tag => String(tag)) : [],
        },
        ok: scriptFailures.length === 0,
        failures: scriptFailures,
        budgetFailures,
        summary,
        objects,
        modes,
    }
}

function groupScriptsByMetadata(scripts, field) {
    const groups = {}
    for (const script of scripts) {
        const key = String(script && script.metadata && script.metadata[field] || "unknown")
        if (!groups[key]) {
            groups[key] = {
                count: 0,
                okCount: 0,
                failureCount: 0,
                budgetFailureCount: 0,
            }
        }
        groups[key].count += 1
        groups[key].okCount += script.ok ? 1 : 0
        groups[key].failureCount += script.failures.length
        groups[key].budgetFailureCount += script.budgetFailures.length
    }
    return groups
}

function workflowMetricFields() {
    return [
        "durationMs",
        "pointerMoves",
        "controllerMutations",
        "renderRequests",
        "hitTests",
        "snapResolutions",
        "handlePlans",
        "revisionDelta",
        "mutationsPerPointerMove",
        "renderRequestsPerPointerMove",
        "hitTestsPerPointerMove",
        "snapResolutionsPerPointerMove",
        "handlePlansPerPointerMove",
    ]
}

function compactFieldSummary(summary, field) {
    const source = summary && summary[field] ? summary[field] : {}
    return {
        count: Number(source.count || 0),
        p95: Number(source.p95 || 0),
        max: Number(source.max || 0),
        mean: Number(source.mean || 0),
    }
}

function compactMetricDigest(records, metricReducer) {
    const list = Array.isArray(records) ? records : []
    const fields = workflowMetricFields()
    const summary = metricReducer.summarizeDistributions(list, fields, {
        percentile: 0.95,
    })
    const digest = {
        samples: list.length,
        fields: {},
    }
    for (const field of fields) {
        digest.fields[field] = compactFieldSummary(summary, field)
    }
    return digest
}

function workflowMetricRecords(scripts) {
    const records = []
    for (const script of scripts) {
        const modes = script && script.modes ? script.modes : {}
        for (const mode of Object.keys(modes)) {
            const modeRecords = Array.isArray(modes[mode] && modes[mode].records) ? modes[mode].records : []
            for (let index = 0; index < modeRecords.length; ++index) {
                const record = modeRecords[index] || {}
                const pointerMoves = Number(record.pointerMoves || 0)
                const controllerMutations = Number(record.controllerMutations || 0)
                const renderRequests = Number(record.renderRequests || 0)
                const hitTests = Number(record.hitTests || 0)
                const snapResolutions = Number(record.snapResolutions || 0)
                const handlePlans = Number(record.handlePlans || 0)
                records.push({
                    sampleId: `${script.fixture}:${mode}:${index}`,
                    script: String(script.fixture || ""),
                    kind: String(script.metadata && script.metadata.kind || "unknown"),
                    category: String(script.metadata && script.metadata.category || "unknown"),
                    tags: Array.isArray(script.metadata && script.metadata.tags) ? script.metadata.tags.map(tag => String(tag)) : [],
                    mode,
                    durationMs: Number(record.durationMs || 0),
                    pointerMoves,
                    controllerMutations,
                    renderRequests,
                    hitTests,
                    snapResolutions,
                    handlePlans,
                    revisionDelta: Number(record.revisionDelta || 0),
                    mutationsPerPointerMove: ratio(controllerMutations, pointerMoves),
                    renderRequestsPerPointerMove: ratio(renderRequests, pointerMoves),
                    hitTestsPerPointerMove: ratio(hitTests, pointerMoves),
                    snapResolutionsPerPointerMove: ratio(snapResolutions, pointerMoves),
                    handlePlansPerPointerMove: ratio(handlePlans, pointerMoves),
                })
            }
        }
    }
    return records
}

function groupMetricDigests(records, field, metricReducer) {
    const groups = {}
    for (const record of records) {
        const key = String(record && record[field] || "unknown")
        if (!groups[key]) {
            groups[key] = []
        }
        groups[key].push(record)
    }
    const result = {}
    for (const key of Object.keys(groups).sort()) {
        result[key] = compactMetricDigest(groups[key], metricReducer)
    }
    return result
}

function workflowMetricsDigest(scripts, metricReducer) {
    const records = workflowMetricRecords(scripts)
    return {
        overall: compactMetricDigest(records, metricReducer),
        byKind: groupMetricDigests(records, "kind", metricReducer),
        byCategory: groupMetricDigests(records, "category", metricReducer),
        byMode: groupMetricDigests(records, "mode", metricReducer),
    }
}

function matchValue(expected, actual) {
    if (expected === undefined) {
        return true
    }
    const values = Array.isArray(expected) ? expected.map(value => String(value)) : [String(expected)]
    return values.indexOf(String(actual || "")) >= 0
}

function matchTag(expected, actualTags) {
    if (expected === undefined) {
        return true
    }
    const expectedTags = Array.isArray(expected) ? expected.map(tag => String(tag)) : [String(expected)]
    const tags = Array.isArray(actualTags) ? actualTags.map(tag => String(tag)) : []
    return expectedTags.some(tag => tags.indexOf(tag) >= 0)
}

function recordMatchesBudget(record, budget) {
    const match = budget && budget.match ? budget.match : {}
    return matchValue(match.fixture, record.script)
        && matchValue(match.kind, record.kind)
        && matchValue(match.category, record.category)
        && matchValue(match.mode, record.mode)
        && matchTag(match.tag !== undefined ? match.tag : match.tags, record.tags)
}

function budgetLimitFields() {
    return {
        maxDurationMs: "durationMs",
        maxPointerMoves: "pointerMoves",
        maxControllerMutations: "controllerMutations",
        maxRenderRequests: "renderRequests",
        maxHitTests: "hitTests",
        maxSnapResolutions: "snapResolutions",
        maxHandlePlans: "handlePlans",
        maxRevisionDelta: "revisionDelta",
        maxMutationsPerPointerMove: "mutationsPerPointerMove",
        maxRenderRequestsPerPointerMove: "renderRequestsPerPointerMove",
        maxHitTestsPerPointerMove: "hitTestsPerPointerMove",
        maxSnapResolutionsPerPointerMove: "snapResolutionsPerPointerMove",
        maxHandlePlansPerPointerMove: "handlePlansPerPointerMove",
    }
}

function compactBudgetMatch(match) {
    return {
        fixture: match.fixture,
        kind: match.kind,
        category: match.category,
        mode: match.mode,
        tag: match.tag !== undefined ? match.tag : match.tags,
    }
}

function budgetFailureSeverity(actual, budget) {
    const limit = Number(budget || 0)
    if (limit === 0) {
        return Number(actual || 0) > 0 ? Number.POSITIVE_INFINITY : 0
    }
    return Number(actual || 0) / limit
}

function groupBudgetFailure(budget, sampleCount, metric, limitName, actual, limit) {
    return {
        budgetId: budget.id,
        match: compactBudgetMatch(budget.match || {}),
        samples: sampleCount,
        metric,
        limit: limitName,
        actual,
        budget: limit,
        severity: budgetFailureSeverity(actual, limit),
        message: `${budget.id} ${metric} expected <= ${limit}, got ${actual}`,
    }
}

function evaluateWorkflowMetricBudgets(records, budgetPolicy, metricReducer) {
    const limitFields = budgetLimitFields()
    const maxFailures = Math.max(1, Math.floor(Number(budgetPolicy.policy && budgetPolicy.policy.maxFailures || 5)))
    const groups = []
    const failures = []

    for (const budget of budgetPolicy.budgets) {
        const matchingRecords = records.filter(record => recordMatchesBudget(record, budget))
        const digest = compactMetricDigest(matchingRecords, metricReducer)
        const budgetFailures = []
        for (const limitName of Object.keys(limitFields)) {
            if (budget.limits[limitName] === undefined) {
                continue
            }
            const metric = limitFields[limitName]
            const actual = Number(digest.fields[metric] && digest.fields[metric].max || 0)
            const limit = Number(budget.limits[limitName])
            if (actual > limit) {
                budgetFailures.push(groupBudgetFailure(budget, matchingRecords.length, metric, limitName, actual, limit))
            }
        }
        budgetFailures.sort((left, right) => right.severity - left.severity)
        const topFailure = budgetFailures.length > 0 ? budgetFailures[0] : null
        if (topFailure) {
            failures.push(topFailure)
        }
        groups.push({
            budgetId: budget.id,
            match: compactBudgetMatch(budget.match || {}),
            samples: matchingRecords.length,
            ok: !topFailure,
            topFailure,
        })
    }

    failures.sort((left, right) => right.severity - left.severity)
    return {
        ok: failures.length === 0,
        policy: {
            schemaVersion: budgetPolicy.schemaVersion,
            path: path.relative(path.join(__dirname, ".."), budgetPolicy.path),
            budgetCount: budgetPolicy.budgets.length,
            maxFailures,
        },
        evaluatedCount: groups.length,
        unmatchedCount: groups.filter(group => group.samples === 0).length,
        totalFailureCount: failures.length,
        budgetFailuresByGroup: failures.slice(0, maxFailures),
        groups,
    }
}

function writeWorkflowReport(repoRoot, manifestPath, workflows, filters, scripts, metricReducer, budgetPolicy) {
    const metricRecords = workflowMetricRecords(scripts)
    const metricBudgetChecks = evaluateWorkflowMetricBudgets(metricRecords, budgetPolicy, metricReducer)
    const report = {
        schemaVersion: 4,
        name: "drawing_control_workflows",
        manifest: path.relative(repoRoot, manifestPath),
        filters,
        ok: scripts.every(script => script.ok) && metricBudgetChecks.ok,
        totalWorkflowCount: workflows.length,
        selectedWorkflowCount: scripts.length,
        scriptCount: scripts.length,
        failureCount: scripts.reduce((sum, script) => sum + script.failures.length, 0) + metricBudgetChecks.totalFailureCount,
        budgetFailureCount: scripts.reduce((sum, script) => sum + script.budgetFailures.length, 0) + metricBudgetChecks.totalFailureCount,
        groups: {
            byKind: groupScriptsByMetadata(scripts, "kind"),
            byCategory: groupScriptsByMetadata(scripts, "category"),
        },
        metrics: workflowMetricsDigest(scripts, metricReducer),
        metricBudgetChecks,
        scripts,
    }
    const outDir = path.join(repoRoot, "tests", "artifacts", "drawing_metrics")
    fs.mkdirSync(outDir, { recursive: true })
    fs.writeFileSync(path.join(outDir, "control_workflows_summary.json"), `${JSON.stringify(report, null, 2)}\n`)
    return report
}

function runWorkflowFilterContract(manifest) {
    expect(manifest.workflows.length >= 13, "workflow manifest should include all control workflows")
    expect(manifest.workflows.every(workflow => workflow.kind.length > 0 && workflow.category.length > 0 && workflow.tags.length > 0),
        "workflow manifest entries should include kind, category, and tags")

    const lineWorkflows = selectWorkflows(manifest.workflows, {
        fixtures: [],
        categories: [],
        tags: ["line"],
    })
    expect(lineWorkflows.length > 0, "line tag filter should select workflows")
    expect(lineWorkflows.length < manifest.workflows.length, "line tag filter should not select every workflow")
    expect(lineWorkflows.every(workflow => workflow.tags.indexOf("line") >= 0), "line tag filter should only select line-tagged workflows")

    const editWorkflows = selectWorkflows(manifest.workflows, {
        fixtures: [],
        categories: ["edit"],
        tags: [],
    })
    expect(editWorkflows.length > 0, "edit category filter should select workflows")
    expect(editWorkflows.every(workflow => workflow.category === "edit"), "edit category filter should only select edit workflows")

    const arcWorkflows = selectWorkflows(manifest.workflows, {
        fixtures: ["arc_create_basic.json"],
        categories: [],
        tags: [],
    })
    expect(arcWorkflows.length === 1, "fixture filter should select one named workflow")
    expect(arcWorkflows[0].fixture === "arc_create_basic.json", "fixture filter should preserve the selected fixture")
}

function runWorkflowReportContract(report) {
    expect(report.schemaVersion === 4, "workflow report should use schema version 4")
    expect(report.ok === true, "workflow report should pass script and group metric budgets")
    expect(report.metrics.overall.samples > 0, "workflow report should include metric samples")
    expect(finite(report.metrics.overall.fields.durationMs.p95), "workflow report should include duration p95")
    expect(finite(report.metrics.overall.fields.renderRequests.max), "workflow report should include render request max")
    expect(Object.keys(report.metrics.byMode).length > 0, "workflow report should group metrics by mode")
    expect(Object.keys(report.metrics.byKind).length > 0, "workflow report should group metrics by kind")
    expect(Object.keys(report.metrics.byCategory).length > 0, "workflow report should group metrics by category")
    expect(report.metricBudgetChecks.policy.budgetCount > 0, "workflow report should include metric budget policy")
    expect(report.metricBudgetChecks.ok === true, "workflow metric budget checks should pass")
    expect(report.metricBudgetChecks.budgetFailuresByGroup.length === 0, "passing workflow metric budgets should not report group failures")
}

function runWorkflowBudgetContract(metricReducer) {
    const records = [
        {
            sampleId: "slow_drag",
            script: "line_drag_end_handle.json",
            kind: "line",
            category: "edit",
            tags: ["line", "handle"],
            mode: "dragging_handle",
            durationMs: 10,
            pointerMoves: 2,
            controllerMutations: 2,
            renderRequests: 6,
            hitTests: 0,
            snapResolutions: 0,
            handlePlans: 0,
            revisionDelta: 2,
            mutationsPerPointerMove: 1,
            renderRequestsPerPointerMove: 3,
            hitTestsPerPointerMove: 0,
            snapResolutionsPerPointerMove: 0,
            handlePlansPerPointerMove: 0,
        },
    ]
    const checks = evaluateWorkflowMetricBudgets(records, {
        schemaVersion: 1,
        path: path.join(__dirname, "synthetic_workflow_metric_budgets.json"),
        policy: { maxFailures: 1 },
        budgets: [
            {
                id: "synthetic_drag_budget",
                match: { kind: "line", category: "edit", mode: "dragging_handle" },
                limits: { maxRenderRequestsPerPointerMove: 1.25 },
            },
        ],
    }, metricReducer)

    expect(!checks.ok, "synthetic expensive workflow should fail group budget")
    expect(checks.budgetFailuresByGroup.length === 1, "group budget failures should be capped and compact")
    expect(checks.budgetFailuresByGroup[0].metric === "renderRequestsPerPointerMove", "group budget should identify failing metric")
}

const repoRoot = path.join(__dirname, "..")
const executable = path.join(repoRoot, "build", "qt_qml_region_split")

if (!fs.existsSync(executable)) {
    console.log("SKIP: build/qt_qml_region_split is not built")
    process.exit(0)
}

const scripts = []
const manifest = workflowFixtures(repoRoot, process.env)
const metricReducer = loadMetricReducer(repoRoot)
const budgetPolicy = workflowBudgetPolicy(repoRoot)
runWorkflowFilterContract(manifest)
runWorkflowBudgetContract(metricReducer)
expect(manifest.selectedWorkflows.length > 0, "workflow filters should select at least one fixture")
for (const workflow of manifest.selectedWorkflows) {
    scripts.push(runFixture(repoRoot, workflow))
}

const report = writeWorkflowReport(repoRoot, manifest.manifestPath, manifest.workflows, manifest.filters, scripts, metricReducer, budgetPolicy)
runWorkflowReportContract(report)

if (process.exitCode) {
    process.exit(process.exitCode)
}
