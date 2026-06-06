import QtQuick
import "DrawingCanvasHitTest.js" as DrawingCanvasHitTest
import "DrawingToolCatalog.js" as DrawingToolCatalog
import "DrawingRuntimeRows.js" as DrawingRuntimeRows

QtObject {
    id: drawingSession

    property int revision: 0
    property bool writeDisabled: true
    property string selectedDrawingToolId: "anchor_points"
    property string selectedDrawingVariantId: "point"
    property string selectedDrawingExternalToolId: ""
    property string selectedDrawingLayerId: "layer_00_canvas"
    property string selectedDrawingObjectId: "artboard_bounds"
    property var selectedDrawingObjectIds: []
    property string selectedDrawingPresetId: "lotus_petal_fit"
    property bool drawingToolPaletteOpen: true
    property real drawingToolPaletteX: 258
    property real drawingToolPaletteY: 26
    property var drawingToolRegistryDocument: ({})
    property string drawingToolRegistryPath: ""
    property var drawingToolModes: DrawingToolCatalog.toolModes()
    property var drawingToolSettingsById: DrawingToolCatalog.toolSettingsById()
    property var drawingPrecisionTools: DrawingToolCatalog.precisionTools()
    property var drawingDataTools: DrawingToolCatalog.dataTools()
    property var drawingImageTools: DrawingToolCatalog.imageTools()
    property var drawingExternalToolSettingsById: DrawingToolCatalog.externalToolSettingsById()
    property var drawingAssetSources: DrawingToolCatalog.assetSources()
    property var drawingPatternFamilies: DrawingToolCatalog.patternFamilies()
    property var drawingToolPresets: DrawingToolCatalog.toolPresets()
    property var drawingLayerStack: DrawingToolCatalog.layerStack()
    property var drawingSidebarSections: DrawingToolCatalog.sidebarSections()
    property real drawingAnchorRootX: 0.50
    property real drawingAnchorRootY: 0.50
    property real drawingAnchorTipX: 0.50
    property real drawingAnchorTipY: 0.20
    property real drawingAnchorRightX: 0.80
    property real drawingAnchorRightY: 0.50
    property real drawingAnchorBottomX: 0.50
    property real drawingAnchorBottomY: 0.80
    property real drawingAnchorLeftX: 0.20
    property real drawingAnchorLeftY: 0.50
    property var drawingNativeController: null
    property var drawingExternalModelDocument: ({})
    property var drawingGeneratedObjects: []
    property var drawingPendingPoint: ({})
    property bool drawingPendingShapeActive: false
    property bool drawingSnapGridEnabled: true
    property int drawingSnapGridStepPx: 32
    property bool drawingObjectSnapEnabled: true
    property int drawingObjectSnapTolerancePx: 14
    property bool drawingObjectSnapEndpointEnabled: true
    property bool drawingObjectSnapMidpointEnabled: true
    property bool drawingObjectSnapCenterEnabled: true
    property bool drawingObjectSnapVertexEnabled: true
    property string drawingCircleArcMode: "circle"
    property real drawingCircleArcStartAngleDeg: 0
    property real drawingCircleArcEndAngleDeg: 90
    property int drawingRegularPolygonSides: 6
    property real drawingRegularPolygonRotationDeg: 30
    property string drawingLineVariant: "straight"
    property string drawingStrokeColor: "#f4d46f"
    property string drawingFillColor: ""
    property real drawingLineThickness: 2
    property string drawingLineStyle: "solid"
    property real drawingStrokeOpacity: 1.0
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
    property bool drawingCanUndoCommand: false
    property bool drawingCanRedoCommand: false
    property var drawingObjectClipboard: ({})
    property int drawingObjectClipboardPasteCount: 0
    property string drawingLastScriptId: ""
    property string drawingLastScriptStatus: "not_run"
    property var drawingLastScriptErrors: []

    signal changed()

    function markChanged() {
        revision += 1
        changed()
    }

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

    function clampThickness(value) {
        var thickness = Number(value)
        if (!Number.isFinite(thickness)) {
            return 2
        }
        return Math.max(1, Math.min(18, Math.round(Number(thickness) * 10) / 10))
    }

    function clampOpacity(value) {
        var opacity = Number(value)
        if (!Number.isFinite(opacity)) {
            return 1
        }
        return Math.max(0, Math.min(1, opacity))
    }

    function normalizeHexColor(value) {
        var raw = String(value || "").trim().toLowerCase()
        if (raw.length === 0) {
            return ""
        }
        if (raw === "none" || raw === "transparent") {
            return ""
        }
        if (raw.indexOf("#") !== 0) {
            raw = "#" + raw
        }
        if (/^#([0-9a-f]{3}|[0-9a-f]{4}|[0-9a-f]{6}|[0-9a-f]{8})$/.test(raw)) {
            return raw
        }
        return ""
    }

    function normalizeLineStyle(value) {
        var style = String(value || "solid").trim().toLowerCase()
        if (style === "dashed" || style === "dot" || style === "dotted") {
            return style === "dot" ? "dotted" : style
        }
        return "solid"
    }

    function normalizeLineVariant(value) {
        var variant = String(value || "straight").trim().toLowerCase()
        return variant === "polyline" || variant === "straight" ? variant : "straight"
    }

    function pendingPointActive(point) {
        var source = point || drawingPendingPoint || ({})
        return source.ok === true
            || (Number.isFinite(Number(source.x)) && Number.isFinite(Number(source.y)))
    }

    function drawingVariantToolId(variantId) {
        var id = String(variantId || "")
        if (id === "select") {
            return "select_move"
        }
        if (id === "point") {
            return "anchor_points"
        }
        if (id === "line_straight" || id === "line_polyline" || id === "line_arrow") {
            return "line_polyline"
        }
        if (id === "circle_full" || id === "circle_arc") {
            return "circle_arc"
        }
        if (id === "rect_box" || id === "rect_rounded" || id === "rect_frame") {
            return "rectangle_polygon"
        }
        if (id === "polygon_triangle" || id === "polygon_hex" || id === "polygon_free") {
            return "regular_polygon"
        }
        if (id === "image_frame" || id === "image_crop") {
            return "image_reference_frame"
        }
        if (id === "ascii_crop" || id === "ascii_glyph_block") {
            return "ascii_crop_frame"
        }
        return ""
    }

    function defaultDrawingVariantIdForTool(toolId) {
        var id = String(toolId || "")
        if (id === "select_move") {
            return "select"
        }
        if (id === "anchor_points") {
            return "point"
        }
        if (id === "line_polyline") {
            return drawingLineVariant === "polyline" ? "line_polyline" : "line_straight"
        }
        if (id === "circle_arc") {
            return drawingCircleArcMode === "arc" ? "circle_arc" : "circle_full"
        }
        if (id === "rectangle_polygon") {
            return "rect_box"
        }
        if (id === "regular_polygon") {
            if (drawingRegularPolygonSides === 3) {
                return "polygon_triangle"
            }
            if (drawingRegularPolygonSides === 6) {
                return "polygon_hex"
            }
            return "polygon_free"
        }
        if (id === "image_reference_frame") {
            return "image_frame"
        }
        if (id === "ascii_crop_frame") {
            return "ascii_crop"
        }
        return ""
    }

    function normalizeSelectedDrawingVariant() {
        var variantToolId = drawingVariantToolId(selectedDrawingVariantId)
        if (variantToolId === selectedDrawingToolId && selectedDrawingVariantId.length > 0) {
            return
        }
        selectedDrawingVariantId = defaultDrawingVariantIdForTool(selectedDrawingToolId)
    }

    function setSelectedDrawingVariantId(variantId) {
        var id = String(variantId || "")
        var toolId = drawingVariantToolId(id)
        if (toolId.length === 0) {
            return
        }
        var changingVariant = selectedDrawingVariantId !== id
        if (changingVariant && pendingPointActive()) {
            cancelDrawingPendingShape()
        }
        if (selectedDrawingToolId !== toolId) {
            selectDrawingTool(toolId)
        }
        selectedDrawingVariantId = id
        if (id === "line_straight" || id === "line_arrow") {
            setDrawingLineVariant("straight")
        } else if (id === "line_polyline") {
            setDrawingLineVariant("polyline")
        } else if (id === "circle_full") {
            setDrawingCircleArcMode("circle")
        } else if (id === "circle_arc") {
            setDrawingCircleArcMode("arc")
        } else if (id === "polygon_triangle") {
            setDrawingRegularPolygonSides(3)
            setDrawingRegularPolygonRotationDeg(30)
        } else if (id === "polygon_hex") {
            setDrawingRegularPolygonSides(6)
            setDrawingRegularPolygonRotationDeg(30)
        }
        markChanged()
    }

    function setDrawingLineVariant(value) {
        var variant = normalizeLineVariant(value)
        if (drawingLineVariant === variant) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter("line_variant", variant)
            syncNativeDrawingModel()
            return
        }
        drawingLineVariant = variant
        markChanged()
    }

    function setDrawingCircleArcMode(mode) {
        if (drawingNativeController) {
            setDrawingToolParameter("circle_arc_mode", mode)
            return
        }
        setDrawingToolParameter("circle_arc_mode", mode)
    }

    function setDrawingStrokeColor(rawColor) {
        var color = normalizeHexColor(rawColor)
        var allowed = rawColor
            ? String(rawColor || "").trim()
            : ""
        var normalized = color
        if (!allowed.length && color.length === 0 && String(rawColor || "").trim().length > 0) {
            return
        }
        if (drawingStrokeColor === normalized) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter("stroke_color", normalized)
            syncNativeDrawingModel()
            return
        }
        drawingStrokeColor = normalized
        markChanged()
    }

    function setDrawingFillColor(rawColor) {
        var color = normalizeHexColor(rawColor)
        var input = String(rawColor || "").trim()
        if (!input.length) {
            color = ""
        } else if (input.length && color.length === 0) {
            return
        }
        if (drawingFillColor === color) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter("fill_color", color)
            syncNativeDrawingModel()
            return
        }
        drawingFillColor = color
        markChanged()
    }

    function setDrawingLineThickness(value) {
        var thickness = clampThickness(value)
        if (drawingLineThickness === thickness) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter("line_thickness", thickness)
            syncNativeDrawingModel()
            return
        }
        drawingLineThickness = thickness
        markChanged()
    }

    function setDrawingLineStyle(value) {
        var style = normalizeLineStyle(value)
        if (drawingLineStyle === style) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter("line_style", style)
            syncNativeDrawingModel()
            return
        }
        drawingLineStyle = style
        markChanged()
    }

    function setDrawingStrokeOpacity(value) {
        var opacity = clampOpacity(value)
        if (drawingStrokeOpacity === opacity) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter("stroke_opacity", opacity)
            syncNativeDrawingModel()
            return
        }
        drawingStrokeOpacity = opacity
        markChanged()
    }

    function toolRegistryItems() {
        return asArray(drawingToolRegistryDocument.tools)
    }

    function toolRegistryEnabled(toolId) {
        var tools = toolRegistryItems()
        if (tools.length === 0) {
            return true
        }
        var id = String(toolId || "")
        for (var index = 0; index < tools.length; ++index) {
            if (String(tools[index].tool_id || "") === id) {
                return tools[index].enabled !== false
            }
        }
        return true
    }

    function filterEnabledDrawingTools(source) {
        var result = []
        var items = asArray(source)
        for (var index = 0; index < items.length; ++index) {
            var item = items[index] || ({})
            if (toolRegistryEnabled(item.id)) {
                result.push(item)
            }
        }
        return result
    }

    function loadDrawingToolRegistry(document, path) {
        drawingToolRegistryDocument = document || ({})
        drawingToolRegistryPath = String(path || "")
        drawingToolModes = filterEnabledDrawingTools(DrawingToolCatalog.toolModes())
        if (!drawingFindById(drawingToolModes, selectedDrawingToolId, null) && drawingToolModes.length > 0) {
            selectedDrawingToolId = String(drawingToolModes[0].id || "")
        }
        markChanged()
    }

    function drawingFindById(items, id, fallback) {
        var list = asArray(items)
        for (var index = 0; index < list.length; ++index) {
            if (String(list[index].id || "") === String(id || "")) {
                return list[index]
            }
        }
        return fallback || ({})
    }

    function selectedDrawingTool() {
        return drawingFindById(drawingToolModes, selectedDrawingToolId, drawingToolModes[0] || ({}))
    }

    function selectedDrawingExternalTool() {
        return drawingFindById(drawingImageTools, selectedDrawingExternalToolId, ({}))
    }

    function selectedDrawingLayer() {
        return drawingFindById(drawingLayerStack, selectedDrawingLayerId, drawingLayerStack[0] || ({}))
    }

    function selectedDrawingObject() {
        var objects = drawingCanvasObjects(revision)
        return drawingFindById(objects, selectedDrawingObjectId, null) || ({})
    }

    function selectedGeneratedDrawingObjectIds() {
        var ids = []
        var incoming = asArray(selectedDrawingObjectIds)
        for (var index = 0; index < incoming.length; ++index) {
            var id = String(incoming[index] || "")
            var object = drawingFindById(drawingGeneratedObjects, id, null)
            if (id.indexOf("script_") === 0 && ids.indexOf(id) < 0 && String(object.id || "") === id) {
                ids.push(id)
            }
        }
        var primaryId = String(selectedDrawingObjectId || "")
        var primaryObject = drawingFindById(drawingGeneratedObjects, primaryId, null)
        if (ids.length === 0 && primaryId.indexOf("script_") === 0 && String(primaryObject.id || "") === primaryId) {
            ids.push(primaryId)
        }
        return ids
    }

    function generatedDrawingObjectsByIds(objectIds) {
        var objects = []
        var ids = asArray(objectIds)
        for (var index = 0; index < ids.length; ++index) {
            var object = drawingFindById(drawingGeneratedObjects, ids[index], null)
            if (String(object.id || "").indexOf("script_") === 0) {
                objects.push(object)
            }
        }
        return objects
    }

    function drawingAnchorPoint(anchorId) {
        if (anchorId === "anchor_root") {
            return { id: "anchor_root", label: "anchor_root", x: drawingAnchorRootX, y: drawingAnchorRootY }
        }
        if (anchorId === "anchor_tip") {
            return { id: "anchor_tip", label: "anchor_tip", x: drawingAnchorTipX, y: drawingAnchorTipY }
        }
        if (anchorId === "anchor_right") {
            return { id: "anchor_right", label: "anchor_right", x: drawingAnchorRightX, y: drawingAnchorRightY }
        }
        if (anchorId === "anchor_bottom") {
            return { id: "anchor_bottom", label: "anchor_bottom", x: drawingAnchorBottomX, y: drawingAnchorBottomY }
        }
        if (anchorId === "anchor_left") {
            return { id: "anchor_left", label: "anchor_left", x: drawingAnchorLeftX, y: drawingAnchorLeftY }
        }
        return { id: anchorId, label: anchorId, x: 0.5, y: 0.5 }
    }

    function setDrawingAnchorPosition(anchorId, x, y) {
        var clampedX = Math.max(0.02, Math.min(0.98, Number(x)))
        var clampedY = Math.max(0.02, Math.min(0.98, Number(y)))
        if (anchorId === "anchor_root") {
            drawingAnchorRootX = clampedX
            drawingAnchorRootY = clampedY
        } else if (anchorId === "anchor_tip") {
            drawingAnchorTipX = clampedX
            drawingAnchorTipY = clampedY
        } else if (anchorId === "anchor_right") {
            drawingAnchorRightX = clampedX
            drawingAnchorRightY = clampedY
        } else if (anchorId === "anchor_bottom") {
            drawingAnchorBottomX = clampedX
            drawingAnchorBottomY = clampedY
        } else if (anchorId === "anchor_left") {
            drawingAnchorLeftX = clampedX
            drawingAnchorLeftY = clampedY
        } else {
            return
        }
        selectedDrawingObjectId = anchorId
        selectedDrawingLayerId = "layer_00_canvas"
        markChanged()
    }

    function selectNearestDrawingAnchor(x, y, tolerance) {
        var anchors = ["anchor_root", "anchor_tip", "anchor_right", "anchor_bottom", "anchor_left"]
        var bestId = ""
        var bestDistance = Number(tolerance || 0.035)
        for (var index = 0; index < anchors.length; ++index) {
            var anchor = drawingAnchorPoint(anchors[index])
            var dx = Number(anchor.x) - Number(x)
            var dy = Number(anchor.y) - Number(y)
            var distance = Math.sqrt(dx * dx + dy * dy)
            if (distance <= bestDistance) {
                bestDistance = distance
                bestId = anchor.id
            }
        }
        if (bestId.length > 0) {
            selectDrawingObject(bestId)
            return bestId
        }
        return ""
    }

    function selectDrawingTool(toolId) {
        var tool = drawingFindById(drawingToolModes, toolId, null)
        if (!tool) {
            return
        }
        var nextToolId = String(tool.id)
        if (selectedDrawingToolId !== nextToolId && pendingPointActive()) {
            cancelDrawingPendingShape()
        }
        selectedDrawingExternalToolId = ""
        selectedDrawingToolId = nextToolId
        normalizeSelectedDrawingVariant()
        if (drawingNativeController) {
            drawingNativeController.selectTool(nextToolId)
            syncNativeDrawingModel()
            normalizeSelectedDrawingVariant()
            drawingToolPaletteOpen = true
            return
        }
        drawingToolPaletteOpen = true
        markChanged()
    }

    function selectDrawingExternalTool(toolId) {
        var tool = drawingFindById(drawingImageTools, toolId, null)
        if (!tool) {
            return
        }
        selectedDrawingExternalToolId = String(tool.id)
        markChanged()
    }

    function selectDrawingPreset(presetId) {
        var preset = drawingFindById(drawingToolPresets, presetId, null)
        if (!preset) {
            return
        }
        selectedDrawingPresetId = String(preset.id)
        markChanged()
    }

    function selectedDrawingPreset() {
        return drawingFindById(drawingToolPresets, selectedDrawingPresetId, drawingToolPresets[0] || ({}))
    }

    function toggleDrawingToolPalette() {
        drawingToolPaletteOpen = !drawingToolPaletteOpen
        markChanged()
    }

    function closeDrawingToolPalette() {
        drawingToolPaletteOpen = false
        markChanged()
    }

    function moveDrawingToolPalette(x, y) {
        drawingToolPaletteX = Number(x)
        drawingToolPaletteY = Number(y)
        markChanged()
    }

    function setDrawingGridVisible(visible) {
        drawingGridVisible = visible === true
        markChanged()
    }

    function setDrawingGridMode(mode) {
        var allowed = ["square", "isometric", "hex"]
        var value = String(mode || "square")
        drawingGridMode = allowed.indexOf(value) >= 0 ? value : "square"
        markChanged()
    }

    function setDrawingGridDivisions(divisions) {
        drawingGridDivisions = Math.max(2, Math.min(128, Math.round(Number(divisions) || 16)))
        markChanged()
    }

    function setDrawingGridMajorEvery(value) {
        drawingGridMajorEvery = Math.max(1, Math.min(32, Math.round(Number(value) || 4)))
        markChanged()
    }

    function setDrawingAsciiCellGridVisible(visible) {
        drawingAsciiCellGridVisible = visible === true
        markChanged()
    }

    function setDrawingAsciiColumns(columns) {
        drawingAsciiColumns = Math.max(8, Math.min(320, Math.round(Number(columns) || 80)))
        markChanged()
    }

    function setDrawingAsciiRows(rows) {
        drawingAsciiRows = Math.max(4, Math.min(240, Math.round(Number(rows) || 40)))
        markChanged()
    }

    function setDrawingAsciiMajorEvery(value) {
        drawingAsciiMajorEvery = Math.max(1, Math.min(64, Math.round(Number(value) || 10)))
        markChanged()
    }

    function setDrawingCenterAxesVisible(visible) {
        drawingCenterAxesVisible = visible === true
        markChanged()
    }

    function setDrawingDiagonalGuidesVisible(visible) {
        drawingDiagonalGuidesVisible = visible === true
        markChanged()
    }

    function setDrawingRadialGuidesVisible(visible) {
        drawingRadialGuidesVisible = visible === true
        markChanged()
    }

    function setDrawingRadialGuideCount(count) {
        drawingRadialGuideCount = Math.max(2, Math.min(64, Math.round(Number(count) || 8)))
        markChanged()
    }

    function setDrawingArtboardBorderVisible(visible) {
        drawingArtboardBorderVisible = visible === true
        markChanged()
    }

    function setDrawingSnapGrid(enabled) {
        drawingSnapGridEnabled = enabled === true
        if (drawingNativeController) {
            drawingNativeController.setSnap(drawingSnapGridEnabled, drawingSnapGridStepPx)
            syncNativeDrawingModel()
            return
        }
        markChanged()
    }

    function setDrawingSnapGridStepPx(stepPx) {
        drawingSnapGridStepPx = Math.max(1, Math.min(drawingCanvasSizePx, Math.round(Number(stepPx) || 32)))
        if (drawingNativeController) {
            drawingNativeController.setSnap(drawingSnapGridEnabled, drawingSnapGridStepPx)
            syncNativeDrawingModel()
            return
        }
        markChanged()
    }

    function setDrawingObjectSnapEnabled(enabled) {
        drawingObjectSnapEnabled = enabled === true
        markChanged()
    }

    function setDrawingObjectSnapTolerancePx(value) {
        drawingObjectSnapTolerancePx = Math.max(2, Math.min(64, Math.round(Number(value) || 14)))
        markChanged()
    }

    function setDrawingObjectSnapEndpointEnabled(enabled) {
        drawingObjectSnapEndpointEnabled = enabled === true
        markChanged()
    }

    function setDrawingObjectSnapMidpointEnabled(enabled) {
        drawingObjectSnapMidpointEnabled = enabled === true
        markChanged()
    }

    function setDrawingObjectSnapCenterEnabled(enabled) {
        drawingObjectSnapCenterEnabled = enabled === true
        markChanged()
    }

    function setDrawingObjectSnapVertexEnabled(enabled) {
        drawingObjectSnapVertexEnabled = enabled === true
        markChanged()
    }

    function setDrawingToolParameter(parameter, value) {
        var parameterId = String(parameter || "")
        if (parameterId.length === 0) {
            return
        }
        if (parameterId === "circle_arc_mode") {
            var mode = String(value || "").trim().toLowerCase()
            if (mode !== "circle" && mode !== "arc") {
                return
            }
            if (drawingNativeController) {
                drawingNativeController.setToolParameter(parameterId, mode)
                syncNativeDrawingModel()
                return
            }
            drawingCircleArcMode = mode
            markChanged()
            return
        }
        var numericValue = Number(value)
        if (!Number.isFinite(numericValue)) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.setToolParameter(parameterId, numericValue)
            syncNativeDrawingModel()
            return
        }
        if (parameterId === "circle_arc_start_angle_deg") {
            drawingCircleArcStartAngleDeg = numericValue
        } else if (parameterId === "circle_arc_end_angle_deg") {
            drawingCircleArcEndAngleDeg = numericValue
        } else if (parameterId === "regular_polygon_sides") {
            drawingRegularPolygonSides = Math.max(3, Math.min(64, Math.round(numericValue)))
        } else if (parameterId === "regular_polygon_rotation_deg") {
            drawingRegularPolygonRotationDeg = numericValue
        }
        markChanged()
    }

    function setDrawingRegularPolygonSides(value) {
        setDrawingToolParameter("regular_polygon_sides", value)
    }

    function setDrawingRegularPolygonRotationDeg(value) {
        setDrawingToolParameter("regular_polygon_rotation_deg", value)
    }

    function updateDrawingToolParameterField(field, rawValue) {
        setDrawingToolParameter(field, rawValue)
    }

    function setDrawingCanvasZoom(zoom) {
        drawingCanvasZoom = Math.max(drawingCanvasZoomMin, Math.min(drawingCanvasZoomMax, Number(zoom) || 1.0))
        markChanged()
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
        markChanged()
    }

    function panDrawingCanvasBy(dx, dy) {
        drawingCanvasPanXPx += Number(dx) || 0
        drawingCanvasPanYPx += Number(dy) || 0
        markChanged()
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
        markChanged()
    }

    function fitDrawingCanvasToView() {
        resetDrawingCanvasZoom()
    }

    function selectDrawingLayer(layerId) {
        var layer = drawingFindById(drawingLayerStack, layerId, null)
        if (!layer) {
            return
        }
        selectedDrawingLayerId = String(layer.id)
        var found = false
        var objects = drawingCanvasObjects(revision)
        for (var index = 0; index < objects.length; ++index) {
            if (String(objects[index].layer_id || "") === selectedDrawingLayerId) {
                selectedDrawingObjectId = String(objects[index].id)
                selectedDrawingObjectIds = [selectedDrawingObjectId]
                found = true
                break
            }
        }
        if (!found) {
            selectedDrawingObjectId = ""
            selectedDrawingObjectIds = []
        }
        markChanged()
    }

    function selectDrawingObject(objectId) {
        if (drawingNativeController && String(objectId || "").indexOf("script_") === 0) {
            drawingNativeController.selectObject(String(objectId))
            syncNativeDrawingModel()
            return
        }
        if (drawingNativeController && String(objectId || "").indexOf("anchor_") === 0) {
            selectedDrawingObjectId = String(objectId)
            selectedDrawingObjectIds = [selectedDrawingObjectId]
            selectedDrawingLayerId = "layer_00_canvas"
            markChanged()
            return
        }
        var object = drawingFindById(drawingCanvasObjects(revision), objectId, null)
        if (!object) {
            selectedDrawingObjectId = ""
            selectedDrawingObjectIds = []
            markChanged()
            return
        }
        selectedDrawingObjectId = String(object.id)
        selectedDrawingObjectIds = [selectedDrawingObjectId]
        selectedDrawingLayerId = String(object.layer_id || selectedDrawingLayerId)
        markChanged()
    }

    function selectDrawingObjects(objectIds) {
        var ids = []
        var incoming = asArray(objectIds)
        for (var index = 0; index < incoming.length; ++index) {
            var id = String(incoming[index] || "")
            if (id.indexOf("script_") === 0 && ids.indexOf(id) < 0) {
                ids.push(id)
            }
        }
        if (drawingNativeController && typeof drawingNativeController.selectObjects === "function") {
            drawingNativeController.selectObjects(ids)
            syncNativeDrawingModel()
            return
        }
        selectedDrawingObjectIds = ids
        selectedDrawingObjectId = ids.length > 0 ? ids[ids.length - 1] : ""
        selectedDrawingLayerId = ids.length > 0 ? "layer_09_script_geometry" : selectedDrawingLayerId
        markChanged()
    }

    function toggleDrawingObjectSelection(objectId) {
        var id = String(objectId || "")
        if (id.indexOf("script_") !== 0) {
            return
        }
        var ids = []
        var incoming = asArray(selectedDrawingObjectIds)
        for (var index = 0; index < incoming.length; ++index) {
            var selectedId = String(incoming[index] || "")
            if (selectedId.indexOf("script_") === 0 && selectedId !== id && ids.indexOf(selectedId) < 0) {
                ids.push(selectedId)
            }
        }
        if (incoming.indexOf(id) < 0) {
            ids.push(id)
        }
        selectDrawingObjects(ids)
    }

    function clearDrawingObjectSelection() {
        if (drawingNativeController) {
            drawingNativeController.selectObject("")
            syncNativeDrawingModel()
            return
        }
        selectedDrawingObjectId = ""
        selectedDrawingObjectIds = []
        drawingPendingPoint = ({})
        markChanged()
    }

    function deleteSelectedDrawingObject() {
        var ids = selectedGeneratedDrawingObjectIds()
        if (ids.length === 0) {
            return
        }
        if (drawingNativeController && typeof drawingNativeController.deleteObjects === "function") {
            drawingNativeController.deleteObjects(ids)
            syncNativeDrawingModel()
            return
        }
        if (drawingNativeController) {
            drawingNativeController.deleteObject(ids[0])
            syncNativeDrawingModel()
            return
        }
        var kept = []
        var generated = asArray(drawingGeneratedObjects)
        for (var index = 0; index < generated.length; ++index) {
            if (ids.indexOf(String(generated[index].id || "")) < 0) {
                kept.push(generated[index])
            }
        }
        drawingGeneratedObjects = kept
        selectedDrawingObjectId = ""
        selectedDrawingObjectIds = []
        markChanged()
    }

    function duplicateSelectedDrawingObject() {
        var ids = selectedGeneratedDrawingObjectIds()
        if (ids.length === 0) {
            return
        }
        var offset = 16 / Math.max(1, drawingCanvasSizePx)
        if (drawingNativeController && typeof drawingNativeController.duplicateObjects === "function") {
            drawingNativeController.duplicateObjects(ids, offset, offset)
            syncNativeDrawingModel()
            return
        }
        var duplicateIds = []
        var sources = generatedDrawingObjectsByIds(ids)
        for (var index = 0; index < sources.length; ++index) {
            var source = sources[index]
            var duplicate = JSON.parse(JSON.stringify(source))
            duplicate.id = String(source.id || "script_object") + "_copy_" + String(index + 1)
            duplicate.duplicate_of = String(source.id || "")
            drawingGeneratedObjects.push(duplicate)
            duplicateIds.push(duplicate.id)
        }
        if (duplicateIds.length > 0) {
            selectedDrawingObjectId = duplicateIds[duplicateIds.length - 1]
            selectedDrawingObjectIds = duplicateIds
            selectedDrawingLayerId = "layer_09_script_geometry"
        }
        markChanged()
    }

    function copySelectedDrawingObject() {
        var ids = selectedGeneratedDrawingObjectIds()
        var sources = generatedDrawingObjectsByIds(ids)
        if (sources.length === 0) {
            return
        }
        drawingObjectClipboard = ({ objects: JSON.parse(JSON.stringify(sources)) })
        drawingObjectClipboardPasteCount = 0
        markChanged()
    }

    function pasteCopiedDrawingObject() {
        var clipboardObjects = asArray(drawingObjectClipboard.objects)
        if (clipboardObjects.length === 0 && String(drawingObjectClipboard.id || "").indexOf("script_") === 0) {
            clipboardObjects = [drawingObjectClipboard]
        }
        if (clipboardObjects.length === 0) {
            return
        }
        var nextCount = drawingObjectClipboardPasteCount + 1
        var offset = 16 * nextCount / Math.max(1, drawingCanvasSizePx)
        if (drawingNativeController && typeof drawingNativeController.pasteObjects === "function") {
            drawingNativeController.pasteObjects(clipboardObjects, offset, offset)
            drawingObjectClipboardPasteCount = nextCount
            syncNativeDrawingModel()
            return
        }
        var pastedIds = []
        for (var index = 0; index < clipboardObjects.length; ++index) {
            var source = clipboardObjects[index]
            var sourceId = String(source.id || "")
            if (sourceId.indexOf("script_") !== 0) {
                continue
            }
            var duplicate = JSON.parse(JSON.stringify(source))
            duplicate.id = sourceId + "_paste_" + String(nextCount) + "_" + String(index + 1)
            duplicate.pasted_from = sourceId
            drawingGeneratedObjects.push(duplicate)
            pastedIds.push(duplicate.id)
        }
        if (pastedIds.length > 0) {
            selectedDrawingObjectId = pastedIds[pastedIds.length - 1]
            selectedDrawingObjectIds = pastedIds
            selectedDrawingLayerId = "layer_09_script_geometry"
        }
        drawingObjectClipboardPasteCount = nextCount
        markChanged()
    }

    function moveDrawingObjectBy(objectId, dx, dy) {
        var id = String(objectId || "")
        var moveX = Number(dx) || 0
        var moveY = Number(dy) || 0
        if (id.length === 0 || id.indexOf("script_") !== 0 || (Math.abs(moveX) < 0.000001 && Math.abs(moveY) < 0.000001)) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.moveObjectBy(id, moveX, moveY)
            syncNativeDrawingModel()
            return
        }
        markChanged()
    }

    function moveDrawingObjectsBy(objectIds, dx, dy) {
        var ids = []
        var incoming = asArray(objectIds)
        for (var index = 0; index < incoming.length; ++index) {
            var id = String(incoming[index] || "")
            if (id.indexOf("script_") === 0 && ids.indexOf(id) < 0) {
                ids.push(id)
            }
        }
        var moveX = Number(dx) || 0
        var moveY = Number(dy) || 0
        if (ids.length === 0 || (Math.abs(moveX) < 0.000001 && Math.abs(moveY) < 0.000001)) {
            return
        }
        if (drawingNativeController && typeof drawingNativeController.moveObjectsBy === "function") {
            drawingNativeController.moveObjectsBy(ids, moveX, moveY)
            syncNativeDrawingModel()
            return
        }
        for (var index = 0; index < ids.length; ++index) {
            moveDrawingObjectBy(ids[index], moveX, moveY)
        }
    }

    function beginDrawingObjectMove() {
        if (drawingNativeController && typeof drawingNativeController.beginMoveGesture === "function") {
            drawingNativeController.beginMoveGesture()
        }
    }

    function endDrawingObjectMove() {
        if (drawingNativeController && typeof drawingNativeController.endMoveGesture === "function") {
            drawingNativeController.endMoveGesture()
        }
    }

    function moveSelectedDrawingObjectBy(dx, dy) {
        var ids = selectedGeneratedDrawingObjectIds()
        if (ids.length > 1) {
            moveDrawingObjectsBy(ids, dx, dy)
            return
        }
        moveDrawingObjectBy(selectedDrawingObjectId, dx, dy)
    }

    function nudgeSelectedDrawingObjectByPx(dxPx, dyPx) {
        var canvas = Math.max(1, Number(drawingCanvasSizePx || 512))
        var moveX = (Number(dxPx) || 0) / canvas
        var moveY = (Number(dyPx) || 0) / canvas
        moveSelectedDrawingObjectBy(moveX, moveY)
    }

    function updateSelectedDrawingObjectField(field, rawValue) {
        var objectId = String(selectedDrawingObjectId || "")
        var fieldId = String(field || "")
        var value = Number(rawValue)
        if (objectId.indexOf("script_") !== 0 || fieldId.length === 0 || !Number.isFinite(value)) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.updateObjectField(objectId, fieldId, value)
            syncNativeDrawingModel()
            return
        }
    }

    function selectDrawingObjectAtNormalized(x, y) {
        var bestId = hitDrawingObjectAtNormalized(x, y)
        if (bestId.length > 0) {
            selectDrawingObject(bestId)
            return bestId
        }
        clearDrawingObjectSelection()
        return ""
    }

    function hitDrawingObjectAtNormalized(x, y) {
        var tolerance = 0.025
        var generated = asArray(drawingGeneratedObjects)
        var bestId = ""
        var bestScore = tolerance
        for (var index = generated.length - 1; index >= 0; --index) {
            var object = generated[index]
            var score = DrawingCanvasHitTest.objectHitScore(object, Number(x), Number(y), tolerance)
            if (score <= bestScore) {
                bestScore = score
                bestId = String(object.id || "")
            }
        }
        return bestId
    }

    function syncNativeDrawingModel() {
        if (!drawingNativeController) {
            return
        }
        drawingCanUndoCommand = typeof drawingNativeController.canUndo === "function" ? drawingNativeController.canUndo() : false
        drawingCanRedoCommand = typeof drawingNativeController.canRedo === "function" ? drawingNativeController.canRedo() : false
        loadInitialDrawingModel(drawingNativeController.modelDocument())
    }

    function handleDrawingCanvasClick(x, y, snapStepPx) {
        if (selectedDrawingToolId === "select_move") {
            selectDrawingObjectAtNormalized(x, y)
            return
        }
        if (drawingNativeController) {
            var activeStepPx = Math.max(1, Math.min(drawingCanvasSizePx, Math.round(Number(snapStepPx) || drawingSnapGridStepPx)))
            drawingNativeController.clickCanvasNormalizedWithSnapStep(Number(x), Number(y), activeStepPx)
            syncNativeDrawingModel()
            return
        }
    }

    function cancelDrawingPendingShape() {
        if (!pendingPointActive()) {
            return
        }
        if (drawingNativeController) {
            drawingNativeController.cancelPending()
            syncNativeDrawingModel()
            return
        }
        drawingPendingPoint = ({})
        drawingPendingShapeActive = false
        markChanged()
    }

    function undoDrawingCommand() {
        if (!drawingNativeController || !drawingCanUndoCommand) {
            return
        }
        drawingNativeController.undo()
        syncNativeDrawingModel()
    }

    function redoDrawingCommand() {
        if (!drawingNativeController || !drawingCanRedoCommand) {
            return
        }
        drawingNativeController.redo()
        syncNativeDrawingModel()
    }

    function resetNativeDrawingDocument() {
        if (drawingNativeController) {
            drawingNativeController.reset()
            syncNativeDrawingModel()
            return
        }
        drawingGeneratedObjects = []
        drawingLastScriptId = ""
        drawingLastScriptStatus = "not_run"
        drawingLastScriptErrors = []
        selectedDrawingObjectId = ""
        selectedDrawingObjectIds = []
        selectedDrawingLayerId = "layer_00_canvas"
        markChanged()
    }

    function loadInitialDrawingModel(document) {
        if (!document || String(document.export_kind || "") !== "pattern_lab_2d_native_model_v0") {
            return
        }
        if (drawingNativeController) {
            drawingCanUndoCommand = typeof drawingNativeController.canUndo === "function" ? drawingNativeController.canUndo() : false
            drawingCanRedoCommand = typeof drawingNativeController.canRedo === "function" ? drawingNativeController.canRedo() : false
        }
        drawingGeneratedObjects = asArray(document.generated_objects)
        drawingLastScriptId = String(document.script_id || "")
        drawingLastScriptStatus = String(document.script_status || "not_run")
        drawingLastScriptErrors = asArray(document.script_errors)
        drawingPendingPoint = document.pending_point || ({})
        drawingPendingShapeActive = pendingPointActive(drawingPendingPoint)
        var canvas = asArray(document.canvas_px)
        if (canvas.length >= 2 && Number(canvas[0]) === Number(canvas[1]) && Number(canvas[0]) > 0) {
            drawingCanvasSizePx = Math.round(Number(canvas[0]))
        }
        var snap = document.snap || ({})
        if (typeof snap.grid_enabled === "boolean") {
            drawingSnapGridEnabled = snap.grid_enabled
        }
        if (Number.isFinite(Number(snap.grid_step_px))) {
            drawingSnapGridStepPx = Math.max(1, Math.round(Number(snap.grid_step_px)))
        }
        var toolParameters = document.tool_parameters || ({})
        var circleArcMode = String(toolParameters.circle_arc_mode || "").toLowerCase()
        if (circleArcMode === "circle" || circleArcMode === "arc") {
            drawingCircleArcMode = circleArcMode
        }
        if (Number.isFinite(Number(toolParameters.circle_arc_start_angle_deg))) {
            drawingCircleArcStartAngleDeg = Number(toolParameters.circle_arc_start_angle_deg)
        }
        if (Number.isFinite(Number(toolParameters.circle_arc_end_angle_deg))) {
            drawingCircleArcEndAngleDeg = Number(toolParameters.circle_arc_end_angle_deg)
        }
        if (Number.isFinite(Number(toolParameters.regular_polygon_sides))) {
            drawingRegularPolygonSides = Math.max(3, Math.min(64, Math.round(Number(toolParameters.regular_polygon_sides))))
        }
        if (Number.isFinite(Number(toolParameters.regular_polygon_rotation_deg))) {
            drawingRegularPolygonRotationDeg = Number(toolParameters.regular_polygon_rotation_deg)
        }
        if (Number.isFinite(Number(toolParameters.line_thickness))) {
            drawingLineThickness = clampThickness(toolParameters.line_thickness)
        }
        if (toolParameters.line_style) {
            drawingLineStyle = normalizeLineStyle(toolParameters.line_style)
        }
        if (Number.isFinite(Number(toolParameters.stroke_opacity))) {
            drawingStrokeOpacity = clampOpacity(toolParameters.stroke_opacity)
        }
        var strokeColor = normalizeHexColor(toolParameters.stroke_color)
        if (String(toolParameters.stroke_color || "") === "") {
            drawingStrokeColor = "#f4d46f"
        } else if (strokeColor.length > 0) {
            drawingStrokeColor = strokeColor
        }
        if (Object.prototype.hasOwnProperty.call(toolParameters, "fill_color")) {
            if (String(toolParameters.fill_color || "") === "") {
                drawingFillColor = ""
            } else {
                var fillColor = normalizeHexColor(toolParameters.fill_color)
                if (fillColor.length > 0) {
                    drawingFillColor = fillColor
                }
            }
        }
        if (Object.prototype.hasOwnProperty.call(toolParameters, "line_variant")) {
            drawingLineVariant = normalizeLineVariant(toolParameters.line_variant)
        }
        if (String(toolParameters.circle_arc_mode || "").length > 0) {
            var loadedCircleArcMode = String(toolParameters.circle_arc_mode || "").trim().toLowerCase()
            if (loadedCircleArcMode === "circle" || loadedCircleArcMode === "arc") {
                drawingCircleArcMode = loadedCircleArcMode
            }
        }
        if (Object.prototype.hasOwnProperty.call(toolParameters, "drawing_style")) {
            var style = toolParameters.drawing_style || ({})
            if (Object.prototype.hasOwnProperty.call(style, "line_variant")) {
                drawingLineVariant = normalizeLineVariant(style.line_variant)
            }
            if (Object.prototype.hasOwnProperty.call(style, "stroke_color")) {
                var loadedStrokeColor = normalizeHexColor(style.stroke_color)
                if (String(style.stroke_color || "").trim().length === 0) {
                    drawingStrokeColor = ""
                } else if (loadedStrokeColor.length > 0) {
                    drawingStrokeColor = loadedStrokeColor
                }
            }
            if (Object.prototype.hasOwnProperty.call(style, "fill_color")) {
                var loadedFillColor = normalizeHexColor(style.fill_color)
                if (String(style.fill_color || "").trim().length === 0) {
                    drawingFillColor = ""
                } else if (loadedFillColor.length > 0) {
                    drawingFillColor = loadedFillColor
                }
            }
            if (Object.prototype.hasOwnProperty.call(style, "line_thickness")) {
                drawingLineThickness = clampThickness(style.line_thickness)
            }
            if (Object.prototype.hasOwnProperty.call(style, "line_style")) {
                drawingLineStyle = normalizeLineStyle(style.line_style)
            }
            if (Object.prototype.hasOwnProperty.call(style, "stroke_opacity")) {
                drawingStrokeOpacity = clampOpacity(style.stroke_opacity)
            }
            if (Object.prototype.hasOwnProperty.call(style, "circle_arc_mode")) {
                var styleCircleArcMode = String(style.circle_arc_mode || "").trim().toLowerCase()
                if (styleCircleArcMode === "circle" || styleCircleArcMode === "arc") {
                    drawingCircleArcMode = styleCircleArcMode
                }
            }
        }
        selectedDrawingToolId = String(document.selected_tool_id || selectedDrawingToolId)
        selectedDrawingVariantId = String(document.selected_variant_id || selectedDrawingVariantId)
        normalizeSelectedDrawingVariant()
        var incomingLayerId = String(document.selected_layer_id || selectedDrawingLayerId)
        selectedDrawingLayerId = String(drawingFindById(drawingLayerStack, incomingLayerId, drawingLayerStack[0] || ({})).id || incomingLayerId)
        var incomingObjectId = String(document.selected_object_id || "")
        selectedDrawingObjectId = ""
        selectedDrawingObjectIds = []
        var incomingObjectIds = asArray(document.selected_object_ids)
        var validIds = []
        for (var idIndex = 0; idIndex < incomingObjectIds.length; ++idIndex) {
            var candidateId = String(incomingObjectIds[idIndex] || "")
            if (candidateId.length > 0 && drawingFindById(drawingCanvasObjects(revision), candidateId, null) && validIds.indexOf(candidateId) < 0) {
                validIds.push(candidateId)
            }
        }
        if (validIds.length > 0) {
            selectedDrawingObjectIds = validIds
            selectedDrawingObjectId = validIds[validIds.length - 1]
        }
        if (incomingObjectId.length > 0 && drawingFindById(drawingCanvasObjects(revision), incomingObjectId, null)) {
            selectedDrawingObjectId = incomingObjectId
            if (selectedDrawingObjectIds.indexOf(incomingObjectId) < 0) {
                selectedDrawingObjectIds.push(incomingObjectId)
            }
        }
        markChanged()
    }

    function drawingCanvasObjects(unusedRevision) {
        var objects = [
            { id: "artboard_bounds", label: "Artboard bounds", kind: "rect", layer_id: "layer_00_canvas", detail: "normalized square artboard" },
            { id: "major_grid", label: "Major grid", kind: "grid", layer_id: "layer_01_grid", detail: drawingGridMode + " / " + String(drawingGridDivisions) + " divisions" }
        ]
        var generated = asArray(drawingGeneratedObjects)
        for (var index = 0; index < generated.length; ++index) {
            objects.push(generated[index])
        }
        return objects
    }

    function drawingCanvasDocument(unusedRevision) {
        return {
            canvas_id: "pattern_lab_2d_native_canvas_v0",
            coordinate_space: "normalized_artboard",
            selected_tool_id: selectedDrawingToolId,
            selected_variant_id: selectedDrawingVariantId,
            selected_layer_id: selectedDrawingLayerId,
            selected_object_id: selectedDrawingObjectId,
            selected_object_ids: selectedDrawingObjectIds,
            pending_point: drawingPendingPoint,
            layers: [
                { id: "layer_00_canvas", visible: true, objects: [
                    { id: "artboard_bounds", kind: "rect", border_visible: drawingArtboardBorderVisible }
                ] },
                { id: "layer_01_grid", visible: drawingGridVisible, objects: [
                    {
                        id: "major_grid",
                        kind: "grid",
                        mode: drawingGridMode,
                        divisions: drawingGridDivisions,
                        major_every: drawingGridMajorEvery,
                        ascii_cell_grid_visible: drawingAsciiCellGridVisible,
                        ascii_columns: drawingAsciiColumns,
                        ascii_rows: drawingAsciiRows,
                        ascii_major_every: drawingAsciiMajorEvery,
                        center_axes_visible: drawingCenterAxesVisible,
                        diagonal_guides_visible: drawingDiagonalGuidesVisible,
                        radial_guides_visible: drawingRadialGuidesVisible,
                        radial_guide_count: drawingRadialGuideCount
                    }
                ] },
                { id: "layer_09_script_geometry", visible: true, objects: asArray(drawingGeneratedObjects) },
                { id: "layer_08_metadata", visible: false, objects: [
                    { id: "last_script_status", kind: "metadata", value: drawingLastScriptStatus }
                ] }
            ]
        }
    }

    function drawingCanvasExportDocument(unusedRevision) {
        if (drawingNativeController) {
            return drawingNativeController.modelDocument()
        }
        var toolParameters = {
            circle_arc_mode: drawingCircleArcMode,
            circle_arc_start_angle_deg: drawingCircleArcStartAngleDeg,
            circle_arc_end_angle_deg: drawingCircleArcEndAngleDeg,
            regular_polygon_sides: drawingRegularPolygonSides,
            regular_polygon_rotation_deg: drawingRegularPolygonRotationDeg,
            line_variant: drawingLineVariant,
            line_thickness: drawingLineThickness,
            line_style: drawingLineStyle,
            stroke_opacity: drawingStrokeOpacity,
            stroke_color: drawingStrokeColor,
            fill_color: drawingFillColor
        }
        return {
            export_kind: "pattern_lab_2d_native_model_v0",
            script_id: drawingLastScriptId,
            script_status: drawingLastScriptStatus,
            script_errors: drawingLastScriptErrors,
            canvas_px: [drawingCanvasSizePx, drawingCanvasSizePx],
            snap: {
                grid_enabled: drawingSnapGridEnabled,
                grid_step_px: drawingSnapGridStepPx
            },
            selected_tool_id: selectedDrawingToolId,
            selected_variant_id: selectedDrawingVariantId,
            selected_layer_id: selectedDrawingLayerId,
            selected_object_id: selectedDrawingObjectId,
            selected_object_ids: selectedDrawingObjectIds,
            tool_parameters: toolParameters,
            drawing_style: {
                selected_variant_id: selectedDrawingVariantId,
                line_variant: drawingLineVariant,
                line_thickness: drawingLineThickness,
                line_style: drawingLineStyle,
                stroke_opacity: drawingStrokeOpacity,
                stroke_color: drawingStrokeColor,
                fill_color: drawingFillColor,
                circle_arc_mode: drawingCircleArcMode
            },
            generated_objects: asArray(drawingGeneratedObjects),
            object_counts: drawingObjectCounts(revision),
            validation: drawingModelValidationRows(revision)
        }
    }

    function drawingCanvasExportJson(unusedRevision) {
        if (drawingNativeController) {
            return drawingNativeController.exportJson()
        }
        return JSON.stringify(drawingCanvasExportDocument(revision), null, 2) + "\n"
    }

    function drawingObjectCounts(unusedRevision) {
        var counts = ({})
        var objects = drawingCanvasObjects(revision)
        for (var index = 0; index < objects.length; ++index) {
            var kind = String(objects[index].kind || "unknown")
            counts[kind] = Number(counts[kind] || 0) + 1
        }
        return counts
    }

    function drawingModelValidationRows(unusedRevision) {
        return DrawingRuntimeRows.modelValidationRows(drawingSession)
    }

    function drawingFitTransform(unusedRevision) {
        return DrawingRuntimeRows.fitTransform(drawingSession)
    }

    function drawingEditNumber(value) {
        return DrawingRuntimeRows.editNumber(value)
    }

    function drawingObjectEditRows(unusedRevision) {
        return DrawingRuntimeRows.objectEditRows(drawingSession)
    }

    function drawingInspectorRows(unusedRevision) {
        return DrawingRuntimeRows.inspectorRows(drawingSession)
    }

    function drawingToolSettingsRows(unusedRevision) {
        return DrawingRuntimeRows.toolSettingsRows(drawingSession)
    }

    function drawingToolParameterEditRows(unusedRevision) {
        return DrawingRuntimeRows.toolParameterEditRows(drawingSession)
    }

    function hasSelectedDrawingExternalTool(unusedRevision) {
        return String(selectedDrawingExternalToolId || "").length > 0
    }

    function drawingExternalToolRows(unusedRevision) {
        return DrawingRuntimeRows.externalToolRows(drawingSession)
    }

    function drawingSidebarRows(section, unusedRevision) {
        return DrawingRuntimeRows.sidebarRows(drawingSession, section)
    }

    function drawingSidebarRowSelected(section, row, unusedRevision) {
        return DrawingRuntimeRows.sidebarRowSelected(drawingSession, section, row)
    }

    function drawingSidebarRowClickable(section, unusedRevision) {
        return DrawingRuntimeRows.sidebarRowClickable(section)
    }

    function drawingSidebarRowClicked(section, row) {
        if (!section || !row) {
            return
        }
        var actionHandlers = ({
            tool: function(item) { selectDrawingTool(item.id) },
            preset: function(item) { selectDrawingPreset(item.id) },
            external_tool: function(item) { selectDrawingExternalTool(item.id) },
            layer: function(item) { selectDrawingLayer(item.id) }
        })
        var handler = actionHandlers[String(section.action || "")]
        if (handler) {
            handler(row)
        }
    }

    function drawingToolPaletteRows(unusedRevision) {
        return DrawingRuntimeRows.toolPaletteRows(drawingSession)
    }

    function drawingValidationRows(unusedRevision) {
        return DrawingRuntimeRows.validationRows(drawingSession)
    }

    function drawingModelObjectRows(unusedRevision) {
        return DrawingRuntimeRows.modelObjectRows(drawingSession)
    }

    function drawingLogRows(unusedRevision) {
        return DrawingRuntimeRows.logRows(drawingSession)
    }

    function drawingExportRows(unusedRevision) {
        return DrawingRuntimeRows.exportRows(drawingSession)
    }

    function drawingManifestRows(unusedRevision) {
        return DrawingRuntimeRows.manifestRows(drawingSession)
    }
}
