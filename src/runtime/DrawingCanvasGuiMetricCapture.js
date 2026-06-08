.pragma library
.import "DrawingCanvasInteractionReplay.js" as CanvasInteractionReplay
.import "DrawingCanvasMetricReport.js" as CanvasMetricReport

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

function telemetryMarker() {
    return "drawing_canvas_interaction_events "
}

function markerValue(marker) {
    var value = String(marker || "")
    return value.length > 0 ? value : telemetryMarker()
}

function lineText(value) {
    return String(value === undefined || value === null ? "" : value)
}

function parseTelemetryLine(line, marker) {
    var text = lineText(line)
    var prefix = markerValue(marker)
    var markerIndex = text.indexOf(prefix)
    if (markerIndex < 0) {
        return {
            ok: true,
            found: false,
            events: [],
            failures: []
        }
    }

    var payload = text.slice(markerIndex + prefix.length).trim()
    if (payload.length <= 0) {
        return {
            ok: false,
            found: true,
            events: [],
            failures: ["telemetry line missing JSON payload"]
        }
    }

    try {
        var parsed = JSON.parse(payload)
        if (!Array.isArray(parsed)) {
            return {
                ok: false,
                found: true,
                events: [],
                failures: ["telemetry payload must be an event array"]
            }
        }
        return {
            ok: true,
            found: true,
            events: parsed,
            failures: []
        }
    } catch (error) {
        return {
            ok: false,
            found: true,
            events: [],
            failures: ["telemetry JSON parse failed: " + String(error && error.message || error)]
        }
    }
}

function splitLines(text) {
    return lineText(text).split(/\r?\n/)
}

function parseTelemetryLines(text, marker) {
    var lines = splitLines(text)
    var streams = []
    var failures = []
    for (var index = 0; index < lines.length; ++index) {
        var result = parseTelemetryLine(lines[index], marker)
        if (!result.found) {
            continue
        }
        if (result.ok) {
            streams.push({
                sampleId: "capture_" + String(streams.length + 1),
                lineIndex: index,
                events: result.events
            })
        } else {
            for (var failureIndex = 0; failureIndex < result.failures.length; ++failureIndex) {
                failures.push({
                    lineIndex: index,
                    message: result.failures[failureIndex]
                })
            }
        }
    }
    return {
        ok: failures.length === 0,
        streams: streams,
        failures: failures
    }
}

function replayCapturedStreams(streams) {
    var list = asArray(streams)
    var records = []
    var failures = []
    for (var index = 0; index < list.length; ++index) {
        var stream = list[index] || ({})
        var result = CanvasInteractionReplay.replayEvents(stream.events)
        if (result.ok) {
            var record = result.record || ({})
            record.sampleId = String(stream.sampleId || "capture_" + String(index + 1))
            records.push(record)
        } else {
            failures.push({
                sampleId: String(stream.sampleId || "capture_" + String(index + 1)),
                lineIndex: stream.lineIndex,
                failures: result.failures
            })
        }
    }
    return {
        ok: failures.length === 0,
        records: records,
        failures: failures
    }
}

function capturedMetricRecords(text, marker) {
    var parsed = parseTelemetryLines(text, marker)
    if (!parsed.ok) {
        return {
            ok: false,
            records: [],
            failures: parsed.failures
        }
    }
    var replayed = replayCapturedStreams(parsed.streams)
    return {
        ok: replayed.ok,
        records: replayed.records,
        failures: replayed.failures
    }
}

function buildCaptureReport(name, text, budget, baseline, policy, marker) {
    var captured = capturedMetricRecords(text, marker)
    if (!captured.ok) {
        return {
            name: String(name || "gui_metric_capture"),
            ok: false,
            samples: captured.records.length,
            failures: captured.failures,
            outliers: [],
            summary: {},
            deltas: []
        }
    }
    return CanvasMetricReport.buildMetricReport(name, captured.records, budget, baseline, policy)
}
