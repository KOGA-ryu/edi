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

    function setupPreviewStroke(ctx) {
        ctx.save()
        ctx.setLineDash([8, 5])
        ctx.lineWidth = 2
        ctx.strokeStyle = UiStyle.colorWarning
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorWarning, 0.16)
    }

    function drawPreviewHandle(ctx, x, y) {
        ctx.setLineDash([])
        ctx.beginPath()
        ctx.arc(x, y, 4, 0, Math.PI * 2)
        ctx.fill()
        ctx.stroke()
        ctx.setLineDash([8, 5])
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
        drawPreviewHandle(ctx, x1, y1)
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
