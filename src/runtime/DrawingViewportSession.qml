import QtQuick

QtObject {
    id: viewportSession

    signal changed()

    property bool drawingSnapGridEnabled: true
    property int drawingSnapGridStepPx: 32
    property bool drawingObjectSnapEnabled: true
    property int drawingObjectSnapTolerancePx: 14
    property bool drawingObjectSnapEndpointEnabled: true
    property bool drawingObjectSnapMidpointEnabled: true
    property bool drawingObjectSnapCenterEnabled: true
    property bool drawingObjectSnapVertexEnabled: true
    property bool drawingGridVisible: true
    property string drawingGridMode: "square"
    property int drawingGridDivisions: 16
    property int drawingGridMajorEvery: 4
    property bool drawingAsciiCellGridVisible: false
    property int drawingAsciiColumns: 80
    property int drawingAsciiRows: 40
    property int drawingAsciiMajorEvery: 10
    property bool drawingCenterAxesVisible: true
    property bool drawingDiagonalGuidesVisible: false
    property bool drawingRadialGuidesVisible: false
    property int drawingRadialGuideCount: 8
    property bool drawingArtboardBorderVisible: true
    property int drawingCanvasSizePx: 512
    property real drawingCanvasZoom: 1.0
    property real drawingCanvasZoomMin: 0.25
    property real drawingCanvasZoomMax: 8.0
    property real drawingCanvasPanXPx: 0
    property real drawingCanvasPanYPx: 0
    property var drawingNativeController: null
    property var syncNativeDrawingModel: null

    function setDrawingGridVisible(visible) {
        drawingGridVisible = visible === true
        changed()
    }

    function setDrawingGridMode(mode) {
        var allowed = ["square", "isometric", "hex"]
        var value = String(mode || "square")
        drawingGridMode = allowed.indexOf(value) >= 0 ? value : "square"
        changed()
    }

    function setDrawingGridDivisions(divisions) {
        drawingGridDivisions = Math.max(2, Math.min(128, Math.round(Number(divisions) || 16)))
        changed()
    }

    function setDrawingGridMajorEvery(value) {
        drawingGridMajorEvery = Math.max(1, Math.min(32, Math.round(Number(value) || 4)))
        changed()
    }

    function setDrawingAsciiCellGridVisible(visible) {
        drawingAsciiCellGridVisible = visible === true
        changed()
    }

    function setDrawingAsciiColumns(columns) {
        drawingAsciiColumns = Math.max(8, Math.min(320, Math.round(Number(columns) || 80)))
        changed()
    }

    function setDrawingAsciiRows(rows) {
        drawingAsciiRows = Math.max(4, Math.min(240, Math.round(Number(rows) || 40)))
        changed()
    }

    function setDrawingAsciiMajorEvery(value) {
        drawingAsciiMajorEvery = Math.max(1, Math.min(64, Math.round(Number(value) || 10)))
        changed()
    }

    function setDrawingCenterAxesVisible(visible) {
        drawingCenterAxesVisible = visible === true
        changed()
    }

    function setDrawingDiagonalGuidesVisible(visible) {
        drawingDiagonalGuidesVisible = visible === true
        changed()
    }

    function setDrawingRadialGuidesVisible(visible) {
        drawingRadialGuidesVisible = visible === true
        changed()
    }

    function setDrawingRadialGuideCount(count) {
        drawingRadialGuideCount = Math.max(2, Math.min(64, Math.round(Number(count) || 8)))
        changed()
    }

    function setDrawingArtboardBorderVisible(visible) {
        drawingArtboardBorderVisible = visible === true
        changed()
    }

    function setDrawingSnapGrid(enabled) {
        drawingSnapGridEnabled = enabled === true
        if (drawingNativeController) {
            drawingNativeController.setSnap(drawingSnapGridEnabled, drawingSnapGridStepPx)
            if (typeof syncNativeDrawingModel === "function") {
                syncNativeDrawingModel()
            }
            return
        }
        changed()
    }

    function setDrawingSnapGridStepPx(stepPx) {
        drawingSnapGridStepPx = Math.max(1, Math.min(drawingCanvasSizePx, Math.round(Number(stepPx) || 32)))
        if (drawingNativeController) {
            drawingNativeController.setSnap(drawingSnapGridEnabled, drawingSnapGridStepPx)
            if (typeof syncNativeDrawingModel === "function") {
                syncNativeDrawingModel()
            }
            return
        }
        changed()
    }

    function setDrawingObjectSnapEnabled(enabled) {
        drawingObjectSnapEnabled = enabled === true
        changed()
    }

    function setDrawingObjectSnapTolerancePx(value) {
        drawingObjectSnapTolerancePx = Math.max(2, Math.min(64, Math.round(Number(value) || 14)))
        changed()
    }

    function setDrawingObjectSnapEndpointEnabled(enabled) {
        drawingObjectSnapEndpointEnabled = enabled === true
        changed()
    }

    function setDrawingObjectSnapMidpointEnabled(enabled) {
        drawingObjectSnapMidpointEnabled = enabled === true
        changed()
    }

    function setDrawingObjectSnapCenterEnabled(enabled) {
        drawingObjectSnapCenterEnabled = enabled === true
        changed()
    }

    function setDrawingObjectSnapVertexEnabled(enabled) {
        drawingObjectSnapVertexEnabled = enabled === true
        changed()
    }

    function setDrawingCanvasZoom(zoom) {
        drawingCanvasZoom = Math.max(drawingCanvasZoomMin, Math.min(drawingCanvasZoomMax, Number(zoom) || 1.0))
        changed()
    }

    function drawingCanvasBaseViewSize(viewWidth, viewHeight) {
        return Math.max(32, Math.min(Number(viewWidth) || 0, Number(viewHeight) || 0) - 16)
    }

    function zoomDrawingCanvasAt(factor, focusX, focusY, viewWidth, viewHeight) {
        var oldZoom = Math.max(drawingCanvasZoomMin, Math.min(drawingCanvasZoomMax, Number(drawingCanvasZoom) || 1.0))
        var zoomFactor = Number(factor) || 1.0
        var newZoom = Math.max(drawingCanvasZoomMin, Math.min(drawingCanvasZoomMax, oldZoom * zoomFactor))
        if (Math.abs(newZoom - oldZoom) < 0.0001) {
            return
        }
        var base = drawingCanvasBaseViewSize(viewWidth, viewHeight)
        var oldBoard = base * oldZoom
        var newBoard = base * newZoom
        var fx = Number.isFinite(Number(focusX)) ? Number(focusX) : Number(viewWidth) / 2
        var fy = Number.isFinite(Number(focusY)) ? Number(focusY) : Number(viewHeight) / 2
        var oldX = (Number(viewWidth) - oldBoard) / 2 + drawingCanvasPanXPx
        var oldY = (Number(viewHeight) - oldBoard) / 2 + drawingCanvasPanYPx
        var unitX = oldBoard > 0 ? (fx - oldX) / oldBoard : 0.5
        var unitY = oldBoard > 0 ? (fy - oldY) / oldBoard : 0.5
        drawingCanvasPanXPx = fx - unitX * newBoard - (Number(viewWidth) - newBoard) / 2
        drawingCanvasPanYPx = fy - unitY * newBoard - (Number(viewHeight) - newBoard) / 2
        drawingCanvasZoom = newZoom
        changed()
    }

    function panDrawingCanvasBy(dx, dy) {
        drawingCanvasPanXPx += Number(dx) || 0
        drawingCanvasPanYPx += Number(dy) || 0
        changed()
    }

    function zoomDrawingCanvasIn() {
        setDrawingCanvasZoom(drawingCanvasZoom * 1.25)
    }

    function zoomDrawingCanvasOut() {
        setDrawingCanvasZoom(drawingCanvasZoom / 1.25)
    }

    function resetDrawingCanvasZoom() {
        drawingCanvasZoom = 1.0
        drawingCanvasPanXPx = 0
        drawingCanvasPanYPx = 0
        changed()
    }

    function fitDrawingCanvasToView() {
        resetDrawingCanvasZoom()
    }
}
