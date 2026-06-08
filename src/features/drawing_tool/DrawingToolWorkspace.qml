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
    property bool initialControlScriptAttempted: false

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

            function drawingControlObjectSummary() {
                var objects = drawingWorkspace.controller ? canvasInput.asArray(drawingWorkspace.controller.drawingCanvasObjects(drawingWorkspace.controller.revision)) : []
                var summary = []
                for (var index = 0; index < objects.length; ++index) {
                    var object = objects[index] || ({})
                    var id = String(object.id || "")
                    if (id.indexOf("script_") !== 0) {
                        continue
                    }
                    summary.push({
                        id: id,
                        kind: String(object.kind || ""),
                        x: Number(object.x !== undefined ? object.x : object.x_px || 0),
                        y: Number(object.y !== undefined ? object.y : object.y_px || 0),
                        cx: Number(object.cx !== undefined ? object.cx : object.cx_px || 0),
                        cy: Number(object.cy !== undefined ? object.cy : object.cy_px || 0),
                        x1: Number(object.x1 !== undefined ? object.x1 : object.x1_px || 0),
                        y1: Number(object.y1 !== undefined ? object.y1 : object.y1_px || 0),
                        x2: Number(object.x2 !== undefined ? object.x2 : object.x2_px || 0),
                        y2: Number(object.y2 !== undefined ? object.y2 : object.y2_px || 0),
                        width: Number(object.width !== undefined ? object.width : object.width_px || 0),
                        height: Number(object.height !== undefined ? object.height : object.height_px || 0),
                        radius: Number(object.radius !== undefined ? object.radius : object.radius_px || 0),
                        sides: Number(object.sides || 0),
                        rotation_deg: Number(object.rotation_deg || 0),
                        start_angle_deg: Number(object.start_angle_deg || 0),
                        end_angle_deg: Number(object.end_angle_deg || 0)
                    })
                }
                return summary
            }

            function drawingControlSelectedCount() {
                var selectedCount = drawingWorkspace.controller ? canvasInput.selectedObjectIdList().length : 0
                if (selectedCount <= 0 && drawingWorkspace.controller && String(drawingWorkspace.controller.selectedDrawingObjectId || "").indexOf("script_") === 0) {
                    selectedCount = 1
                }
                return selectedCount
            }

            function drawingControlScriptObjectCount() {
                return drawingControlObjectSummary().length
            }

            function drawingControlScriptSummary(result) {
                return {
                    ok: result && result.ok === true,
                    failures: drawingControlFailureDetails(result && result.failures ? result.failures : []),
                    executed: result && result.executed !== undefined ? Number(result.executed || 0) : 0,
                    objectCount: drawingControlScriptObjectCount(),
                    objects: drawingControlObjectSummary(),
                    selectedObjectId: drawingWorkspace.controller ? String(drawingWorkspace.controller.selectedDrawingObjectId || "") : "",
                    selectedCount: drawingControlSelectedCount(),
                    revision: drawingWorkspace.controller ? Number(drawingWorkspace.controller.revision || 0) : 0,
                    viewport: {
                        zoom: drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasZoom || 1) : 1,
                        panX: drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanXPx || 0) : 0,
                        panY: drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanYPx || 0) : 0
                    }
                }
            }

            function drawingControlFailureDetails(failures) {
                var result = []
                var source = failures || []
                for (var index = 0; index < source.length; ++index) {
                    var failure = source[index]
                    if (failure && typeof failure === "object") {
                        result.push({
                            stepIndex: Number(failure.stepIndex !== undefined ? failure.stepIndex : -1),
                            stepType: String(failure.stepType || "runtime"),
                            message: String(failure.message || "control script failure")
                        })
                    } else {
                        result.push({
                            stepIndex: -1,
                            stepType: "validation",
                            message: String(failure || "control script validation failure")
                        })
                    }
                }
                return result
            }

            function maybeRunInitialControlScript() {
                if (drawingWorkspace.initialControlScriptAttempted
                        || !drawingWorkspace.visible
                        || !drawingWorkspace.controller
                        || !drawingWorkspace.controller.drawingControlScriptRequested) {
                    return
                }
                drawingWorkspace.initialControlScriptAttempted = true
                var plan = drawingToolScriptRuntime.executionPlan(
                            drawingWorkspace.controller.drawingControlScriptDocument,
                            drawingWorkspace.controller.drawingControlLibraryDocument)
                var result = plan.ok
                        ? controlRunner.runExecutionPlan(plan.plan)
                        : ({ ok: false, failures: plan.failures, executed: 0 })
                var summary = drawingControlScriptSummary(result)
                drawingWorkspace.controller.drawingControlScriptResult = summary
                console.log("drawing_control_script_result " + JSON.stringify(summary))
                if (drawingWorkspace.controller.drawingControlScriptExitOnComplete) {
                    Qt.callLater(Qt.quit)
                }
            }

            Timer {
                id: initialControlScriptTimer
                interval: 100
                repeat: false
                onTriggered: canvasFrame.maybeRunInitialControlScript()
            }

            Component.onCompleted: initialControlScriptTimer.start()
            onVisibleChanged: {
                if (visible) {
                    initialControlScriptTimer.restart()
                }
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
                    return drawingCanvasRuntime.canvasToScreenX(bounds, normalizedX)
                }

                function pxY(bounds, normalizedY) {
                    return drawingCanvasRuntime.canvasToScreenY(bounds, normalizedY)
                }

                function boardBounds() {
                    var zoom = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasZoom || 1.0) : 1.0
                    var panX = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanXPx || 0) : 0
                    var panY = drawingWorkspace.controller ? Number(drawingWorkspace.controller.drawingCanvasPanYPx || 0) : 0
                    return drawingCanvasRuntime.boardBounds(width, height, zoom, panX, panY)
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
                    if (!drawingCanvasRuntime.isMarquee(canvasInput.gestureState)) {
                        return
                    }
                    var minX = Math.min(canvasInput.gestureState.startPoint.x, canvasInput.gestureState.lastPoint.x)
                    var minY = Math.min(canvasInput.gestureState.startPoint.y, canvasInput.gestureState.lastPoint.y)
                    var maxX = Math.max(canvasInput.gestureState.startPoint.x, canvasInput.gestureState.lastPoint.x)
                    var maxY = Math.max(canvasInput.gestureState.startPoint.y, canvasInput.gestureState.lastPoint.y)
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

            DrawingCanvasControlRunner {
                id: controlRunner
                controller: drawingWorkspace.controller
                inputArea: canvasInput
                constructionCanvas: constructionCanvas
            }

            MouseArea {
                id: canvasInput
                anchors.fill: constructionCanvas
                hoverEnabled: true
                z: 2
                property string dragAnchorId: ""
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
                property int activeModifiers: Qt.NoModifier
                property var gestureState: drawingCanvasRuntime.initialGestureState()
                property var interactionMetricsState: drawingInteractionRuntime.initialMetricsState()
                property var lastInteractionMetrics: ({})
                property bool interactionMetricsLogEnabled: drawingWorkspace.controller ? !!drawingWorkspace.controller.drawingInteractionMetricsLogEnabled : false
                property var interactionTelemetryState: drawingInteractionRuntime.initialTelemetryState()
                property var lastInteractionEvents: []
                property bool interactionTelemetryLogEnabled: drawingWorkspace.controller ? !!drawingWorkspace.controller.drawingInteractionTelemetryLogEnabled : false
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
                    if (drawingCanvasRuntime.isHandleDrag(gestureState) || drawingCanvasRuntime.isObjectDrag(gestureState) || drawingCanvasRuntime.isMarquee(gestureState)) {
                        return drawingCanvasRuntime.gestureLabel(gestureState)
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
                    if (drawingCanvasRuntime.isHandleDrag(gestureState) || hoverHandleId.length > 0) {
                        return Qt.SizeAllCursor
                    }
                    if (drawingCanvasRuntime.isObjectDrag(gestureState)) {
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

                function visibleObjectCount() {
                    return drawingWorkspace.controller ? asArray(drawingWorkspace.controller.drawingCanvasObjects(drawingWorkspace.controller.revision)).length : 0
                }

                function metricsSnapshot() {
                    var selectedIds = selectedObjectIdList()
                    var selectedCount = selectedIds.length
                    if (selectedCount <= 0 && drawingWorkspace.controller && String(drawingWorkspace.controller.selectedDrawingObjectId || "").indexOf("script_") === 0) {
                        selectedCount = 1
                    }
                    return {
                        revision: drawingWorkspace.controller ? Number(drawingWorkspace.controller.revision || 0) : 0,
                        selectedCount: selectedCount,
                        visibleObjectCount: visibleObjectCount()
                    }
                }

                function metricsNowMs() {
                    return Date.now()
                }

                function beginInteractionMetrics(mode) {
                    interactionMetricsState = drawingInteractionRuntime.beginMetricsInteraction(interactionMetricsState, mode, metricsNowMs(), metricsSnapshot())
                    interactionTelemetryState = drawingInteractionRuntime.beginTelemetryInteraction(interactionTelemetryState, mode, metricsNowMs(), metricsSnapshot())
                }

                function finishInteractionMetrics(canceled) {
                    if (!interactionMetricsState.active) {
                        return
                    }
                    var finished = canceled
                            ? drawingInteractionRuntime.cancelMetricsInteraction(interactionMetricsState, metricsNowMs(), metricsSnapshot())
                            : drawingInteractionRuntime.finishMetricsInteraction(interactionMetricsState, metricsNowMs(), metricsSnapshot())
                    interactionMetricsState = finished.state
                    lastInteractionMetrics = finished.record
                    if (interactionMetricsLogEnabled) {
                        console.log("drawing_canvas_interaction_metrics " + JSON.stringify(lastInteractionMetrics))
                    }
                    var telemetry = canceled
                            ? drawingInteractionRuntime.cancelTelemetryInteraction(interactionTelemetryState, metricsNowMs(), metricsSnapshot())
                            : drawingInteractionRuntime.finishTelemetryInteraction(interactionTelemetryState, metricsNowMs(), metricsSnapshot())
                    interactionTelemetryState = telemetry.state
                    lastInteractionEvents = telemetry.events
                    if (interactionTelemetryLogEnabled) {
                        console.log("drawing_canvas_interaction_events " + JSON.stringify(lastInteractionEvents))
                    }
                }

                function recordPointerMoveMetric() {
                    interactionMetricsState = drawingInteractionRuntime.recordMetricsPointerMove(interactionMetricsState)
                    interactionTelemetryState = drawingInteractionRuntime.recordTelemetryPointerMove(interactionTelemetryState)
                }

                function recordControllerMutationMetric(kind) {
                    interactionMetricsState = drawingInteractionRuntime.recordMetricsControllerMutation(interactionMetricsState, kind)
                    interactionTelemetryState = drawingInteractionRuntime.recordTelemetryControllerMutation(interactionTelemetryState, kind)
                }

                function recordRenderRequestMetric() {
                    interactionMetricsState = drawingInteractionRuntime.recordMetricsRenderRequest(interactionMetricsState)
                    interactionTelemetryState = drawingInteractionRuntime.recordTelemetryRenderRequest(interactionTelemetryState)
                }

                function recordHitTestMetric() {
                    interactionMetricsState = drawingInteractionRuntime.recordMetricsHitTest(interactionMetricsState)
                    interactionTelemetryState = drawingInteractionRuntime.recordTelemetryHitTest(interactionTelemetryState)
                }

                function recordSnapMetric() {
                    interactionMetricsState = drawingInteractionRuntime.recordMetricsSnap(interactionMetricsState)
                    interactionTelemetryState = drawingInteractionRuntime.recordTelemetrySnap(interactionTelemetryState)
                }

                function recordHandlePlanMetric() {
                    interactionMetricsState = drawingInteractionRuntime.recordMetricsHandlePlan(interactionMetricsState)
                    interactionTelemetryState = drawingInteractionRuntime.recordTelemetryHandlePlan(interactionTelemetryState)
                }

                function requestCanvasPaint() {
                    recordRenderRequestMetric()
                    constructionCanvas.requestPaint()
                }

                function controlClickCanvasPoint(point) {
                    if (!drawingWorkspace.controller) {
                        return { ok: false, message: "controller unavailable" }
                    }
                    beginInteractionMetrics("draw_click")
                    var snapped = drawingPointForMouse({ modifiers: Qt.NoModifier }, point)
                    drawingWorkspace.controller.handleDrawingCanvasClick(snapped.x, snapped.y, Math.round(Number(snapped.stepPx || snapResolver.effectiveGridStepPx())))
                    recordControllerMutationMetric("draw_click")
                    requestCanvasPaint()
                    finishInteractionMetrics(false)
                    return { ok: true }
                }

                function controlPanCanvasBy(dx, dy) {
                    if (!drawingWorkspace.controller) {
                        return { ok: false, message: "controller unavailable" }
                    }
                    beginInteractionMetrics("panning")
                    drawingWorkspace.controller.panDrawingCanvasBy(dx, dy)
                    recordControllerMutationMetric("pan_viewport")
                    requestCanvasPaint()
                    finishInteractionMetrics(false)
                    return { ok: true }
                }

                function controlZoomCanvasAt(factor, screenPoint) {
                    if (!drawingWorkspace.controller) {
                        return { ok: false, message: "controller unavailable" }
                    }
                    beginInteractionMetrics("zooming")
                    drawingWorkspace.controller.zoomDrawingCanvasAt(factor, screenPoint.x, screenPoint.y, constructionCanvas.width, constructionCanvas.height)
                    recordControllerMutationMetric("zoom_viewport")
                    requestCanvasPaint()
                    finishInteractionMetrics(false)
                    return { ok: true }
                }

                function finiteControlPoint(point) {
                    var x = Number(point && point.x)
                    var y = Number(point && point.y)
                    if (!Number.isFinite(x) || !Number.isFinite(y)) {
                        return null
                    }
                    return {
                        x: Math.max(0, Math.min(1, x)),
                        y: Math.max(0, Math.min(1, y))
                    }
                }

                function controlMoveCount(pointerMoves) {
                    return Math.max(1, Math.round(Number(pointerMoves || 1)))
                }

                function interpolatedPoint(fromPoint, toPoint, index, count) {
                    var t = Math.max(0, Math.min(1, Number(index || 0) / Math.max(1, Number(count || 1))))
                    return {
                        x: fromPoint.x + (toPoint.x - fromPoint.x) * t,
                        y: fromPoint.y + (toPoint.y - fromPoint.y) * t
                    }
                }

                function screenPointForNormalizedPoint(point) {
                    var bounds = boardBounds()
                    return {
                        x: drawingCanvasRuntime.canvasToScreenX(bounds, point.x),
                        y: drawingCanvasRuntime.canvasToScreenY(bounds, point.y)
                    }
                }

                function ensureControlSelectionAt(point) {
                    if (!drawingWorkspace.controller) {
                        return ""
                    }
                    var objectId = drawingWorkspace.controller.hitDrawingObjectAtNormalized(point.x, point.y)
                    if (String(objectId || "").length > 0 && !selectedObjectIdsContain(objectId)) {
                        drawingWorkspace.controller.selectDrawingObjectAtNormalized(point.x, point.y)
                    }
                    return String(objectId || "")
                }

                function controlDragObject(fromPoint, toPoint, pointerMoves) {
                    if (!drawingWorkspace.controller) {
                        return { ok: false, message: "controller unavailable" }
                    }
                    var from = finiteControlPoint(fromPoint)
                    var to = finiteControlPoint(toPoint)
                    if (!from || !to) {
                        return { ok: false, message: "drag object requires finite from/to points" }
                    }
                    var objectId = ensureControlSelectionAt(from)
                    if (objectId.length <= 0) {
                        return { ok: false, message: "drag object target not found" }
                    }
                    activeModifiers = Qt.NoModifier
                    var dragStart = snapResolver.gridSnappedPoint(from)
                    beginInteractionMetrics("dragging_object")
                    recordHitTestMetric()
                    recordSnapMetric()
                    gestureState = drawingCanvasRuntime.beginObjectDrag(gestureState, objectId, dragStart, selectedObjectIdList(), gestureModifiers(Qt.NoModifier))
                    drawingWorkspace.controller.beginDrawingObjectMove()

                    var moves = controlMoveCount(pointerMoves)
                    for (var index = 1; index <= moves; ++index) {
                        recordPointerMoveMetric()
                        recordSnapMetric()
                        var movePoint = snapResolver.gridSnappedPoint(interpolatedPoint(from, to, index, moves))
                        var previousPoint = gestureState.lastPoint
                        gestureState = drawingCanvasRuntime.updateGesture(gestureState, {
                            point: movePoint,
                            modifiers: gestureModifiers(Qt.NoModifier)
                        })
                        var dx = movePoint.x - previousPoint.x
                        var dy = movePoint.y - previousPoint.y
                        if (Math.abs(dx) >= 0.000001 || Math.abs(dy) >= 0.000001) {
                            drawingWorkspace.controller.moveSelectedDrawingObjectBy(dx, dy)
                            recordControllerMutationMetric("move_selected")
                            requestCanvasPaint()
                        }
                    }

                    finishIncrementalActiveGesture(to, true)
                    updateSelectionHover(screenPointForNormalizedPoint(to).x, screenPointForNormalizedPoint(to).y, to)
                    return { ok: true }
                }

                function controlDragHandle(handleId, fromPoint, toPoint, pointerMoves) {
                    if (!drawingWorkspace.controller) {
                        return { ok: false, message: "controller unavailable" }
                    }
                    var from = finiteControlPoint(fromPoint)
                    var to = finiteControlPoint(toPoint)
                    if (!from || !to) {
                        return { ok: false, message: "drag handle requires finite from/to points" }
                    }
                    ensureControlSelectionAt(from)
                    var screenFrom = screenPointForNormalizedPoint(from)
                    var handle = hitSelectedHandle(screenFrom.x, screenFrom.y)
                    if (String(handle.id || "") !== String(handleId || "")) {
                        return { ok: false, message: "drag handle target not found: " + String(handleId || "") }
                    }
                    activeModifiers = Qt.NoModifier
                    beginInteractionMetrics("dragging_handle")
                    recordHitTestMetric()
                    gestureState = drawingCanvasRuntime.beginHandleDrag(gestureState, String(selectedGeneratedObject().id || ""), String(handle.id || ""), from, gestureModifiers(Qt.NoModifier))
                    drawingWorkspace.controller.beginDrawingObjectMove()

                    var moves = controlMoveCount(pointerMoves)
                    for (var index = 1; index <= moves; ++index) {
                        recordPointerMoveMetric()
                        recordSnapMetric()
                        var handlePoint = snapResolver.gridSnappedPoint(interpolatedPoint(from, to, index, moves))
                        gestureState = drawingCanvasRuntime.updateGesture(gestureState, {
                            point: handlePoint,
                            modifiers: gestureModifiers(Qt.NoModifier)
                        })
                        applySelectedHandleDrag(gestureState.handleId, handlePoint)
                        requestCanvasPaint()
                    }

                    finishIncrementalActiveGesture(to, true)
                    updateSelectionHover(screenPointForNormalizedPoint(to).x, screenPointForNormalizedPoint(to).y, to)
                    return { ok: true }
                }

                function controlMarqueeSelect(fromPoint, toPoint, pointerMoves) {
                    if (!drawingWorkspace.controller) {
                        return { ok: false, message: "controller unavailable" }
                    }
                    var from = finiteControlPoint(fromPoint)
                    var to = finiteControlPoint(toPoint)
                    if (!from || !to) {
                        return { ok: false, message: "marquee select requires finite from/to points" }
                    }
                    activeModifiers = Qt.NoModifier
                    beginInteractionMetrics("marquee_select")
                    recordHitTestMetric()
                    gestureState = drawingCanvasRuntime.beginMarquee(gestureState, from, gestureModifiers(Qt.NoModifier))
                    requestCanvasPaint()

                    var moves = controlMoveCount(pointerMoves)
                    for (var index = 1; index <= moves; ++index) {
                        recordPointerMoveMetric()
                        var point = interpolatedPoint(from, to, index, moves)
                        gestureState = drawingCanvasRuntime.updateGesture(gestureState, {
                            point: point,
                            modifiers: gestureModifiers(Qt.NoModifier),
                            moveTolerance: 0
                        })
                        requestCanvasPaint()
                    }

                    finishMarqueeActiveGesture(to, marqueeSelectionIds())
                    updateSelectionHover(screenPointForNormalizedPoint(to).x, screenPointForNormalizedPoint(to).y, to)
                    return { ok: true }
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
                    return drawingCanvasRuntime.screenToCanvas(boardBounds(), mouseX, mouseY)
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

                function marqueeSelectionIds() {
                    var minX = Math.max(0, Math.min(gestureState.startPoint.x, gestureState.lastPoint.x))
                    var minY = Math.max(0, Math.min(gestureState.startPoint.y, gestureState.lastPoint.y))
                    var maxX = Math.min(1, Math.max(gestureState.startPoint.x, gestureState.lastPoint.x))
                    var maxY = Math.min(1, Math.max(gestureState.startPoint.y, gestureState.lastPoint.y))
                    var ids = []
                    var objects = drawingWorkspace.controller ? drawingWorkspace.controller.drawingCanvasObjects(drawingWorkspace.controller.revision) : []
                    for (var index = 0; index < objects.length; ++index) {
                        var object = objects[index] || ({})
                        var id = String(object.id || "")
                        if (id.indexOf("script_") !== 0) {
                            continue
                        }
                        if (drawingCanvasRuntime.objectIntersectsBounds(object, minX, minY, maxX, maxY)) {
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
                    recordHitTestMetric()
                    var hit = drawingCanvasRuntime.hitHandleAt(object, mouseX, mouseY, boardBounds(), handleSettings())
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
                    recordHitTestMetric()
                    hoverObjectId = String(drawingWorkspace.controller.hitDrawingObjectAtNormalized(rawPoint.x, rawPoint.y) || "")
                    if (!drawingCanvasRuntime.isDragging(gestureState) && !drawingCanvasRuntime.isMarquee(gestureState)) {
                        gestureState = drawingCanvasRuntime.beginHover(gestureState, rawPoint, {
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
                    recordSnapMetric()
                    return constrainLinePoint(snapResolver.snappedPoint(rawPoint), mouse.modifiers)
                }

                function modifierHandlePoint(mouse) {
                    var rawPoint = normalizedPoint(mouse.x, mouse.y)
                    if (modifierOptionDown(mouse.modifiers)) {
                        return rawPoint
                    }
                    recordSnapMetric()
                    return snapResolver.gridSnappedPoint(rawPoint)
                }

                function applySelectedHandleDrag(handleId, point) {
                    var object = selectedGeneratedObject()
                    if (String(object.id || "") !== String(gestureState.objectId || "") || String(handleId || "").length === 0) {
                        return
                    }
                    var plan = drawingCanvasRuntime.handleUpdatePlan(object, handleId, point, handleSettings())
                    recordHandlePlanMetric()
                    if (!plan.ok) {
                        return
                    }
                    var updates = plan.updates || []
                    for (var index = 0; index < updates.length; ++index) {
                        var update = updates[index] || ({})
                        drawingWorkspace.controller.updateSelectedDrawingObjectField(update.field, update.value)
                        recordControllerMutationMetric("update_handle_field")
                    }
                }

                function resetActiveGestureLifecycle(cancelled, endObjectMove) {
                    finishInteractionMetrics(cancelled)
                    gestureState = drawingCanvasRuntime.initialGestureState()
                    dragAnchorId = ""
                    selectionTogglePressed = false
                    if (endObjectMove && drawingWorkspace.controller) {
                        drawingWorkspace.controller.endDrawingObjectMove()
                    }
                }

                function finishIncrementalActiveGesture(point, endObjectMove) {
                    drawingCanvasRuntime.finishGesture(gestureState, {
                        point: point,
                        incremental: true
                    })
                    resetActiveGestureLifecycle(false, endObjectMove)
                }

                function finishMarqueeActiveGesture(point, objectIds) {
                    var marqueeFinish = drawingCanvasRuntime.finishGesture(gestureState, {
                        point: point,
                        objectIds: objectIds
                    })
                    if (marqueeFinish.intent.kind === "select_objects" && drawingWorkspace.controller) {
                        drawingWorkspace.controller.selectDrawingObjects(marqueeFinish.intent.objectIds)
                        recordControllerMutationMetric("select_objects")
                    }
                    resetActiveGestureLifecycle(false, false)
                    return marqueeFinish.intent
                }

                function applyReleaseAction(action, releasePoint) {
                    if (drawingWorkspace.controller && action.shouldSelectMarquee) {
                        finishMarqueeActiveGesture(releasePoint, marqueeSelectionIds())
                        suppressClickOnce = true
                    } else if (action.shouldFinishIncrementalDrag) {
                        finishIncrementalActiveGesture(releasePoint, true)
                    }
                    if (selectionTogglePressed || dragAnchorId.length > 0 || action.shouldSuppressClick) {
                        suppressClickOnce = true
                    }
                    if (action.shouldResetLifecycle
                            && !action.shouldFinishIncrementalDrag
                            && !action.shouldSelectMarquee) {
                        resetActiveGestureLifecycle(false, action.shouldEndObjectMove)
                    }
                }

                function cancelActiveGesture() {
                    drawingCanvasRuntime.cancelGesture(gestureState)
                    resetActiveGestureLifecycle(true, true)
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
                        requestCanvasPaint()
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
                        requestCanvasPaint()
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
                    requestCanvasPaint()
                    return point
                }

                onExited: {
                    hoverInside = false
                    hoverObjectId = ""
                    hoverHandleId = ""
                    hoverSnapKind = "none"
                    hoverSnapLabel = "none"
                    if (gestureState.mode === "hovering") {
                        gestureState = drawingCanvasRuntime.initialGestureState()
                    }
                    constructionCanvas.previewActive = false
                    requestCanvasPaint()
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
                            beginInteractionMetrics("dragging_handle")
                            recordHitTestMetric()
                            gestureState = drawingCanvasRuntime.beginHandleDrag(gestureState, String(selectedGeneratedObject().id || ""), String(handle.id || ""), rawPoint, gestureModifiers(mouse.modifiers))
                            drawingWorkspace.controller.beginDrawingObjectMove()
                            mouse.accepted = true
                            return
                        }
                        var objectId = drawingWorkspace.controller.hitDrawingObjectAtNormalized(rawPoint.x, rawPoint.y)
                        var shiftSelecting = !!(mouse.modifiers & Qt.ShiftModifier)
                        if (String(objectId || "").length > 0 && shiftSelecting) {
                            drawingWorkspace.controller.toggleDrawingObjectSelection(String(objectId || ""))
                            recordControllerMutationMetric("toggle_selection")
                            selectionTogglePressed = true
                            requestCanvasPaint()
                            mouse.accepted = true
                            return
                        }
                        if (String(objectId || "").length > 0) {
                            if (!selectedObjectIdsContain(objectId)) {
                                drawingWorkspace.controller.selectDrawingObjectAtNormalized(rawPoint.x, rawPoint.y)
                            }
                        } else if (shiftSelecting) {
                            selectionTogglePressed = true
                        }
                        recordSnapMetric()
                        var dragStart = snapResolver.gridSnappedPoint(rawPoint)
                        if (String(objectId || "").length > 0) {
                            beginInteractionMetrics("dragging_object")
                            recordHitTestMetric()
                            recordSnapMetric()
                            gestureState = drawingCanvasRuntime.beginObjectDrag(gestureState, String(objectId || ""), dragStart, selectedObjectIdList(), gestureModifiers(mouse.modifiers))
                            drawingWorkspace.controller.beginDrawingObjectMove()
                            mouse.accepted = true
                        }
                        if (String(objectId || "").length === 0) {
                            beginInteractionMetrics("marquee_select")
                            recordHitTestMetric()
                            gestureState = drawingCanvasRuntime.beginMarquee(gestureState, rawPoint, gestureModifiers(mouse.modifiers))
                            requestCanvasPaint()
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
                    if (drawingWorkspace.controller && pressed && drawingCanvasRuntime.isHandleDrag(gestureState)) {
                        recordPointerMoveMetric()
                        var handlePoint = modifierHandlePoint(mouse)
                        hoverRawX = handlePoint.x
                        hoverRawY = handlePoint.y
                        hoverInside = handlePoint.x >= 0 && handlePoint.x <= 1 && handlePoint.y >= 0 && handlePoint.y <= 1
                        gestureState = drawingCanvasRuntime.updateGesture(gestureState, {
                            point: handlePoint,
                            modifiers: gestureModifiers(mouse.modifiers)
                        })
                        applySelectedHandleDrag(gestureState.handleId, handlePoint)
                        requestCanvasPaint()
                        return
                    }
                    if (drawingWorkspace.controller && pressed && drawingCanvasRuntime.isMarquee(gestureState)) {
                        recordPointerMoveMetric()
                        var rawMarqueePoint = normalizedPoint(mouse.x, mouse.y)
                        hoverRawX = rawMarqueePoint.x
                        hoverRawY = rawMarqueePoint.y
                        hoverInside = rawMarqueePoint.x >= 0 && rawMarqueePoint.x <= 1 && rawMarqueePoint.y >= 0 && rawMarqueePoint.y <= 1
                        var bounds = boardBounds()
                        gestureState = drawingCanvasRuntime.updateGesture(gestureState, {
                            point: rawMarqueePoint,
                            modifiers: gestureModifiers(mouse.modifiers),
                            moveTolerance: 4 / Math.max(1, bounds.size)
                        })
                        requestCanvasPaint()
                        return
                    }
                    if (drawingWorkspace.controller && pressed && drawingCanvasRuntime.isObjectDrag(gestureState)) {
                        recordPointerMoveMetric()
                        recordSnapMetric()
                        var movePoint = snapResolver.gridSnappedPoint(normalizedPoint(mouse.x, mouse.y))
                        hoverRawX = movePoint.x
                        hoverRawY = movePoint.y
                        hoverInside = movePoint.x >= 0 && movePoint.x <= 1 && movePoint.y >= 0 && movePoint.y <= 1
                        var previousPoint = gestureState.lastPoint
                        gestureState = drawingCanvasRuntime.updateGesture(gestureState, {
                            point: movePoint,
                            modifiers: gestureModifiers(mouse.modifiers)
                        })
                        var dx = movePoint.x - previousPoint.x
                        var dy = movePoint.y - previousPoint.y
                        if (Math.abs(dx) >= 0.000001 || Math.abs(dy) >= 0.000001) {
                            drawingWorkspace.controller.moveSelectedDrawingObjectBy(dx, dy)
                            recordControllerMutationMetric("move_selected")
                            requestCanvasPaint()
                        }
                        return
                    }
                    updatePreviewPoint(mouse.x, mouse.y)
                    if (pressed && interactionMetricsState.active) {
                        recordPointerMoveMetric()
                    }
                    if (!drawingWorkspace.controller || dragAnchorId.length === 0 || !pressed) {
                        return
                    }
                    recordSnapMetric()
                    var point = snapResolver.snappedPoint(normalizedPoint(mouse.x, mouse.y))
                    drawingWorkspace.controller.setDrawingAnchorPosition(dragAnchorId, point.x, point.y)
                    recordControllerMutationMetric("move_anchor")
                    requestCanvasPaint()
                }

                onReleased: function(mouse) {
                    activeModifiers = mouse.modifiers
                    var releasePoint = normalizedPoint(mouse.x, mouse.y)
                    applyReleaseAction(drawingCanvasRuntime.finishAction(gestureState), releasePoint)
                    updateSelectionHover(mouse.x, mouse.y, releasePoint)
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
                    beginInteractionMetrics("draw_click")
                    recordSnapMetric()
                    drawingWorkspace.controller.handleDrawingCanvasClick(point.x, point.y, Math.round(Number(point.stepPx || snapResolver.effectiveGridStepPx())))
                    recordControllerMutationMetric("draw_click")
                    requestCanvasPaint()
                    finishInteractionMetrics(false)
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
                        beginInteractionMetrics("zooming")
                        drawingWorkspace.controller.zoomDrawingCanvasAt(zoomFactor, wheel.x, wheel.y, constructionCanvas.width, constructionCanvas.height)
                        recordControllerMutationMetric("zoom_viewport")
                        requestCanvasPaint()
                        finishInteractionMetrics(false)
                        wheel.accepted = true
                        return
                    }
                    var panState = drawingCanvasRuntime.beginPan(drawingCanvasRuntime.initialGestureState(), { x: 0, y: 0 }, gestureModifiers(wheel.modifiers))
                    panState = drawingCanvasRuntime.updateGesture(panState, { screenPoint: { x: pixelX, y: pixelY }, modifiers: gestureModifiers(wheel.modifiers) })
                    var panFinish = drawingCanvasRuntime.finishGesture(panState, { screenPoint: { x: pixelX, y: pixelY } })
                    if (panFinish.intent.kind === "pan") {
                        beginInteractionMetrics("panning")
                        drawingWorkspace.controller.panDrawingCanvasBy(panFinish.intent.dxPx, panFinish.intent.dyPx)
                        recordControllerMutationMetric("pan_viewport")
                    }
                    requestCanvasPaint()
                    finishInteractionMetrics(false)
                    wheel.accepted = true
                }
            }

            function runDrawingControlPlan(plan) {
                return controlRunner.runExecutionPlan(plan)
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
