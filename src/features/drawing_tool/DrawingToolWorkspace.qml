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
                    var snapColor = canvasInput.hoverSnapKind === "grid" ? UiStyle.colorTextMuted : UiStyle.colorWarning
                    ctx.save()
                    ctx.lineWidth = 1.5
                    ctx.strokeStyle = snapColor
                    ctx.fillStyle = UiStyle.mix(UiStyle.colorWorkspace, snapColor, 0.28)
                    ctx.beginPath()
                    ctx.arc(x, y, 7, 0, Math.PI * 2)
                    ctx.fill()
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(x - 10, y)
                    ctx.lineTo(x + 10, y)
                    ctx.moveTo(x, y - 10)
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
                onStateRevisionChanged: requestPaint()
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
                        var objectId = drawingWorkspace.controller.selectDrawingObjectAtNormalized(rawPoint.x, rawPoint.y)
                        dragObjectId = String(objectId || "")
                        dragObjectMoved = false
                        var dragStart = snapResolver.gridSnappedPoint(rawPoint)
                        dragObjectLastX = dragStart.x
                        dragObjectLastY = dragStart.y
                        if (dragObjectId.length > 0) {
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
                    if (dragAnchorId.length > 0 || dragObjectMoved) {
                        suppressClickOnce = true
                    }
                    dragAnchorId = ""
                    dragObjectId = ""
                    dragObjectMoved = false
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
                    drawingWorkspace.controller.handleDrawingCanvasClick(point.x, point.y)
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
                    if (drawingWorkspace.controller.drawingCanvasZoom > 1.001) {
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
