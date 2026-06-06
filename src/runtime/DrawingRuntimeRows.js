.pragma library

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

function editNumber(value) {
    var number = Number(value || 0)
    return Number.isFinite(number) ? String(Math.round(number * 1000) / 1000) : "0"
}

function fitTransform(controller) {
    var root = controller.drawingAnchorPoint("anchor_root")
    var tip = controller.drawingAnchorPoint("anchor_tip")
    var dx = Number(tip.x) - Number(root.x)
    var dy = Number(tip.y) - Number(root.y)
    var targetLength = Math.sqrt(dx * dx + dy * dy)
    var sourceDx = 0.0
    var sourceDy = -0.30
    var sourceLength = Math.sqrt(sourceDx * sourceDx + sourceDy * sourceDy)
    var ok = targetLength > 0.001 && sourceLength > 0.001
    var rotationDeg = ok ? (Math.atan2(dy, dx) - Math.atan2(sourceDy, sourceDx)) * 180 / Math.PI : 0
    return {
        ok: ok,
        source_length: sourceLength,
        target_length: targetLength,
        scale: ok ? targetLength / sourceLength : 0,
        rotation_deg: rotationDeg,
        root_x: Number(root.x),
        root_y: Number(root.y),
        tip_x: Number(tip.x),
        tip_y: Number(tip.y),
        dx: dx,
        dy: dy
    }
}

function appendPointInspectorRows(rows, object) {
    rows.push({ label: "Point px", value: asArray(object.point_px).join(", ") })
}

function appendLineInspectorRows(rows, object) {
    rows.push({ label: "From px", value: asArray(object.from_px).join(", ") })
    rows.push({ label: "To px", value: asArray(object.to_px).join(", ") })
}

function appendCircleInspectorRows(rows, object) {
    rows.push({ label: "Center px", value: asArray(object.center_px).join(", ") })
    rows.push({ label: "Radius px", value: String(Number(object.radius_px || 0).toFixed(1)) })
}

function appendArcInspectorRows(rows, object) {
    appendCircleInspectorRows(rows, object)
    rows.push({ label: "Angles", value: String(object.start_angle_deg || 0) + " -> " + String(object.end_angle_deg || 0) })
}

function appendPolygonInspectorRows(rows, object) {
    appendCircleInspectorRows(rows, object)
    rows.push({ label: "Sides", value: String(object.sides || "") })
}

function appendRectangleInspectorRows(rows, object) {
    rows.push({ label: "Rect px", value: asArray(object.rect_px).join(", ") })
}

function appendInspectorRows(rows, object) {
    var kind = String(object.kind || "")
    if (kind === "point" || kind === "tone_probe") {
        appendPointInspectorRows(rows, object)
    } else if (kind === "line" || kind === "glyph_baseline") {
        appendLineInspectorRows(rows, object)
    } else if (kind === "circle") {
        appendCircleInspectorRows(rows, object)
    } else if (kind === "arc") {
        appendArcInspectorRows(rows, object)
    } else if (kind === "polygon") {
        appendPolygonInspectorRows(rows, object)
    } else if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
        appendRectangleInspectorRows(rows, object)
    }
}

function objectEditRows(controller) {
    var object = controller.selectedDrawingObject()
    var id = String(object.id || "")
    if (id.indexOf("script_") !== 0) {
        return []
    }
    var kind = String(object.kind || "")
    if (kind === "point" || kind === "tone_probe") {
        var point = asArray(object.point_px)
        return [
            { label: "X px", field: "x_px", value: editNumber(point[0]) },
            { label: "Y px", field: "y_px", value: editNumber(point[1]) }
        ]
    }
    if (kind === "line" || kind === "glyph_baseline") {
        var from = asArray(object.from_px)
        var to = asArray(object.to_px)
        return [
            { label: "X1 px", field: "x1_px", value: editNumber(from[0]) },
            { label: "Y1 px", field: "y1_px", value: editNumber(from[1]) },
            { label: "X2 px", field: "x2_px", value: editNumber(to[0]) },
            { label: "Y2 px", field: "y2_px", value: editNumber(to[1]) }
        ]
    }
    if (kind === "circle") {
        var center = asArray(object.center_px)
        return [
            { label: "Center X px", field: "cx_px", value: editNumber(center[0]) },
            { label: "Center Y px", field: "cy_px", value: editNumber(center[1]) },
            { label: "Radius px", field: "radius_px", value: editNumber(object.radius_px) }
        ]
    }
    if (kind === "arc") {
        var arcCenter = asArray(object.center_px)
        return [
            { label: "Center X px", field: "cx_px", value: editNumber(arcCenter[0]) },
            { label: "Center Y px", field: "cy_px", value: editNumber(arcCenter[1]) },
            { label: "Radius px", field: "radius_px", value: editNumber(object.radius_px) },
            { label: "Start deg", field: "start_angle_deg", value: editNumber(object.start_angle_deg) },
            { label: "End deg", field: "end_angle_deg", value: editNumber(object.end_angle_deg) }
        ]
    }
    if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
        var rect = asArray(object.rect_px)
        return [
            { label: "X px", field: "x_px", value: editNumber(rect[0]) },
            { label: "Y px", field: "y_px", value: editNumber(rect[1]) },
            { label: "Width px", field: "width_px", value: editNumber(rect[2]) },
            { label: "Height px", field: "height_px", value: editNumber(rect[3]) }
        ]
    }
    if (kind === "polygon") {
        var polygonCenter = asArray(object.center_px)
        return [
            { label: "Center X px", field: "cx_px", value: editNumber(polygonCenter[0]) },
            { label: "Center Y px", field: "cy_px", value: editNumber(polygonCenter[1]) },
            { label: "Radius px", field: "radius_px", value: editNumber(object.radius_px) },
            { label: "Sides", field: "sides", value: editNumber(object.sides) },
            { label: "Rotation deg", field: "rotation_deg", value: editNumber(object.rotation_deg) }
        ]
    }
    return []
}

function inspectorRows(controller) {
    var tool = controller.selectedDrawingTool()
    var layer = controller.selectedDrawingLayer()
    var object = controller.selectedDrawingObject()
    var hasObject = String(object.id || "").length > 0
    var rows = [
        { label: "Tool", value: String(tool.label || controller.selectedDrawingToolId) },
        { label: "Layer", value: String(layer.label || controller.selectedDrawingLayerId) },
        { label: "Object", value: hasObject ? String(object.label || controller.selectedDrawingObjectId) : "none" },
        { label: "Kind", value: hasObject ? String(object.kind || "unknown") : "none" },
        { label: "Detail", value: hasObject ? String(object.detail || "model-backed drawing object") : "No object selected" }
    ]
    appendInspectorRows(rows, object)
    rows.push({ label: "Fit status", value: fitTransform(controller).ok ? "root-tip transform valid" : "invalid root-tip vector" })
    return rows
}

function toolSettingsRows(controller) {
    var rows = controller.drawingToolSettingsById[String(controller.selectedDrawingToolId || "")]
    var result = rows || controller.drawingToolSettingsById.select_move
    if (String(controller.selectedDrawingToolId || "") === "circle_arc") {
        return result.concat([
            { label: "Current mode", value: controller.drawingCircleArcMode },
            { label: "Arc span", value: editNumber(controller.drawingCircleArcStartAngleDeg) + " -> " + editNumber(controller.drawingCircleArcEndAngleDeg) + " deg" }
        ])
    }
    if (String(controller.selectedDrawingToolId || "") === "regular_polygon") {
        return result.concat([
            { label: "Sides", value: String(controller.drawingRegularPolygonSides) },
            { label: "Rotation", value: editNumber(controller.drawingRegularPolygonRotationDeg) + " deg" }
        ])
    }
    return result
}

function toolParameterEditRows(controller) {
    if (String(controller.selectedDrawingToolId || "") === "circle_arc") {
        return [
            { label: "Mode", field: "circle_arc_mode", value: controller.drawingCircleArcMode, numeric: false },
            { label: "Start deg", field: "circle_arc_start_angle_deg", value: editNumber(controller.drawingCircleArcStartAngleDeg), numeric: true },
            { label: "End deg", field: "circle_arc_end_angle_deg", value: editNumber(controller.drawingCircleArcEndAngleDeg), numeric: true }
        ]
    }
    if (String(controller.selectedDrawingToolId || "") === "regular_polygon") {
        return [
            { label: "Sides", field: "regular_polygon_sides", value: editNumber(controller.drawingRegularPolygonSides), numeric: true },
            { label: "Rotation deg", field: "regular_polygon_rotation_deg", value: editNumber(controller.drawingRegularPolygonRotationDeg), numeric: true }
        ]
    }
    return []
}

function modelValidationRows(controller) {
    if (controller.drawingNativeController) {
        return asArray(controller.drawingNativeController.modelDocument().validation)
    }
    var rows = []
    var generated = asArray(controller.drawingGeneratedObjects)
    rows.push({ id: "script_status", status: controller.drawingLastScriptStatus, detail: controller.drawingLastScriptErrors.length === 0 ? "no replay errors" : controller.drawingLastScriptErrors.join("; ") })
    rows.push({ id: "generated_count", status: generated.length > 0 ? "pass" : "empty", detail: String(generated.length) })
    for (var index = 0; index < generated.length; ++index) {
        var object = generated[index]
        var finite = true
        var keys = ["x1", "y1", "x2", "y2"]
        for (var keyIndex = 0; keyIndex < keys.length; ++keyIndex) {
            var value = Number(object[keys[keyIndex]])
            if (!Number.isFinite(value)) {
                finite = false
            }
            if (value < 0 || value > 1) {
                finite = false
            }
        }
        rows.push({ id: object.id + "_bounds", status: finite ? "pass" : "fail", detail: finite ? "inside normalized artboard" : "invalid coordinate" })
    }
    return rows
}

function externalToolRows(controller) {
    var rows = controller.drawingExternalToolSettingsById[String(controller.selectedDrawingExternalToolId || "")]
    return rows || []
}

function sidebarRows(controller, section) {
    if (!section) {
        return []
    }
    var source = controller[String(section.source || "")] || []
    var ids = asArray(section.ids)
    if (ids.length === 0) {
        return source
    }
    var rowsById = ({})
    for (var sourceIndex = 0; sourceIndex < source.length; ++sourceIndex) {
        rowsById[String(source[sourceIndex].id || "")] = source[sourceIndex]
    }
    var rows = []
    for (var idIndex = 0; idIndex < ids.length; ++idIndex) {
        var row = rowsById[String(ids[idIndex])]
        if (row) {
            rows.push(row)
        }
    }
    return rows
}

function sidebarRowSelected(controller, section, row) {
    if (!section || !row || String(section.selectedProperty || "").length === 0) {
        return false
    }
    return String(controller[String(section.selectedProperty)] || "") === String(row.id || "")
}

function sidebarRowClickable(section) {
    return section && String(section.action || "").length > 0
}

function toolPaletteRows(controller) {
    var preset = controller.selectedDrawingPreset()
    return [
        { label: "Tool", value: String(controller.selectedDrawingTool().label || controller.selectedDrawingToolId) },
        { label: "Preset", value: String(preset.label || controller.selectedDrawingPresetId) },
        { label: "Snap", value: "grid + object + polar" },
        { label: "Layer", value: String(controller.selectedDrawingLayer().label || controller.selectedDrawingLayerId) },
        { label: "Object", value: String(controller.selectedDrawingObject().label || controller.selectedDrawingObjectId) }
    ]
}

function validationRows(controller) {
    if (controller.drawingNativeController) {
        var model = controller.drawingNativeController.modelDocument()
        var validation = asArray(model.validation)
        var rows = [
            { label: "State source", value: "C++ DrawingDocumentController" },
            { label: "Engine", value: String(model.engine || "cpp_drawing_core_v1") },
            { label: "Status", value: String(model.script_status || "unknown") }
        ]
        for (var index = 0; index < validation.length; ++index) {
            rows.push({
                label: String(validation[index].id || "validation"),
                value: String(validation[index].status || "") + " / " + String(validation[index].detail || "")
            })
        }
        return rows
    }
    return [
        { label: "State source", value: "runtime controller" },
        { label: "Canvas model", value: "drawingCanvasDocument()" },
        { label: "Fit transform", value: fitTransform(controller).ok ? "valid" : "invalid" },
        { label: "Writes", value: controller.writeDisabled ? "disabled" : "enabled" },
        { label: "Layer count", value: String(controller.drawingLayerStack.length) },
        { label: "Object count", value: String(controller.drawingCanvasObjects(controller.revision).length) }
    ]
}

function modelObjectRows(controller) {
    var generated = asArray(controller.drawingGeneratedObjects)
    var rows = []
    for (var index = generated.length - 1; index >= 0; --index) {
        var object = generated[index]
        rows.push({
            id: String(object.id || ""),
            label: String(object.id || "object"),
            meta: String(object.kind || "unknown"),
            selected: String(object.id || "") === controller.selectedDrawingObjectId
        })
    }
    if (rows.length === 0) {
        rows.push({ id: "", label: "No model objects", meta: "draw something", selected: false })
    }
    return rows
}

function logRows(controller) {
    if (controller.drawingNativeController) {
        var model = controller.drawingNativeController.modelDocument()
        var commands = asArray(model.command_log)
        var lastCommand = commands.length > 0 ? String((commands[commands.length - 1] || ({})).cmd || "unknown") : "none"
        return [
            { label: "Native document", value: "active" },
            { label: "Selected tool", value: String(controller.selectedDrawingTool().label || controller.selectedDrawingToolId) },
            { label: "Selected object", value: String(model.selected_object_id || "none") },
            { label: "Script status", value: String(model.script_status || "not_run") },
            { label: "Model objects", value: String(asArray(model.generated_objects).length) },
            { label: "Commands", value: String(commands.length) },
            { label: "Last command", value: lastCommand }
        ]
    }
    return [
        { label: "Native shell booted", value: "ok" },
        { label: "Selected tool", value: String(controller.selectedDrawingTool().label || controller.selectedDrawingToolId) },
        { label: "Selected layer", value: String(controller.selectedDrawingLayer().label || controller.selectedDrawingLayerId) },
        { label: "Selected object", value: String(controller.selectedDrawingObject().label || controller.selectedDrawingObjectId) },
        { label: "Fit scale", value: fitTransform(controller).scale.toFixed(3) },
        { label: "Fit rotation", value: fitTransform(controller).rotation_deg.toFixed(1) + " deg" }
    ]
}

function exportRows(controller) {
    if (controller.drawingNativeController) {
        return [
            { label: "Model JSON", value: "native exportJson() ready" },
            { label: "SVG", value: "native exportSvg() ready" },
            { label: "Interactive writes", value: "in-memory document" }
        ]
    }
    return [
        { label: "Model JSON", value: "runtime fallback document" },
        { label: "SVG", value: "native controller required" },
        { label: "Interactive writes", value: controller.writeDisabled ? "disabled" : "enabled" }
    ]
}

function manifestRows(controller) {
    if (controller.drawingNativeController) {
        var model = controller.drawingNativeController.modelDocument()
        var counts = model.object_counts || ({})
        var keys = Object.keys(counts)
        var countText = keys.length > 0 ? keys.map(function(key) { return key + ":" + counts[key] }).join(", ") : "none"
        return [
            { label: "Export kind", value: String(model.export_kind || "") },
            { label: "Engine", value: String(model.engine || "") },
            { label: "Canvas", value: asArray(model.canvas_px).join(" x ") },
            { label: "Counts", value: countText },
            { label: "Commands", value: String(model.command_count || asArray(model.command_log).length) },
            { label: "Snap", value: (model.snap && model.snap.grid_enabled ? "grid " : "off ") + String(model.snap ? model.snap.grid_step_px : "") }
        ]
    }
    return [
        { label: "Recipe-first", value: "required" },
        { label: "Layer metadata", value: "model-backed" },
        { label: "Object counts", value: JSON.stringify(controller.drawingObjectCounts(controller.revision)) },
        { label: "Validation rows", value: String(controller.drawingModelValidationRows(controller.revision).length) },
        { label: "Native controller", value: controller.drawingNativeController ? "active" : "missing" }
    ]
}
