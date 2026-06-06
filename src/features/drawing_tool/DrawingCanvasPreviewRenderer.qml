import QtQuick
import "../../style"

QtObject {
    id: previewRenderer

    property var controller: null

    function pxX(bounds, normalizedX) {
        return bounds.x + Number(normalizedX) * bounds.size
    }

    function pxY(bounds, normalizedY) {
        return bounds.y + Number(normalizedY) * bounds.size
    }

    function hasPendingPoint(doc) {
        var pending = doc.pending_point || ({})
        return Number.isFinite(Number(pending.x)) && Number.isFinite(Number(pending.y))
    }

    function previewSupported(toolId) {
        var twoPointTools = [
            "line_polyline",
            "circle_arc",
            "rectangle_polygon",
            "regular_polygon",
            "image_reference_frame",
            "ascii_crop_frame",
            "ascii_cell_region",
            "glyph_baseline"
        ]
        return twoPointTools.indexOf(String(toolId || "")) >= 0
    }

    function normalizeLineStyle(value) {
        var style = String(value || "solid").trim().toLowerCase()
        if (style === "dashed" || style === "dot" || style === "dotted") {
            return style === "dot" ? "dotted" : style
        }
        return "solid"
    }

    function setupPreviewStroke(ctx) {
        ctx.save()
        var style = normalizeLineStyle(controller ? controller.drawingLineStyle : "solid")
        if (style === "dotted") {
            ctx.setLineDash([2, 5])
        } else {
            ctx.setLineDash([9, 5])
        }
        var width = Number.isFinite(Number(controller && controller.drawingLineThickness))
            ? Number(controller.drawingLineThickness)
            : 2
        ctx.lineWidth = Math.max(2, width + 1)
        ctx.strokeStyle = UiStyle.colorWarning
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorWarning, 0.22)
        ctx.globalAlpha = 0.96
    }

    function drawPendingHandle(ctx, x, y) {
        ctx.setLineDash([])
        ctx.save()
        ctx.globalAlpha = 1
        ctx.lineWidth = 2
        ctx.strokeStyle = UiStyle.colorWarning
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorWarning, 0.34)
        ctx.beginPath()
        ctx.arc(x, y, 9, 0, Math.PI * 2)
        ctx.fill()
        ctx.stroke()
        ctx.beginPath()
        ctx.moveTo(x - 13, y)
        ctx.lineTo(x + 13, y)
        ctx.moveTo(x, y - 13)
        ctx.lineTo(x, y + 13)
        ctx.stroke()
        ctx.restore()
    }

    function drawPreviewHandle(ctx, x, y) {
        ctx.setLineDash([])
        ctx.save()
        ctx.globalAlpha = 0.92
        ctx.lineWidth = 1.5
        ctx.strokeStyle = UiStyle.colorAccent
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, 0.24)
        ctx.beginPath()
        ctx.arc(x, y, 6, 0, Math.PI * 2)
        ctx.fill()
        ctx.stroke()
        ctx.restore()
    }

    function drawRectanglePreview(ctx, x1, y1, x2, y2) {
        var left = Math.min(x1, x2)
        var top = Math.min(y1, y2)
        var width = Math.abs(x2 - x1)
        var height = Math.abs(y2 - y1)
        ctx.strokeRect(left, top, width, height)
    }

    function drawRegularPolygonPreview(ctx, cx, cy, x2, y2, sides, rotationDeg) {
        var radius = Math.sqrt(Math.pow(x2 - cx, 2) + Math.pow(y2 - cy, 2))
        if (radius <= 0) {
            return
        }
        var totalSides = Math.max(3, Number(sides || 6))
        var rotation = Number(rotationDeg || 30) * Math.PI / 180
        ctx.beginPath()
        for (var index = 0; index < totalSides; ++index) {
            var angle = rotation + Math.PI * 2 * index / totalSides
            var px = cx + Math.cos(angle) * radius
            var py = cy + Math.sin(angle) * radius
            if (index === 0) {
                ctx.moveTo(px, py)
            } else {
                ctx.lineTo(px, py)
            }
        }
        ctx.closePath()
        ctx.stroke()
    }

    function drawLivePreview(ctx, bounds, doc, previewActive, previewX, previewY) {
        if (!previewActive || !hasPendingPoint(doc) || !previewSupported(doc.selected_tool_id)) {
            return
        }
        var pending = doc.pending_point || ({})
        var x1 = pxX(bounds, pending.x)
        var y1 = pxY(bounds, pending.y)
        var x2 = pxX(bounds, previewX)
        var y2 = pxY(bounds, previewY)
        setupPreviewStroke(ctx)
        drawPendingHandle(ctx, x1, y1)
        drawPreviewHandle(ctx, x2, y2)
        var tool = String(doc.selected_tool_id || "")
        if (tool === "circle_arc") {
            var radius = Math.sqrt(Math.pow(x2 - x1, 2) + Math.pow(y2 - y1, 2))
            if (radius > 0) {
                var arcMode = controller ? String(controller.drawingCircleArcMode || "circle") : "circle"
                var startAngle = controller ? Number(controller.drawingCircleArcStartAngleDeg || 0) : 0
                var endAngle = controller ? Number(controller.drawingCircleArcEndAngleDeg || 90) : 90
                ctx.beginPath()
                if (arcMode === "arc") {
                    ctx.arc(x1, y1, radius, startAngle * Math.PI / 180, endAngle * Math.PI / 180)
                } else {
                    ctx.arc(x1, y1, radius, 0, Math.PI * 2)
                }
                ctx.stroke()
            }
        } else if (tool === "regular_polygon") {
            var polygonSides = controller ? Number(controller.drawingRegularPolygonSides || 6) : 6
            var polygonRotation = controller ? Number(controller.drawingRegularPolygonRotationDeg || 30) : 30
            drawRegularPolygonPreview(ctx, x1, y1, x2, y2, polygonSides, polygonRotation)
        } else if (tool === "rectangle_polygon"
                   || tool === "image_reference_frame"
                   || tool === "ascii_crop_frame"
                   || tool === "ascii_cell_region") {
            drawRectanglePreview(ctx, x1, y1, x2, y2)
        } else {
            ctx.beginPath()
            ctx.moveTo(x1, y1)
            ctx.lineTo(x2, y2)
            ctx.stroke()
        }
        ctx.restore()
    }
}
