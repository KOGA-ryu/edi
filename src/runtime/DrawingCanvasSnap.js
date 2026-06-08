.pragma library

// Legacy parity reference for the C++ drawing_canvas_core runtime.
// App QML should call drawingCanvasRuntime instead of importing this file.

function finiteNumber(value, fallback) {
    var number = Number(value)
    return Number.isFinite(number) ? number : fallback
}

function clamp01(value) {
    return Math.max(0, Math.min(1, finiteNumber(value, 0)))
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

function boolSetting(settings, name, fallback) {
    if (!settings || settings[name] === undefined || settings[name] === null) {
        return fallback
    }
    return settings[name] === true
}

function normalizedPoint(point) {
    return {
        x: clamp01(point && point.x),
        y: clamp01(point && point.y)
    }
}

function boardSizePx(settings) {
    var board = finiteNumber(settings && settings.boardSizePx, 0)
    if (board > 0) {
        return board
    }
    return Math.max(1, finiteNumber(settings && settings.canvasSizePx, 512))
}

function effectiveGridStepPx(settings) {
    var baseStep = Math.max(1, finiteNumber(settings && settings.gridStepPx, 32))
    var zoom = Math.max(0.1, finiteNumber(settings && settings.zoom, 1.0))
    if (zoom >= 6.0) {
        return Math.max(1, baseStep / 8)
    }
    if (zoom >= 3.0) {
        return Math.max(1, baseStep / 4)
    }
    if (zoom >= 1.65) {
        return Math.max(1, baseStep / 2)
    }
    if (zoom < 0.62) {
        return Math.max(1, baseStep * 2)
    }
    return baseStep
}

function noneSnap(point, settings) {
    var clamped = normalizedPoint(point)
    return {
        x: clamped.x,
        y: clamped.y,
        kind: "none",
        label: "none",
        sourceObjectId: "",
        sourceKind: "",
        stepPx: effectiveGridStepPx(settings)
    }
}

function gridSnap(point, settings) {
    var clamped = normalizedPoint(point)
    var canvasPx = Math.max(1, finiteNumber(settings && settings.canvasSizePx, 512))
    var stepPx = effectiveGridStepPx(settings)
    return {
        x: clamp01(Math.round(clamped.x * canvasPx / stepPx) * stepPx / canvasPx),
        y: clamp01(Math.round(clamped.y * canvasPx / stepPx) * stepPx / canvasPx),
        kind: "grid",
        label: "grid " + String(stepPx) + "px",
        sourceObjectId: "",
        sourceKind: "",
        stepPx: stepPx
    }
}

function pushSnapCandidate(candidates, sourceKind, x, y, objectId) {
    if (!Number.isFinite(Number(x)) || !Number.isFinite(Number(y))) {
        return
    }
    candidates.push({
        x: clamp01(x),
        y: clamp01(y),
        sourceKind: String(sourceKind || ""),
        label: String(sourceKind || ""),
        sourceObjectId: String(objectId || "")
    })
}

function pushLineSnapCandidates(candidates, object, x1, y1, x2, y2, settings) {
    if (boolSetting(settings, "endpointEnabled", true)) {
        pushSnapCandidate(candidates, "endpoint", x1, y1, object.id)
        pushSnapCandidate(candidates, "endpoint", x2, y2, object.id)
    }
    if (boolSetting(settings, "midpointEnabled", true)) {
        pushSnapCandidate(candidates, "midpoint", (x1 + x2) / 2, (y1 + y2) / 2, object.id)
    }
}

function pushRectangleSnapCandidates(candidates, object, settings) {
    var x = finiteNumber(object.x, 0)
    var y = finiteNumber(object.y, 0)
    var width = finiteNumber(object.width, 0)
    var height = finiteNumber(object.height, 0)
    if (width <= 0 || height <= 0) {
        return
    }
    var left = x
    var right = x + width
    var top = y
    var bottom = y + height
    var centerX = (left + right) / 2
    var centerY = (top + bottom) / 2
    if (boolSetting(settings, "vertexEnabled", true)) {
        pushSnapCandidate(candidates, "vertex", left, top, object.id)
        pushSnapCandidate(candidates, "vertex", right, top, object.id)
        pushSnapCandidate(candidates, "vertex", right, bottom, object.id)
        pushSnapCandidate(candidates, "vertex", left, bottom, object.id)
    }
    if (boolSetting(settings, "midpointEnabled", true)) {
        pushSnapCandidate(candidates, "midpoint", centerX, top, object.id)
        pushSnapCandidate(candidates, "midpoint", right, centerY, object.id)
        pushSnapCandidate(candidates, "midpoint", centerX, bottom, object.id)
        pushSnapCandidate(candidates, "midpoint", left, centerY, object.id)
    }
    if (boolSetting(settings, "centerEnabled", true)) {
        pushSnapCandidate(candidates, "center", centerX, centerY, object.id)
    }
}

function pushPolylineSnapCandidates(candidates, object, settings) {
    var points = asArray(object.points)
    if (points.length <= 0) {
        return
    }
    for (var index = 0; index < points.length; ++index) {
        var point = points[index] || [0, 0]
        var x = finiteNumber(point[0], 0)
        var y = finiteNumber(point[1], 0)
        var isEndpoint = index === 0 || index === points.length - 1
        if (isEndpoint && boolSetting(settings, "endpointEnabled", true)) {
            pushSnapCandidate(candidates, "endpoint", x, y, object.id)
        } else if (!isEndpoint && boolSetting(settings, "vertexEnabled", true)) {
            pushSnapCandidate(candidates, "vertex", x, y, object.id)
        }
        if (index > 0 && boolSetting(settings, "midpointEnabled", true)) {
            var previous = points[index - 1] || [0, 0]
            pushSnapCandidate(candidates, "midpoint", (finiteNumber(previous[0], 0) + x) / 2, (finiteNumber(previous[1], 0) + y) / 2, object.id)
        }
    }
}

function pushPolygonSnapCandidates(candidates, object, settings) {
    var points = asArray(object.points)
    if (points.length < 3) {
        return
    }
    var sumX = 0
    var sumY = 0
    for (var index = 0; index < points.length; ++index) {
        var point = points[index] || [0, 0]
        var x = finiteNumber(point[0], 0)
        var y = finiteNumber(point[1], 0)
        sumX += x
        sumY += y
        if (boolSetting(settings, "vertexEnabled", true)) {
            pushSnapCandidate(candidates, "vertex", x, y, object.id)
        }
        if (boolSetting(settings, "midpointEnabled", true)) {
            var next = points[(index + 1) % points.length] || [0, 0]
            pushSnapCandidate(candidates, "midpoint", (x + finiteNumber(next[0], 0)) / 2, (y + finiteNumber(next[1], 0)) / 2, object.id)
        }
    }
    if (boolSetting(settings, "centerEnabled", true)) {
        pushSnapCandidate(candidates, "center", sumX / points.length, sumY / points.length, object.id)
    }
}

function snapCandidatesForObject(object, settings) {
    var candidates = []
    var kind = String(object && object.kind || "")
    if (kind === "grid" || kind === "rect" || kind === "metadata") {
        return candidates
    }
    if (kind === "line" || kind === "glyph_baseline") {
        pushLineSnapCandidates(candidates, object, finiteNumber(object.x1, 0), finiteNumber(object.y1, 0), finiteNumber(object.x2, 0), finiteNumber(object.y2, 0), settings)
    } else if (kind === "point" || kind === "tone_probe" || kind === "anchor") {
        if (boolSetting(settings, "endpointEnabled", true)) {
            pushSnapCandidate(candidates, "endpoint", finiteNumber(object.x, 0), finiteNumber(object.y, 0), object.id)
        }
    } else if (kind === "polyline") {
        pushPolylineSnapCandidates(candidates, object, settings)
    } else if (kind === "circle" || kind === "arc") {
        if (boolSetting(settings, "centerEnabled", true)) {
            pushSnapCandidate(candidates, "center", finiteNumber(object.cx, 0), finiteNumber(object.cy, 0), object.id)
        }
    } else if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
        pushRectangleSnapCandidates(candidates, object, settings)
    } else if (kind === "polygon") {
        pushPolygonSnapCandidates(candidates, object, settings)
    }
    return candidates
}

function snapCandidates(objects, settings) {
    var result = []
    var list = asArray(objects)
    for (var index = 0; index < list.length; ++index) {
        var objectCandidates = snapCandidatesForObject(list[index] || ({}), settings)
        for (var candidateIndex = 0; candidateIndex < objectCandidates.length; ++candidateIndex) {
            result.push(objectCandidates[candidateIndex])
        }
    }
    return result
}

function nearestObjectSnap(point, objects, settings) {
    if (!boolSetting(settings, "objectSnapEnabled", false)) {
        return null
    }
    var clamped = normalizedPoint(point)
    var tolerancePx = Math.max(0, finiteNumber(settings && settings.objectSnapTolerancePx, 14))
    var sizePx = boardSizePx(settings)
    var candidates = snapCandidates(objects, settings)
    var best = null
    var bestDistance = tolerancePx
    for (var index = 0; index < candidates.length; ++index) {
        var candidate = candidates[index]
        var dx = (candidate.x - clamped.x) * sizePx
        var dy = (candidate.y - clamped.y) * sizePx
        var distance = Math.sqrt(dx * dx + dy * dy)
        if (distance <= bestDistance) {
            bestDistance = distance
            best = candidate
        }
    }
    if (!best) {
        return null
    }
    return {
        x: best.x,
        y: best.y,
        kind: "object",
        label: best.label,
        sourceObjectId: best.sourceObjectId,
        sourceKind: best.sourceKind,
        stepPx: effectiveGridStepPx(settings)
    }
}

function resolveSnap(rawPoint, objects, settings) {
    var objectFirst = String(settings && settings.objectPriority || "before_grid") === "before_grid"
    if (objectFirst) {
        var objectSnap = nearestObjectSnap(rawPoint, objects, settings)
        if (objectSnap) {
            return objectSnap
        }
    }
    if (boolSetting(settings, "gridEnabled", false)) {
        return gridSnap(rawPoint, settings)
    }
    if (!objectFirst) {
        var lateObjectSnap = nearestObjectSnap(rawPoint, objects, settings)
        if (lateObjectSnap) {
            return lateObjectSnap
        }
    }
    return noneSnap(rawPoint, settings)
}
