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
                        var handlePoint = snapResolver.gridSnappedPoint(normalizedPoint(mouse.x, mouse.y))
                        applySelectedHandleDrag(dragHandleId, handlePoint)
                        dragHandleMoved = true
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
                    if (dragAnchorId.length > 0 || dragHandleMoved || dragObjectMoved) {
                        suppressClickOnce = true
                    }
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
                    var panGesture = (wheel.modifiers & Qt.ShiftModifier)
                    if (!panGesture) {
                        var rawDelta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y
                        var zoomFactor = Math.pow(1.0015, rawDelta)
                        drawingWorkspace.controller.zoomDrawingCanvasAt(zoomFactor, wheel.x, wheel.y, constructionCanvas.width, constructionCanvas.height)
                        constructionCanvas.requestPaint()
                        wheel.accepted = true
                        return
                    }
                    if (drawingWorkspace.controller.drawingCanvasZoom > 1.001 || panGesture) {
                        drawingWorkspace.controller.panDrawingCanvasBy(pixelX, pixelY)
                        constructionCanvas.requestPaint()
                        wheel.accepted = true
                    }
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
