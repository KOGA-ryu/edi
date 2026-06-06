import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

Rectangle {
    id: drawingWorkspace

    property string dataUi: "drawing_tool_workspace"
    property string dataState: "draftsman_native_drawing"
    property string placementRole: "drawing_canvas_host"
    property string surfaceRecipeId: "draftsman_native_canvas_surface"
    property var controller: null

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    function asArray(value) {
        if (!value) {
            return []
        }
        if (Array.isArray(value)) {
            return value
        }
        return []
    }

    function isWorkingToolId(toolId) {
        return ["select_move", "anchor_points", "line_polyline", "circle_arc", "rectangle_polygon", "regular_polygon", "image_reference_frame", "ascii_crop_frame"].indexOf(String(toolId || "")) >= 0
    }

    function workingTools() {
        var result = []
        if (!drawingWorkspace.controller) {
            return result
        }
        var modes = asArray(drawingWorkspace.controller.drawingToolModes)
        for (var index = 0; index < modes.length; ++index) {
            var mode = modes[index]
            if (isWorkingToolId(mode.id)) {
                result.push(mode)
            }
        }
        return result
    }

    function selectedObjectId() {
        if (!drawingWorkspace.controller) {
            return ""
        }
        return String(drawingWorkspace.controller.selectedDrawingObjectId || "")
    }

    function hasSelectedObject() {
        return selectedObjectId().length > 0
    }

    function hasSelectedGeneratedObject() {
        var objectId = selectedObjectId()
        return objectId.length > 0 && objectId.indexOf("script_") === 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space4
        spacing: UiStyle.space6

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: UiStyle.colorPanelAlt
            radius: UiStyle.radiusSm
            border.width: UiStyle.borderNone

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: UiStyle.space6
                anchors.rightMargin: UiStyle.space6
                spacing: UiStyle.space4

                Repeater {
                    model: drawingWorkspace.workingTools()
                    delegate: UiButton {
                        Layout.preferredHeight: 24
                        Layout.preferredWidth: Math.max(66, implicitWidth + UiStyle.space6)
                        label: modelData.label
                        tooltip: modelData.label + " tool"
                        selected: drawingWorkspace.controller && drawingWorkspace.controller.selectedDrawingToolId === modelData.id
                        onClicked: if (drawingWorkspace.controller) drawingWorkspace.controller.selectDrawingTool(modelData.id)
                    }
                }

                Item { Layout.fillWidth: true }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Delete"
                    iconText: "X"
                    enabled: drawingWorkspace.hasSelectedGeneratedObject()
                    tooltip: drawingWorkspace.hasSelectedGeneratedObject() ? "Delete selected object" : "No deletable selected object"
                    onClicked: if (drawingWorkspace.hasSelectedGeneratedObject() && drawingWorkspace.controller) {
                        drawingWorkspace.controller.deleteSelectedDrawingObject()
                    }
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Clear"
                    iconText: "C"
                    enabled: drawingWorkspace.hasSelectedObject()
                    tooltip: drawingWorkspace.hasSelectedObject() ? "Clear selection" : "Nothing selected"
                    onClicked: if (drawingWorkspace.hasSelectedObject() && drawingWorkspace.controller) {
                        drawingWorkspace.controller.clearDrawingObjectSelection()
                    }
                }

                UiIconButton {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 24
                    label: "Fit"
                    iconText: "↔"
                    tooltip: "Reset canvas view"
                    onClicked: if (drawingWorkspace.controller) drawingWorkspace.controller.resetDrawingCanvasZoom()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            color: UiStyle.colorTransparent
            border.width: UiStyle.borderNone

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: UiStyle.space4
                anchors.rightMargin: UiStyle.space4
                spacing: UiStyle.space8

                Text {
                    Layout.fillWidth: true
                    text: canvasFrame.selectedToolLabel()
                    color: UiStyle.colorTextMuted
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                }

                Text {
                    Layout.fillWidth: true
                    text: "selected: " + (drawingWorkspace.hasSelectedObject() ? selectedObjectId() : "none")
                    color: UiStyle.colorTextMuted
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: canvasFrame.toolStateText()
                    color: UiStyle.colorTextMuted
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    elide: Text.ElideRight
                }

                Text {
                    Layout.preferredWidth: 150
                    text: canvasFrame.coordinateText()
                    color: UiStyle.colorTextFaint
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                }

                Text {
                    Layout.preferredWidth: 84
                    text: canvasFrame.viewText()
                    color: UiStyle.colorTextFaint
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        Rectangle {
            id: canvasFrame
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: UiStyle.colorWorkspace
            border.width: UiStyle.borderNone
            radius: 0
            clip: true
            focus: true

            function selectedToolId() {
                return drawingWorkspace.controller ? String(drawingWorkspace.controller.selectedDrawingToolId || "") : ""
            }

            function selectedToolLabel() {
                if (!drawingWorkspace.controller) {
                    return "No tool"
                }
                var tool = drawingWorkspace.controller.selectedDrawingTool()
                return String(tool.label || drawingWorkspace.controller.selectedDrawingToolId || "Tool")
            }

            function hasPendingPoint() {
                if (!drawingWorkspace.controller) {
                    return false
                }
                var pending = drawingWorkspace.controller.drawingPendingPoint || ({})
                return Number.isFinite(Number(pending.x)) && Number.isFinite(Number(pending.y))
            }

            function twoPointToolActive() {
                return previewRenderer.previewSupported(selectedToolId())
            }

            function toolStateText() {
                var toolId = selectedToolId()
                if (hasPendingPoint() && twoPointToolActive()) {
                    return "Preview active: move mouse, click to finish"
                }
                if (twoPointToolActive()) {
                    return "Click first point"
                }
                if (toolId === "anchor_points" || toolId === "tone_probe") {
                    return "Click to place point"
                }
                if (toolId === "select_move") {
                    return "Select mode"
                }
                return "Ready"
            }

            function coordinateText() {
                if (!canvasInput.hoverInside) {
                    return "cursor outside artboard"
                }
                var canvasPx = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasSizePx || 512) : 512
                var px = Math.round(canvasInput.hoverSnapX * canvasPx)
                var py = Math.round(canvasInput.hoverSnapY * canvasPx)
                return "cursor " + px + "," + py + " / " + canvasInput.hoverSnapX.toFixed(3) + "," + canvasInput.hoverSnapY.toFixed(3)
            }

            function viewText() {
                if (!drawingWorkspace.controller) {
                    return ""
                }
                return "zoom " + Math.round(Number(drawingWorkspace.controller.drawingCanvasZoom || 1) * 100) + "%"
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
}
