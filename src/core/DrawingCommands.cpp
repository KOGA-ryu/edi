#include "DrawingCoreInternal.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <vector>

namespace drawing_core {

bool parameterNumber(const QJsonValue &value, double &result) {
    if (!value.isDouble()) {
        return false;
    }
    result = value.toDouble();
    return std::isfinite(result);
}

QString normalizedHexColor(const QString &value) {
    QString raw = value.trimmed().toLower();
    if (raw.isEmpty() || raw == QStringLiteral("none") || raw == QStringLiteral("transparent")) {
        return {};
    }
    if (!raw.startsWith('#')) {
        raw.prepend('#');
    }
    static const QRegularExpression hexPattern(QStringLiteral("^#([0-9a-f]{3}|[0-9a-f]{4}|[0-9a-f]{6}|[0-9a-f]{8})$"));
    return hexPattern.match(raw).hasMatch() ? raw : QString();
}

QString normalizedLineStyle(const QString &value) {
    const QString style = value.trimmed().toLower();
    if (style == QStringLiteral("dashed")) {
        return QStringLiteral("dashed");
    }
    if (style == QStringLiteral("dot") || style == QStringLiteral("dotted")) {
        return QStringLiteral("dotted");
    }
    return QStringLiteral("solid");
}

void applyActiveStyle(State &state, QJsonObject &object) {
    object.insert("line_variant", state.lineVariant);
    object.insert("line_style", state.lineStyle);
    object.insert("line_thickness", state.lineThickness);
    object.insert("stroke_opacity", state.strokeOpacity);
    object.insert("stroke_color", state.strokeColor);
    object.insert("fill_color", state.fillColor);
}

void setToolParameter(State &state, const QString &parameter, const QJsonValue &value) {
    if (parameter.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    double numericValue = 0.0;
    if (parameter == QStringLiteral("circle_arc_mode")) {
        const QString mode = value.toString().trimmed().toLower();
        if (mode != QStringLiteral("circle") && mode != QStringLiteral("arc")) {
            state.errors.append("set_tool_parameter invalid circle_arc_mode: " + value.toString());
            return;
        }
        state.circleArcMode = mode;
        state.pendingPoint = {};
        return;
    }
    if (parameter == QStringLiteral("line_variant")) {
        const QString variant = value.toString().trimmed().toLower();
        if (variant != QStringLiteral("straight") && variant != QStringLiteral("polyline")) {
            state.errors.append("set_tool_parameter invalid line_variant: " + value.toString());
            return;
        }
        state.lineVariant = variant;
        state.pendingPoint = {};
        return;
    }
    if (parameter == QStringLiteral("line_style")) {
        state.lineStyle = normalizedLineStyle(value.toString());
        return;
    }
    if (parameter == QStringLiteral("stroke_color")) {
        const QString color = normalizedHexColor(value.toString());
        if (color.isEmpty() && !value.toString().trimmed().isEmpty()) {
            state.errors.append("set_tool_parameter invalid stroke_color: " + value.toString());
            return;
        }
        state.strokeColor = color;
        return;
    }
    if (parameter == QStringLiteral("fill_color")) {
        const QString input = value.toString().trimmed();
        const QString lowerInput = input.toLower();
        const QString color = normalizedHexColor(input);
        if (color.isEmpty() && !input.isEmpty() && lowerInput != QStringLiteral("none") && lowerInput != QStringLiteral("transparent")) {
            state.errors.append("set_tool_parameter invalid fill_color: " + value.toString());
            return;
        }
        state.fillColor = color;
        return;
    }
    if (parameter == QStringLiteral("circle_arc_start_angle_deg")) {
        if (!parameterNumber(value, numericValue)) {
            state.errors.append("set_tool_parameter requires numeric value for: " + parameter);
            return;
        }
        state.circleArcStartAngleDeg = numericValue;
        state.pendingPoint = {};
        return;
    }
    if (parameter == QStringLiteral("circle_arc_end_angle_deg")) {
        if (!parameterNumber(value, numericValue)) {
            state.errors.append("set_tool_parameter requires numeric value for: " + parameter);
            return;
        }
        state.circleArcEndAngleDeg = numericValue;
        state.pendingPoint = {};
        return;
    }
    if (parameter == QStringLiteral("regular_polygon_sides")) {
        if (!parameterNumber(value, numericValue)) {
            state.errors.append("set_tool_parameter requires numeric value for: " + parameter);
            return;
        }
        state.regularPolygonSides = std::clamp(static_cast<int>(std::round(numericValue)), 3, 64);
        state.pendingPoint = {};
        return;
    }
    if (parameter == QStringLiteral("regular_polygon_rotation_deg")) {
        if (!parameterNumber(value, numericValue)) {
            state.errors.append("set_tool_parameter requires numeric value for: " + parameter);
            return;
        }
        state.regularPolygonRotationDeg = numericValue;
        state.pendingPoint = {};
        return;
    }
    if (parameter == QStringLiteral("line_thickness")) {
        if (!parameterNumber(value, numericValue)) {
            state.errors.append("set_tool_parameter requires numeric value for: " + parameter);
            return;
        }
        state.lineThickness = std::clamp(std::round(numericValue * 10.0) / 10.0, 1.0, 18.0);
        return;
    }
    if (parameter == QStringLiteral("stroke_opacity")) {
        if (!parameterNumber(value, numericValue)) {
            state.errors.append("set_tool_parameter requires numeric value for: " + parameter);
            return;
        }
        state.strokeOpacity = std::clamp(numericValue, 0.0, 1.0);
        return;
    }
    state.errors.append("set_tool_parameter unsupported parameter: " + parameter);
}

void addPointObject(State &state, const Point &point, const QString &kind, const QString &label, const QString &detail) {
    if (!point.ok) {
        state.errors.append("point command has invalid coordinates");
        return;
    }
    QJsonObject object;
    object.insert("id", nextId(state, kind));
    object.insert("label", label);
    object.insert("kind", kind);
    object.insert("detail", detail);
    applyActiveStyle(state, object);
    object.insert("x", point.nx);
    object.insert("y", point.ny);
    object.insert("point_px", pointArray(point.x, point.y));
    pushObject(state, object);
}

void addPoint(State &state, const Point &point) {
    addPointObject(state, point, QStringLiteral("point"), QStringLiteral("script point"), QStringLiteral("C++ generated point"));
}

void addLineObject(State &state, const Point &start, const Point &end, const QString &kind, const QString &label, const QString &detail) {
    if (!start.ok || !end.ok) {
        state.errors.append("line command has invalid endpoints");
        return;
    }
    QJsonObject object;
    object.insert("id", nextId(state, kind));
    object.insert("label", label);
    object.insert("kind", kind);
    object.insert("detail", detail);
    applyActiveStyle(state, object);
    object.insert("x1", start.nx);
    object.insert("y1", start.ny);
    object.insert("x2", end.nx);
    object.insert("y2", end.ny);
    object.insert("from_px", pointArray(start.x, start.y));
    object.insert("to_px", pointArray(end.x, end.y));
    pushObject(state, object);
}

void addLine(State &state, const Point &start, const Point &end) {
    addLineObject(state, start, end, QStringLiteral("line"), QStringLiteral("script line"), QStringLiteral("C++ generated line"));
}

void addPolyline(State &state, const QJsonArray &rawPoints) {
    if (rawPoints.size() < 2) {
        state.errors.append("polyline requires at least two points");
        return;
    }
    QJsonArray points;
    QJsonArray pointsPx;
    for (const QJsonValue value : rawPoints) {
        const Point point = pointFromArray(state, value.toArray());
        if (!point.ok) {
            state.errors.append("polyline contains invalid point");
            return;
        }
        points.append(pointArray(point.nx, point.ny));
        pointsPx.append(pointArray(point.x, point.y));
    }
    QJsonObject object;
    object.insert("id", nextId(state, "polyline"));
    object.insert("label", "script polyline");
    object.insert("kind", "polyline");
    object.insert("detail", "C++ generated polyline");
    applyActiveStyle(state, object);
    object.insert("points", points);
    object.insert("points_px", pointsPx);
    pushObject(state, object);
}

void addCircle(State &state, const Point &center, const QJsonObject &command) {
    if (!center.ok) {
        state.errors.append("circle command has invalid center");
        return;
    }
    double radiusPx = numberAt(command, "radius_px", -1.0);
    if (command.contains("radius_point")) {
        const Point radiusPoint = pointFromArray(state, arrayAt(command, "radius_point"));
        if (!radiusPoint.ok) {
            state.errors.append("circle command has invalid radius point");
            return;
        }
        radiusPx = distancePx(center, radiusPoint);
    }
    if (!std::isfinite(radiusPx) || radiusPx <= 0.0) {
        state.errors.append("circle command has invalid radius");
        return;
    }
    QJsonObject object;
    object.insert("id", nextId(state, "circle"));
    object.insert("label", "script circle");
    object.insert("kind", "circle");
    object.insert("detail", "C++ generated circle");
    applyActiveStyle(state, object);
    object.insert("cx", center.nx);
    object.insert("cy", center.ny);
    object.insert("radius", radiusPx / state.canvasPx);
    object.insert("center_px", pointArray(center.x, center.y));
    object.insert("radius_px", radiusPx);
    pushObject(state, object);
}

void addArc(State &state, const Point &center, const QJsonObject &command) {
    if (!center.ok) {
        state.errors.append("arc command has invalid center");
        return;
    }
    const double radiusPx = numberAt(command, "radius_px", 0.0);
    if (!std::isfinite(radiusPx) || radiusPx <= 0.0) {
        state.errors.append("arc command has invalid radius");
        return;
    }
    QJsonObject object;
    object.insert("id", nextId(state, "arc"));
    object.insert("label", "script arc");
    object.insert("kind", "arc");
    object.insert("detail", "C++ generated arc");
    applyActiveStyle(state, object);
    object.insert("cx", center.nx);
    object.insert("cy", center.ny);
    object.insert("radius", radiusPx / state.canvasPx);
    object.insert("center_px", pointArray(center.x, center.y));
    object.insert("radius_px", radiusPx);
    object.insert("start_angle_deg", numberAt(command, "start_angle_deg"));
    object.insert("end_angle_deg", numberAt(command, "end_angle_deg", 90.0));
    pushObject(state, object);
}

void addRectangleObject(State &state, const Point &start, const Point &end, const QString &kind, const QString &label, const QString &detail) {
    if (!start.ok || !end.ok) {
        state.errors.append("rectangle command has invalid corners");
        return;
    }
    const double left = std::min(start.x, end.x);
    const double top = std::min(start.y, end.y);
    const double width = std::abs(end.x - start.x);
    const double height = std::abs(end.y - start.y);
    QJsonObject object;
    object.insert("id", nextId(state, kind));
    object.insert("label", label);
    object.insert("kind", kind);
    object.insert("detail", detail);
    applyActiveStyle(state, object);
    object.insert("x", left / state.canvasPx);
    object.insert("y", top / state.canvasPx);
    object.insert("width", width / state.canvasPx);
    object.insert("height", height / state.canvasPx);
    object.insert("from_px", pointArray(start.x, start.y));
    object.insert("to_px", pointArray(end.x, end.y));
    QJsonArray rectPx;
    rectPx.append(left);
    rectPx.append(top);
    rectPx.append(width);
    rectPx.append(height);
    object.insert("rect_px", rectPx);
    pushObject(state, object);
}

void addRectangle(State &state, const Point &start, const Point &end) {
    addRectangleObject(state, start, end, QStringLiteral("rectangle"), QStringLiteral("script rectangle"), QStringLiteral("C++ generated rectangle"));
}

void addPolygon(State &state, const Point &center, const QJsonObject &command) {
    if (!center.ok) {
        state.errors.append("polygon command has invalid center");
        return;
    }
    const int sides = std::max(3, static_cast<int>(std::round(numberAt(command, "sides", 3.0))));
    const double radiusPx = numberAt(command, "radius_px", 0.0);
    const double rotationDeg = numberAt(command, "rotation_deg", 0.0);
    if (!std::isfinite(radiusPx) || radiusPx <= 0.0) {
        state.errors.append("polygon command has invalid radius");
        return;
    }
    QJsonArray points;
    QJsonArray pointsPx;
    for (int index = 0; index < sides; ++index) {
        const double angle = degreesToRadians(rotationDeg + 360.0 * index / sides);
        const double px = center.x + std::cos(angle) * radiusPx;
        const double py = center.y + std::sin(angle) * radiusPx;
        points.append(pointArray(px / state.canvasPx, py / state.canvasPx));
        pointsPx.append(pointArray(px, py));
    }
    QJsonObject object;
    object.insert("id", nextId(state, "polygon"));
    object.insert("label", "script polygon");
    object.insert("kind", "polygon");
    object.insert("detail", "C++ generated regular polygon");
    applyActiveStyle(state, object);
    object.insert("cx", center.nx);
    object.insert("cy", center.ny);
    object.insert("center_px", pointArray(center.x, center.y));
    object.insert("radius", radiusPx / state.canvasPx);
    object.insert("radius_px", radiusPx);
    object.insert("sides", sides);
    object.insert("rotation_deg", rotationDeg);
    object.insert("points", points);
    object.insert("points_px", pointsPx);
    pushObject(state, object);
}

void runTwoPointTool(State &state, const Point &point, const std::function<void(const Point &, const Point &)> &complete) {
    if (!state.pendingPoint.ok) {
        state.pendingPoint = point;
        return;
    }
    complete(state.pendingPoint, point);
    state.pendingPoint = {};
}

void handleClickCanvas(State &state, const QJsonObject &command) {
    const int storedGridStepPx = state.gridStepPx;
    if (command.contains("grid_step_px")) {
        state.gridStepPx = std::max(1, command.value("grid_step_px").toInt(state.gridStepPx));
    }
    const Point point = snapPoint(state, numberAt(command, "x"), numberAt(command, "y"));
    state.gridStepPx = storedGridStepPx;
    if (!point.ok) {
        state.errors.append("click_canvas has invalid coordinates");
        return;
    }

    struct ToolClickHandler {
        const char *toolId;
        std::function<void()> run;
    };
    const std::vector<ToolClickHandler> handlers = {
        {"line_polyline", [&]() {
             runTwoPointTool(state, point, [&](const Point &start, const Point &end) {
                 if (state.lineVariant == QStringLiteral("polyline")) {
                     QJsonArray points;
                     points.append(pointArray(start.x, start.y));
                     points.append(pointArray(end.x, end.y));
                     addPolyline(state, points);
                     return;
                 }
                 addLine(state, start, end);
             });
         }},
        {"anchor_points", [&]() {
             addPoint(state, point);
        }},
        {"circle_arc", [&]() {
             runTwoPointTool(state, point, [&](const Point &center, const Point &radiusPoint) {
                 const double radius = distancePx(center, radiusPoint);
                 if (state.circleArcMode == QStringLiteral("arc")) {
                     QJsonObject arcCommand;
                     arcCommand.insert("radius_px", radius);
                     arcCommand.insert("start_angle_deg", state.circleArcStartAngleDeg);
                     arcCommand.insert("end_angle_deg", state.circleArcEndAngleDeg);
                     addArc(state, center, arcCommand);
                     return;
                 }
                 QJsonObject circleCommand;
                 circleCommand.insert("radius_px", radius);
                 addCircle(state, center, circleCommand);
             });
         }},
        {"rectangle_polygon", [&]() {
             runTwoPointTool(state, point, [&](const Point &start, const Point &end) {
                 addRectangle(state, start, end);
             });
         }},
        {"regular_polygon", [&]() {
             runTwoPointTool(state, point, [&](const Point &center, const Point &radiusPoint) {
                 QJsonObject polygonCommand;
                 polygonCommand.insert("radius_px", distancePx(center, radiusPoint));
                 polygonCommand.insert("sides", state.regularPolygonSides);
                 polygonCommand.insert("rotation_deg", state.regularPolygonRotationDeg);
                 addPolygon(state, center, polygonCommand);
             });
         }},
        {"image_reference_frame", [&]() {
             runTwoPointTool(state, point, [&](const Point &start, const Point &end) {
                 addRectangleObject(
                     state,
                     start,
                     end,
                     QStringLiteral("image_reference_frame"),
                     QStringLiteral("image reference frame"),
                     QStringLiteral("ASCII source image placement frame"));
             });
         }},
        {"ascii_crop_frame", [&]() {
             runTwoPointTool(state, point, [&](const Point &start, const Point &end) {
                 addRectangleObject(
                     state,
                     start,
                     end,
                     QStringLiteral("ascii_crop_frame"),
                     QStringLiteral("ASCII crop frame"),
                     QStringLiteral("ASCII workbench output crop frame"));
             });
         }},
        {"ascii_cell_region", [&]() {
             runTwoPointTool(state, point, [&](const Point &start, const Point &end) {
                 addRectangleObject(
                     state,
                     start,
                     end,
                     QStringLiteral("ascii_cell_region"),
                     QStringLiteral("ASCII cell region"),
                     QStringLiteral("Character-cell planning region"));
             });
         }},
        {"tone_probe", [&]() {
             addPointObject(
                 state,
                 point,
                 QStringLiteral("tone_probe"),
                 QStringLiteral("tone probe"),
                 QStringLiteral("Future luminance and glyph-density sample point"));
         }},
        {"glyph_baseline", [&]() {
             runTwoPointTool(state, point, [&](const Point &start, const Point &end) {
                 addLineObject(
                     state,
                     start,
                     end,
                     QStringLiteral("glyph_baseline"),
                     QStringLiteral("glyph baseline"),
                     QStringLiteral("Text/glyph flow baseline guide"));
             });
        }},
        {"select_move", [&]() {
             selectObject(state, QString());
         }},
    };

    const auto handler = std::find_if(handlers.begin(), handlers.end(), [&](const ToolClickHandler &entry) {
        return state.selectedTool == QString::fromLatin1(entry.toolId);
    });
    if (handler == handlers.end()) {
        state.errors.append("click_canvas unsupported for tool " + state.selectedTool);
        return;
    }
    handler->run();
}

bool runCommand(State &state, const QJsonObject &command) {
    struct CommandHandler {
        const char *commandName;
        std::function<void()> run;
    };
    const std::vector<CommandHandler> handlers = {
        {"select_tool", [&]() {
             const QString nextTool = stringAt(command, "tool", state.selectedTool);
             if (nextTool != state.selectedTool) {
                 state.pendingPoint = {};
             }
             state.selectedTool = nextTool;
        }},
        {"select_object", [&]() {
             selectObject(state, stringAt(command, "object_id", state.selectedObject));
         }},
        {"select_objects", [&]() {
             selectObjects(state, stringListFromArray(arrayAt(command, "object_ids")));
         }},
        {"delete_object", [&]() {
             deleteObject(state, stringAt(command, "object_id", state.selectedObject));
         }},
        {"delete_objects", [&]() {
             deleteObjects(state, stringListFromArray(arrayAt(command, "object_ids")));
         }},
        {"duplicate_object", [&]() {
             const double dxN = commandDeltaFromCommand(state, command, QStringLiteral("dx"), 16.0 / state.canvasPx);
             const double dyN = commandDeltaFromCommand(state, command, QStringLiteral("dy"), 16.0 / state.canvasPx);
             duplicateObject(state, stringAt(command, "object_id", state.selectedObject), dxN, dyN);
         }},
        {"duplicate_objects", [&]() {
             const double dxN = commandDeltaFromCommand(state, command, QStringLiteral("dx"), 16.0 / state.canvasPx);
             const double dyN = commandDeltaFromCommand(state, command, QStringLiteral("dy"), 16.0 / state.canvasPx);
             duplicateObjects(state, stringListFromArray(arrayAt(command, "object_ids")), dxN, dyN);
         }},
        {"paste_object", [&]() {
             const double dxN = commandDeltaFromCommand(state, command, QStringLiteral("dx"), 16.0 / state.canvasPx);
             const double dyN = commandDeltaFromCommand(state, command, QStringLiteral("dy"), 16.0 / state.canvasPx);
             pasteObject(state, command.value("object").toObject(), dxN, dyN);
         }},
        {"paste_objects", [&]() {
             const double dxN = commandDeltaFromCommand(state, command, QStringLiteral("dx"), 16.0 / state.canvasPx);
             const double dyN = commandDeltaFromCommand(state, command, QStringLiteral("dy"), 16.0 / state.canvasPx);
             pasteObjects(state, arrayAt(command, "objects"), dxN, dyN);
         }},
        {"move_object", [&]() {
             const double dxN = commandDeltaFromCommand(state, command, QStringLiteral("dx"), 0.0);
             const double dyN = commandDeltaFromCommand(state, command, QStringLiteral("dy"), 0.0);
             moveObject(state, stringAt(command, "object_id", state.selectedObject), dxN, dyN);
         }},
        {"move_objects", [&]() {
             const double dxN = commandDeltaFromCommand(state, command, QStringLiteral("dx"), 0.0);
             const double dyN = commandDeltaFromCommand(state, command, QStringLiteral("dy"), 0.0);
             moveObjects(state, stringListFromArray(arrayAt(command, "object_ids")), dxN, dyN);
         }},
        {"update_object", [&]() {
             updateObjectField(
                 state,
                 stringAt(command, "object_id", state.selectedObject),
                 stringAt(command, "field"),
                 numberAt(command, "value"));
         }},
        {"set_object_metadata", [&]() {
             setObjectMetadataField(
                 state,
                 stringAt(command, "object_id", state.selectedObject),
                 stringAt(command, "field"),
                 command.value("value"));
         }},
        {"set_tool_parameter", [&]() {
             setToolParameter(state, stringAt(command, "parameter"), command.value("value"));
         }},
        {"cancel_pending", [&]() {
             state.pendingPoint = {};
         }},
        {"set_snap", [&]() {
             if (command.contains("grid")) {
                 state.gridSnap = command.value("grid").toBool(state.gridSnap);
             }
             if (command.contains("grid_step_px")) {
                 state.gridStepPx = std::max(1, command.value("grid_step_px").toInt(state.gridStepPx));
             }
         }},
        {"click_canvas", [&]() {
             handleClickCanvas(state, command);
         }},
        {"add_point", [&]() {
             addPoint(state, pointFromCommand(state, command));
         }},
        {"add_line", [&]() {
             addLine(state, pointFromArray(state, arrayAt(command, "from")), pointFromArray(state, arrayAt(command, "to")));
         }},
        {"add_polyline", [&]() {
             addPolyline(state, arrayAt(command, "points"));
         }},
        {"add_circle", [&]() {
             addCircle(state, pointFromCommand(state, command), command);
         }},
        {"add_arc", [&]() {
             addArc(state, pointFromCommand(state, command), command);
         }},
        {"add_rectangle", [&]() {
             addRectangle(state, pointFromArray(state, arrayAt(command, "from")), pointFromArray(state, arrayAt(command, "to")));
         }},
        {"add_polygon", [&]() {
             addPolygon(state, pointFromCommand(state, command), command);
         }},
    };
    const QString name = stringAt(command, "cmd");
    const auto handler = std::find_if(handlers.begin(), handlers.end(), [&](const CommandHandler &entry) {
        return name == QString::fromLatin1(entry.commandName);
    });
    if (handler == handlers.end()) {
        return false;
    }
    handler->run();
    return true;
}

} // namespace drawing_core
