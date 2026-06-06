import QtQuick
import "../../style"

QtObject {
    id: navigationRenderer

    property var controller: null

    function canvasPx() {
        return Math.max(1, Number(controller ? controller.drawingCanvasSizePx : 512))
    }

    function pxX(bounds, normalizedX) {
        return bounds.x + Number(normalizedX) * bounds.size
    }

    function pxY(bounds, normalizedY) {
        return bounds.y + Number(normalizedY) * bounds.size
    }

    function rulerStep(bounds) {
        var screenPerCanvasPx = bounds.size / canvasPx()
        var steps = [1, 2, 4, 8, 16, 32, 64, 128, 256]
        for (var index = 0; index < steps.length; ++index) {
            if (steps[index] * screenPerCanvasPx >= 48) {
                return steps[index]
            }
        }
        return 512
    }

    function visibleRange(bounds, axisStart, axisSize) {
        var canvas = canvasPx()
        var min = Math.max(0, Math.floor((axisStart - bounds.x) / bounds.size * canvas))
        var max = Math.min(canvas, Math.ceil((axisStart + axisSize - bounds.x) / bounds.size * canvas))
        return { min: min, max: max }
    }

    function drawOriginMarker(ctx, bounds) {
        var x = pxX(bounds, 0)
        var y = pxY(bounds, 0)
        if (x < bounds.x - 18 || x > bounds.x + bounds.size + 18 || y < bounds.y - 18 || y > bounds.y + bounds.size + 18) {
            return
        }
        ctx.save()
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWarning, UiStyle.colorAccent, 0.28)
        ctx.lineWidth = 1.2
        ctx.beginPath()
        ctx.moveTo(x - 12, y)
        ctx.lineTo(x + 12, y)
        ctx.moveTo(x, y - 12)
        ctx.lineTo(x, y + 12)
        ctx.stroke()
        ctx.beginPath()
        ctx.arc(x, y, 5, 0, Math.PI * 2)
        ctx.stroke()
        ctx.restore()
    }

    function drawCursorGuide(ctx, bounds, cursorX, cursorY, hoverInside) {
        if (!hoverInside) {
            return
        }
        var x = pxX(bounds, cursorX)
        var y = pxY(bounds, cursorY)
        if (x < bounds.x || x > bounds.x + bounds.size || y < bounds.y || y > bounds.y + bounds.size) {
            return
        }
        ctx.save()
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, 0.34)
        ctx.lineWidth = 0.6
        ctx.setLineDash([3, 8])
        ctx.beginPath()
        ctx.moveTo(x, bounds.y)
        ctx.lineTo(x, bounds.y + bounds.size)
        ctx.moveTo(bounds.x, y)
        ctx.lineTo(bounds.x + bounds.size, y)
        ctx.stroke()
        ctx.restore()
    }

    function drawTopRuler(ctx, bounds, canvasWidth) {
        var height = 21
        var step = rulerStep(bounds)
        var minorStep = Math.max(1, step / 4)
        var range = visibleRange(bounds, 0, canvasWidth)
        ctx.save()
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorBase, 0.42)
        ctx.globalAlpha = 0.88
        ctx.fillRect(0, 0, canvasWidth, height)
        ctx.globalAlpha = 1
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.18)
        ctx.lineWidth = 0.7
        ctx.beginPath()
        ctx.moveTo(0, height - 0.5)
        ctx.lineTo(canvasWidth, height - 0.5)
        ctx.stroke()

        for (var value = Math.floor(range.min / minorStep) * minorStep; value <= range.max; value += minorStep) {
            if (value < 0 || value > canvasPx()) {
                continue
            }
            var major = Math.abs(value % step) < 0.001
            var x = pxX(bounds, value / canvasPx())
            if (x < 0 || x > canvasWidth) {
                continue
            }
            ctx.strokeStyle = major ? UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.48) : UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.22)
            ctx.lineWidth = major ? 0.9 : 0.55
            ctx.beginPath()
            ctx.moveTo(x, height)
            ctx.lineTo(x, major ? height - 10 : height - 5)
            ctx.stroke()
            if (major) {
                ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.72)
                ctx.font = "10px " + UiStyle.fontMono
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(String(Math.round(value)), x, 8)
            }
        }
        ctx.restore()
    }

    function drawLeftRuler(ctx, bounds, canvasHeight) {
        var width = 29
        var step = rulerStep(bounds)
        var minorStep = Math.max(1, step / 4)
        var range = {
            min: Math.max(0, Math.floor((0 - bounds.y) / bounds.size * canvasPx())),
            max: Math.min(canvasPx(), Math.ceil((canvasHeight - bounds.y) / bounds.size * canvasPx()))
        }
        ctx.save()
        ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorBase, 0.42)
        ctx.globalAlpha = 0.88
        ctx.fillRect(0, 0, width, canvasHeight)
        ctx.globalAlpha = 1
        ctx.strokeStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.18)
        ctx.lineWidth = 0.7
        ctx.beginPath()
        ctx.moveTo(width - 0.5, 0)
        ctx.lineTo(width - 0.5, canvasHeight)
        ctx.stroke()

        for (var value = Math.floor(range.min / minorStep) * minorStep; value <= range.max; value += minorStep) {
            if (value < 0 || value > canvasPx()) {
                continue
            }
            var major = Math.abs(value % step) < 0.001
            var y = pxY(bounds, value / canvasPx())
            if (y < 0 || y > canvasHeight) {
                continue
            }
            ctx.strokeStyle = major ? UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.48) : UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.22)
            ctx.lineWidth = major ? 0.9 : 0.55
            ctx.beginPath()
            ctx.moveTo(width, y)
            ctx.lineTo(major ? width - 10 : width - 5, y)
            ctx.stroke()
            if (major) {
                ctx.save()
                ctx.translate(8, y)
                ctx.rotate(-Math.PI / 2)
                ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.72)
                ctx.font = "10px " + UiStyle.fontMono
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(String(Math.round(value)), 0, 0)
                ctx.restore()
            }
        }
        ctx.restore()
    }

    function drawNavigation(ctx, bounds, canvasWidth, canvasHeight, cursorX, cursorY, hoverInside) {
        drawCursorGuide(ctx, bounds, cursorX, cursorY, hoverInside)
        drawOriginMarker(ctx, bounds)
        drawTopRuler(ctx, bounds, canvasWidth)
        drawLeftRuler(ctx, bounds, canvasHeight)
    }
}
