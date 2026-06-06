import QtQuick

QtObject {
    id: snapResolver

    property var controller: null
    property real boardSizePx: 512

    function numberOr(value, fallback) {
        var number = Number(value)
        return Number.isFinite(number) ? number : fallback
    }

    function effectiveGridStepPx() {
        if (!controller) {
            return 32
        }
        var baseStep = Math.max(1, Number(controller.drawingSnapGridStepPx || 32))
        var zoom = Math.max(0.1, Number(controller.drawingCanvasZoom || 1.0))
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

    function gridSnappedPoint(point) {
        if (!controller || !controller.drawingSnapGridEnabled) {
            return point
        }
        var canvasPx = Math.max(1, Number(controller.drawingCanvasSizePx || 512))
        var stepPx = effectiveGridStepPx()
        return {
            x: Math.max(0, Math.min(1, Math.round(point.x * canvasPx / stepPx) * stepPx / canvasPx)),
            y: Math.max(0, Math.min(1, Math.round(point.y * canvasPx / stepPx) * stepPx / canvasPx)),
            stepPx: stepPx
        }
    }

    function pushSnapCandidate(candidates, kind, label, x, y, objectId) {
        if (!Number.isFinite(Number(x)) || !Number.isFinite(Number(y))) {
            return
        }
        candidates.push({
            kind: kind,
            label: label,
            x: Number(x),
            y: Number(y),
            objectId: String(objectId || "")
        })
    }

    function pushLineSnapCandidates(candidates, object, x1, y1, x2, y2) {
        if (controller.drawingObjectSnapEndpointEnabled) {
            pushSnapCandidate(candidates, "endpoint", "endpoint", x1, y1, object.id)
            pushSnapCandidate(candidates, "endpoint", "endpoint", x2, y2, object.id)
        }
        if (controller.drawingObjectSnapMidpointEnabled) {
            pushSnapCandidate(candidates, "midpoint", "midpoint", (x1 + x2) / 2, (y1 + y2) / 2, object.id)
        }
    }

    function pushRectangleSnapCandidates(candidates, object) {
        var x = numberOr(object.x, 0)
        var y = numberOr(object.y, 0)
        var width = numberOr(object.width, 0)
        var height = numberOr(object.height, 0)
        if (width <= 0 || height <= 0) {
            return
        }
        var left = x
        var right = x + width
        var top = y
        var bottom = y + height
        if (controller.drawingObjectSnapVertexEnabled) {
            pushSnapCandidate(candidates, "vertex", "vertex", left, top, object.id)
            pushSnapCandidate(candidates, "vertex", "vertex", right, top, object.id)
            pushSnapCandidate(candidates, "vertex", "vertex", right, bottom, object.id)
            pushSnapCandidate(candidates, "vertex", "vertex", left, bottom, object.id)
        }
        if (controller.drawingObjectSnapMidpointEnabled) {
            pushSnapCandidate(candidates, "midpoint", "midpoint", (left + right) / 2, top, object.id)
            pushSnapCandidate(candidates, "midpoint", "midpoint", right, (top + bottom) / 2, object.id)
            pushSnapCandidate(candidates, "midpoint", "midpoint", (left + right) / 2, bottom, object.id)
            pushSnapCandidate(candidates, "midpoint", "midpoint", left, (top + bottom) / 2, object.id)
        }
        if (controller.drawingObjectSnapCenterEnabled) {
            pushSnapCandidate(candidates, "center", "center", (left + right) / 2, (top + bottom) / 2, object.id)
        }
    }

    function pushPolylineSnapCandidates(candidates, object) {
        var points = object.points || []
        if (points.length <= 0) {
            return
        }
        for (var index = 0; index < points.length; ++index) {
            var point = points[index] || [0, 0]
            var x = numberOr(point[0], 0)
            var y = numberOr(point[1], 0)
            var isEndpoint = index === 0 || index === points.length - 1
            if (isEndpoint && controller.drawingObjectSnapEndpointEnabled) {
                pushSnapCandidate(candidates, "endpoint", "endpoint", x, y, object.id)
            } else if (!isEndpoint && controller.drawingObjectSnapVertexEnabled) {
                pushSnapCandidate(candidates, "vertex", "vertex", x, y, object.id)
            }
            if (index > 0 && controller.drawingObjectSnapMidpointEnabled) {
                var previous = points[index - 1] || [0, 0]
                pushSnapCandidate(candidates, "midpoint", "midpoint", (numberOr(previous[0], 0) + x) / 2, (numberOr(previous[1], 0) + y) / 2, object.id)
            }
        }
    }

    function pushPolygonSnapCandidates(candidates, object) {
        var points = object.points || []
        if (points.length < 3) {
            return
        }
        var sumX = 0
        var sumY = 0
        for (var index = 0; index < points.length; ++index) {
            var point = points[index] || [0, 0]
            var x = numberOr(point[0], 0)
            var y = numberOr(point[1], 0)
            sumX += x
            sumY += y
            if (controller.drawingObjectSnapVertexEnabled) {
                pushSnapCandidate(candidates, "vertex", "vertex", x, y, object.id)
            }
            if (controller.drawingObjectSnapMidpointEnabled) {
                var next = points[(index + 1) % points.length] || [0, 0]
                pushSnapCandidate(candidates, "midpoint", "midpoint", (x + numberOr(next[0], 0)) / 2, (y + numberOr(next[1], 0)) / 2, object.id)
            }
        }
        if (controller.drawingObjectSnapCenterEnabled) {
            pushSnapCandidate(candidates, "center", "center", sumX / points.length, sumY / points.length, object.id)
        }
    }

    function objectSnapCandidates() {
        var candidates = []
        if (!controller) {
            return candidates
        }
        var objects = controller.drawingCanvasObjects(controller.revision) || []
        for (var index = 0; index < objects.length; ++index) {
            var object = objects[index] || ({})
            var kind = String(object.kind || "")
            if (kind === "grid" || kind === "rect" || kind === "metadata") {
                continue
            }
            if (kind === "line" || kind === "glyph_baseline") {
                pushLineSnapCandidates(candidates, object, numberOr(object.x1, 0), numberOr(object.y1, 0), numberOr(object.x2, 0), numberOr(object.y2, 0))
            } else if (kind === "point" || kind === "tone_probe" || kind === "anchor") {
                if (controller.drawingObjectSnapEndpointEnabled) {
                    pushSnapCandidate(candidates, "endpoint", "endpoint", numberOr(object.x, 0), numberOr(object.y, 0), object.id)
                }
            } else if (kind === "polyline") {
                pushPolylineSnapCandidates(candidates, object)
            } else if (kind === "circle" || kind === "arc") {
                if (controller.drawingObjectSnapCenterEnabled) {
                    pushSnapCandidate(candidates, "center", "center", numberOr(object.cx, 0), numberOr(object.cy, 0), object.id)
                }
            } else if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
                pushRectangleSnapCandidates(candidates, object)
            } else if (kind === "polygon") {
                pushPolygonSnapCandidates(candidates, object)
            }
        }
        return candidates
    }

    function nearestObjectSnap(point) {
        if (!controller || !controller.drawingObjectSnapEnabled) {
            return null
        }
        var tolerancePx = Math.max(1, Number(controller.drawingObjectSnapTolerancePx || 14))
        var candidates = objectSnapCandidates()
        var best = null
        var bestDistance = tolerancePx
        for (var index = 0; index < candidates.length; ++index) {
            var candidate = candidates[index]
            var dx = (candidate.x - point.x) * boardSizePx
            var dy = (candidate.y - point.y) * boardSizePx
            var distance = Math.sqrt(dx * dx + dy * dy)
            if (distance <= bestDistance) {
                bestDistance = distance
                best = candidate
            }
        }
        return best
    }

    function snapResult(point, allowObjectSnap) {
        if (allowObjectSnap) {
            var objectSnap = nearestObjectSnap(point)
            if (objectSnap) {
                return {
                    x: Math.max(0, Math.min(1, objectSnap.x)),
                    y: Math.max(0, Math.min(1, objectSnap.y)),
                    kind: objectSnap.kind,
                    label: objectSnap.label
                }
            }
        }
        var gridPoint = gridSnappedPoint(point)
        if (gridPoint.x !== point.x || gridPoint.y !== point.y) {
            return { x: gridPoint.x, y: gridPoint.y, kind: "grid", label: String(gridPoint.stepPx) + "px", stepPx: gridPoint.stepPx }
        }
        return { x: point.x, y: point.y, kind: "free", label: "free", stepPx: effectiveGridStepPx() }
    }

    function snappedPoint(point) {
        return snapResult(point, true)
    }
}
