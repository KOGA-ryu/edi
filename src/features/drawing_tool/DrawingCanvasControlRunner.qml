import QtQuick
import "../../runtime/DrawingCanvasViewport.js" as CanvasViewport

QtObject {
    id: controlRunner

    property var controller: null
    property var inputArea: null
    property var constructionCanvas: null
    property var lastResult: ({ ok: true, failures: [], executed: 0 })

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

    function finiteNumber(value, fallback) {
        var number = Number(value)
        return Number.isFinite(number) ? number : fallback
    }

    function canvasSizePx() {
        return Math.max(1, finiteNumber(controller && controller.drawingCanvasSizePx, 512))
    }

    function normalizedPoint(point) {
        var size = canvasSizePx()
        return {
            x: Math.max(0, Math.min(1, finiteNumber(point && point.x, 0) / size)),
            y: Math.max(0, Math.min(1, finiteNumber(point && point.y, 0) / size))
        }
    }

    function screenPointForCanvasPx(point) {
        var normalized = normalizedPoint(point)
        var bounds = constructionCanvas && typeof constructionCanvas.boardBounds === "function"
                ? constructionCanvas.boardBounds()
                : ({ x: 0, y: 0, size: canvasSizePx() })
        return {
            x: CanvasViewport.canvasToScreenX(bounds, normalized.x),
            y: CanvasViewport.canvasToScreenY(bounds, normalized.y)
        }
    }

    function fail(message) {
        return {
            ok: false,
            message: String(message || "control runner failure")
        }
    }

    function runSetTool(step) {
        if (!controller || typeof controller.selectDrawingTool !== "function") {
            return fail("controller.selectDrawingTool unavailable")
        }
        controller.selectDrawingTool(String(step.toolId || ""))
        if (inputArea && typeof inputArea.requestCanvasPaint === "function") {
            inputArea.requestCanvasPaint()
        }
        return { ok: true }
    }

    function runClickCanvas(step) {
        if (!inputArea || typeof inputArea.controlClickCanvasPoint !== "function") {
            return fail("inputArea.controlClickCanvasPoint unavailable")
        }
        return inputArea.controlClickCanvasPoint(normalizedPoint(step.point))
    }

    function runPanCanvas(step) {
        if (!inputArea || typeof inputArea.controlPanCanvasBy !== "function") {
            return fail("inputArea.controlPanCanvasBy unavailable")
        }
        return inputArea.controlPanCanvasBy(finiteNumber(step.dx, 0), finiteNumber(step.dy, 0))
    }

    function runZoomCanvas(step) {
        if (!inputArea || typeof inputArea.controlZoomCanvasAt !== "function") {
            return fail("inputArea.controlZoomCanvasAt unavailable")
        }
        return inputArea.controlZoomCanvasAt(finiteNumber(step.factor, 1), screenPointForCanvasPx(step.point || ({ x: canvasSizePx() / 2, y: canvasSizePx() / 2 })))
    }

    function pointerMoveCount(step) {
        return Math.max(1, Math.round(finiteNumber(step && step.pointerMoves, 1)))
    }

    function runDragObject(step) {
        if (!inputArea || typeof inputArea.controlDragObject !== "function") {
            return fail("inputArea.controlDragObject unavailable")
        }
        if (!step.from || !step.to) {
            return fail("dragObject requires resolved from/to points")
        }
        return inputArea.controlDragObject(normalizedPoint(step.from), normalizedPoint(step.to), pointerMoveCount(step))
    }

    function runDragHandle(step) {
        if (!inputArea || typeof inputArea.controlDragHandle !== "function") {
            return fail("inputArea.controlDragHandle unavailable")
        }
        if (!step.from || !step.to) {
            return fail("dragHandle requires resolved from/to points")
        }
        return inputArea.controlDragHandle(String(step.handleId || ""), normalizedPoint(step.from), normalizedPoint(step.to), pointerMoveCount(step))
    }

    function runMarqueeSelect(step) {
        if (!inputArea || typeof inputArea.controlMarqueeSelect !== "function") {
            return fail("inputArea.controlMarqueeSelect unavailable")
        }
        if (!step.from || !step.to) {
            return fail("marqueeSelect requires resolved from/to points")
        }
        return inputArea.controlMarqueeSelect(normalizedPoint(step.from), normalizedPoint(step.to), pointerMoveCount(step))
    }

    function runStep(step) {
        var type = String(step && step.type || "")
        if (type === "selectTool") {
            return runSetTool(step)
        }
        if (type === "clickCanvas") {
            return runClickCanvas(step)
        }
        if (type === "panCanvas") {
            return runPanCanvas(step)
        }
        if (type === "zoomCanvas") {
            return runZoomCanvas(step)
        }
        if (type === "dragObject") {
            return runDragObject(step)
        }
        if (type === "dragHandle") {
            return runDragHandle(step)
        }
        if (type === "marqueeSelect") {
            return runMarqueeSelect(step)
        }
        return fail("unsupported internal control step " + type)
    }

    function runExecutionPlan(plan) {
        var source = plan && plan.plan ? plan.plan : plan
        var steps = asArray(source && source.steps)
        var failures = []
        var executed = 0
        for (var index = 0; index < steps.length; ++index) {
            var result = runStep(steps[index])
            if (!result.ok) {
                failures.push({
                    stepIndex: index,
                    stepType: String(steps[index] && steps[index].type || ""),
                    message: result.message
                })
                break
            }
            ++executed
        }
        lastResult = {
            ok: failures.length === 0,
            failures: failures,
            executed: executed
        }
        return lastResult
    }
}
