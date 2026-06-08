import QtQuick
import "../../runtime/DrawingCanvasSnap.js" as CanvasSnap

QtObject {
    id: snapResolver

    property var controller: null
    property real boardSizePx: 512

    function objects() {
        if (!controller) {
            return []
        }
        return controller.drawingCanvasObjects(controller.revision) || []
    }

    function settings() {
        return {
            canvasSizePx: controller ? Number(controller.drawingCanvasSizePx || 512) : 512,
            boardSizePx: Math.max(1, Number(boardSizePx || 512)),
            zoom: controller ? Number(controller.drawingCanvasZoom || 1.0) : 1.0,
            gridEnabled: !!(controller && controller.drawingSnapGridEnabled),
            gridStepPx: controller ? Number(controller.drawingSnapGridStepPx || 32) : 32,
            objectSnapEnabled: !!(controller && controller.drawingObjectSnapEnabled),
            objectSnapTolerancePx: controller ? Number(controller.drawingObjectSnapTolerancePx || 14) : 14,
            endpointEnabled: !controller || controller.drawingObjectSnapEndpointEnabled,
            midpointEnabled: !controller || controller.drawingObjectSnapMidpointEnabled,
            centerEnabled: !controller || controller.drawingObjectSnapCenterEnabled,
            vertexEnabled: !controller || controller.drawingObjectSnapVertexEnabled,
            objectPriority: "before_grid"
        }
    }

    function effectiveGridStepPx() {
        return CanvasSnap.effectiveGridStepPx(settings())
    }

    function gridSnappedPoint(point) {
        var snapSettings = settings()
        if (!snapSettings.gridEnabled) {
            return CanvasSnap.noneSnap(point, snapSettings)
        }
        return CanvasSnap.gridSnap(point, snapSettings)
    }

    function snappedPoint(point) {
        return CanvasSnap.resolveSnap(point, objects(), settings())
    }
}
