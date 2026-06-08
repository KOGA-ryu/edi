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
    object.insert("style_id", QStringLiteral("inline_active_stroke"));
    object.insert("line_variant", state.lineVariant);
    object.insert("line_style", state.lineStyle);
    object.insert("line_thickness", state.lineThickness);
    object.insert("stroke_opacity", state.strokeOpacity);
    object.insert("stroke_color", state.strokeColor);
    object.insert("fill_color", state.fillColor);
}

void applyCreationMetadata(QJsonObject &object, const QString &createdBy) {
    QJsonObject metadata;
    metadata.insert(QStringLiteral("created_by"), createdBy);
    metadata.insert(QStringLiteral("version"), 1);
    object.insert(QStringLiteral("metadata"), metadata);
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
    const QString objectId = nextId(state, kind);
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), label);
    attributes.insert(QStringLiteral("kind"), kind);
    attributes.insert(QStringLiteral("detail"), detail);
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Point;
    object.geometry = PointGeometry{{point.nx, point.ny}};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(
        QStringLiteral("created_by"),
        kind == QStringLiteral("tone_probe") ? QStringLiteral("ToneProbeTool") : QStringLiteral("PointTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("point command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
}

void addPoint(State &state, const Point &point) {
    addPointObject(state, point, QStringLiteral("point"), QStringLiteral("script point"), QStringLiteral("C++ generated point"));
}

void addLineObject(State &state, const Point &start, const Point &end, const QString &kind, const QString &label, const QString &detail) {
    if (!start.ok || !end.ok) {
        state.errors.append("line command has invalid endpoints");
        return;
    }
    const QString objectId = nextId(state, kind);
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), label);
    attributes.insert(QStringLiteral("kind"), kind);
    attributes.insert(QStringLiteral("detail"), detail);
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Line;
    object.geometry = LineGeometry{{start.nx, start.ny}, {end.nx, end.ny}};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(
        QStringLiteral("created_by"),
        kind == QStringLiteral("glyph_baseline") ? QStringLiteral("GlyphBaselineTool") : QStringLiteral("LineTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("line command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
}

void addLine(State &state, const Point &start, const Point &end) {
    addLineObject(state, start, end, QStringLiteral("line"), QStringLiteral("script line"), QStringLiteral("C++ generated line"));
}

void addPolyline(State &state, const QJsonArray &rawPoints) {
    if (rawPoints.size() < 2) {
        state.errors.append("polyline requires at least two points");
        return;
    }
    std::vector<Point2D> points;
    for (const QJsonValue value : rawPoints) {
        const Point point = pointFromArray(state, value.toArray());
        if (!point.ok) {
            state.errors.append("polyline contains invalid point");
            return;
        }
        points.push_back({point.nx, point.ny});
    }
    const QString objectId = nextId(state, QStringLiteral("polyline"));
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), QStringLiteral("script polyline"));
    attributes.insert(QStringLiteral("kind"), QStringLiteral("polyline"));
    attributes.insert(QStringLiteral("detail"), QStringLiteral("C++ generated polyline"));
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Polyline;
    object.geometry = PolylineGeometry{points};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("PolylineTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("polyline command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
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
    const QString objectId = nextId(state, QStringLiteral("circle"));
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), QStringLiteral("script circle"));
    attributes.insert(QStringLiteral("kind"), QStringLiteral("circle"));
    attributes.insert(QStringLiteral("detail"), QStringLiteral("C++ generated circle"));
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Circle;
    object.geometry = CircleGeometry{{center.nx, center.ny}, radiusPx / state.canvasPx};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("CircleTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("circle command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
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
    const QString objectId = nextId(state, QStringLiteral("arc"));
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), QStringLiteral("script arc"));
    attributes.insert(QStringLiteral("kind"), QStringLiteral("arc"));
    attributes.insert(QStringLiteral("detail"), QStringLiteral("C++ generated arc"));
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Circle;
    object.geometry = ArcGeometry{{center.nx, center.ny}, radiusPx / state.canvasPx, numberAt(command, "start_angle_deg"), numberAt(command, "end_angle_deg", 90.0)};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("ArcTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("arc command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
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
    const QString objectId = nextId(state, kind);
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), label);
    attributes.insert(QStringLiteral("kind"), kind);
    attributes.insert(QStringLiteral("detail"), detail);
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Rectangle;
    object.geometry = RectangleGeometry{{left / state.canvasPx, top / state.canvasPx}, width / state.canvasPx, height / state.canvasPx, 0.0};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(
        QStringLiteral("created_by"),
        kind == QStringLiteral("image_reference_frame") ? QStringLiteral("ImageReferenceFrameTool")
            : kind == QStringLiteral("ascii_crop_frame") ? QStringLiteral("AsciiCropFrameTool")
            : kind == QStringLiteral("ascii_cell_region") ? QStringLiteral("AsciiCellRegionTool")
                                                         : QStringLiteral("RectangleTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("rectangle command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
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
    std::vector<Point2D> points;
    for (int index = 0; index < sides; ++index) {
        const double angle = degreesToRadians(rotationDeg + 360.0 * index / sides);
        const double px = center.x + std::cos(angle) * radiusPx;
        const double py = center.y + std::sin(angle) * radiusPx;
        points.push_back({px / state.canvasPx, py / state.canvasPx});
    }
    const QString objectId = nextId(state, QStringLiteral("polygon"));
    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), QStringLiteral("script polygon"));
    attributes.insert(QStringLiteral("kind"), QStringLiteral("polygon"));
    attributes.insert(QStringLiteral("detail"), QStringLiteral("C++ generated regular polygon"));
    applyActiveStyle(state, attributes);

    DrawingObject object;
    object.id = {objectId};
    object.kind = ShapeKind::Polygon;
    object.geometry = PolygonGeometry{{center.nx, center.ny}, radiusPx / state.canvasPx, sides, rotationDeg, points};
    object.style = {QStringLiteral("inline_active_stroke")};
    object.layer = {QString::fromLatin1(kScriptLayer)};
    object.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("PolygonTool"));
    object.metadata.values.insert(QStringLiteral("version"), 1);
    object.attributes = attributes;

    if (!state.store.addObject(object)) {
        state.errors.append(QStringLiteral("polygon command could not add typed object: ") + objectId);
        return;
    }
    pushObject(state, state.store.serializeObject({objectId}, state.canvasPx));
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
