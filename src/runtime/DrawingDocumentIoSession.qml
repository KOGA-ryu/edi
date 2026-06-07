import QtQuick

QtObject {
    id: drawingDocumentIoSession

    property var drawingSession: null
    property var drawingNativeController: null
    property bool drawingDocumentIoOk: true
    property string drawingDocumentIoStatus: "not saved"
    property string drawingDocumentPath: ""
    property bool drawingDocumentDirty: false
    property string drawingDocumentCleanSnapshot: ""
    property var drawingRecentFiles: []
    property string drawingRecentFilesPath: ""

    signal changed()

    function currentDrawingModelDocument() {
        if (drawingNativeController && typeof drawingNativeController.modelDocument === "function") {
            return drawingNativeController.modelDocument()
        }
        if (drawingSession && typeof drawingSession.drawingCanvasExportDocument === "function") {
            return drawingSession.drawingCanvasExportDocument(0)
        }
        return ({})
    }

    function drawingDocumentSnapshot() {
        return JSON.stringify(currentDrawingModelDocument())
    }

    function refreshDrawingDocumentDirty() {
        if (drawingDocumentCleanSnapshot.length === 0) {
            drawingDocumentDirty = false
            return
        }
        drawingDocumentDirty = drawingDocumentSnapshot() !== drawingDocumentCleanSnapshot
    }

    function markDrawingDocumentClean(status) {
        drawingDocumentCleanSnapshot = drawingDocumentSnapshot()
        drawingDocumentDirty = false
        drawingDocumentIoStatus = String(status || "saved")
        changed()
    }

    function newDrawingDocument() {
        if (drawingSession && typeof drawingSession.resetNativeDrawingDocument === "function") {
            drawingSession.resetNativeDrawingDocument()
        }
        drawingDocumentPath = ""
        drawingDocumentIoOk = true
        markDrawingDocumentClean("new drawing")
        return true
    }

    function drawingDocumentFileName() {
        if (!drawingDocumentPath.length) {
            return "untitled"
        }
        var parts = drawingDocumentPath.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : drawingDocumentPath
    }

    function drawingDocumentStatusText() {
        var state = drawingDocumentDirty ? "unsaved" : (drawingDocumentPath.length ? "saved" : "not saved")
        return drawingDocumentFileName() + " / " + state
    }

    function drawingRecentFilePath(item) {
        if (typeof item === "string") {
            return item
        }
        return String(item && item.path ? item.path : "")
    }

    function drawingRecentFileLabel(item) {
        var path = drawingRecentFilePath(item)
        var label = String(item && item.label ? item.label : "")
        if (label.length > 0) {
            return label
        }
        if (!path.length) {
            return "Untitled"
        }
        var parts = path.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : path
    }

    function recordDrawingRecentFile(path) {
        var normalizedPath = String(path || "")
        if (!normalizedPath.length || typeof drawingRecentFilesStore === "undefined"
                || !drawingRecentFilesStore || typeof drawingRecentFilesStore.add !== "function") {
            return false
        }
        var result = drawingRecentFilesStore.add(normalizedPath)
        if (result && result.ok && result.files) {
            drawingRecentFiles = result.files
            changed()
            return true
        }
        return false
    }

    function saveDrawingDocument(url) {
        if (typeof drawingDocumentStore === "undefined" || !drawingDocumentStore || typeof drawingDocumentStore.save !== "function") {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing storage unavailable"
            changed()
            return false
        }
        var result = drawingDocumentStore.save(url, currentDrawingModelDocument())
        drawingDocumentIoOk = !!result.ok
        drawingDocumentIoStatus = String(result.message || (drawingDocumentIoOk ? "saved drawing" : "save failed"))
        if (drawingDocumentIoOk) {
            drawingDocumentPath = String(result.path || "")
            markDrawingDocumentClean("saved drawing")
            recordDrawingRecentFile(drawingDocumentPath)
        }
        changed()
        return drawingDocumentIoOk
    }

    function saveCurrentDrawingDocument() {
        if (!drawingDocumentPath.length) {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing path missing"
            changed()
            return false
        }
        return saveDrawingDocument(drawingDocumentPath)
    }

    function currentDrawingSvgText() {
        if (drawingNativeController && typeof drawingNativeController.exportSvg === "function") {
            return drawingNativeController.exportSvg()
        }
        return ""
    }

    function exportDrawingSvg(url) {
        if (typeof drawingDocumentStore === "undefined" || !drawingDocumentStore || typeof drawingDocumentStore.exportSvg !== "function") {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "svg export unavailable"
            changed()
            return false
        }
        var result = drawingDocumentStore.exportSvg(url, currentDrawingSvgText())
        drawingDocumentIoOk = !!result.ok
        drawingDocumentIoStatus = String(result.message || (drawingDocumentIoOk ? "exported svg" : "svg export failed"))
        changed()
        return drawingDocumentIoOk
    }

    function exportDrawingBlenderSvgBundle(url) {
        if (typeof drawingDocumentStore === "undefined" || !drawingDocumentStore || typeof drawingDocumentStore.exportBlenderSvgBundle !== "function") {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "Blender SVG bundle unavailable"
            changed()
            return false
        }
        var result = drawingDocumentStore.exportBlenderSvgBundle(url, currentDrawingSvgText(), currentDrawingModelDocument())
        drawingDocumentIoOk = !!result.ok
        drawingDocumentIoStatus = String(result.message || (drawingDocumentIoOk ? "exported Blender SVG bundle" : "Blender SVG bundle failed"))
        changed()
        return drawingDocumentIoOk
    }

    function openDrawingDocument(url) {
        if (typeof drawingDocumentStore === "undefined" || !drawingDocumentStore || typeof drawingDocumentStore.open !== "function") {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing storage unavailable"
            changed()
            return false
        }
        var result = drawingDocumentStore.open(url)
        drawingDocumentIoOk = !!result.ok
        drawingDocumentIoStatus = String(result.message || (drawingDocumentIoOk ? "opened drawing" : "open failed"))
        if (!drawingDocumentIoOk) {
            changed()
            return false
        }
        if (!drawingNativeController || typeof drawingNativeController.loadModel !== "function" || !drawingNativeController.loadModel(result.model || ({}))) {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing model rejected"
            changed()
            return false
        }
        drawingDocumentPath = String(result.path || "")
        if (drawingSession && typeof drawingSession.syncNativeDrawingModel === "function") {
            drawingSession.syncNativeDrawingModel()
        }
        markDrawingDocumentClean("opened drawing")
        recordDrawingRecentFile(drawingDocumentPath)
        changed()
        return true
    }

    function openRecentDrawing(item) {
        var path = drawingRecentFilePath(item)
        if (!path.length) {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "recent drawing path missing"
            changed()
            return false
        }
        return openDrawingDocument(path)
    }
}
