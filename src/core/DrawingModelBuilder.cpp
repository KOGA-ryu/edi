#include "DrawingCoreInternal.h"

namespace drawing_core {

QJsonObject objectCounts(const QJsonArray &objects) {
    QJsonObject counts;
    for (const QJsonValue value : objects) {
        const QString kind = value.toObject().value("kind").toString("unknown");
        counts.insert(kind, counts.value(kind).toInt() + 1);
    }
    return counts;
}

QJsonObject pendingPointObject(const Point &point) {
    QJsonObject object;
    if (!point.ok) {
        return object;
    }
    object.insert("x", point.nx);
    object.insert("y", point.ny);
    object.insert("point_px", pointArray(point.x, point.y));
    return object;
}

QJsonArray validationRows(const State &state) {
    QJsonArray rows;
    QJsonObject status;
    status.insert("id", "script_status");
    status.insert("status", state.errors.isEmpty() ? "pass" : "fail");
    status.insert("detail", state.errors.isEmpty() ? "no replay errors" : "see script_errors");
    rows.append(status);

    QJsonObject count;
    count.insert("id", "generated_count");
    count.insert("status", state.generatedObjects.isEmpty() ? "empty" : "pass");
    count.insert("detail", QString::number(state.generatedObjects.size()));
    rows.append(count);
    return rows;
}

QJsonObject buildModel(const State &state) {
    QJsonObject model;
    model.insert("export_kind", "pattern_lab_2d_native_model_v0");
    model.insert("engine", "cpp_drawing_core_v1");
    model.insert("script_id", state.scriptId);
    model.insert("script_status", state.errors.isEmpty() ? "pass" : "fail");
    model.insert("script_errors", state.errors);
    model.insert("canvas_px", pointArray(state.canvasPx, state.canvasPx));
    QJsonObject toolParameters;
    toolParameters.insert("circle_arc_mode", state.circleArcMode);
    toolParameters.insert("circle_arc_start_angle_deg", state.circleArcStartAngleDeg);
    toolParameters.insert("circle_arc_end_angle_deg", state.circleArcEndAngleDeg);
    toolParameters.insert("regular_polygon_sides", state.regularPolygonSides);
    toolParameters.insert("regular_polygon_rotation_deg", state.regularPolygonRotationDeg);
    toolParameters.insert("line_variant", state.lineVariant);
    toolParameters.insert("line_thickness", state.lineThickness);
    toolParameters.insert("line_style", state.lineStyle);
    toolParameters.insert("stroke_opacity", state.strokeOpacity);
    toolParameters.insert("stroke_color", state.strokeColor);
    toolParameters.insert("fill_color", state.fillColor);
    model.insert("tool_parameters", toolParameters);
    QJsonObject snap;
    snap.insert("grid_enabled", state.gridSnap);
    snap.insert("grid_step_px", state.gridStepPx);
    model.insert("snap", snap);
    model.insert("selected_tool_id", state.selectedTool);
    model.insert("selected_layer_id", state.selectedLayer);
    model.insert("selected_object_id", state.selectedObject);
    model.insert("selected_object_ids", stringListToJsonArray(state.selectedObjects));
    model.insert("pending_point", pendingPointObject(state.pendingPoint));
    model.insert("command_log", state.commandLog);
    model.insert("command_count", state.commandLog.size());
    model.insert("generated_objects", state.generatedObjects);
    model.insert("object_counts", objectCounts(state.generatedObjects));
    model.insert("validation", validationRows(state));
    return model;
}

} // namespace drawing_core
