.pragma library

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

function isRectangleLike(kind) {
    return kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region"
}

function canvasSizePx(settings) {
    return Math.max(1, finiteNumber(settings && settings.canvasSizePx, 512))
}

function rotateHandleOffsetPx(settings) {
    return Math.max(1, finiteNumber(settings && settings.rotateHandleOffsetPx, 28))
}

function updatePlan(updates) {
    return {
        ok: true,
        updates: updates
    }
}

function emptyUpdatePlan() {
    return {
        ok: false,
        updates: []
    }
}

function normalizedToPx(value, settings) {
    return Math.round(clamp01(value) * canvasSizePx(settings) * 1000) / 1000
}

function rawNormalizedToPx(value, settings) {
    return Math.round(finiteNumber(value, 0) * canvasSizePx(settings) * 1000) / 1000
}

function roundDegrees(value) {
    return Math.round(finiteNumber(value, 0) * 1000) / 1000
}

function rotatedRectCenter(object) {
    return {
        x: finiteNumber(object && object.x, 0) + finiteNumber(object && object.width, 0) / 2,
        y: finiteNumber(object && object.y, 0) + finiteNumber(object && object.height, 0) / 2
    }
}

function rotatedRectCorners(object) {
    var x = finiteNumber(object && object.x, 0)
    var y = finiteNumber(object && object.y, 0)
    var width = finiteNumber(object && object.width, 0)
    var height = finiteNumber(object && object.height, 0)
    var cx = x + width / 2
    var cy = y + height / 2
    var angle = finiteNumber(object && object.rotation_deg, 0) * Math.PI / 180
    var cosA = Math.cos(angle)
    var sinA = Math.sin(angle)
    var source = [
        { id: "rect_nw", role: "corner", cursor: "resize", field: "x_y_width_height", updateFields: ["x_px", "y_px", "width_px", "height_px"], x: x, y: y },
        { id: "rect_ne", role: "corner", cursor: "resize", field: "x_y_width_height", updateFields: ["x_px", "y_px", "width_px", "height_px"], x: x + width, y: y },
        { id: "rect_sw", role: "corner", cursor: "resize", field: "x_y_width_height", updateFields: ["x_px", "y_px", "width_px", "height_px"], x: x, y: y + height },
        { id: "rect_se", role: "corner", cursor: "resize", field: "x_y_width_height", updateFields: ["x_px", "y_px", "width_px", "height_px"], x: x + width, y: y + height }
    ]
    var result = []
    for (var index = 0; index < source.length; ++index) {
        var dx = source[index].x - cx
        var dy = source[index].y - cy
        result.push({
            id: source[index].id,
            role: source[index].role,
            cursor: source[index].cursor,
            field: source[index].field,
            updateFields: source[index].updateFields,
            x: cx + dx * cosA - dy * sinA,
            y: cy + dx * sinA + dy * cosA
        })
    }
    return result
}

function rotatedRectTopMidpoint(object) {
    var corners = rotatedRectCorners(object)
    if (corners.length < 2) {
        return rotatedRectCenter(object)
    }
    return {
        x: (finiteNumber(corners[0].x, 0) + finiteNumber(corners[1].x, 0)) / 2,
        y: (finiteNumber(corners[0].y, 0) + finiteNumber(corners[1].y, 0)) / 2
    }
}

function rotatedRectRotationHandle(object, settings) {
    var center = rotatedRectCenter(object)
    var top = rotatedRectTopMidpoint(object)
    var dx = top.x - center.x
    var dy = top.y - center.y
    var length = Math.max(0.000001, Math.sqrt(dx * dx + dy * dy))
    var offset = rotateHandleOffsetPx(settings) / canvasSizePx(settings)
    return {
        id: "rect_rotate",
        role: "rotate",
        cursor: "rotate",
        field: "rotation_deg",
        updateFields: ["rotation_deg"],
        x: top.x + dx / length * offset,
        y: top.y + dy / length * offset,
        anchorX: top.x,
        anchorY: top.y
    }
}

function unrotatePointForRect(object, x, y) {
    var center = rotatedRectCenter(object)
    var angle = -finiteNumber(object && object.rotation_deg, 0) * Math.PI / 180
    var dx = finiteNumber(x, 0) - center.x
    var dy = finiteNumber(y, 0) - center.y
    var cosA = Math.cos(angle)
    var sinA = Math.sin(angle)
    return {
        x: center.x + dx * cosA - dy * sinA,
        y: center.y + dx * sinA + dy * cosA
    }
}

function pointHandles(object) {
    return [
        {
            id: "point_position",
            role: "point",
            cursor: "move",
            field: "x_y",
            updateFields: ["x_px", "y_px"],
            x: clamp01(object && object.x),
            y: clamp01(object && object.y)
        }
    ]
}

function lineHandles(object) {
    return [
        {
            id: "line_start",
            role: "endpoint",
            cursor: "resize",
            field: "x1_y1",
            updateFields: ["x1_px", "y1_px"],
            x: clamp01(object && object.x1),
            y: clamp01(object && object.y1)
        },
        {
            id: "line_end",
            role: "endpoint",
            cursor: "resize",
            field: "x2_y2",
            updateFields: ["x2_px", "y2_px"],
            x: clamp01(object && object.x2),
            y: clamp01(object && object.y2)
        }
    ]
}

function circleHandles(object) {
    var cx = clamp01(object && object.cx)
    var cy = clamp01(object && object.cy)
    var radius = Math.max(0, finiteNumber(object && object.radius, 0))
    return [
        {
            id: "circle_center",
            role: "center",
            cursor: "move",
            field: "cx_cy",
            updateFields: ["cx_px", "cy_px"],
            x: cx,
            y: cy
        },
        {
            id: "circle_radius",
            role: "radius",
            cursor: "resize",
            field: "radius",
            updateFields: ["radius_px"],
            x: clamp01(cx + radius),
            y: cy
        }
    ]
}

function rectangleHandles(object, settings) {
    var handles = rotatedRectCorners(object)
    handles.push(rotatedRectRotationHandle(object, settings))
    return handles
}

function readOnlyVertexHandles(object) {
    var points = asArray(object && object.points)
    var handles = []
    for (var index = 0; index < points.length; ++index) {
        var point = asArray(points[index])
        if (point.length < 2) {
            continue
        }
        handles.push({
            id: "vertex_" + String(index),
            role: "vertex",
            cursor: "default",
            field: "",
            updateFields: [],
            x: clamp01(point[0]),
            y: clamp01(point[1]),
            readOnly: true
        })
    }
    return handles
}

function handlesForObject(object, settings) {
    var kind = String(object && object.kind || "")
    if (kind === "point" || kind === "tone_probe") {
        return pointHandles(object)
    }
    if (kind === "line" || kind === "glyph_baseline") {
        return lineHandles(object)
    }
    if (kind === "circle" || kind === "arc") {
        return circleHandles(object)
    }
    if (isRectangleLike(kind)) {
        return rectangleHandles(object, settings)
    }
    if (kind === "polyline" || kind === "polygon") {
        return readOnlyVertexHandles(object)
    }
    return []
}

function visibleHandlesForObject(object, settings) {
    var handles = handlesForObject(object, settings)
    var visible = []
    for (var index = 0; index < handles.length; ++index) {
        if (handles[index] && handles[index].visible === false) {
            continue
        }
        visible.push(handles[index])
    }
    return visible
}

function handleById(object, handleId, settings) {
    var targetId = String(handleId || "")
    var handles = handlesForObject(object, settings)
    for (var index = 0; index < handles.length; ++index) {
        if (String(handles[index].id || "") === targetId) {
            return handles[index]
        }
    }
    return {}
}

function hitHandleAt(object, screenX, screenY, viewportBounds, settings) {
    var handles = visibleHandlesForObject(object, settings)
    var bounds = viewportBounds || {}
    var boundsX = finiteNumber(bounds.x, 0)
    var boundsY = finiteNumber(bounds.y, 0)
    var boundsSize = Math.max(0.000001, finiteNumber(bounds.size, 1))
    var best = {
        ok: false,
        id: "",
        kind: "none",
        handle: {},
        distance: 999
    }
    for (var index = 0; index < handles.length; ++index) {
        var handle = handles[index]
        var px = boundsX + finiteNumber(handle.x, 0) * boundsSize
        var py = boundsY + finiteNumber(handle.y, 0) * boundsSize
        var dx = finiteNumber(screenX, 0) - px
        var dy = finiteNumber(screenY, 0) - py
        var distance = Math.sqrt(dx * dx + dy * dy)
        var threshold = String(handle.role || "") === "rotate"
                ? Math.max(0, finiteNumber(settings && settings.rotateHandleHitTolerancePx, 18))
                : Math.max(0, finiteNumber(settings && settings.handleHitTolerancePx, 14))
        if (distance <= threshold && distance <= best.distance) {
            best = {
                ok: true,
                id: String(handle.id || ""),
                kind: "handle",
                handle: handle,
                distance: distance
            }
        }
    }
    return best
}

function angleSnappedDegrees(degrees, increment) {
    var step = Math.max(1, finiteNumber(increment, 15))
    return Math.round(finiteNumber(degrees, 0) / step) * step
}

function rectangleCornerUpdatePlan(object, handleId, point, settings) {
    var left = finiteNumber(object && object.x, 0)
    var top = finiteNumber(object && object.y, 0)
    var right = left + finiteNumber(object && object.width, 0)
    var bottom = top + finiteNumber(object && object.height, 0)
    var localPoint = unrotatePointForRect(object, point && point.x, point && point.y)
    var fixedX = handleId === "rect_nw" || handleId === "rect_sw" ? right : left
    var fixedY = handleId === "rect_nw" || handleId === "rect_ne" ? bottom : top
    var nextLeft = Math.min(fixedX, localPoint.x)
    var nextTop = Math.min(fixedY, localPoint.y)
    var nextWidth = Math.max(1 / canvasSizePx(settings), Math.abs(fixedX - localPoint.x))
    var nextHeight = Math.max(1 / canvasSizePx(settings), Math.abs(fixedY - localPoint.y))
    if (settings && settings.shiftConstrain === true) {
        var aspect = Math.max(0.000001, finiteNumber(object && object.width, 0)) / Math.max(0.000001, finiteNumber(object && object.height, 0))
        if (nextWidth / Math.max(0.000001, nextHeight) > aspect) {
            nextHeight = nextWidth / aspect
        } else {
            nextWidth = nextHeight * aspect
        }
        nextLeft = fixedX < localPoint.x ? fixedX : fixedX - nextWidth
        nextTop = fixedY < localPoint.y ? fixedY : fixedY - nextHeight
    }
    return updatePlan([
        { field: "x_px", value: rawNormalizedToPx(nextLeft, settings) },
        { field: "y_px", value: rawNormalizedToPx(nextTop, settings) },
        { field: "width_px", value: rawNormalizedToPx(nextWidth, settings) },
        { field: "height_px", value: rawNormalizedToPx(nextHeight, settings) }
    ])
}

function rectangleRotateUpdatePlan(object, point, settings) {
    var center = rotatedRectCenter(object)
    var rotation = Math.atan2(finiteNumber(point && point.y, 0) - center.y, finiteNumber(point && point.x, 0) - center.x) * 180 / Math.PI + 90
    var normalizedRotation = ((rotation % 360) + 360) % 360
    if (settings && settings.shiftConstrain === true) {
        normalizedRotation = ((angleSnappedDegrees(normalizedRotation, settings.angleSnapDeg) % 360) + 360) % 360
    }
    return updatePlan([
        { field: "rotation_deg", value: roundDegrees(normalizedRotation) }
    ])
}

function handleUpdatePlan(object, handleId, point, settings) {
    var handle = handleById(object, handleId, settings)
    if (String(handle.id || "").length === 0 || handle.readOnly === true) {
        return emptyUpdatePlan()
    }
    var kind = String(object && object.kind || "")
    var x = clamp01(point && point.x)
    var y = clamp01(point && point.y)
    if ((kind === "point" || kind === "tone_probe") && handleId === "point_position") {
        return updatePlan([
            { field: "x_px", value: normalizedToPx(x, settings) },
            { field: "y_px", value: normalizedToPx(y, settings) }
        ])
    }
    if ((kind === "line" || kind === "glyph_baseline") && handleId === "line_start") {
        return updatePlan([
            { field: "x1_px", value: normalizedToPx(x, settings) },
            { field: "y1_px", value: normalizedToPx(y, settings) }
        ])
    }
    if ((kind === "line" || kind === "glyph_baseline") && handleId === "line_end") {
        return updatePlan([
            { field: "x2_px", value: normalizedToPx(x, settings) },
            { field: "y2_px", value: normalizedToPx(y, settings) }
        ])
    }
    if ((kind === "circle" || kind === "arc") && handleId === "circle_center") {
        return updatePlan([
            { field: "cx_px", value: normalizedToPx(x, settings) },
            { field: "cy_px", value: normalizedToPx(y, settings) }
        ])
    }
    if ((kind === "circle" || kind === "arc") && handleId === "circle_radius") {
        var cx = finiteNumber(object && object.cx, 0)
        var cy = finiteNumber(object && object.cy, 0)
        var dx = x - cx
        var dy = y - cy
        var radius = Math.sqrt(dx * dx + dy * dy)
        return updatePlan([
            { field: "radius_px", value: rawNormalizedToPx(radius, settings) }
        ])
    }
    if (isRectangleLike(kind) && handleId === "rect_rotate") {
        return rectangleRotateUpdatePlan(object, point, settings)
    }
    if (isRectangleLike(kind) && (handleId === "rect_nw" || handleId === "rect_ne" || handleId === "rect_sw" || handleId === "rect_se")) {
        return rectangleCornerUpdatePlan(object, handleId, point, settings)
    }
    return emptyUpdatePlan()
}
