.pragma library

function finiteNumber(value, fallback) {
    var number = Number(value)
    return Number.isFinite(number) ? number : fallback
}

function asArray(value) {
    if (!value) {
        return []
    }
    if (Array.isArray(value)) {
        return value
    }
    if (typeof value.length === "number") {
        var result = []
        for (var index = 0; index < value.length; ++index) {
            result.push(value[index])
        }
        return result
    }
    return []
}

function metricFields() {
    return [
        "durationMs",
        "pointerMoves",
        "controllerMutations",
        "renderRequests",
        "hitTests",
        "snapResolutions",
        "handlePlans",
        "revisionDelta"
    ]
}

function defaultPolicy() {
    return {
        percentile: 0.95,
        outlierMedianMultiplier: 3,
        maxFailures: 3,
        maxOutliers: 3,
        baselineDeltaThreshold: 0
    }
}

function policyValue(policy, name) {
    var fallback = defaultPolicy()[name]
    return finiteNumber(policy && policy[name], fallback)
}

function sortedValues(records, field) {
    var list = asArray(records)
    var values = []
    for (var index = 0; index < list.length; ++index) {
        var value = Number(list[index] && list[index][field])
        if (Number.isFinite(value)) {
            values.push(value)
        }
    }
    values.sort(function(left, right) {
        return left - right
    })
    return values
}

function percentile(values, ratio) {
    var list = asArray(values)
    if (list.length <= 0) {
        return 0
    }
    var clampedRatio = Math.max(0, Math.min(1, finiteNumber(ratio, 0.95)))
    var index = Math.ceil(clampedRatio * list.length) - 1
    return list[Math.max(0, Math.min(list.length - 1, index))]
}

function summarizeField(records, field, policy) {
    var values = sortedValues(records, field)
    if (values.length <= 0) {
        return {
            count: 0,
            min: 0,
            p50: 0,
            p95: 0,
            max: 0,
            mean: 0
        }
    }

    var sum = 0
    for (var index = 0; index < values.length; ++index) {
        sum += values[index]
    }
    return {
        count: values.length,
        min: values[0],
        p50: percentile(values, 0.50),
        p95: percentile(values, policyValue(policy, "percentile")),
        max: values[values.length - 1],
        mean: Math.round((sum / values.length) * 100) / 100
    }
}

function summarizeDistributions(records, fields, policy) {
    var selectedFields = asArray(fields)
    if (selectedFields.length <= 0) {
        selectedFields = metricFields()
    }
    var summary = {}
    for (var index = 0; index < selectedFields.length; ++index) {
        var field = String(selectedFields[index] || "")
        if (field.length > 0) {
            summary[field] = summarizeField(records, field, policy)
        }
    }
    return summary
}

function budgetLimitForField(budget, field) {
    var limits = {
        durationMs: "maxDurationMs",
        pointerMoves: "maxPointerMoves",
        controllerMutations: "maxControllerMutations",
        renderRequests: "maxRenderRequests",
        hitTests: "maxHitTests",
        snapResolutions: "maxSnapResolutions",
        handlePlans: "maxHandlePlans"
    }
    if (field === "revisionDelta" && budget && budget.revisionDelta !== undefined) {
        return {
            kind: "exact",
            value: finiteNumber(budget.revisionDelta, 0)
        }
    }
    var budgetField = limits[field]
    if (budgetField && budget && budget[budgetField] !== undefined) {
        return {
            kind: "max",
            value: finiteNumber(budget[budgetField], 0)
        }
    }
    return {
        kind: "none",
        value: 0
    }
}

function sampleId(record, index) {
    var explicitId = String(record && (record.sampleId || record.id || record.name) || "")
    return explicitId.length > 0 ? explicitId : "sample_" + String(index)
}

function budgetFailureMessage(field, limit, actual) {
    if (limit.kind === "exact") {
        return field + " expected " + String(limit.value) + ", got " + String(actual)
    }
    return field + " expected <= " + String(limit.value) + ", got " + String(actual)
}

function findBudgetFailures(records, budget, fields, policy) {
    var list = asArray(records)
    var selectedFields = asArray(fields)
    if (selectedFields.length <= 0) {
        selectedFields = metricFields()
    }
    var failures = []
    var maxFailures = Math.max(1, Math.floor(policyValue(policy, "maxFailures")))

    for (var recordIndex = 0; recordIndex < list.length; ++recordIndex) {
        var record = list[recordIndex] || ({})
        if (budget && budget.mode !== undefined && String(record.mode || "") !== String(budget.mode || "")) {
            failures.push({
                sampleId: sampleId(record, recordIndex),
                index: recordIndex,
                field: "mode",
                expected: String(budget.mode || ""),
                actual: String(record.mode || ""),
                message: "mode expected " + String(budget.mode || "") + ", got " + String(record.mode || "")
            })
        }
        for (var fieldIndex = 0; fieldIndex < selectedFields.length; ++fieldIndex) {
            var field = String(selectedFields[fieldIndex] || "")
            var limit = budgetLimitForField(budget, field)
            if (limit.kind === "none") {
                continue
            }
            var actual = finiteNumber(record[field], 0)
            var failed = limit.kind === "exact" ? actual !== limit.value : actual > limit.value
            if (failed) {
                failures.push({
                    sampleId: sampleId(record, recordIndex),
                    index: recordIndex,
                    field: field,
                    budget: limit.value,
                    actual: actual,
                    message: budgetFailureMessage(field, limit, actual)
                })
            }
            if (failures.length >= maxFailures) {
                return failures
            }
        }
        if (failures.length >= maxFailures) {
            return failures
        }
    }
    return failures
}

function findOutliers(records, fields, policy) {
    var list = asArray(records)
    var selectedFields = asArray(fields)
    if (selectedFields.length <= 0) {
        selectedFields = metricFields()
    }
    var outliers = []
    var maxOutliers = Math.max(1, Math.floor(policyValue(policy, "maxOutliers")))
    var multiplier = Math.max(1, policyValue(policy, "outlierMedianMultiplier"))

    for (var fieldIndex = 0; fieldIndex < selectedFields.length; ++fieldIndex) {
        var field = String(selectedFields[fieldIndex] || "")
        var values = sortedValues(list, field)
        if (values.length <= 0) {
            continue
        }
        var median = percentile(values, 0.50)
        var threshold = median <= 0 ? values[values.length - 1] : median * multiplier
        for (var recordIndex = 0; recordIndex < list.length; ++recordIndex) {
            var actual = finiteNumber(list[recordIndex] && list[recordIndex][field], Number.NaN)
            if (Number.isFinite(actual) && median > 0 && actual > threshold) {
                outliers.push({
                    sampleId: sampleId(list[recordIndex], recordIndex),
                    index: recordIndex,
                    field: field,
                    median: median,
                    actual: actual,
                    threshold: threshold
                })
                if (outliers.length >= maxOutliers) {
                    return outliers
                }
            }
        }
    }
    return outliers
}

function baselineSummaryValue(baseline, field, metric) {
    if (!baseline) {
        return Number.NaN
    }
    var source = baseline.summary ? baseline.summary : baseline
    return finiteNumber(source && source[field] && source[field][metric], Number.NaN)
}

function compareToBaseline(summary, baseline, fields, policy) {
    var selectedFields = asArray(fields)
    if (selectedFields.length <= 0) {
        selectedFields = metricFields()
    }
    var deltas = []
    var threshold = Math.max(0, policyValue(policy, "baselineDeltaThreshold"))
    for (var index = 0; index < selectedFields.length; ++index) {
        var field = String(selectedFields[index] || "")
        var baselineP95 = baselineSummaryValue(baseline, field, "p95")
        var currentP95 = finiteNumber(summary && summary[field] && summary[field].p95, Number.NaN)
        if (!Number.isFinite(baselineP95) || !Number.isFinite(currentP95)) {
            continue
        }
        var delta = currentP95 - baselineP95
        if (delta > threshold) {
            deltas.push({
                field: field,
                metric: "p95",
                baseline: baselineP95,
                current: currentP95,
                delta: delta
            })
        }
    }
    return deltas
}

function reduceMetrics(records, budget, baseline, policy) {
    var list = asArray(records)
    var fields = metricFields()
    var summary = summarizeDistributions(list, fields, policy)
    var failures = findBudgetFailures(list, budget, fields, policy)
    var outliers = findOutliers(list, fields, policy)
    var deltas = compareToBaseline(summary, baseline, fields, policy)
    return {
        ok: failures.length === 0,
        samples: list.length,
        failures: failures,
        outliers: outliers,
        summary: summary,
        deltas: deltas
    }
}
