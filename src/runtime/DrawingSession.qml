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
    property var drawingMetadataPresetsDocument: ({})
    property string drawingMetadataPresetsPath: ""
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
    property DrawingAnchorSession anchorSession: DrawingAnchorSession {
        id: anchorSession
    }
    property var drawingExternalModelDocument: ({})
    property var drawingGeneratedObjects: []
    property var drawingPendingPoint: ({})
    property bool drawingPendingShapeActive: false
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
    property var drawingNativeController: null
    property DrawingViewportSession viewportSession: DrawingViewportSession {
        id: viewportSession
        drawingNativeController: drawingSession.drawingNativeController
        syncNativeDrawingModel: function () {
            syncNativeDrawingModel()
        }
        onChanged: markChanged()
    }
    property DrawingCanvasDocumentSession canvasDocumentSession: DrawingCanvasDocumentSession {
        id: canvasDocumentSession
    }
    property alias drawingSnapGridEnabled: viewportSession.drawingSnapGridEnabled
    property alias drawingSnapGridStepPx: viewportSession.drawingSnapGridStepPx
    property alias drawingObjectSnapEnabled: viewportSession.drawingObjectSnapEnabled
    property alias drawingObjectSnapTolerancePx: viewportSession.drawingObjectSnapTolerancePx
    property alias drawingObjectSnapEndpointEnabled: viewportSession.drawingObjectSnapEndpointEnabled
    property alias drawingObjectSnapMidpointEnabled: viewportSession.drawingObjectSnapMidpointEnabled
    property alias drawingObjectSnapCenterEnabled: viewportSession.drawingObjectSnapCenterEnabled
    property alias drawingObjectSnapVertexEnabled: viewportSession.drawingObjectSnapVertexEnabled
    property alias drawingGridVisible: viewportSession.drawingGridVisible
    property alias drawingGridMode: viewportSession.drawingGridMode
    property alias drawingGridDivisions: viewportSession.drawingGridDivisions
    property alias drawingGridMajorEvery: viewportSession.drawingGridMajorEvery
    property alias drawingAsciiCellGridVisible: viewportSession.drawingAsciiCellGridVisible
    property alias drawingAsciiColumns: viewportSession.drawingAsciiColumns
    property alias drawingAsciiRows: viewportSession.drawingAsciiRows
    property alias drawingAsciiMajorEvery: viewportSession.drawingAsciiMajorEvery
    property alias drawingCenterAxesVisible: viewportSession.drawingCenterAxesVisible
    property alias drawingDiagonalGuidesVisible: viewportSession.drawingDiagonalGuidesVisible
    property alias drawingRadialGuidesVisible: viewportSession.drawingRadialGuidesVisible
    property alias drawingRadialGuideCount: viewportSession.drawingRadialGuideCount
    property alias drawingArtboardBorderVisible: viewportSession.drawingArtboardBorderVisible
    property alias drawingCanvasSizePx: viewportSession.drawingCanvasSizePx
    property alias drawingCanvasZoom: viewportSession.drawingCanvasZoom
    property alias drawingCanvasZoomMin: viewportSession.drawingCanvasZoomMin
    property alias drawingCanvasZoomMax: viewportSession.drawingCanvasZoomMax
    property alias drawingCanvasPanXPx: viewportSession.drawingCanvasPanXPx
    property alias drawingCanvasPanYPx: viewportSession.drawingCanvasPanYPx
    property alias drawingAnchorRootX: anchorSession.drawingAnchorRootX
    property alias drawingAnchorRootY: anchorSession.drawingAnchorRootY
    property alias drawingAnchorTipX: anchorSession.drawingAnchorTipX
    property alias drawingAnchorTipY: anchorSession.drawingAnchorTipY
    property alias drawingAnchorRightX: anchorSession.drawingAnchorRightX
    property alias drawingAnchorRightY: anchorSession.drawingAnchorRightY
    property alias drawingAnchorBottomX: anchorSession.drawingAnchorBottomX
    property alias drawingAnchorBottomY: anchorSession.drawingAnchorBottomY
    property alias drawingAnchorLeftX: anchorSession.drawingAnchorLeftX
    property alias drawingAnchorLeftY: anchorSession.drawingAnchorLeftY
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

    function metadataPresetValues(key, fallback) {
        var values = asArray(drawingMetadataPresetsDocument[key])
        return values.length > 0 ? values : fallback
    }

    function loadDrawingMetadataPresets(document, path) {
        drawingMetadataPresetsDocument = document || ({})
        drawingMetadataPresetsPath = String(path || "")
        markChanged()
    }

    function drawingMetadataPresetRows(unusedRevision) {
        return [
            { label: "quick", field: "tags", mode: "tag", options: metadataPresetValues("quick_tags", ["wall", "floor", "door", "spawn", "secret", "encounter", "collision", "decor"]) },
            { label: "role", field: "role", mode: "set", options: metadataPresetValues("roles", ["wall", "floor", "cutout", "collider"]) },
            { label: "mat", field: "material", mode: "set", options: metadataPresetValues("materials", ["stone", "metal", "glass", "wood"]) },
            { label: "group", field: "export_group", mode: "set", options: metadataPresetValues("export_groups", ["room_a", "collision", "shell"]) },
            { label: "tag", field: "tags", mode: "tag", options: metadataPresetValues("tags", ["block", "spawn", "secret", "review"]) }
        ]
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
        return anchorSession.drawingAnchorPoint(anchorId)
    }

    function setDrawingAnchorPosition(anchorId, x, y) {
        if (!anchorSession.setAnchorPosition(anchorId, x, y)) {
            return
        }
        selectedDrawingObjectId = anchorId
        selectedDrawingLayerId = "layer_00_canvas"
        markChanged()
    }

    function selectNearestDrawingAnchor(x, y, tolerance) {
        var bestId = anchorSession.nearestAnchor(x, y, tolerance)
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
        viewportSession.setDrawingGridVisible(visible)
    }

    function setDrawingGridMode(mode) {
        viewportSession.setDrawingGridMode(mode)
    }

    function setDrawingGridDivisions(divisions) {
        viewportSession.setDrawingGridDivisions(divisions)
    }

    function setDrawingGridMajorEvery(value) {
        viewportSession.setDrawingGridMajorEvery(value)
    }

    function setDrawingAsciiCellGridVisible(visible) {
        viewportSession.setDrawingAsciiCellGridVisible(visible)
    }

    function setDrawingAsciiColumns(columns) {
        viewportSession.setDrawingAsciiColumns(columns)
    }

    function setDrawingAsciiRows(rows) {
        viewportSession.setDrawingAsciiRows(rows)
    }

    function setDrawingAsciiMajorEvery(value) {
        viewportSession.setDrawingAsciiMajorEvery(value)
    }

    function setDrawingCenterAxesVisible(visible) {
        viewportSession.setDrawingCenterAxesVisible(visible)
    }

    function setDrawingDiagonalGuidesVisible(visible) {
        viewportSession.setDrawingDiagonalGuidesVisible(visible)
    }

    function setDrawingRadialGuidesVisible(visible) {
        viewportSession.setDrawingRadialGuidesVisible(visible)
    }

    function setDrawingRadialGuideCount(count) {
        viewportSession.setDrawingRadialGuideCount(count)
    }

    function setDrawingArtboardBorderVisible(visible) {
        viewportSession.setDrawingArtboardBorderVisible(visible)
    }

    function setDrawingSnapGrid(enabled) {
        viewportSession.setDrawingSnapGrid(enabled)
    }

    function setDrawingSnapGridStepPx(stepPx) {
        viewportSession.setDrawingSnapGridStepPx(stepPx)
    }

    function setDrawingObjectSnapEnabled(enabled) {
        viewportSession.setDrawingObjectSnapEnabled(enabled)
    }

    function setDrawingObjectSnapTolerancePx(value) {
        viewportSession.setDrawingObjectSnapTolerancePx(value)
    }

    function setDrawingObjectSnapEndpointEnabled(enabled) {
        viewportSession.setDrawingObjectSnapEndpointEnabled(enabled)
    }

    function setDrawingObjectSnapMidpointEnabled(enabled) {
        viewportSession.setDrawingObjectSnapMidpointEnabled(enabled)
    }

    function setDrawingObjectSnapCenterEnabled(enabled) {
        viewportSession.setDrawingObjectSnapCenterEnabled(enabled)
    }

    function setDrawingObjectSnapVertexEnabled(enabled) {
        viewportSession.setDrawingObjectSnapVertexEnabled(enabled)
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
        viewportSession.setDrawingCanvasZoom(zoom)
    }

    function drawingCanvasBaseViewSize(viewWidth, viewHeight) {
        return viewportSession.drawingCanvasBaseViewSize(viewWidth, viewHeight)
    }

    function zoomDrawingCanvasAt(factor, focusX, focusY, viewWidth, viewHeight) {
        viewportSession.zoomDrawingCanvasAt(factor, focusX, focusY, viewWidth, viewHeight)
    }

    function panDrawingCanvasBy(dx, dy) {
        viewportSession.panDrawingCanvasBy(dx, dy)
    }

    function zoomDrawingCanvasIn() {
        viewportSession.zoomDrawingCanvasIn()
    }

    function zoomDrawingCanvasOut() {
        viewportSession.zoomDrawingCanvasOut()
    }

    function resetDrawingCanvasZoom() {
        viewportSession.resetDrawingCanvasZoom()
    }

    function fitDrawingCanvasToView() {
        viewportSession.fitDrawingCanvasToView()
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

    function updateSelectedDrawingObjectMetadataField(field, rawValue) {
        var objectId = String(selectedDrawingObjectId || "")
        var fieldId = String(field || "")
        if (objectId.indexOf("script_") !== 0 || fieldId.length === 0) {
            return
        }
        if (drawingNativeController && typeof drawingNativeController.updateObjectMetadataField === "function") {
            drawingNativeController.updateObjectMetadataField(objectId, fieldId, rawValue)
            syncNativeDrawingModel()
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
        var hit = DrawingCanvasHitTest.hitObjectAt(drawingGeneratedObjects, Number(x), Number(y), tolerance)
        return String(hit.objectId || "")
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
        return canvasDocumentSession.drawingCanvasObjects(drawingSession, unusedRevision)
    }

    function drawingCanvasDocument(unusedRevision) {
        return canvasDocumentSession.drawingCanvasDocument(drawingSession, unusedRevision)
    }

    function drawingCanvasExportDocument(unusedRevision) {
        return canvasDocumentSession.drawingCanvasExportDocument(drawingSession, unusedRevision)
    }

    function drawingCanvasExportJson(unusedRevision) {
        return canvasDocumentSession.drawingCanvasExportJson(drawingSession, unusedRevision)
    }

    function drawingObjectCounts(unusedRevision) {
        return canvasDocumentSession.drawingObjectCounts(drawingSession, unusedRevision)
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
