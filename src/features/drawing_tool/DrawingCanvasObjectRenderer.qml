import QtQuick
import "../../style"

QtObject {
    id: canvasObjectRenderer

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

    function selectedObjectSelectionBounds(bounds, object) {
        var minX = Number.POSITIVE_INFINITY
        var minY = Number.POSITIVE_INFINITY
        var maxX = Number.NEGATIVE_INFINITY
        var maxY = Number.NEGATIVE_INFINITY
        var kind = String(object.kind || "")

        function include(nx, ny, pad) {
            var x = Number(nx)
            var y = Number(ny)
            if (!Number.isFinite(x) || !Number.isFinite(y)) {
                return
            }
            var padding = Number.isFinite(Number(pad)) ? Number(pad) : 0
            minX = Math.min(minX, x - padding)
            minY = Math.min(minY, y - padding)
            maxX = Math.max(maxX, x + padding)
            maxY = Math.max(maxY, y + padding)
        }

        if (kind === "point" || kind === "tone_probe") {
            include(object.x || 0, object.y || 0, 0.02)
        } else if (kind === "line" || kind === "glyph_baseline") {
            include(object.x1 || 0, object.y1 || 0, 0)
            include(object.x2 || 0, object.y2 || 0, 0)
        } else if (kind === "circle" || kind === "arc") {
            var radius = Number(object.radius || 0)
            include((object.cx || 0) - radius, (object.cy || 0) - radius, 0)
            include((object.cx || 0) + radius, (object.cy || 0) + radius, 0)
        } else if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
            include(object.x || 0, object.y || 0, 0)
            include(Number(object.x || 0) + Number(object.width || 0), Number(object.y || 0) + Number(object.height || 0), 0)
        } else if (kind === "polyline" || kind === "polygon") {
            var points = asArray(object.points)
            for (var pointIndex = 0; pointIndex < points.length; ++pointIndex) {
                var point = points[pointIndex] || [0, 0]
                include(point[0] || 0, point[1] || 0, 0)
            }
            if (kind === "polygon" && points.length === 0) {
                return {}
            }
        } else if (kind === "grid") {
            include(0, 0, 0)
            include(1, 1, 0)
        }

        if (!Number.isFinite(minX) || !Number.isFinite(minY) || !Number.isFinite(maxX) || !Number.isFinite(maxY)) {
            return {}
        }
        if (maxX < minX || maxY < minY) {
            return {}
        }
        return {
            x: pxX(bounds, minX),
            y: pxY(bounds, minY),
            width: pxX(bounds, maxX) - pxX(bounds, minX),
            height: pxY(bounds, maxY) - pxY(bounds, minY)
        }
    }

    function drawSelectionHalo(ctx, bounds) {
        if (!bounds || bounds.width === undefined || bounds.height === undefined) {
            return
        }
        var pad = 8
        var left = bounds.x - pad
        var top = bounds.y - pad
        var right = bounds.x + bounds.width + pad
        var bottom = bounds.y + bounds.height + pad
        var width = Math.max(1, right - left)
        var height = Math.max(1, bottom - top)

        ctx.save()
        ctx.strokeStyle = UiStyle.colorWarning
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorWarning, 0.14)
        ctx.lineWidth = 2
        ctx.setLineDash([8, 4])
        ctx.beginPath()
        ctx.rect(left - 2, top - 2, width + 4, height + 4)
        ctx.fill()
        ctx.stroke()

        ctx.setLineDash([])
        var handle = 5
        var points = [
            [left, top],
            [left + width, top],
            [left, top + height],
            [left + width, top + height]
        ]
        for (var handleIndex = 0; handleIndex < points.length; ++handleIndex) {
            var handlePoint = points[handleIndex]
            ctx.fillStyle = UiStyle.colorWarning
            ctx.beginPath()
            ctx.rect(handlePoint[0] - handle, handlePoint[1] - handle, handle * 2, handle * 2)
            ctx.fill()
        }
        ctx.restore()
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
            drawSquareGrid(ctx, bounds, divisions, majorEvery, layerSelected)
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
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.lineWidth = objectSelected ? 4 : layerSelected ? 3 : 2
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.stroke()
        ctx.fillStyle = UiStyle.colorWarning
        ctx.beginPath()
        ctx.arc(x1, y1, objectSelected ? 5 : 3, 0, Math.PI * 2)
        ctx.fill()
        ctx.beginPath()
        ctx.arc(x2, y2, objectSelected ? 5 : 3, 0, Math.PI * 2)
        ctx.fill()
    }

    function drawPoint(ctx, bounds, object, layerSelected, objectSelected) {
        var x = pxX(bounds, object.x || 0)
        var y = pxY(bounds, object.y || 0)
        ctx.fillStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.strokeStyle = UiStyle.colorWorkspace
        ctx.lineWidth = 1
        ctx.beginPath()
        ctx.arc(x, y, objectSelected ? 7 : 5, 0, Math.PI * 2)
        ctx.fill()
        ctx.stroke()
    }

    function drawPolyline(ctx, bounds, object, layerSelected, objectSelected) {
        var points = object.points || []
        if (points.length < 2) {
            return
        }
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.lineWidth = objectSelected ? 4 : layerSelected ? 3 : 2
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
    }

    function drawCirclePrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var cx = pxX(bounds, object.cx || 0)
        var cy = pxY(bounds, object.cy || 0)
        var radius = Number(object.radius || 0) * bounds.size
        if (radius <= 0) {
            return
        }
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.lineWidth = objectSelected ? 4 : layerSelected ? 3 : 2
        ctx.beginPath()
        ctx.arc(cx, cy, radius, 0, Math.PI * 2)
        ctx.stroke()
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
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.lineWidth = objectSelected ? 4 : layerSelected ? 3 : 2
        ctx.beginPath()
        ctx.arc(cx, cy, radius, start, end)
        ctx.stroke()
    }

    function drawRectanglePrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var x = pxX(bounds, object.x || 0)
        var y = pxY(bounds, object.y || 0)
        var width = Number(object.width || 0) * bounds.size
        var height = Number(object.height || 0) * bounds.size
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.lineWidth = objectSelected ? 4 : layerSelected ? 3 : 2
        ctx.strokeRect(x, y, width, height)
    }

    function drawPolygonPrimitive(ctx, bounds, object, layerSelected, objectSelected) {
        var points = object.points || []
        if (points.length < 3) {
            return
        }
        ctx.strokeStyle = objectSelected || layerSelected ? UiStyle.colorWarning : UiStyle.colorAccent
        ctx.lineWidth = objectSelected ? 4 : layerSelected ? 3 : 2
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
    }

    function drawObject(ctx, bounds, doc, layer, object) {
        var objectSelected = selectedObject(doc, object.id)
        var layerSelected = selectedLayer(doc, layer.id)
        var rendererName = String(objectRendererByKind[String(object.kind || "")] || "")
        var renderer = rendererName.length > 0 ? canvasObjectRenderer[rendererName] : null
        if (renderer) {
            renderer(ctx, bounds, object, layerSelected, objectSelected)
        }
        if (objectSelected) {
            var selectionBounds = selectedObjectSelectionBounds(bounds, object)
            drawSelectionHalo(ctx, selectionBounds)
        }
    }
}
