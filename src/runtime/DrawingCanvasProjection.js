.pragma library
.import "DrawingCanvasHandles.js" as CanvasHandles

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

function emptyBounds() {
    return {
        ok: false,
        minX: 0,
        minY: 0,
        maxX: 0,
        maxY: 0
    }
}

function includePointInBounds(bounds, x, y) {
    var px = Number(x)
    var py = Number(y)
    if (!Number.isFinite(px) || !Number.isFinite(py)) {
        return bounds
    }
    if (!bounds.ok) {
        bounds.ok = true
        bounds.minX = px
        bounds.maxX = px
        bounds.minY = py
        bounds.maxY = py
        return bounds
    }
    bounds.minX = Math.min(bounds.minX, px)
    bounds.maxX = Math.max(bounds.maxX, px)
    bounds.minY = Math.min(bounds.minY, py)
    bounds.maxY = Math.max(bounds.maxY, py)
    return bounds
}

function boundsIntersects(bounds, minX, minY, maxX, maxY) {
    if (!bounds || !bounds.ok) {
        return false
    }
    return bounds.maxX >= minX && bounds.minX <= maxX && bounds.maxY >= minY && bounds.minY <= maxY
}

function normalizedObjectBounds(object) {
    var result = emptyBounds()
    var kind = String(object && object.kind || "")
    if (kind === "point" || kind === "tone_probe" || kind === "anchor") {
        return includePointInBounds(result, object.x, object.y)
    }
    if (kind === "line" || kind === "glyph_baseline") {
        includePointInBounds(result, object.x1, object.y1)
        return includePointInBounds(result, object.x2, object.y2)
    }
    if (kind === "circle" || kind === "arc") {
        var cx = finiteNumber(object.cx, 0)
        var cy = finiteNumber(object.cy, 0)
        var radius = Math.max(0, finiteNumber(object.radius, 0))
        includePointInBounds(result, cx - radius, cy - radius)
        return includePointInBounds(result, cx + radius, cy + radius)
    }
    if (CanvasHandles.isRectangleLike(kind)) {
        var corners = CanvasHandles.rotatedRectCorners(object)
        for (var cornerIndex = 0; cornerIndex < corners.length; ++cornerIndex) {
            includePointInBounds(result, corners[cornerIndex].x, corners[cornerIndex].y)
        }
        return result
    }
    if (kind === "polyline" || kind === "polygon") {
        var points = asArray(object.points)
        for (var index = 0; index < points.length; ++index) {
            var point = asArray(points[index])
            if (point.length >= 2) {
                includePointInBounds(result, point[0], point[1])
            }
        }
    }
    return result
}

function objectIntersectsBounds(object, minX, minY, maxX, maxY) {
    return boundsIntersects(normalizedObjectBounds(object), minX, minY, maxX, maxY)
}

function selectedObjectIds(doc) {
    return asArray(doc && doc.selected_object_ids).map(function(id) { return String(id || "") })
}

function selectedObject(doc, objectId) {
    var selectedIds = selectedObjectIds(doc)
    if (selectedIds.length > 0) {
        return selectedIds.indexOf(String(objectId || "")) >= 0
    }
    return String(doc && doc.selected_object_id || "") === String(objectId || "")
}

function selectedLayer(doc, layerId) {
    return String(doc && doc.selected_layer_id || "") === String(layerId || "")
}

function combinedSelectionBounds(doc) {
    var selectedIds = selectedObjectIds(doc)
    if (selectedIds.length <= 1) {
        return emptyBounds()
    }
    var selectedBounds = emptyBounds()
    var layers = asArray(doc && doc.layers)
    for (var layerIndex = 0; layerIndex < layers.length; ++layerIndex) {
        var layer = layers[layerIndex] || ({})
        if (layer.visible === false) {
            continue
        }
        var objects = asArray(layer.objects)
        for (var objectIndex = 0; objectIndex < objects.length; ++objectIndex) {
            var object = objects[objectIndex] || ({})
            var objectId = String(object.id || "")
            if (objectId.indexOf("script_") !== 0 || selectedIds.indexOf(objectId) < 0) {
                continue
            }
            var objectBounds = normalizedObjectBounds(object)
            if (!objectBounds.ok) {
                continue
            }
            includePointInBounds(selectedBounds, objectBounds.minX, objectBounds.minY)
            includePointInBounds(selectedBounds, objectBounds.maxX, objectBounds.maxY)
        }
    }
    return selectedBounds
}
