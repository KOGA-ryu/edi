import QtQuick
import "../../style"
import "../../runtime/DrawingCanvasViewport.js" as CanvasViewport

QtObject {
    id: canvasObjectRenderer

    property var controller: null
    property string hoverObjectId: ""
    property string hoverHandleId: ""

    function asArray(value) {
        if (!value) {
            return []
        }
        if (Array.isArray(value)) {
            return value
        }
        if (typeof value.length === "number") {
            var values = []
            for (var index = 0; index < value.length; ++index) {
                values.push(value[index])
            }
            return values
        }
        return []
    }

    function clampOpacity(value) {
        var opacity = Number(value)
        if (!Number.isFinite(opacity)) {
            return 1
        }
        return Math.max(0, Math.min(1, opacity))
    }

    function clampThickness(value) {
        var thickness = Number(value)
        if (!Number.isFinite(thickness)) {
            return 2
        }
        return Math.max(1, Math.min(18, Math.round(Number(thickness) * 10) / 10))
    }

    function normalizeLineStyle(value) {
        var style = String(value || "solid").trim().toLowerCase()
        if (style === "dashed" || style === "dot" || style === "dotted") {
            return style === "dot" ? "dotted" : style
        }
        return "solid"
    }

    function objectNumeric(value, fallback) {
        var numeric = Number(value)
        return Number.isFinite(numeric) ? numeric : fallback
    }

    function selectedOrLayerStyleColor(objectColor, defaultColor) {
        if (String(objectColor || "").trim().length > 0) {
            return String(objectColor)
        }
        return String(defaultColor || "")
    }

    function styleStrokeColor(object) {
        var objectColor = selectedOrLayerStyleColor(object.stroke_color || object.strokeColor, "")
        return objectColor.length > 0 ? objectColor : String(canvasObjectRenderer.controller && canvasObjectRenderer.controller.drawingStrokeColor || "#f4d46f")
    }

    function styleFillColor(object) {
        var fillColor = selectedOrLayerStyleColor(object.fill_color || object.fillColor, "")
        if (fillColor.length > 0) {
            return fillColor
        }
        if (canvasObjectRenderer.controller && String(canvasObjectRenderer.controller.drawingFillColor || "").length > 0) {
            return String(canvasObjectRenderer.controller.drawingFillColor)
        }
        return ""
    }

    function styleLineThickness(object, selected, layerSelected, hovered) {
        var thickness = objectNumeric(object.line_thickness, Number.NEGATIVE_INFINITY)
        if (!Number.isFinite(thickness)) {
            thickness = objectNumeric(object.lineWidth, Number.NEGATIVE_INFINITY)
        }
        if (!Number.isFinite(thickness)) {
            thickness = objectNumeric(object.strokeWidth, Number.NEGATIVE_INFINITY)
        }
        if (!Number.isFinite(thickness)) {
            thickness = objectNumeric(canvasObjectRenderer.controller && canvasObjectRenderer.controller.drawingLineThickness, 2)
        }
        var scaled = clampThickness(thickness)
        if (selected) {
            return Math.max(1.4, scaled + 0.65)
        }
        if (hovered) {
            return Math.max(1.2, scaled + 0.35)
        }
        if (layerSelected) {
            return Math.max(1.25, scaled + 0.25)
        }
        return scaled
    }

    function styleLineOpacity(object) {
        var rawOpacity = Object.prototype.hasOwnProperty.call(object, "stroke_opacity") ? object.stroke_opacity : object.opacity
        var opacity = objectNumeric(rawOpacity, Number.NEGATIVE_INFINITY)
        if (!Number.isFinite(opacity)) {
            opacity = objectNumeric(canvasObjectRenderer.controller && canvasObjectRenderer.controller.drawingStrokeOpacity, 1)
        }
        return clampOpacity(opacity)
    }

    function styleLineDash(object, objectSelected) {
        var style = normalizeLineStyle(object.line_style || object.style || (canvasObjectRenderer.controller ? canvasObjectRenderer.controller.drawingLineStyle : "solid"))
        if (style === "dashed") {
            return [8, 5]
        }
        if (style === "dotted") {
            return [2, 3]
        }
        if (objectSelected && style === "solid") {
            return []
        }
        return []
    }

    function applyFill(ctx, object, closedShape) {
        if (!closedShape) {
            return
        }
        var fillColor = styleFillColor(object)
        if (!fillColor.length) {
            return
        }
        var fillOpacity = styleLineOpacity(object)
        ctx.save()
        ctx.globalAlpha = fillOpacity
        ctx.fillStyle = fillColor
        ctx.fill()
        ctx.restore()
    }

    function beginStyle(ctx, object, selected, layerSelected, hovered) {
        var baseStroke = styleStrokeColor(object)
        var strokeColor = selected
                ? UiStyle.mix(baseStroke, UiStyle.colorWarning, 0.42)
                : hovered
                    ? UiStyle.mix(baseStroke, UiStyle.colorAccent, 0.42)
                    : layerSelected
                    ? UiStyle.mix(baseStroke, UiStyle.colorAccent, 0.28)
                    : baseStroke
        ctx.save()
        ctx.strokeStyle = strokeColor
        ctx.lineWidth = styleLineThickness(object, selected, layerSelected, hovered)
        ctx.setLineDash(styleLineDash(object, selected))
        ctx.globalAlpha = styleLineOpacity(object)
    }

    function endStyle(ctx) {
        ctx.restore()
    }

    property var objectRendererByKind: ({
        rect: "drawRect",
        grid: "drawGrid",
        anchor: "drawAnchor",
        line: "drawLine",
        glyph_baseline: "drawLine",
        point: "drawPoint",
        tone_probe: "drawPoint",
        polyline: "drawPolyline",
        circle: "drawCirclePrimitive",
        arc: "drawArcPrimitive",
        rectangle: "drawRectanglePrimitive",
        image_reference_frame: "drawRectanglePrimitive",
        ascii_crop_frame: "drawRectanglePrimitive",
        ascii_cell_region: "drawRectanglePrimitive",
        polygon: "drawPolygonPrimitive"
    })

    function pxX(bounds, normalizedX) {
        return CanvasViewport.canvasToScreenX(bounds, normalizedX)
    }

    function pxY(bounds, normalizedY) {
        return CanvasViewport.canvasToScreenY(bounds, normalizedY)
    }

    function pointFromArray(value, fallbackX, fallbackY) {
        var point = asArray(value)
        return {
            x: point.length > 0 ? Number(point[0]) : Number(fallbackX || 0),
            y: point.length > 1 ? Number(point[1]) : Number(fallbackY || 0)
        }
    }

    function rotatedRectCorners(object) {
        var x = Number(object.x || 0)
        var y = Number(object.y || 0)
        var width = Number(object.width || 0)
        var height = Number(object.height || 0)
        var cx = x + width / 2
        var cy = y + height / 2
        var angle = Number(object.rotation_deg || 0) * Math.PI / 180
        var cosA = Math.cos(angle)
        var sinA = Math.sin(angle)
        var source = [
            { id: "rect_nw", x: x, y: y },
            { id: "rect_ne", x: x + width, y: y },
            { id: "rect_sw", x: x, y: y + height },
            { id: "rect_se", x: x + width, y: y + height }
        ]
        var result = []
        for (var index = 0; index < source.length; ++index) {
            var dx = source[index].x - cx
            var dy = source[index].y - cy
            result.push({
                id: source[index].id,
                x: cx + dx * cosA - dy * sinA,
                y: cy + dx * sinA + dy * cosA
            })
        }
        return result
    }

    function rotatedRectCenter(object) {
        return {
            x: Number(object.x || 0) + Number(object.width || 0) / 2,
            y: Number(object.y || 0) + Number(object.height || 0) / 2
        }
    }

    function rotatedRectTopMidpoint(object) {
        var corners = rotatedRectCorners(object)
        if (corners.length < 2) {
            return rotatedRectCenter(object)
        }
        return {
            x: (Number(corners[0].x || 0) + Number(corners[1].x || 0)) / 2,
            y: (Number(corners[0].y || 0) + Number(corners[1].y || 0)) / 2
        }
    }

    function rotatedRectRotationHandle(object) {
        var center = rotatedRectCenter(object)
        var top = rotatedRectTopMidpoint(object)
        var dx = top.x - center.x
        var dy = top.y - center.y
        var length = Math.max(0.000001, Math.sqrt(dx * dx + dy * dy))
        var canvasPx = Math.max(1, Number(canvasObjectRenderer.controller ? canvasObjectRenderer.controller.drawingCanvasSizePx : 512))
        var offset = 28 / canvasPx
        return {
            id: "rect_rotate",
            role: "rotate",
            x: top.x + dx / length * offset,
            y: top.y + dy / length * offset,
            anchorX: top.x,
            anchorY: top.y
        }
    }

    function objectEditHandles(object) {
        var kind = String(object.kind || "")
        if (kind === "line" || kind === "glyph_baseline") {
            return [
                { id: "line_start", x: Number(object.x1 || 0), y: Number(object.y1 || 0) },
                { id: "line_end", x: Number(object.x2 || 0), y: Number(object.y2 || 0) }
            ]
        }
        if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
            var handles = rotatedRectCorners(object)
            handles.push(rotatedRectRotationHandle(object))
            return handles
        }
        if (kind === "circle" || kind === "arc") {
            var center = pointFromArray(object.center_px, Number(object.cx || 0) * 512, Number(object.cy || 0) * 512)
            var canvasPx = Math.max(1, Number(canvasObjectRenderer.controller ? canvasObjectRenderer.controller.drawingCanvasSizePx : 512))
            var cx = Number(object.cx || (center.x / canvasPx))
            var cy = Number(object.cy || (center.y / canvasPx))
            var radius = Number(object.radius || 0)
            return [
                { id: "circle_center", x: cx, y: cy },
                { id: "circle_radius", x: Math.min(1, cx + radius), y: cy }
            ]
        }
        return []
    }

    function drawEditHandle(ctx, bounds, handle, primary) {
        var x = pxX(bounds, handle.x)
        var y = pxY(bounds, handle.y)
        var hovered = String(handle.id || "") === canvasObjectRenderer.hoverHandleId
        var rotateHandle = String(handle.role || "") === "rotate"
        var size = rotateHandle ? hovered ? 12 : 10 : hovered ? 12 : primary ? 10 : 8
        ctx.save()
        ctx.fillStyle = hovered || primary ? UiStyle.colorWarning : UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorWarning, 0.72)
        ctx.strokeStyle = UiStyle.colorWorkspace
        ctx.lineWidth = hovered ? 1.5 : 1
        ctx.beginPath()
        if (rotateHandle) {
            ctx.arc(x, y, size / 2, 0, Math.PI * 2)
        } else {
            ctx.rect(x - size / 2, y - size / 2, size, size)
        }
        ctx.fill()
        ctx.stroke()
        ctx.restore()
    }

    function drawRotatedRectSelectionOutline(ctx, bounds, object) {
        var kind = String(object.kind || "")
        if (kind !== "rectangle" && kind !== "image_reference_frame" && kind !== "ascii_crop_frame" && kind !== "ascii_cell_region") {
            return
        }
        var corners = rotatedRectCorners(object)
        if (corners.length < 4) {
            return
        }
        var rotationHandle = rotatedRectRotationHandle(object)
        ctx.save()
        ctx.strokeStyle = UiStyle.colorWarning
        ctx.lineWidth = 1
        ctx.setLineDash([7, 5])
        ctx.globalAlpha = 0.96
        ctx.beginPath()
        ctx.moveTo(pxX(bounds, corners[0].x), pxY(bounds, corners[0].y))
        ctx.lineTo(pxX(bounds, corners[1].x), pxY(bounds, corners[1].y))
        ctx.lineTo(pxX(bounds, corners[3].x), pxY(bounds, corners[3].y))
        ctx.lineTo(pxX(bounds, corners[2].x), pxY(bounds, corners[2].y))
        ctx.closePath()
        ctx.stroke()
        ctx.setLineDash([])
        ctx.globalAlpha = 0.72
        strokeGridLine(ctx, pxX(bounds, rotationHandle.anchorX), pxY(bounds, rotationHandle.anchorY), pxX(bounds, rotationHandle.x), pxY(bounds, rotationHandle.y))
        ctx.restore()
    }

    function drawSelectedEditHandles(ctx, bounds, object) {
        var handles = objectEditHandles(object)
        if (handles.length <= 0 || String(object.id || "").indexOf("script_") !== 0) {
            return
        }
        drawRotatedRectSelectionOutline(ctx, bounds, object)
        for (var index = 0; index < handles.length; ++index) {
            drawEditHandle(ctx, bounds, handles[index], index === 0)
        }
    }

    function selectedObject(doc, objectId) {
        var selectedIds = asArray(doc.selected_object_ids)
        if (selectedIds.length > 0) {
            return selectedIds.indexOf(String(objectId || "")) >= 0
        }
        return String(doc.selected_object_id || "") === String(objectId || "")
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

    function normalizedObjectBounds(object) {
        var result = ({ ok: false, minX: 0, minY: 0, maxX: 0, maxY: 0 })
        var kind = String(object.kind || "")
        if (kind === "point" || kind === "tone_probe") {
            return includePointInBounds(result, object.x, object.y)
        }
        if (kind === "line" || kind === "glyph_baseline") {
            includePointInBounds(result, object.x1, object.y1)
            return includePointInBounds(result, object.x2, object.y2)
        }
        if (kind === "circle" || kind === "arc") {
            var cx = Number(object.cx || 0)
            var cy = Number(object.cy || 0)
            var radius = Number(object.radius || 0)
            includePointInBounds(result, cx - radius, cy - radius)
            return includePointInBounds(result, cx + radius, cy + radius)
        }
        if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
            var corners = rotatedRectCorners(object)
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

    function drawCombinedSelectionOutline(ctx, bounds, doc) {
        var selectedIds = asArray(doc.selected_object_ids)
        if (selectedIds.length <= 1) {
            return
        }
        var selectedBounds = ({ ok: false, minX: 0, minY: 0, maxX: 0, maxY: 0 })
        var layers = asArray(doc.layers)
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
        if (!selectedBounds.ok) {
            return
        }
        var pad = 8
        var x1 = pxX(bounds, selectedBounds.minX) - pad
        var y1 = pxY(bounds, selectedBounds.minY) - pad
        var x2 = pxX(bounds, selectedBounds.maxX) + pad
        var y2 = pxY(bounds, selectedBounds.maxY) + pad
        ctx.save()
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorAccent, UiStyle.colorWarning, 0.28)
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, 0.12)
        ctx.lineWidth = 1
        ctx.setLineDash([7, 5])
        ctx.globalAlpha = 0.34
        ctx.fillRect(x1, y1, Math.max(1, x2 - x1), Math.max(1, y2 - y1))
        ctx.globalAlpha = 0.92
        ctx.strokeRect(x1, y1, Math.max(1, x2 - x1), Math.max(1, y2 - y1))
        ctx.restore()
    }

    function selectedLayer(doc, layerId) {
        return String(doc.selected_layer_id || "") === String(layerId || "")
    }

    function setGridStroke(ctx, layerSelected, major) {
        ctx.lineWidth = layerSelected && major ? 1.35 : major ? 1 : 0.65
        ctx.strokeStyle = layerSelected && major
                ? UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, 0.54)
                : major
                    ? UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.18)
                    : UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.075)
    }

    function strokeGridLine(ctx, x1, y1, x2, y2) {
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.stroke()
    }

    function draftingGridLevels(object) {
        var canvasPx = Math.max(1, Number(canvasObjectRenderer.controller ? canvasObjectRenderer.controller.drawingCanvasSizePx : 512))
        var divisions = Math.max(2, Number(object.divisions || 16))
        var majorEvery = Math.max(1, Number(object.major_every || 4))
        var baseStep = Math.max(1, canvasPx / divisions)
        var majorStep = Math.max(baseStep, baseStep * majorEvery)
        var zoom = Math.max(0.1, Number(canvasObjectRenderer.controller ? canvasObjectRenderer.controller.drawingCanvasZoom : 1.0))
        var levels = [
            { step: majorStep, alpha: 0.24, width: 1.0, major: true, points: false }
        ]
        if (zoom >= 0.62) {
            levels.push({ step: baseStep, alpha: 0.095, width: 0.65, major: false, points: zoom >= 1.25 })
        }
        if (zoom >= 1.65) {
            levels.push({ step: Math.max(1, baseStep / 2), alpha: 0.07, width: 0.48, major: false, points: true })
        }
        if (zoom >= 3.0) {
            levels.push({ step: Math.max(1, baseStep / 4), alpha: 0.052, width: 0.38, major: false, points: true })
        }
        if (zoom >= 6.0) {
            levels.push({ step: Math.max(1, baseStep / 8), alpha: 0.045, width: 0.32, major: false, points: true })
        }
        return levels
    }

    function drawDraftingGridLevel(ctx, bounds, canvasPx, level, layerSelected) {
        var step = Math.max(1, Number(level.step || 32))
        var screenStep = bounds.size * step / canvasPx
        if (screenStep < 3) {
            return
        }
        var lineAlpha = Number(level.alpha || 0.08) + (layerSelected ? 0.055 : 0)
        ctx.lineWidth = Math.max(0.25, Number(level.width || 0.5))
        ctx.strokeStyle = level.major
                ? UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.36 : 0.24)
                : UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, lineAlpha)
        var count = Math.floor(canvasPx / step)
        for (var index = 0; index <= count; ++index) {
            var normalized = Math.min(1, index * step / canvasPx)
            var offset = normalized * bounds.size
            strokeGridLine(ctx, bounds.x + offset, bounds.y, bounds.x + offset, bounds.y + bounds.size)
            strokeGridLine(ctx, bounds.x, bounds.y + offset, bounds.x + bounds.size, bounds.y + offset)
        }
        if (level.points && screenStep >= 12) {
            drawDraftingGridPoints(ctx, bounds, canvasPx, step, screenStep, layerSelected)
        }
    }

    function drawDraftingGridPoints(ctx, bounds, canvasPx, step, screenStep, layerSelected) {
        var count = Math.floor(canvasPx / step)
        var radius = Math.max(0.8, Math.min(2.4, screenStep / 12))
        ctx.save()
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.38 : 0.24)
        ctx.globalAlpha = Math.max(0.18, Math.min(0.52, screenStep / 64))
        for (var xIndex = 0; xIndex <= count; ++xIndex) {
            var x = bounds.x + Math.min(1, xIndex * step / canvasPx) * bounds.size
            for (var yIndex = 0; yIndex <= count; ++yIndex) {
                var y = bounds.y + Math.min(1, yIndex * step / canvasPx) * bounds.size
                ctx.beginPath()
                ctx.arc(x, y, radius, 0, Math.PI * 2)
                ctx.fill()
            }
        }
        ctx.restore()
    }

    function drawDraftingSquareGrid(ctx, bounds, object, layerSelected) {
        var canvasPx = Math.max(1, Number(canvasObjectRenderer.controller ? canvasObjectRenderer.controller.drawingCanvasSizePx : 512))
        var levels = draftingGridLevels(object)
        for (var index = 0; index < levels.length; ++index) {
            drawDraftingGridLevel(ctx, bounds, canvasPx, levels[index], layerSelected)
        }
    }

    function drawSquareGrid(ctx, bounds, divisions, majorEvery, layerSelected) {
        for (var i = 0; i <= divisions; ++i) {
            var offset = i * bounds.size / divisions
            var major = i === 0 || i === divisions || i % majorEvery === 0
            setGridStroke(ctx, layerSelected, major)
            strokeGridLine(ctx, bounds.x + offset, bounds.y, bounds.x + offset, bounds.y + bounds.size)
            strokeGridLine(ctx, bounds.x, bounds.y + offset, bounds.x + bounds.size, bounds.y + offset)
        }
    }

    function drawDiagonalFamily(ctx, bounds, divisions, majorEvery, layerSelected, slope) {
        var step = bounds.size / divisions
        var span = bounds.size * 2
        for (var i = -divisions; i <= divisions * 2; ++i) {
            var major = Math.abs(i) % majorEvery === 0
            var x1 = bounds.x - span
            var y1 = bounds.y + i * step - slope * span
            var x2 = bounds.x + bounds.size + span
            var y2 = bounds.y + i * step + slope * (bounds.size + span)
            setGridStroke(ctx, layerSelected, major)
            strokeGridLine(ctx, x1, y1, x2, y2)
        }
    }

    function drawIsometricGrid(ctx, bounds, divisions, majorEvery, layerSelected) {
        var step = bounds.size / divisions
        for (var i = 0; i <= divisions; ++i) {
            var offset = i * step
            var major = i === 0 || i === divisions || i % majorEvery === 0
            setGridStroke(ctx, layerSelected, major)
            strokeGridLine(ctx, bounds.x + offset, bounds.y, bounds.x + offset, bounds.y + bounds.size)
        }
        drawDiagonalFamily(ctx, bounds, divisions, majorEvery, layerSelected, 0.577350269)
        drawDiagonalFamily(ctx, bounds, divisions, majorEvery, layerSelected, -0.577350269)
    }

    function drawHexGuideGrid(ctx, bounds, divisions, majorEvery, layerSelected) {
        var step = bounds.size / divisions
        for (var i = 0; i <= divisions; ++i) {
            var offset = i * step
            var major = i === 0 || i === divisions || i % majorEvery === 0
            setGridStroke(ctx, layerSelected, major)
            strokeGridLine(ctx, bounds.x, bounds.y + offset, bounds.x + bounds.size, bounds.y + offset)
        }
        drawDiagonalFamily(ctx, bounds, divisions, majorEvery, layerSelected, 1.732050808)
        drawDiagonalFamily(ctx, bounds, divisions, majorEvery, layerSelected, -1.732050808)
    }

    function drawCenterAxes(ctx, bounds, layerSelected) {
        var cx = bounds.x + bounds.size / 2
        var cy = bounds.y + bounds.size / 2
        ctx.lineWidth = layerSelected ? 1.5 : 1
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.58 : 0.34)
        strokeGridLine(ctx, cx, bounds.y, cx, bounds.y + bounds.size)
        strokeGridLine(ctx, bounds.x, cy, bounds.x + bounds.size, cy)
    }

    function drawAsciiCellGrid(ctx, bounds, columns, rows, majorEvery, layerSelected) {
        var colCount = Math.max(1, Number(columns || 80))
        var rowCount = Math.max(1, Number(rows || 40))
        var major = Math.max(1, Number(majorEvery || 10))
        var minorStroke = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.18 : 0.10)
        var majorStroke = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.48 : 0.26)
        for (var col = 0; col <= colCount; ++col) {
            ctx.lineWidth = col === 0 || col === colCount || col % major === 0 ? 0.95 : 0.45
            ctx.strokeStyle = col === 0 || col === colCount || col % major === 0 ? majorStroke : minorStroke
            var x = bounds.x + col * bounds.size / colCount
            strokeGridLine(ctx, x, bounds.y, x, bounds.y + bounds.size)
        }
        for (var row = 0; row <= rowCount; ++row) {
            ctx.lineWidth = row === 0 || row === rowCount || row % major === 0 ? 0.95 : 0.45
            ctx.strokeStyle = row === 0 || row === rowCount || row % major === 0 ? majorStroke : minorStroke
            var y = bounds.y + row * bounds.size / rowCount
            strokeGridLine(ctx, bounds.x, y, bounds.x + bounds.size, y)
        }
    }

    function drawDiagonalGuides(ctx, bounds, layerSelected) {
        ctx.lineWidth = layerSelected ? 1.35 : 0.85
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.48 : 0.22)
        strokeGridLine(ctx, bounds.x, bounds.y, bounds.x + bounds.size, bounds.y + bounds.size)
        strokeGridLine(ctx, bounds.x + bounds.size, bounds.y, bounds.x, bounds.y + bounds.size)
    }

    function drawRadialGuides(ctx, bounds, count, layerSelected) {
        var cx = bounds.x + bounds.size / 2
        var cy = bounds.y + bounds.size / 2
        var radius = bounds.size * 0.5
        var total = Math.max(2, Number(count || 8))
        ctx.lineWidth = layerSelected ? 1.25 : 0.8
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, layerSelected ? 0.46 : 0.20)
        for (var i = 0; i < total; ++i) {
            var angle = Math.PI * 2 * i / total
            strokeGridLine(ctx, cx, cy, cx + Math.cos(angle) * radius, cy + Math.sin(angle) * radius)
        }
    }

    function drawGrid(ctx, bounds, object, layerSelected) {
        var divisions = Math.max(2, Number(object.divisions || 16))
        var majorEvery = Math.max(1, Number(object.major_every || 4))
        var mode = String(object.mode || "square")

        ctx.save()
        ctx.beginPath()
        ctx.rect(bounds.x, bounds.y, bounds.size, bounds.size)
        ctx.clip()

        if (mode === "isometric") {
            drawIsometricGrid(ctx, bounds, divisions, majorEvery, layerSelected)
        } else if (mode === "hex") {
            drawHexGuideGrid(ctx, bounds, divisions, majorEvery, layerSelected)
        } else {
            drawDraftingSquareGrid(ctx, bounds, object, layerSelected)
        }

        if (object.diagonal_guides_visible === true) {
            drawDiagonalGuides(ctx, bounds, layerSelected)
        }
        if (object.radial_guides_visible === true) {
            drawRadialGuides(ctx, bounds, object.radial_guide_count, layerSelected)
        }
        if (object.center_axes_visible === true) {
            drawCenterAxes(ctx, bounds, layerSelected)
        }
        if (object.ascii_cell_grid_visible === true) {
            drawAsciiCellGrid(ctx, bounds, object.ascii_columns, object.ascii_rows, object.ascii_major_every, layerSelected)
        }

        ctx.restore()
    }

    function drawRect(ctx, bounds, object, layerSelected, objectSelected) {
        if (object.border_visible === false) {
            return
        }
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorAccent : UiStyle.colorBorderMajor
        ctx.lineWidth = objectSelected ? 3 : 2
        ctx.strokeRect(bounds.x, bounds.y, bounds.size, bounds.size)
    }

    function drawAnchor(ctx, bounds, object, layerSelected, objectSelected) {
        var x = pxX(bounds, object.x || 0.5)
        var y = pxY(bounds, object.y || 0.5)
        ctx.fillStyle = objectSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.strokeStyle = layerSelected ? UiStyle.colorText : UiStyle.colorWorkspace
        ctx.lineWidth = objectSelected ? 2 : 1
        ctx.beginPath()
        ctx.arc(x, y, objectSelected ? 6 : 4, 0, Math.PI * 2)
        ctx.fill()
        ctx.stroke()
    }

    function drawLine(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var x1 = pxX(bounds, object.x1 || 0)
        var y1 = pxY(bounds, object.y1 || 0)
        var x2 = pxX(bounds, object.x2 || 0)
        var y2 = pxY(bounds, object.y2 || 0)
        beginStyle(ctx, object, objectSelected, layerSelected, objectHovered)
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.stroke()
        ctx.fillStyle = objectSelected
                ? UiStyle.mix(styleStrokeColor(object), UiStyle.colorWarning, 0.42)
                : layerSelected
                    ? UiStyle.mix(styleStrokeColor(object), UiStyle.colorAccent, 0.28)
                    : styleStrokeColor(object)
        ctx.setLineDash([])
        ctx.beginPath()
        ctx.arc(x1, y1, objectSelected ? 5 : 3, 0, Math.PI * 2)
        ctx.fill()
        ctx.beginPath()
        ctx.arc(x2, y2, objectSelected ? 5 : 3, 0, Math.PI * 2)
        ctx.fill()
        endStyle(ctx)
    }

    function drawPoint(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var x = pxX(bounds, object.x || 0)
        var y = pxY(bounds, object.y || 0)
        ctx.fillStyle = objectSelected
                ? UiStyle.mix(styleStrokeColor(object), UiStyle.colorWarning, 0.42)
                : objectHovered
                    ? UiStyle.mix(styleStrokeColor(object), UiStyle.colorAccent, 0.42)
                : layerSelected
                    ? UiStyle.mix(styleStrokeColor(object), UiStyle.colorAccent, 0.28)
                    : styleStrokeColor(object)
        ctx.strokeStyle = UiStyle.colorWorkspace
        ctx.lineWidth = 1
        ctx.beginPath()
        ctx.arc(x, y, objectSelected ? 7 : objectHovered ? 6 : 5, 0, Math.PI * 2)
        ctx.globalAlpha = styleLineOpacity(object)
        ctx.fill()
        ctx.stroke()
        ctx.globalAlpha = 1
    }

    function drawPolyline(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var points = object.points || []
        if (points.length < 2) {
            return
        }
        beginStyle(ctx, object, objectSelected, layerSelected, objectHovered)
        ctx.beginPath()
        for (var index = 0; index < points.length; ++index) {
            var point = points[index] || [0, 0]
            var x = pxX(bounds, point[0] || 0)
            var y = pxY(bounds, point[1] || 0)
            if (index === 0) {
                ctx.moveTo(x, y)
            } else {
                ctx.lineTo(x, y)
            }
        }
        ctx.stroke()
        endStyle(ctx)
    }

    function drawCirclePrimitive(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var cx = pxX(bounds, object.cx || 0)
        var cy = pxY(bounds, object.cy || 0)
        var radius = Number(object.radius || 0) * bounds.size
        if (radius <= 0) {
            return
        }
        beginStyle(ctx, object, objectSelected, layerSelected, objectHovered)
        ctx.beginPath()
        ctx.arc(cx, cy, radius, 0, Math.PI * 2)
        ctx.stroke()
        applyFill(ctx, object, true)
        endStyle(ctx)
    }

    function drawArcPrimitive(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var cx = pxX(bounds, object.cx || 0)
        var cy = pxY(bounds, object.cy || 0)
        var radius = Number(object.radius || 0) * bounds.size
        if (radius <= 0) {
            return
        }
        var start = Number(object.start_angle_deg || 0) * Math.PI / 180
        var end = Number(object.end_angle_deg || 0) * Math.PI / 180
        beginStyle(ctx, object, objectSelected, layerSelected, objectHovered)
        ctx.beginPath()
        ctx.arc(cx, cy, radius, start, end)
        ctx.stroke()
        applyFill(ctx, object, false)
        endStyle(ctx)
    }

    function drawRectanglePrimitive(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var x = pxX(bounds, object.x || 0)
        var y = pxY(bounds, object.y || 0)
        var width = Number(object.width || 0) * bounds.size
        var height = Number(object.height || 0) * bounds.size
        var rotation = Number(object.rotation_deg || 0) * Math.PI / 180
        var cx = x + width / 2
        var cy = y + height / 2
        beginStyle(ctx, object, objectSelected, layerSelected, objectHovered)
        ctx.save()
        ctx.translate(cx, cy)
        ctx.rotate(rotation)
        ctx.beginPath()
        ctx.rect(-width / 2, -height / 2, width, height)
        ctx.stroke()
        applyFill(ctx, object, true)
        ctx.restore()
        endStyle(ctx)
    }

    function drawPolygonPrimitive(ctx, bounds, object, layerSelected, objectSelected, objectHovered) {
        var points = object.points || []
        if (points.length < 3) {
            return
        }
        beginStyle(ctx, object, objectSelected, layerSelected, objectHovered)
        ctx.beginPath()
        for (var index = 0; index < points.length; ++index) {
            var point = points[index] || [0, 0]
            var x = pxX(bounds, point[0] || 0)
            var y = pxY(bounds, point[1] || 0)
            if (index === 0) {
                ctx.moveTo(x, y)
            } else {
                ctx.lineTo(x, y)
            }
        }
        ctx.closePath()
        ctx.stroke()
        applyFill(ctx, object, true)
        endStyle(ctx)
    }

    function drawObject(ctx, bounds, doc, layer, object) {
        var objectSelected = selectedObject(doc, object.id)
        var objectHovered = !objectSelected && String(canvasObjectRenderer.hoverObjectId || "") === String(object.id || "")
        var layerSelected = selectedLayer(doc, layer.id)
        var rendererName = String(objectRendererByKind[String(object.kind || "")] || "")
        var renderer = rendererName.length > 0 ? canvasObjectRenderer[rendererName] : null
        if (renderer) {
            renderer(ctx, bounds, object, layerSelected, objectSelected, objectHovered)
        }
        if (String(doc.selected_object_id || "") === String(object.id || "")) {
            drawSelectedEditHandles(ctx, bounds, object)
        }
    }
}
