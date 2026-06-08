const fs = require("fs")
const path = require("path")
const vm = require("vm")

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

function workflowCoverageExpectations(repoRoot) {
    const expectationsPath = path.join(repoRoot, "tests", "fixtures", "drawing_tool_scripts", "workflow_coverage_expectations.json")
    const expectations = readJson(expectationsPath)
    return {
        path: expectationsPath,
        schemaVersion: Number(expectations.schemaVersion || 1),
        minimums: expectations.minimums || {},
    }
}

function incrementCount(target, key) {
    const name = String(key || "unknown")
    target[name] = Number(target[name] || 0) + 1
}

function workflowCoverage(workflows) {
    const coverage = {
        byKind: {},
        byCategory: {},
        byTag: {},
    }
    const list = Array.isArray(workflows) ? workflows : []
    for (const workflow of list) {
        incrementCount(coverage.byKind, workflow.kind)
        incrementCount(coverage.byCategory, workflow.category)
        const tags = Array.isArray(workflow.tags) ? workflow.tags : []
        for (const tag of tags) {
            incrementCount(coverage.byTag, tag)
        }
    }
    return coverage
}

function coverageFailuresForGroup(actualGroup, expectedGroup, label) {
    const failures = []
    const expected = expectedGroup || {}
    for (const key of Object.keys(expected).sort()) {
        const actual = Number(actualGroup && actualGroup[key] || 0)
        const minimum = Number(expected[key] || 0)
        if (actual < minimum) {
            failures.push({
                group: label,
                key,
                actual,
                minimum,
                message: `${label}.${key} expected >= ${minimum}, got ${actual}`,
            })
        }
    }
    return failures
}

function evaluateWorkflowCoverage(coverage, expectations) {
    const minimums = expectations && expectations.minimums ? expectations.minimums : {}
    const failures = []
    failures.push(...coverageFailuresForGroup(coverage.byKind, minimums.kinds, "kinds"))
    failures.push(...coverageFailuresForGroup(coverage.byCategory, minimums.categories, "categories"))
    failures.push(...coverageFailuresForGroup(coverage.byTag, minimums.tags, "tags"))
    return {
        ok: failures.length === 0,
        schemaVersion: Number(expectations && expectations.schemaVersion || 1),
        failureCount: failures.length,
        failures,
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

function ratio(numerator, denominator) {
    return Number(denominator) > 0 ? Number(numerator) / Number(denominator) : 0
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

function workflowMetricsDigestFromRecords(records, metricReducer) {
    return {
        overall: compactMetricDigest(records, metricReducer),
        byKind: groupMetricDigests(records, "kind", metricReducer),
        byCategory: groupMetricDigests(records, "category", metricReducer),
        byMode: groupMetricDigests(records, "mode", metricReducer),
    }
}

function workflowMetricsDigest(scripts, metricReducer) {
    return workflowMetricsDigestFromRecords(workflowMetricRecords(scripts), metricReducer)
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

function evaluateWorkflowMetricBudgets(records, budgetPolicy, metricReducer, repoRoot) {
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
            path: path.relative(repoRoot || process.cwd(), budgetPolicy.path),
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
    const metricBudgetChecks = evaluateWorkflowMetricBudgets(metricRecords, budgetPolicy, metricReducer, repoRoot)
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
        metrics: workflowMetricsDigestFromRecords(metricRecords, metricReducer),
        metricBudgetChecks,
        scripts,
    }
    const outDir = path.join(repoRoot, "tests", "artifacts", "drawing_metrics")
    fs.mkdirSync(outDir, { recursive: true })
    fs.writeFileSync(path.join(outDir, "control_workflows_summary.json"), `${JSON.stringify(report, null, 2)}\n`)
    return report
}

module.exports = {
    compactMetricDigest,
    evaluateWorkflowMetricBudgets,
    loadMetricReducer,
    readJson,
    selectWorkflows,
    evaluateWorkflowCoverage,
    workflowCoverage,
    workflowCoverageExpectations,
    workflowBudgetPolicy,
    workflowFilters,
    workflowFixtures,
    workflowMetricRecords,
    workflowMetricsDigest,
    writeWorkflowReport,
}
