import QtQuick
import QtQuick.Controls
import "../../style"

Rectangle {
    id: drawingWorkspace

    property string dataUi: "drawing_tool_workspace"
    property string dataState: "draftsman_native_drawing"
    property string placementRole: "drawing_canvas_host"
    property string surfaceRecipeId: "draftsman_native_canvas_surface"
    property var controller: null

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    Rectangle {
        id: canvasFrame
        anchors.fill: parent
        color: UiStyle.colorWorkspace
        border.width: UiStyle.borderNone
        radius: 0
        clip: true
        focus: true

            function selectedToolId() {
                return drawingWorkspace.controller ? String(drawingWorkspace.controller.selectedDrawingToolId || "") : ""
            }

            Shortcut {
                sequence: "Esc"
                context: Qt.ApplicationShortcut
                onActivated: {
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.cancelDrawingPendingShape()
                    }
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Shortcut {
                sequences: [StandardKey.Undo]
                context: Qt.ApplicationShortcut
                enabled: drawingWorkspace.controller && drawingWorkspace.controller.drawingCanUndoCommand
                onActivated: {
                    drawingWorkspace.controller.undoDrawingCommand()
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Shortcut {
                sequences: [StandardKey.Redo]
                context: Qt.ApplicationShortcut
                enabled: drawingWorkspace.controller && drawingWorkspace.controller.drawingCanRedoCommand
                onActivated: {
                    drawingWorkspace.controller.redoDrawingCommand()
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Shortcut {
                sequences: ["Meta+D", "Ctrl+D"]
                context: Qt.ApplicationShortcut
                onActivated: {
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.duplicateSelectedDrawingObject()
                    }
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Shortcut {
                sequences: [StandardKey.Copy]
                context: Qt.ApplicationShortcut
                onActivated: {
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.copySelectedDrawingObject()
                    }
                }
            }

            Shortcut {
                sequences: [StandardKey.Paste]
                context: Qt.ApplicationShortcut
                onActivated: {
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.pasteCopiedDrawingObject()
                    }
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Shortcut {
                sequence: "Del"
                context: Qt.ApplicationShortcut
                onActivated: {
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.deleteSelectedDrawingObject()
                    }
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Shortcut {
                sequence: "Backspace"
                context: Qt.ApplicationShortcut
                onActivated: {
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.deleteSelectedDrawingObject()
                    }
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }
            }

            Canvas {
                id: constructionCanvas
                anchors.fill: parent
                anchors.margins: UiStyle.space4
                antialiasing: true
                property int stateRevision: drawingWorkspace.controller ? drawingWorkspace.controller.revision : 0
                property bool previewActive: false
                property real previewX: 0
                property real previewY: 0

                function pxX(bounds, normalizedX) {
                    return bounds.x + Number(normalizedX) * bounds.size
                }

                function pxY(bounds, normalizedY) {
                    return bounds.y + Number(normalizedY) * bounds.size
                }

                function boardBounds() {
                    var zoom = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasZoom || 1.0) : 1.0
                    var panX = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanXPx || 0) : 0
                    var panY = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanYPx || 0) : 0
                    var board = Math.max(32, Math.min(width, height) - 16) * zoom
                    return {
                        x: Math.round((width - board) / 2 + panX),
                        y: Math.round((height - board) / 2 + panY),
                        size: board
                    }
                }

                function drawSnapIndicator(ctx, bounds) {
                    if (!canvasInput.hoverInside || canvasInput.hoverSnapKind === "free" || canvasInput.hoverSnapKind === "none") {
                        return
                    }
                    var x = pxX(bounds, canvasInput.hoverSnapX)
                    var y = pxY(bounds, canvasInput.hoverSnapY)
                    var snapColor = canvasInput.hoverSnapKind === "grid" ? UiStyle.colorAccentSoft : UiStyle.colorWarning
                    ctx.save()
                    ctx.globalAlpha = canvasInput.hoverSnapKind === "grid" ? 0.88 : 0.96
                    ctx.lineWidth = 1.4
                    ctx.strokeStyle = snapColor
                    ctx.fillStyle = UiStyle.colorTransparent
                    ctx.beginPath()
                    ctx.rect(x - 5, y - 5, 10, 10)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(x - 10, y)
                    ctx.lineTo(x - 6, y)
                    ctx.moveTo(x + 6, y)
                    ctx.lineTo(x + 10, y)
                    ctx.moveTo(x, y - 10)
                    ctx.lineTo(x, y - 6)
                    ctx.moveTo(x, y + 6)
                    ctx.lineTo(x, y + 10)
                    ctx.stroke()
                    ctx.restore()
                }

                function drawMarquee(ctx, bounds) {
                    if (!canvasInput.marqueeActive) {
                        return
                    }
                    var minX = Math.min(canvasInput.marqueeStartX, canvasInput.marqueeEndX)
                    var minY = Math.min(canvasInput.marqueeStartY, canvasInput.marqueeEndY)
                    var maxX = Math.max(canvasInput.marqueeStartX, canvasInput.marqueeEndX)
                    var maxY = Math.max(canvasInput.marqueeStartY, canvasInput.marqueeEndY)
                    var x = pxX(bounds, minX)
                    var y = pxY(bounds, minY)
                    var width = Math.max(1, (maxX - minX) * bounds.size)
                    var height = Math.max(1, (maxY - minY) * bounds.size)
                    ctx.save()
                    ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorAccent, 0.24)
                    ctx.globalAlpha = 0.24
                    ctx.fillRect(x, y, width, height)
                    ctx.globalAlpha = 1
                    ctx.strokeStyle = UiStyle.mix(UiStyle.colorAccent, UiStyle.colorWarning, 0.25)
                    ctx.lineWidth = 1
                    ctx.setLineDash([5, 4])
                    ctx.strokeRect(x, y, width, height)
                    ctx.restore()
                }

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorText, 0.025)
                    ctx.fillRect(0, 0, width, height)

                    var bounds = boardBounds()
                    var doc = drawingWorkspace.controller ? drawingWorkspace.controller.drawingCanvasDocument(drawingWorkspace.controller.revision) : ({ layers: [] })
                    var layers = doc.layers || []
                    for (var layerIndex = 0; layerIndex < layers.length; ++layerIndex) {
                        var layer = layers[layerIndex]
                        if (layer.visible === false) {
                            continue
                        }
                        var objects = layer.objects || []
                        for (var objectIndex = 0; objectIndex < objects.length; ++objectIndex) {
                            objectRenderer.drawObject(ctx, bounds, doc, layer, objects[objectIndex])
                        }
                    }
                    previewRenderer.drawLivePreview(ctx, bounds, doc, previewActive, previewX, previewY)
                    drawSnapIndicator(ctx, bounds)
                    navigationRenderer.drawNavigation(ctx, bounds, width, height, canvasInput.hoverRawX, canvasInput.hoverRawY, canvasInput.hoverInside)
                    drawMarquee(ctx, bounds)
                }

                Component.onCompleted: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onStateRevisionChanged: {
                    if (drawingWorkspace.controller && !drawingWorkspace.controller.drawingPendingShapeActive) {
                        previewActive = false
                    }
                    requestPaint()
                }
            }

            DrawingCanvasSnapResolver {
                id: snapResolver
                controller: drawingWorkspace.controller
                boardSizePx: constructionCanvas.boardBounds().size
            }

            DrawingCanvasObjectRenderer {
                id: objectRenderer
                controller: drawingWorkspace.controller
            }

            DrawingCanvasPreviewRenderer {
                id: previewRenderer
                controller: drawingWorkspace.controller
            }

            DrawingCanvasNavigationRenderer {
                id: navigationRenderer
                controller: drawingWorkspace.controller
            }

            MouseArea {
                id: canvasInput
                anchors.fill: constructionCanvas
                hoverEnabled: true
                z: 2
                property string dragAnchorId: ""
                property string dragHandleId: ""
                property string dragHandleObjectId: ""
                property bool dragHandleMoved: false
                property string dragObjectId: ""
                property bool dragObjectMoved: false
                property real dragObjectLastX: 0
                property real dragObjectLastY: 0
                property bool suppressClickOnce: false
                property bool hoverInside: false
                property real hoverRawX: 0
                property real hoverRawY: 0
                property real hoverSnapX: 0
                property real hoverSnapY: 0
                property string hoverSnapKind: "none"
                property string hoverSnapLabel: "none"
                property real hoverSnapStepPx: 32
                property bool marqueeActive: false
                property bool marqueeMoved: false
                property real marqueeStartX: 0
                property real marqueeStartY: 0
                property real marqueeEndX: 0
                property real marqueeEndY: 0

                function selectedToolLabel() {
                    var id = String(drawingWorkspace.controller ? drawingWorkspace.controller.selectedDrawingToolId : "")
                    if (id === "select_move") {
                        return "select"
                    }
                    if (id === "anchor_points") {
                        return "point"
                    }
                    if (id === "line_polyline") {
                        return "line"
                    }
                    if (id === "circle_arc") {
                        return "circle"
                    }
                    if (id === "rectangle_polygon") {
                        return "rect"
                    }
                    if (id === "regular_polygon") {
                        return "polygon"
                    }
                    if (id === "image_reference_frame") {
                        return "image"
                    }
                    if (id === "ascii_crop_frame") {
                        return "ascii"
                    }
                    return id.length > 0 ? id : "tool"
                }

                function actionLabel() {
                    if (dragHandleId.length > 0) {
                        return "drag handle"
                    }
                    if (dragObjectId.length > 0) {
                        return "move object"
                    }
                    if (dragAnchorId.length > 0) {
                        return "drag anchor"
                    }
                    return selectedToolLabel()
                }

                function coordinateLabel() {
                    if (!hoverInside || !drawingWorkspace.controller) {
                        return "x --  y --"
                    }
                    var canvasPx = Math.max(1, Number(drawingWorkspace.controller.drawingCanvasSizePx || 512))
                    var x = Math.round(Math.max(0, Math.min(1, hoverRawX)) * canvasPx * 100) / 100
                    var y = Math.round(Math.max(0, Math.min(1, hoverRawY)) * canvasPx * 100) / 100
                    return "x " + x + "  y " + y
                }

                function snapLabel() {
                    if (!hoverInside) {
                        return "snap --"
                    }
                    if (hoverSnapKind === "grid") {
                        return "grid " + Math.round(hoverSnapStepPx) + "px"
                    }
                    if (hoverSnapKind === "free") {
                        return "free"
                    }
                    return hoverSnapLabel.length > 0 ? hoverSnapLabel : hoverSnapKind
                }

                function selectionLabel() {
                    if (!drawingWorkspace.controller) {
                        return "none"
                    }
                    var ids = asArray(drawingWorkspace.controller.selectedDrawingObjectIds)
                    if (ids.length > 1) {
                        return String(ids.length) + " objects"
                    }
                    var id = String(drawingWorkspace.controller.selectedDrawingObjectId || "")
                    return id.length > 0 && id.indexOf("script_") === 0 ? id : "none"
                }

                function boardBounds() {
                    return constructionCanvas.boardBounds()
                }

                function normalizedPoint(mouseX, mouseY) {
                    var bounds = boardBounds()
                    return {
                        x: (mouseX - bounds.x) / bounds.size,
                        y: (mouseY - bounds.y) / bounds.size
                    }
                }

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

                function selectedGeneratedObject() {
                    if (!drawingWorkspace.controller) {
                        return ({})
                    }
                    var object = drawingWorkspace.controller.selectedDrawingObject() || ({})
                    return String(object.id || "").indexOf("script_") === 0 ? object : ({})
                }

                function includePoint(bounds, x, y) {
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

                function objectBounds(object) {
                    var bounds = ({ ok: false, minX: 0, minY: 0, maxX: 0, maxY: 0 })
                    var kind = String(object.kind || "")
                    if (kind === "point" || kind === "tone_probe") {
                        return includePoint(bounds, object.x, object.y)
                    }
                    if (kind === "line" || kind === "glyph_baseline") {
                        includePoint(bounds, object.x1, object.y1)
                        return includePoint(bounds, object.x2, object.y2)
                    }
                    if (kind === "circle" || kind === "arc") {
                        var cx = Number(object.cx || 0)
                        var cy = Number(object.cy || 0)
                        var radius = Number(object.radius || 0)
                        includePoint(bounds, cx - radius, cy - radius)
                        return includePoint(bounds, cx + radius, cy + radius)
                    }
                    if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
                        var x = Number(object.x || 0)
                        var y = Number(object.y || 0)
                        includePoint(bounds, x, y)
                        return includePoint(bounds, x + Number(object.width || 0), y + Number(object.height || 0))
                    }
                    if (kind === "polyline" || kind === "polygon") {
                        var points = asArray(object.points)
                        for (var pointIndex = 0; pointIndex < points.length; ++pointIndex) {
                            var point = asArray(points[pointIndex])
                            if (point.length >= 2) {
                                includePoint(bounds, point[0], point[1])
                            }
                        }
                    }
                    return bounds
                }

                function boundsIntersectsSelection(bounds, minX, minY, maxX, maxY) {
                    if (!bounds.ok) {
                        return false
                    }
                    return bounds.maxX >= minX && bounds.minX <= maxX && bounds.maxY >= minY && bounds.minY <= maxY
                }

                function marqueeSelectionIds() {
                    var minX = Math.max(0, Math.min(marqueeStartX, marqueeEndX))
                    var minY = Math.max(0, Math.min(marqueeStartY, marqueeEndY))
                    var maxX = Math.min(1, Math.max(marqueeStartX, marqueeEndX))
                    var maxY = Math.min(1, Math.max(marqueeStartY, marqueeEndY))
                    var ids = []
                    var objects = drawingWorkspace.controller ? drawingWorkspace.controller.drawingCanvasObjects(drawingWorkspace.controller.revision) : []
                    for (var index = 0; index < objects.length; ++index) {
                        var object = objects[index] || ({})
                        var id = String(object.id || "")
                        if (id.indexOf("script_") !== 0) {
                            continue
                        }
                        if (boundsIntersectsSelection(objectBounds(object), minX, minY, maxX, maxY)) {
                            ids.push(id)
                        }
                    }
                    return ids
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
                        var x = Number(object.x || 0)
                        var y = Number(object.y || 0)
                        var width = Number(object.width || 0)
                        var height = Number(object.height || 0)
                        return [
                            { id: "rect_nw", x: x, y: y },
                            { id: "rect_ne", x: x + width, y: y },
                            { id: "rect_sw", x: x, y: y + height },
                            { id: "rect_se", x: x + width, y: y + height }
                        ]
                    }
                    if (kind === "circle" || kind === "arc") {
                        var cx = Number(object.cx || 0)
                        var cy = Number(object.cy || 0)
                        var radius = Number(object.radius || 0)
                        return [
                            { id: "circle_center", x: cx, y: cy },
                            { id: "circle_radius", x: Math.min(1, cx + radius), y: cy }
                        ]
                    }
                    return []
                }

                function hitSelectedHandle(mouseX, mouseY) {
                    var object = selectedGeneratedObject()
                    if (String(object.id || "").length === 0) {
                        return ({})
                    }
                    var bounds = boardBounds()
                    var handles = objectEditHandles(object)
                    var best = ({})
                    var bestDistance = 14
                    for (var index = 0; index < handles.length; ++index) {
                        var handle = handles[index]
                        var x = bounds.x + Number(handle.x || 0) * bounds.size
                        var y = bounds.y + Number(handle.y || 0) * bounds.size
                        var dx = Number(mouseX) - x
                        var dy = Number(mouseY) - y
                        var distance = Math.sqrt(dx * dx + dy * dy)
                        if (distance <= bestDistance) {
                            bestDistance = distance
                            best = handle
                        }
                    }
                    return best
                }

                function updateObjectFieldPx(field, normalizedValue) {
                    var canvasPx = Math.max(1, Number(drawingWorkspace.controller ? drawingWorkspace.controller.drawingCanvasSizePx : 512))
                    drawingWorkspace.controller.updateSelectedDrawingObjectField(field, Math.round(Number(normalizedValue || 0) * canvasPx * 1000) / 1000)
                }

                function updateObjectRawField(field, value) {
                    drawingWorkspace.controller.updateSelectedDrawingObjectField(field, value)
                }

                function modifierHandlePoint(mouse) {
                    var rawPoint = normalizedPoint(mouse.x, mouse.y)
                    var optionDown = !!(mouse.modifiers & Qt.AltModifier)
                    var shiftDown = !!(mouse.modifiers & Qt.ShiftModifier)
                    if (optionDown) {
                        return rawPoint
                    }
                    if (shiftDown) {
                        return forceGridSnappedPoint(rawPoint)
                    }
                    return snapResolver.gridSnappedPoint(rawPoint)
                }

                function forceGridSnappedPoint(point) {
                    var canvasPx = Math.max(1, Number(drawingWorkspace.controller ? drawingWorkspace.controller.drawingCanvasSizePx : 512))
                    var stepPx = Math.max(1, Number(snapResolver.effectiveGridStepPx()))
                    return {
                        x: Math.max(0, Math.min(1, Math.round(Number(point.x || 0) * canvasPx / stepPx) * stepPx / canvasPx)),
                        y: Math.max(0, Math.min(1, Math.round(Number(point.y || 0) * canvasPx / stepPx) * stepPx / canvasPx)),
                        stepPx: stepPx
                    }
                }

                function applySelectedHandleDrag(handleId, point) {
                    var object = selectedGeneratedObject()
                    var kind = String(object.kind || "")
                    if (String(object.id || "") !== dragHandleObjectId || String(handleId || "").length === 0) {
                        return
                    }
                    var x = Math.max(0, Math.min(1, Number(point.x || 0)))
                    var y = Math.max(0, Math.min(1, Number(point.y || 0)))
                    if ((kind === "line" || kind === "glyph_baseline") && handleId === "line_start") {
                        updateObjectFieldPx("x1_px", x)
                        updateObjectFieldPx("y1_px", y)
                        return
                    }
                    if ((kind === "line" || kind === "glyph_baseline") && handleId === "line_end") {
                        updateObjectFieldPx("x2_px", x)
                        updateObjectFieldPx("y2_px", y)
                        return
                    }
                    if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
                        var left = Number(object.x || 0)
                        var top = Number(object.y || 0)
                        var right = left + Number(object.width || 0)
                        var bottom = top + Number(object.height || 0)
                        var fixedX = handleId === "rect_nw" || handleId === "rect_sw" ? right : left
                        var fixedY = handleId === "rect_nw" || handleId === "rect_ne" ? bottom : top
                        var nextLeft = Math.min(fixedX, x)
                        var nextTop = Math.min(fixedY, y)
                        var nextWidth = Math.max(1 / Math.max(1, Number(drawingWorkspace.controller.drawingCanvasSizePx || 512)), Math.abs(fixedX - x))
                        var nextHeight = Math.max(1 / Math.max(1, Number(drawingWorkspace.controller.drawingCanvasSizePx || 512)), Math.abs(fixedY - y))
                        updateObjectFieldPx("x_px", nextLeft)
                        updateObjectFieldPx("y_px", nextTop)
                        updateObjectFieldPx("width_px", nextWidth)
                        updateObjectFieldPx("height_px", nextHeight)
                        return
                    }
                    if ((kind === "circle" || kind === "arc") && handleId === "circle_center") {
                        updateObjectFieldPx("cx_px", x)
                        updateObjectFieldPx("cy_px", y)
                        return
                    }
                    if ((kind === "circle" || kind === "arc") && handleId === "circle_radius") {
                        var cx = Number(object.cx || 0)
                        var cy = Number(object.cy || 0)
                        var dx = x - cx
                        var dy = y - cy
                        var radius = Math.sqrt(dx * dx + dy * dy)
                        updateObjectFieldPx("radius_px", radius)
                    }
                }

                function updatePreviewPoint(mouseX, mouseY) {
                    var rawPoint = normalizedPoint(mouseX, mouseY)
                    hoverRawX = rawPoint.x
                    hoverRawY = rawPoint.y
                    if (rawPoint.x < 0 || rawPoint.x > 1 || rawPoint.y < 0 || rawPoint.y > 1) {
                        hoverInside = false
                        hoverSnapKind = "none"
                        hoverSnapLabel = "none"
                        constructionCanvas.previewActive = false
                        constructionCanvas.requestPaint()
                        return rawPoint
                    }
                    var point = snapResolver.snappedPoint(rawPoint)
                    hoverInside = true
                    hoverSnapX = point.x
                    hoverSnapY = point.y
                    hoverSnapKind = point.kind
                    hoverSnapLabel = point.label
                    hoverSnapStepPx = Number(point.stepPx || snapResolver.effectiveGridStepPx())
                    constructionCanvas.previewX = point.x
                    constructionCanvas.previewY = point.y
                    constructionCanvas.previewActive = true
                    constructionCanvas.requestPaint()
                    return point
                }

                onExited: {
                    hoverInside = false
                    hoverSnapKind = "none"
                    hoverSnapLabel = "none"
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }

                onPressed: function(mouse) {
                    if (!drawingWorkspace.controller) {
                        return
                    }
                    var rawPoint = normalizedPoint(mouse.x, mouse.y)
                    var point = updatePreviewPoint(mouse.x, mouse.y)
                    if (drawingWorkspace.controller.selectedDrawingToolId === "select_move") {
                        var handle = hitSelectedHandle(mouse.x, mouse.y)
                        if (String(handle.id || "").length > 0) {
                            dragHandleId = String(handle.id || "")
                            dragHandleObjectId = String(selectedGeneratedObject().id || "")
                            dragHandleMoved = false
                            drawingWorkspace.controller.beginDrawingObjectMove()
                            mouse.accepted = true
                            return
                        }
                        var objectId = drawingWorkspace.controller.selectDrawingObjectAtNormalized(rawPoint.x, rawPoint.y)
                        dragObjectId = String(objectId || "")
                        dragObjectMoved = false
                        var dragStart = snapResolver.gridSnappedPoint(rawPoint)
                        dragObjectLastX = dragStart.x
                        dragObjectLastY = dragStart.y
                        if (dragObjectId.length > 0) {
                            drawingWorkspace.controller.beginDrawingObjectMove()
                            mouse.accepted = true
                        }
                        if (dragObjectId.length === 0) {
                            marqueeActive = true
                            marqueeMoved = false
                            marqueeStartX = rawPoint.x
                            marqueeStartY = rawPoint.y
                            marqueeEndX = rawPoint.x
                            marqueeEndY = rawPoint.y
                            constructionCanvas.requestPaint()
                            mouse.accepted = true
                        }
                        return
                    }
                    dragAnchorId = drawingWorkspace.controller.selectedDrawingToolId === "anchor_points"
                            ? drawingWorkspace.controller.selectNearestDrawingAnchor(point.x, point.y, 0.045)
                            : ""
                    if (dragAnchorId.length > 0) {
                        mouse.accepted = true
                    }
                }

                onPositionChanged: function(mouse) {
                    updatePreviewPoint(mouse.x, mouse.y)
                    if (drawingWorkspace.controller && pressed && dragHandleId.length > 0) {
                        var handlePoint = modifierHandlePoint(mouse)
                        applySelectedHandleDrag(dragHandleId, handlePoint)
                        dragHandleMoved = true
                        constructionCanvas.requestPaint()
                        return
                    }
                    if (drawingWorkspace.controller && pressed && marqueeActive) {
                        var rawMarqueePoint = normalizedPoint(mouse.x, mouse.y)
                        marqueeEndX = rawMarqueePoint.x
                        marqueeEndY = rawMarqueePoint.y
                        var bounds = boardBounds()
                        marqueeMoved = Math.abs((marqueeEndX - marqueeStartX) * bounds.size) > 4 || Math.abs((marqueeEndY - marqueeStartY) * bounds.size) > 4
                        constructionCanvas.requestPaint()
                        return
                    }
                    if (drawingWorkspace.controller && pressed && dragObjectId.length > 0) {
                        var movePoint = snapResolver.gridSnappedPoint(normalizedPoint(mouse.x, mouse.y))
                        var dx = movePoint.x - dragObjectLastX
                        var dy = movePoint.y - dragObjectLastY
                        if (Math.abs(dx) >= 0.000001 || Math.abs(dy) >= 0.000001) {
                            drawingWorkspace.controller.moveDrawingObjectBy(dragObjectId, dx, dy)
                            dragObjectLastX = movePoint.x
                            dragObjectLastY = movePoint.y
                            dragObjectMoved = true
                            constructionCanvas.requestPaint()
                        }
                        return
                    }
                    if (!drawingWorkspace.controller || dragAnchorId.length === 0 || !pressed) {
                        return
                    }
                    var point = snapResolver.snappedPoint(normalizedPoint(mouse.x, mouse.y))
                    drawingWorkspace.controller.setDrawingAnchorPosition(dragAnchorId, point.x, point.y)
                    constructionCanvas.requestPaint()
                }

                onReleased: {
                    if (drawingWorkspace.controller && marqueeActive && marqueeMoved) {
                        drawingWorkspace.controller.selectDrawingObjects(marqueeSelectionIds())
                        suppressClickOnce = true
                    }
                    if (dragAnchorId.length > 0 || dragHandleMoved || dragObjectMoved) {
                        suppressClickOnce = true
                    }
                    marqueeActive = false
                    marqueeMoved = false
                    dragAnchorId = ""
                    dragHandleId = ""
                    dragHandleObjectId = ""
                    dragHandleMoved = false
                    dragObjectId = ""
                    dragObjectMoved = false
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.endDrawingObjectMove()
                    }
                }

                onClicked: function(mouse) {
                    if (!drawingWorkspace.controller) {
                        return
                    }
                    if (suppressClickOnce) {
                        suppressClickOnce = false
                        return
                    }
                    var point = updatePreviewPoint(mouse.x, mouse.y)
                    if (point.x < 0 || point.x > 1 || point.y < 0 || point.y > 1) {
                        return
                    }
                    drawingWorkspace.controller.handleDrawingCanvasClick(point.x, point.y, Math.round(Number(point.stepPx || snapResolver.effectiveGridStepPx())))
                    constructionCanvas.requestPaint()
                }

                onWheel: function(wheel) {
                    if (!drawingWorkspace.controller) {
                        return
                    }
                    var pixelX = wheel.pixelDelta.x !== 0 ? wheel.pixelDelta.x : wheel.angleDelta.x / 2
                    var pixelY = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y / 2
                    var zoomGesture = (wheel.modifiers & Qt.ControlModifier) || (wheel.modifiers & Qt.MetaModifier)
                    if (zoomGesture) {
                        var rawDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y
                        var zoomFactor = Math.pow(1.0015, rawDelta)
                        drawingWorkspace.controller.zoomDrawingCanvasAt(zoomFactor, wheel.x, wheel.y, constructionCanvas.width, constructionCanvas.height)
                        constructionCanvas.requestPaint()
                        wheel.accepted = true
                        return
                    }
                    drawingWorkspace.controller.panDrawingCanvasBy(pixelX, pixelY)
                    constructionCanvas.requestPaint()
                    wheel.accepted = true
                }
            }

            Rectangle {
                id: canvasStatusStrip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 24
                z: 3
                color: UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorBase, 0.52)
                opacity: 0.94

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(110, parent.width * 0.22)
                    text: canvasInput.coordinateLabel()
                    color: UiStyle.colorTextMuted
                    font.family: UiStyle.fontMono
                    font.pixelSize: UiStyle.fontSizeXs
                    elide: Text.ElideRight
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Math.max(120, parent.width * 0.24)
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(90, parent.width * 0.18)
                    text: canvasInput.snapLabel()
                    color: UiStyle.colorText
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    font.weight: UiStyle.fontWeightSemiBold
                    elide: Text.ElideRight
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(100, parent.width * 0.18)
                    text: canvasInput.actionLabel()
                    color: UiStyle.colorTextMuted
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(110, parent.width * 0.22)
                    text: "selected " + canvasInput.selectionLabel()
                    color: UiStyle.colorTextFaint
                    font.family: UiStyle.fontMono
                    font.pixelSize: UiStyle.fontSizeXs
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }
            }

            PinchHandler {
                id: canvasPinch
                target: null
                property real previousScale: 1.0

                onActiveChanged: {
                    previousScale = 1.0
                }

                onScaleChanged: {
                    if (!active || !drawingWorkspace.controller) {
                        return
                    }
                    var factor = scale / Math.max(previousScale, 0.001)
                    drawingWorkspace.controller.zoomDrawingCanvasAt(factor, centroid.position.x, centroid.position.y, constructionCanvas.width, constructionCanvas.height)
                    previousScale = scale
                    constructionCanvas.requestPaint()
                }
            }
        }
    }
