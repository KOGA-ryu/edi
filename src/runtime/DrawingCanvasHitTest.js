.pragma library

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

function distanceToSegment(px, py, ax, ay, bx, by) {
    var dx = bx - ax
    var dy = by - ay
    var lengthSq = dx * dx + dy * dy
    if (lengthSq <= 0.0000001) {
        var pointDx = px - ax
        var pointDy = py - ay
        return Math.sqrt(pointDx * pointDx + pointDy * pointDy)
    }
    var t = Math.max(0, Math.min(1, ((px - ax) * dx + (py - ay) * dy) / lengthSq))
    var hitX = ax + t * dx
    var hitY = ay + t * dy
    var hitDx = px - hitX
    var hitDy = py - hitY
    return Math.sqrt(hitDx * hitDx + hitDy * hitDy)
}

function pointInPolygon(px, py, points) {
    var inside = false
    var list = asArray(points)
    for (var index = 0, prev = list.length - 1; index < list.length; prev = index++) {
        var current = list[index] || [0, 0]
        var previous = list[prev] || [0, 0]
        var x1 = Number(current[0] || 0)
        var y1 = Number(current[1] || 0)
        var x2 = Number(previous[0] || 0)
        var y2 = Number(previous[1] || 0)
        if (((y1 > py) !== (y2 > py)) && (px < (x2 - x1) * (py - y1) / Math.max(0.0000001, y2 - y1) + x1)) {
            inside = !inside
        }
    }
    return inside
}

function angleInArc(angleDeg, startDeg, endDeg) {
    var angle = ((angleDeg % 360) + 360) % 360
    var start = ((Number(startDeg || 0) % 360) + 360) % 360
    var end = ((Number(endDeg || 0) % 360) + 360) % 360
    if (start <= end) {
        return angle >= start && angle <= end
    }
    return angle >= start || angle <= end
}

function objectHitScore(object, x, y, tolerance) {
    var kind = String(object.kind || "")
    if (kind === "point" || kind === "tone_probe") {
        var pointDx = Number(object.x || 0) - x
        var pointDy = Number(object.y || 0) - y
        return Math.sqrt(pointDx * pointDx + pointDy * pointDy)
    }
    if (kind === "line" || kind === "glyph_baseline") {
        return distanceToSegment(x, y, Number(object.x1 || 0), Number(object.y1 || 0), Number(object.x2 || 0), Number(object.y2 || 0))
    }
    if (kind === "polyline") {
        var polyline = asArray(object.points)
        var bestLine = 999
        for (var lineIndex = 1; lineIndex < polyline.length; ++lineIndex) {
            var a = polyline[lineIndex - 1] || [0, 0]
            var b = polyline[lineIndex] || [0, 0]
            bestLine = Math.min(bestLine, distanceToSegment(x, y, Number(a[0] || 0), Number(a[1] || 0), Number(b[0] || 0), Number(b[1] || 0)))
        }
        return bestLine
    }
    if (kind === "circle" || kind === "arc") {
        var circleDx = x - Number(object.cx || 0)
        var circleDy = y - Number(object.cy || 0)
        var radiusDelta = Math.abs(Math.sqrt(circleDx * circleDx + circleDy * circleDy) - Number(object.radius || 0))
        if (kind === "arc") {
            var angle = Math.atan2(circleDy, circleDx) * 180 / Math.PI
            if (!angleInArc(angle, object.start_angle_deg, object.end_angle_deg)) {
                return 999
            }
        }
        return radiusDelta
    }
    if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
        var left = Number(object.x || 0)
        var top = Number(object.y || 0)
        var right = left + Number(object.width || 0)
        var bottom = top + Number(object.height || 0)
        if (x >= left && x <= right && y >= top && y <= bottom) {
            return 0
        }
        var clampedX = Math.max(left, Math.min(right, x))
        var clampedY = Math.max(top, Math.min(bottom, y))
        var rectDx = x - clampedX
        var rectDy = y - clampedY
        return Math.sqrt(rectDx * rectDx + rectDy * rectDy)
    }
    if (kind === "polygon") {
        var polygon = asArray(object.points)
        if (pointInPolygon(x, y, polygon)) {
            return 0
        }
        var bestEdge = 999
        for (var polyIndex = 0; polyIndex < polygon.length; ++polyIndex) {
            var p1 = polygon[polyIndex] || [0, 0]
            var p2 = polygon[(polyIndex + 1) % polygon.length] || [0, 0]
            bestEdge = Math.min(bestEdge, distanceToSegment(x, y, Number(p1[0] || 0), Number(p1[1] || 0), Number(p2[0] || 0), Number(p2[1] || 0)))
        }
        return bestEdge
    }
    return 999
}
