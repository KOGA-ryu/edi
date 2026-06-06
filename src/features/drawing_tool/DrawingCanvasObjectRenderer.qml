import QtQuick
import "../../style"

QtObject {
    id: canvasObjectRenderer

    property var controller: null

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

    function styleLineThickness(object, selected, layerSelected) {
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
            return Math.max(1.6, scaled * 1.5)
        }
        if (layerSelected) {
            return Math.max(1.4, scaled * 1.2)
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

    function beginStyle(ctx, object, selected, layerSelected) {
        var strokeColor = selected || layerSelected ? UiStyle.colorWarning : styleStrokeColor(object)
        ctx.save()
        ctx.strokeStyle = strokeColor
        ctx.lineWidth = styleLineThickness(object, selected, layerSelected)
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
        return bounds.x + Number(normalizedX) * bounds.size
    }

    function pxY(bounds, normalizedY) {
        return bounds.y + Number(normalizedY) * bounds.size
    }

    function selectedObject(doc, objectId) {
        return String(doc.selected_object_id || "") === String(objectId || "")
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

    function drawLine(ctx, bounds, object, layerSelected, objectSelected) {
        var x1 = pxX(bounds, object.x1 || 0)
        var y1 = pxY(bounds, object.y1 || 0)
        var x2 = pxX(bounds, object.x2 || 0)
        var y2 = pxY(bounds, object.y2 || 0)
        beginStyle(ctx, object, objectSelected, layerSelected)
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.stroke()
        ctx.fillStyle = objectSelected || layerSelected ? UiStyle.colorWarning : styleStrokeColor(object)
        ctx.setLineDash([])
        ctx.beginPath()
        ctx.arc(x1, y1, objectSelected ? 5 : 3, 0, Math.PI * 2)
        ctx.fill()
        ctx.beginPath()
        ctx.arc(x2, y2, objectSelected ? 5 : 3, 0, Math.PI * 2)
        ctx.fill()
        endStyle(ctx)
    }

    function drawPoint(ctx, bounds, object, layerSelected, objectSelected) {
        var x = pxX(bounds, object.x || 0)
        var y = pxY(bounds, object.y || 0)
        ctx.fillStyle = objectSelected || layerSelected ? UiStyle.colorWarning : styleStrokeColor(object)
        ctx.strokeStyle = UiStyle.colorWorkspace
        ctx.lineWidth = 1
        ctx.beginPath()
        ctx.arc(x, y, objectSelected ? 7 : 5, 0, Math.PI * 2)
        ctx.globalAlpha = styleLineOpacity(object)
        ctx.fill()
        ctx.stroke()
        ctx.globalAlpha = 1
    }

    function drawPolyline(ctx, bounds, object, layerSelected, objectSelected) {
        var points = object.points || []
        if (points.length < 2) {
            return
        }
        beginStyle(ctx, object, objectSelected, layerSelected)
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

    function drawCirclePrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var cx = pxX(bounds, object.cx || 0)
        var cy = pxY(bounds, object.cy || 0)
        var radius = Number(object.radius || 0) * bounds.size
        if (radius <= 0) {
            return
        }
        beginStyle(ctx, object, objectSelected, layerSelected)
        ctx.beginPath()
        ctx.arc(cx, cy, radius, 0, Math.PI * 2)
        ctx.stroke()
        applyFill(ctx, object, true)
        endStyle(ctx)
    }

    function drawArcPrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var cx = pxX(bounds, object.cx || 0)
        var cy = pxY(bounds, object.cy || 0)
        var radius = Number(object.radius || 0) * bounds.size
        if (radius <= 0) {
            return
        }
        var start = Number(object.start_angle_deg || 0) * Math.PI / 180
        var end = Number(object.end_angle_deg || 0) * Math.PI / 180
        beginStyle(ctx, object, objectSelected, layerSelected)
        ctx.beginPath()
        ctx.arc(cx, cy, radius, start, end)
        ctx.stroke()
        applyFill(ctx, object, false)
        endStyle(ctx)
    }

    function drawRectanglePrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var x = pxX(bounds, object.x || 0)
        var y = pxY(bounds, object.y || 0)
        var width = Number(object.width || 0) * bounds.size
        var height = Number(object.height || 0) * bounds.size
        beginStyle(ctx, object, objectSelected, layerSelected)
        ctx.strokeRect(x, y, width, height)
        applyFill(ctx, object, true)
        endStyle(ctx)
    }

    function drawPolygonPrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var points = object.points || []
        if (points.length < 3) {
            return
        }
        beginStyle(ctx, object, objectSelected, layerSelected)
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
        var layerSelected = selectedLayer(doc, layer.id)
        var rendererName = String(objectRendererByKind[String(object.kind || "")] || "")
        var renderer = rendererName.length > 0 ? canvasObjectRenderer[rendererName] : null
        if (renderer) {
            renderer(ctx, bounds, object, layerSelected, objectSelected)
        }
    }
}
