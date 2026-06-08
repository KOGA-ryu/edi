import QtQuick
import QtQuick.Controls
import "../../style"
import "../../runtime/DrawingCanvasHandles.js" as CanvasHandles
import "../../runtime/DrawingCanvasGestureState.js" as CanvasGestureState
import "../../runtime/DrawingCanvasViewport.js" as CanvasViewport

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

            function nudgeStepPx(mode) {
                if (mode === "fine") {
                    return 1
                }
                var gridStep = Math.max(1, Number(drawingWorkspace.controller ? drawingWorkspace.controller.drawingSnapGridStepPx : 32))
                return mode === "large" ? gridStep * 4 : gridStep
            }

            function nudgeSelection(dx, dy, mode) {
                if (!drawingWorkspace.controller) {
                    return
                }
                var step = nudgeStepPx(mode)
                drawingWorkspace.controller.nudgeSelectedDrawingObjectByPx(dx * step, dy * step)
                constructionCanvas.previewActive = false
                constructionCanvas.requestPaint()
            }

            Shortcut {
                sequence: "Esc"
                context: Qt.ApplicationShortcut
                onActivated: {
                    canvasInput.cancelActiveGesture()
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

            Shortcut {
                sequence: "Left"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(-1, 0, "grid")
            }

            Shortcut {
                sequence: "Right"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(1, 0, "grid")
            }

            Shortcut {
                sequence: "Up"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(0, -1, "grid")
            }

            Shortcut {
                sequence: "Down"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(0, 1, "grid")
            }

            Shortcut {
                sequence: "Alt+Left"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(-1, 0, "fine")
            }

            Shortcut {
                sequence: "Alt+Right"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(1, 0, "fine")
            }

            Shortcut {
                sequence: "Alt+Up"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(0, -1, "fine")
            }

            Shortcut {
                sequence: "Alt+Down"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(0, 1, "fine")
            }

            Shortcut {
                sequence: "Shift+Left"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(-1, 0, "large")
            }

            Shortcut {
                sequence: "Shift+Right"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(1, 0, "large")
            }

            Shortcut {
                sequence: "Shift+Up"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(0, -1, "large")
            }

            Shortcut {
                sequence: "Shift+Down"
                context: Qt.ApplicationShortcut
                onActivated: nudgeSelection(0, 1, "large")
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
                    return CanvasViewport.canvasToScreenX(bounds, normalizedX)
                }

                function pxY(bounds, normalizedY) {
                    return CanvasViewport.canvasToScreenY(bounds, normalizedY)
                }

                function boardBounds() {
                    var zoom = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasZoom || 1.0) : 1.0
                    var panX = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanXPx || 0) : 0
                    var panY = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanYPx || 0) : 0
                    return CanvasViewport.boardBounds(width, height, zoom, panX, panY)
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
                    objectRenderer.drawCombinedSelectionOutline(ctx, bounds, doc)
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
                hoverObjectId: canvasInput.hoverObjectId
                hoverHandleId: canvasInput.hoverHandleId
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
                property bool selectionTogglePressed: false
                property string hoverObjectId: ""
                property string hoverHandleId: ""
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
                property int activeModifiers: Qt.NoModifier
                property var gestureState: CanvasGestureState.initialGestureState()
                cursorShape: selectionCursorShape()

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
                    if (CanvasGestureState.isHandleDrag(gestureState) || CanvasGestureState.isObjectDrag(gestureState) || CanvasGestureState.isMarquee(gestureState)) {
                        return CanvasGestureState.gestureLabel(gestureState)
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
                        return String(ids.length) + " selected"
                    }
                    var id = String(drawingWorkspace.controller.selectedDrawingObjectId || "")
                    return id.length > 0 && id.indexOf("script_") === 0 ? id : "none"
                }

                function selectionStatusLabel() {
                    var label = selectionLabel()
                    return label.indexOf(" selected") > 0 ? label : "selected " + label
                }

                function selectionCursorShape() {
                    if (!drawingWorkspace.controller) {
                        return Qt.ArrowCursor
                    }
                    if (CanvasGestureState.isHandleDrag(gestureState) || hoverHandleId.length > 0) {
                        return Qt.SizeAllCursor
                    }
                    if (CanvasGestureState.isObjectDrag(gestureState)) {
                        return Qt.ClosedHandCursor
                    }
                    if (hoverObjectId.length > 0) {
                        return Qt.OpenHandCursor
                    }
                    return drawingWorkspace.controller.selectedDrawingToolId === "select_move" ? Qt.ArrowCursor : Qt.CrossCursor
                }

                function selectedObjectIdList() {
                    return asArray(drawingWorkspace.controller ? drawingWorkspace.controller.selectedDrawingObjectIds : [])
                }

                function selectedObjectIdsContain(objectId) {
                    var ids = selectedObjectIdList()
                    return ids.indexOf(String(objectId || "")) >= 0
                }

                function gestureModifiers(modifiers) {
                    return {
                        shift: modifierShiftDown(modifiers),
                        alt: modifierOptionDown(modifiers),
                        control: !!(modifiers & Qt.ControlModifier),
                        meta: !!(modifiers & Qt.MetaModifier)
                    }
                }

                function boardBounds() {
                    return constructionCanvas.boardBounds()
                }

                function normalizedPoint(mouseX, mouseY) {
                    return CanvasViewport.screenToCanvas(boardBounds(), mouseX, mouseY)
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

                function handleSettings() {
                    return {
                        canvasSizePx: drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasSizePx || 512) : 512,
                        rotateHandleOffsetPx: 28,
                        handleHitTolerancePx: 14,
                        rotateHandleHitTolerancePx: 18,
                        shiftConstrain: modifierShiftDown(activeModifiers),
                        angleSnapDeg: 15
                    }
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
                    if (CanvasHandles.isRectangleLike(kind)) {
                        var corners = CanvasHandles.rotatedRectCorners(object)
                        for (var cornerIndex = 0; cornerIndex < corners.length; ++cornerIndex) {
                            includePoint(bounds, corners[cornerIndex].x, corners[cornerIndex].y)
                        }
                        return bounds
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

                function hitSelectedHandle(mouseX, mouseY) {
                    var object = selectedGeneratedObject()
                    if (String(object.id || "").length === 0) {
                        return ({})
                    }
                    var hit = CanvasHandles.hitHandleAt(object, mouseX, mouseY, boardBounds(), handleSettings())
                    if (!hit.ok || hit.handle.readOnly === true) {
                        return ({})
                    }
                    return hit.handle
                }

                function updateSelectionHover(mouseX, mouseY, rawPoint) {
                    hoverObjectId = ""
                    hoverHandleId = ""
                    if (!drawingWorkspace.controller || drawingWorkspace.controller.selectedDrawingToolId !== "select_move") {
                        return
                    }
                    var handle = hitSelectedHandle(mouseX, mouseY)
                    if (String(handle.id || "").length > 0) {
                        hoverHandleId = String(handle.id || "")
                        hoverObjectId = String(selectedGeneratedObject().id || "")
                        return
                    }
                    hoverObjectId = String(drawingWorkspace.controller.hitDrawingObjectAtNormalized(rawPoint.x, rawPoint.y) || "")
                    if (!CanvasGestureState.isDragging(gestureState) && !CanvasGestureState.isMarquee(gestureState)) {
                        gestureState = CanvasGestureState.beginHover(gestureState, rawPoint, {
                            kind: hoverHandleId.length > 0 ? "handle" : hoverObjectId.length > 0 ? "object" : "none",
                            objectId: hoverObjectId,
                            handleId: hoverHandleId,
                            modifiers: gestureModifiers(activeModifiers)
                        })
                    }
                }

                function modifierShiftDown(modifiers) {
                    return !!(modifiers & Qt.ShiftModifier)
                }

                function modifierOptionDown(modifiers) {
                    return !!(modifiers & Qt.AltModifier)
                }

                function angleSnappedDegrees(degrees, increment) {
                    var step = Math.max(1, Number(increment || 15))
                    return Math.round(Number(degrees || 0) / step) * step
                }

                function constrainedVectorPoint(startX, startY, endX, endY, incrementDegrees) {
                    var dx = Number(endX || 0) - Number(startX || 0)
                    var dy = Number(endY || 0) - Number(startY || 0)
                    var distance = Math.sqrt(dx * dx + dy * dy)
                    if (distance <= 0.0000001) {
                        return { x: endX, y: endY }
                    }
                    var angle = angleSnappedDegrees(Math.atan2(dy, dx) * 180 / Math.PI, incrementDegrees) * Math.PI / 180
                    return {
                        x: Math.max(0, Math.min(1, Number(startX || 0) + Math.cos(angle) * distance)),
                        y: Math.max(0, Math.min(1, Number(startY || 0) + Math.sin(angle) * distance))
                    }
                }

                function pendingPoint() {
                    return drawingWorkspace.controller ? drawingWorkspace.controller.drawingPendingPoint || ({}) : ({})
                }

                function pendingPointActive() {
                    var point = pendingPoint()
                    return Number.isFinite(Number(point.x)) && Number.isFinite(Number(point.y))
                }

                function lineConstraintActive(modifiers) {
                    if (!drawingWorkspace.controller || !modifierShiftDown(modifiers) || !pendingPointActive()) {
                        return false
                    }
                    return String(drawingWorkspace.controller.selectedDrawingToolId || "") === "line_polyline"
                }

                function constrainLinePoint(point, modifiers) {
                    if (!lineConstraintActive(modifiers)) {
                        return point
                    }
                    var pending = pendingPoint()
                    var constrained = constrainedVectorPoint(Number(pending.x || 0), Number(pending.y || 0), Number(point.x || 0), Number(point.y || 0), 45)
                    return {
                        x: constrained.x,
                        y: constrained.y,
                        kind: "angle",
                        label: "angle",
                        stepPx: point.stepPx
                    }
                }

                function drawingPointForMouse(mouse, rawPoint) {
                    if (modifierOptionDown(mouse.modifiers)) {
                        return constrainLinePoint({
                            x: rawPoint.x,
                            y: rawPoint.y,
                            kind: "free",
                            label: "free",
                            stepPx: snapResolver.effectiveGridStepPx()
                        }, mouse.modifiers)
                    }
                    return constrainLinePoint(snapResolver.snappedPoint(rawPoint), mouse.modifiers)
                }

                function modifierHandlePoint(mouse) {
                    var rawPoint = normalizedPoint(mouse.x, mouse.y)
                    if (modifierOptionDown(mouse.modifiers)) {
                        return rawPoint
                    }
                    return snapResolver.gridSnappedPoint(rawPoint)
                }

                function applySelectedHandleDrag(handleId, point) {
                    var object = selectedGeneratedObject()
                    if (String(object.id || "") !== dragHandleObjectId || String(handleId || "").length === 0) {
                        return
                    }
                    var plan = CanvasHandles.handleUpdatePlan(object, handleId, point, handleSettings())
                    if (!plan.ok) {
                        return
                    }
                    var updates = plan.updates || []
                    for (var index = 0; index < updates.length; ++index) {
                        var update = updates[index] || ({})
                        drawingWorkspace.controller.updateSelectedDrawingObjectField(update.field, update.value)
                    }
                }

                function cancelActiveGesture() {
                    var canceled = CanvasGestureState.cancelGesture(gestureState)
                    gestureState = canceled.state
                    marqueeActive = false
                    marqueeMoved = false
                    dragHandleId = ""
                    dragHandleObjectId = ""
                    dragHandleMoved = false
                    dragObjectId = ""
                    dragObjectMoved = false
                    dragAnchorId = ""
                    selectionTogglePressed = false
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.endDrawingObjectMove()
                    }
                }

                function updatePreviewPoint(mouseX, mouseY) {
                    var rawPoint = normalizedPoint(mouseX, mouseY)
                    hoverRawX = rawPoint.x
                    hoverRawY = rawPoint.y
                    if (rawPoint.x < 0 || rawPoint.x > 1 || rawPoint.y < 0 || rawPoint.y > 1) {
                        hoverInside = false
                        hoverObjectId = ""
                        hoverHandleId = ""
                        hoverSnapKind = "none"
                        hoverSnapLabel = "none"
                        constructionCanvas.previewActive = false
                        constructionCanvas.requestPaint()
                        return rawPoint
                    }
                    updateSelectionHover(mouseX, mouseY, rawPoint)
                    if (drawingWorkspace.controller && drawingWorkspace.controller.selectedDrawingToolId === "select_move") {
                        hoverInside = true
                        hoverSnapX = rawPoint.x
                        hoverSnapY = rawPoint.y
                        hoverSnapKind = hoverHandleId.length > 0 ? "handle" : hoverObjectId.length > 0 ? "object" : "free"
                        hoverSnapLabel = hoverHandleId.length > 0 ? "handle" : hoverObjectId.length > 0 ? "object" : "select"
                        hoverSnapStepPx = Number(snapResolver.effectiveGridStepPx())
                        constructionCanvas.previewActive = false
                        constructionCanvas.requestPaint()
                        return rawPoint
                    }
                    var point = drawingPointForMouse({ modifiers: activeModifiers }, rawPoint)
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
                    hoverObjectId = ""
                    hoverHandleId = ""
                    hoverSnapKind = "none"
                    hoverSnapLabel = "none"
                    if (gestureState.mode === "hovering") {
                        gestureState = CanvasGestureState.initialGestureState()
                    }
                    constructionCanvas.previewActive = false
                    constructionCanvas.requestPaint()
                }

                onPressed: function(mouse) {
                    if (!drawingWorkspace.controller) {
                        return
                    }
                    activeModifiers = mouse.modifiers
                    var rawPoint = normalizedPoint(mouse.x, mouse.y)
                    var point = updatePreviewPoint(mouse.x, mouse.y)
                    if (drawingWorkspace.controller.selectedDrawingToolId === "select_move") {
                        selectionTogglePressed = false
                        var handle = hitSelectedHandle(mouse.x, mouse.y)
                        if (String(handle.id || "").length > 0) {
                            dragHandleId = String(handle.id || "")
                            dragHandleObjectId = String(selectedGeneratedObject().id || "")
                            dragHandleMoved = false
                            gestureState = CanvasGestureState.beginHandleDrag(gestureState, dragHandleObjectId, dragHandleId, rawPoint, gestureModifiers(mouse.modifiers))
                            drawingWorkspace.controller.beginDrawingObjectMove()
                            mouse.accepted = true
                            return
                        }
                        var objectId = drawingWorkspace.controller.hitDrawingObjectAtNormalized(rawPoint.x, rawPoint.y)
                        dragObjectId = String(objectId || "")
                        dragObjectMoved = false
                        var shiftSelecting = !!(mouse.modifiers & Qt.ShiftModifier)
                        if (dragObjectId.length > 0 && shiftSelecting) {
                            drawingWorkspace.controller.toggleDrawingObjectSelection(dragObjectId)
                            selectionTogglePressed = true
                            constructionCanvas.requestPaint()
                            mouse.accepted = true
                            return
                        }
                        if (dragObjectId.length > 0) {
                            if (!selectedObjectIdsContain(dragObjectId)) {
                                drawingWorkspace.controller.selectDrawingObjectAtNormalized(rawPoint.x, rawPoint.y)
                            }
                        } else if (shiftSelecting) {
                            selectionTogglePressed = true
                        }
                        var dragStart = snapResolver.gridSnappedPoint(rawPoint)
                        dragObjectLastX = dragStart.x
                        dragObjectLastY = dragStart.y
                        if (dragObjectId.length > 0) {
                            gestureState = CanvasGestureState.beginObjectDrag(gestureState, dragObjectId, dragStart, selectedObjectIdList(), gestureModifiers(mouse.modifiers))
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
                            gestureState = CanvasGestureState.beginMarquee(gestureState, rawPoint, gestureModifiers(mouse.modifiers))
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
                    activeModifiers = mouse.modifiers
                    updatePreviewPoint(mouse.x, mouse.y)
                    if (drawingWorkspace.controller && pressed && dragHandleId.length > 0) {
                        var handlePoint = modifierHandlePoint(mouse)
                        gestureState = CanvasGestureState.updateGesture(gestureState, {
                            point: handlePoint,
                            modifiers: gestureModifiers(mouse.modifiers)
                        })
                        applySelectedHandleDrag(dragHandleId, handlePoint)
                        dragHandleMoved = gestureState.moved
                        constructionCanvas.requestPaint()
                        return
                    }
                    if (drawingWorkspace.controller && pressed && marqueeActive) {
                        var rawMarqueePoint = normalizedPoint(mouse.x, mouse.y)
                        marqueeEndX = rawMarqueePoint.x
                        marqueeEndY = rawMarqueePoint.y
                        var bounds = boardBounds()
                        marqueeMoved = Math.abs((marqueeEndX - marqueeStartX) * bounds.size) > 4 || Math.abs((marqueeEndY - marqueeStartY) * bounds.size) > 4
                        gestureState = CanvasGestureState.updateGesture(gestureState, {
                            point: rawMarqueePoint,
                            modifiers: gestureModifiers(mouse.modifiers),
                            moveTolerance: 4 / Math.max(1, bounds.size)
                        })
                        marqueeMoved = gestureState.moved
                        constructionCanvas.requestPaint()
                        return
                    }
                    if (drawingWorkspace.controller && pressed && dragObjectId.length > 0) {
                        var movePoint = snapResolver.gridSnappedPoint(normalizedPoint(mouse.x, mouse.y))
                        gestureState = CanvasGestureState.updateGesture(gestureState, {
                            point: movePoint,
                            modifiers: gestureModifiers(mouse.modifiers)
                        })
                        var dx = movePoint.x - dragObjectLastX
                        var dy = movePoint.y - dragObjectLastY
                        if (Math.abs(dx) >= 0.000001 || Math.abs(dy) >= 0.000001) {
                            drawingWorkspace.controller.moveSelectedDrawingObjectBy(dx, dy)
                            dragObjectLastX = movePoint.x
                            dragObjectLastY = movePoint.y
                            dragObjectMoved = gestureState.moved
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

                onReleased: function(mouse) {
                    activeModifiers = mouse.modifiers
                    var releasePoint = normalizedPoint(mouse.x, mouse.y)
                    if (drawingWorkspace.controller && marqueeActive && marqueeMoved) {
                        var marqueeFinish = CanvasGestureState.finishGesture(gestureState, {
                            point: releasePoint,
                            objectIds: marqueeSelectionIds()
                        })
                        if (marqueeFinish.intent.kind === "select_objects") {
                            drawingWorkspace.controller.selectDrawingObjects(marqueeFinish.intent.objectIds)
                        }
                        suppressClickOnce = true
                    } else if (CanvasGestureState.isDragging(gestureState)) {
                        CanvasGestureState.finishGesture(gestureState, {
                            point: releasePoint,
                            incremental: true
                        })
                    }
                    if (selectionTogglePressed || dragObjectId.length > 0 || dragAnchorId.length > 0 || dragHandleMoved || dragObjectMoved) {
                        suppressClickOnce = true
                    }
                    gestureState = CanvasGestureState.initialGestureState()
                    marqueeActive = false
                    marqueeMoved = false
                    dragAnchorId = ""
                    dragHandleId = ""
                    dragHandleObjectId = ""
                    dragHandleMoved = false
                    dragObjectId = ""
                    dragObjectMoved = false
                    selectionTogglePressed = false
                    updateSelectionHover(mouse.x, mouse.y, releasePoint)
                    if (drawingWorkspace.controller) {
                        drawingWorkspace.controller.endDrawingObjectMove()
                    }
                }

                onClicked: function(mouse) {
                    if (!drawingWorkspace.controller) {
                        return
                    }
                    activeModifiers = mouse.modifiers
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
                    var panState = CanvasGestureState.beginPan(CanvasGestureState.initialGestureState(), { x: 0, y: 0 }, gestureModifiers(wheel.modifiers))
                    panState = CanvasGestureState.updateGesture(panState, { screenPoint: { x: pixelX, y: pixelY }, modifiers: gestureModifiers(wheel.modifiers) })
                    var panFinish = CanvasGestureState.finishGesture(panState, { screenPoint: { x: pixelX, y: pixelY } })
                    if (panFinish.intent.kind === "pan") {
                        drawingWorkspace.controller.panDrawingCanvasBy(panFinish.intent.dxPx, panFinish.intent.dyPx)
                    }
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
                    text: canvasInput.selectionStatusLabel()
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
