.pragma library
.import "DrawingCanvasMetricReducer.js" as CanvasMetricReducer

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

function cloneValue(value) {
    if (value === undefined || value === null) {
        return value
    }
    return JSON.parse(JSON.stringify(value))
}

function safeArtifactName(name) {
    var raw = String(name || "drawing_metric_report").trim().toLowerCase()
    var result = ""
    var previousUnderscore = false
    for (var index = 0; index < raw.length; ++index) {
        var ch = raw.charAt(index)
        var allowed = (ch >= "a" && ch <= "z") || (ch >= "0" && ch <= "9") || ch === "-"
        if (allowed) {
            result += ch
            previousUnderscore = false
        } else if (!previousUnderscore) {
            result += "_"
            previousUnderscore = true
        }
    }
    result = result.replace(/^_+/, "").replace(/_+$/, "")
    return result.length > 0 ? result : "drawing_metric_report"
}

function reportFileNames(name) {
    var safeName = safeArtifactName(name)
    return {
        rawJsonl: safeName + ".jsonl",
        summaryJson: safeName + ".summary.json"
    }
}

function joinPath(left, right) {
    var base = String(left || "")
    if (base.length <= 0) {
        return String(right || "")
    }
    if (base.charAt(base.length - 1) === "/") {
        return base + String(right || "")
    }
    return base + "/" + String(right || "")
}

function artifactPaths(rootDir, name) {
    var files = reportFileNames(name)
    var root = String(rootDir || "tests/artifacts/drawing_metrics")
    return {
        rawDir: joinPath(root, "raw"),
        reportsDir: joinPath(root, "reports"),
        rawJsonl: joinPath(joinPath(root, "raw"), files.rawJsonl),
        summaryJson: joinPath(joinPath(root, "reports"), files.summaryJson)
    }
}

function buildMetricReport(name, records, budget, baseline, policy) {
    var reduced = CanvasMetricReducer.reduceMetrics(records, budget, baseline, policy)
    var grouped = CanvasMetricReducer.reduceMetricsByMode
            ? CanvasMetricReducer.reduceMetricsByMode(records, budget && budget.modes ? budget.modes : null, baseline && baseline.modes ? baseline.modes : null, policy)
            : ({ modes: {} })
    return {
        name: String(name || "drawing_metric_report"),
        ok: reduced.ok === true,
        samples: reduced.samples,
        failures: cloneValue(reduced.failures) || [],
        outliers: cloneValue(reduced.outliers) || [],
        summary: cloneValue(reduced.summary) || ({}),
        deltas: cloneValue(reduced.deltas) || [],
        modes: cloneValue(grouped.modes) || ({})
    }
}

function buildMetricModeReport(name, records, budgetByMode, baselineByMode, policy) {
    var grouped = CanvasMetricReducer.reduceMetricsByMode(records, budgetByMode, baselineByMode, policy)
    return {
        name: String(name || "drawing_metric_mode_report"),
        ok: grouped.ok === true,
        samples: grouped.samples,
        failures: cloneValue(grouped.failures) || [],
        outliers: cloneValue(grouped.outliers) || [],
        deltas: cloneValue(grouped.deltas) || [],
        modes: cloneValue(grouped.modes) || ({})
    }
}

function rawRecordsJsonl(records) {
    var list = asArray(records)
    var lines = []
    for (var index = 0; index < list.length; ++index) {
        lines.push(JSON.stringify(list[index] || ({})))
    }
    return lines.join("\n") + (lines.length > 0 ? "\n" : "")
}

function parseRawRecordsJsonl(text) {
    var rawLines = String(text || "").split(/\r?\n/)
    var records = []
    for (var index = 0; index < rawLines.length; ++index) {
        var line = rawLines[index].trim()
        if (line.length <= 0) {
            continue
        }
        records.push(JSON.parse(line))
    }
    return records
}

function summaryReportJson(report) {
    return JSON.stringify(report || ({}), null, 2) + "\n"
}
