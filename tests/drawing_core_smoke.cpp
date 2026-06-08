#include "core/DrawingCore.h"
#include "core/DrawingObjectModel.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QTextStream>

#include <cmath>

namespace {

QJsonArray point(double x, double y) {
    QJsonArray result;
    result.append(x);
    result.append(y);
    return result;
}

QJsonObject command(const QString &name) {
    QJsonObject result;
    result.insert(QStringLiteral("cmd"), name);
    return result;
}

QJsonObject selectTool(const QString &toolId) {
    QJsonObject result = command(QStringLiteral("select_tool"));
    result.insert(QStringLiteral("tool"), toolId);
    return result;
}

QJsonObject setToolParameter(const QString &parameter, const QJsonValue &value) {
    QJsonObject result = command(QStringLiteral("set_tool_parameter"));
    result.insert(QStringLiteral("parameter"), parameter);
    result.insert(QStringLiteral("value"), value);
    return result;
}

QJsonObject click(double x, double y) {
    QJsonObject result = command(QStringLiteral("click_canvas"));
    result.insert(QStringLiteral("x"), x);
    result.insert(QStringLiteral("y"), y);
    return result;
}

QJsonObject clickWithSnapStep(double x, double y, int gridStepPx) {
    QJsonObject result = click(x, y);
    result.insert(QStringLiteral("grid_step_px"), gridStepPx);
    return result;
}

QJsonObject script(QJsonArray commands) {
    QJsonObject result;
    result.insert(QStringLiteral("script_id"), QStringLiteral("drawing_core_visible_tools_smoke"));
    result.insert(QStringLiteral("canvas_px"), point(512, 512));
    result.insert(QStringLiteral("commands"), commands);
    result.insert(QStringLiteral("allow_pending"), false);
    return result;
}

bool expect(bool condition, const QString &message) {
    if (!condition) {
        QTextStream(stderr) << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

QJsonObject firstObjectOfKind(const QJsonArray &objects, const QString &kind) {
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        if (object.value(QStringLiteral("kind")).toString() == kind) {
            return object;
        }
    }
    return {};
}

QJsonObject objectById(const QJsonArray &objects, const QString &id) {
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        if (object.value(QStringLiteral("id")).toString() == id) {
            return object;
        }
    }
    return {};
}

int kindCount(const QJsonArray &objects, const QString &kind) {
    int count = 0;
    for (const QJsonValue value : objects) {
        if (value.toObject().value(QStringLiteral("kind")).toString() == kind) {
            ++count;
        }
    }
    return count;
}

int commandCount(const QJsonArray &commands, const QString &name) {
    int count = 0;
    for (const QJsonValue value : commands) {
        if (value.toObject().value(QStringLiteral("cmd")).toString() == name) {
            ++count;
        }
    }
    return count;
}

bool expectKindCount(const QJsonArray &objects, const QString &kind, int expected) {
    return expect(kindCount(objects, kind) == expected,
                  QStringLiteral("expected %1 %2 object(s), got %3")
                      .arg(expected)
                      .arg(kind)
                      .arg(kindCount(objects, kind)));
}

bool expectNear(double actual, double expected, const QString &message) {
    return expect(std::abs(actual - expected) < 0.0001,
                  QStringLiteral("%1; expected %2, got %3").arg(message).arg(expected).arg(actual));
}

bool finiteNumber(const QJsonObject &object, const QString &field) {
    return object.contains(field) && std::isfinite(object.value(field).toDouble());
}

drawing_core::Bounds2D expectedBoundsForObject(const QJsonObject &object) {
    using namespace drawing_core;

    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        return computeBounds(pointGeometryFromObject(object));
    }
    if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        return computeBounds(lineGeometryFromObject(object));
    }
    if (kind == QStringLiteral("circle")) {
        return computeBounds(circleGeometryFromObject(object));
    }
    if (kind == QStringLiteral("arc")) {
        return computeBounds(arcGeometryFromObject(object));
    }
    if (kind == QStringLiteral("rectangle")
        || kind == QStringLiteral("image_reference_frame")
        || kind == QStringLiteral("ascii_crop_frame")
        || kind == QStringLiteral("ascii_cell_region")) {
        return computeBounds(rectangleGeometryFromObject(object));
    }
    if (kind == QStringLiteral("polyline")) {
        return computeBounds(polylineGeometryFromObject(object));
    }
    if (kind == QStringLiteral("polygon")) {
        return computeBounds(polygonGeometryFromObject(object));
    }
    return {};
}

bool expectObjectInvariants(const QJsonObject &object, int canvasPx) {
    const QString id = object.value(QStringLiteral("id")).toString();
    const QString kind = object.value(QStringLiteral("kind")).toString();
    bool ok = true;
    ok &= expect(!id.isEmpty(), QStringLiteral("generated object should have an id"));
    ok &= expect(id.startsWith(QStringLiteral("script_")), QStringLiteral("%1 should use a generated script id").arg(id));
    ok &= expect(!kind.isEmpty(), QStringLiteral("%1 should have a kind").arg(id));
    ok &= expect(object.contains(QStringLiteral("geometry")), QStringLiteral("%1 should contain typed geometry").arg(id));
    ok &= expect(object.contains(QStringLiteral("bounds")), QStringLiteral("%1 should contain derived bounds").arg(id));
    ok &= expect(object.value(QStringLiteral("metadata")).toObject().contains(QStringLiteral("created_by")),
                 QStringLiteral("%1 should preserve creation metadata").arg(id));

    const QJsonObject bounds = object.value(QStringLiteral("bounds")).toObject();
    ok &= expect(finiteNumber(bounds, QStringLiteral("x"))
                     && finiteNumber(bounds, QStringLiteral("y"))
                     && finiteNumber(bounds, QStringLiteral("w"))
                     && finiteNumber(bounds, QStringLiteral("h")),
                 QStringLiteral("%1 bounds should contain finite x/y/w/h").arg(id));
    ok &= expect(bounds.value(QStringLiteral("w")).toDouble() >= 0.0
                     && bounds.value(QStringLiteral("h")).toDouble() >= 0.0,
                 QStringLiteral("%1 bounds should not be negative").arg(id));

    const drawing_core::Bounds2D expected = expectedBoundsForObject(object);
    ok &= expect(expected.ok, QStringLiteral("%1 should have computable bounds for kind %2").arg(id, kind));
    if (expected.ok) {
        ok &= expectNear(bounds.value(QStringLiteral("x")).toDouble(), expected.x * canvasPx,
                         QStringLiteral("%1 bounds.x should derive from geometry").arg(id));
        ok &= expectNear(bounds.value(QStringLiteral("y")).toDouble(), expected.y * canvasPx,
                         QStringLiteral("%1 bounds.y should derive from geometry").arg(id));
        ok &= expectNear(bounds.value(QStringLiteral("w")).toDouble(), expected.w * canvasPx,
                         QStringLiteral("%1 bounds.w should derive from geometry").arg(id));
        ok &= expectNear(bounds.value(QStringLiteral("h")).toDouble(), expected.h * canvasPx,
                         QStringLiteral("%1 bounds.h should derive from geometry").arg(id));
    }
    return ok;
}

bool expectDrawingModelInvariants(const QJsonObject &model) {
    const QJsonArray canvas = model.value(QStringLiteral("canvas_px")).toArray();
    const int canvasPx = canvas.size() >= 1 ? canvas.at(0).toInt(512) : 512;
    const QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    QSet<QString> objectIds;
    bool ok = true;

    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        ok &= expect(!objectIds.contains(id), QStringLiteral("%1 should be unique in generated_objects").arg(id));
        objectIds.insert(id);
        ok &= expectObjectInvariants(object, canvasPx);
    }

    const QString selectedId = model.value(QStringLiteral("selected_object_id")).toString();
    ok &= expect(selectedId.isEmpty() || objectIds.contains(selectedId),
                 QStringLiteral("selected_object_id should be empty or reference an existing object"));

    QSet<QString> selectedIds;
    const QJsonArray selectedArray = model.value(QStringLiteral("selected_object_ids")).toArray();
    for (const QJsonValue value : selectedArray) {
        const QString id = value.toString();
        ok &= expect(!id.isEmpty(), QStringLiteral("selected_object_ids should not contain empty ids"));
        ok &= expect(objectIds.contains(id), QStringLiteral("selected object %1 should exist").arg(id));
        ok &= expect(!selectedIds.contains(id), QStringLiteral("selected object %1 should not be duplicated").arg(id));
        selectedIds.insert(id);
    }
    if (!selectedArray.isEmpty()) {
        ok &= expect(selectedId == selectedArray.last().toString(),
                     QStringLiteral("selected_object_id should match the last selected_object_ids entry"));
    }
    return ok;
}

QJsonObject checkedControllerModel(const DrawingDocumentController &controller, bool &ok) {
    QJsonObject model = QJsonObject::fromVariantMap(controller.modelDocument());
    ok &= expectDrawingModelInvariants(model);
    return model;
}

bool runVisibleToolCreationSmoke() {
    QJsonArray commands;
    commands.append(selectTool(QStringLiteral("anchor_points")));
    commands.append(click(64, 64));

    commands.append(selectTool(QStringLiteral("line_polyline")));
    commands.append(setToolParameter(QStringLiteral("line_variant"), QStringLiteral("straight")));
    commands.append(click(96, 64));
    commands.append(click(192, 64));

    commands.append(setToolParameter(QStringLiteral("line_variant"), QStringLiteral("polyline")));
    commands.append(click(96, 96));
    commands.append(click(192, 128));

    commands.append(selectTool(QStringLiteral("circle_arc")));
    commands.append(setToolParameter(QStringLiteral("circle_arc_mode"), QStringLiteral("circle")));
    commands.append(click(256, 128));
    commands.append(click(320, 128));

    commands.append(setToolParameter(QStringLiteral("circle_arc_mode"), QStringLiteral("arc")));
    commands.append(setToolParameter(QStringLiteral("circle_arc_start_angle_deg"), 15));
    commands.append(setToolParameter(QStringLiteral("circle_arc_end_angle_deg"), 120));
    commands.append(click(256, 192));
    commands.append(click(320, 192));

    commands.append(selectTool(QStringLiteral("rectangle_polygon")));
    commands.append(setToolParameter(QStringLiteral("stroke_color"), QStringLiteral("#abcdef")));
    commands.append(setToolParameter(QStringLiteral("fill_color"), QStringLiteral("#123456")));
    commands.append(setToolParameter(QStringLiteral("line_thickness"), 4));
    commands.append(setToolParameter(QStringLiteral("line_style"), QStringLiteral("dashed")));
    commands.append(setToolParameter(QStringLiteral("stroke_opacity"), 0.5));
    commands.append(click(64, 224));
    commands.append(click(160, 288));

    commands.append(selectTool(QStringLiteral("regular_polygon")));
    commands.append(setToolParameter(QStringLiteral("regular_polygon_sides"), 5));
    commands.append(setToolParameter(QStringLiteral("regular_polygon_rotation_deg"), 18));
    commands.append(click(256, 256));
    commands.append(click(320, 256));

    commands.append(selectTool(QStringLiteral("image_reference_frame")));
    commands.append(click(64, 320));
    commands.append(click(160, 384));

    commands.append(selectTool(QStringLiteral("ascii_crop_frame")));
    commands.append(click(192, 320));
    commands.append(click(320, 416));

    const DrawingCoreResult result = DrawingCore::runScript(script(commands));
    const QJsonObject model = result.model;
    bool ok = true;
    ok &= expectDrawingModelInvariants(model);
    ok &= expect(model.value(QStringLiteral("script_status")).toString() == QStringLiteral("pass"),
                 QStringLiteral("visible tool script should pass"));

    const QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    ok &= expectKindCount(objects, QStringLiteral("point"), 1);
    ok &= expectKindCount(objects, QStringLiteral("line"), 1);
    ok &= expectKindCount(objects, QStringLiteral("polyline"), 1);
    ok &= expectKindCount(objects, QStringLiteral("circle"), 1);
    ok &= expectKindCount(objects, QStringLiteral("arc"), 1);
    ok &= expectKindCount(objects, QStringLiteral("rectangle"), 1);
    ok &= expectKindCount(objects, QStringLiteral("polygon"), 1);
    ok &= expectKindCount(objects, QStringLiteral("image_reference_frame"), 1);
    ok &= expectKindCount(objects, QStringLiteral("ascii_crop_frame"), 1);
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        ok &= expect(object.contains(QStringLiteral("geometry")),
                     QStringLiteral("%1 should include typed geometry").arg(object.value(QStringLiteral("id")).toString()));
        ok &= expect(object.contains(QStringLiteral("bounds")),
                     QStringLiteral("%1 should include derived bounds").arg(object.value(QStringLiteral("id")).toString()));
        ok &= expect(object.value(QStringLiteral("metadata")).toObject().contains(QStringLiteral("created_by")),
                     QStringLiteral("%1 should include creation metadata").arg(object.value(QStringLiteral("id")).toString()));
    }

    const QJsonObject pointObject = firstObjectOfKind(objects, QStringLiteral("point"));
    const QJsonObject pointGeometry = pointObject.value(QStringLiteral("geometry")).toObject();
    ok &= expectNear(pointGeometry.value(QStringLiteral("point")).toObject().value(QStringLiteral("x")).toDouble(), 64.0, QStringLiteral("point geometry x should be explicit in pixels"));
    ok &= expectNear(pointObject.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble(), 64.0, QStringLiteral("point bounds should derive from geometry"));
    ok &= expect(pointObject.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("PointTool"),
                 QStringLiteral("point metadata should record creation provenance"));

    const QJsonObject line = firstObjectOfKind(objects, QStringLiteral("line"));
    ok &= expect(line.value(QStringLiteral("line_variant")).toString() == QStringLiteral("straight"),
                 QStringLiteral("straight line should keep straight variant"));
    ok &= expect(line.value(QStringLiteral("style_id")).toString() == QStringLiteral("inline_active_stroke"),
                 QStringLiteral("line should carry an explicit style reference"));
    const QJsonObject lineGeometry = line.value(QStringLiteral("geometry")).toObject();
    const QJsonObject lineA = lineGeometry.value(QStringLiteral("a")).toObject();
    const QJsonObject lineB = lineGeometry.value(QStringLiteral("b")).toObject();
    ok &= expectNear(lineA.value(QStringLiteral("x")).toDouble(), 96.0, QStringLiteral("line geometry a.x should be explicit in pixels"));
    ok &= expectNear(lineA.value(QStringLiteral("y")).toDouble(), 64.0, QStringLiteral("line geometry a.y should be explicit in pixels"));
    ok &= expectNear(lineB.value(QStringLiteral("x")).toDouble(), 192.0, QStringLiteral("line geometry b.x should be explicit in pixels"));
    ok &= expectNear(lineB.value(QStringLiteral("y")).toDouble(), 64.0, QStringLiteral("line geometry b.y should be explicit in pixels"));
    const QJsonObject lineBounds = line.value(QStringLiteral("bounds")).toObject();
    ok &= expectNear(lineBounds.value(QStringLiteral("x")).toDouble(), 96.0, QStringLiteral("line bounds x should derive from geometry"));
    ok &= expectNear(lineBounds.value(QStringLiteral("y")).toDouble(), 64.0, QStringLiteral("line bounds y should derive from geometry"));
    ok &= expectNear(lineBounds.value(QStringLiteral("w")).toDouble(), 96.0, QStringLiteral("line bounds width should derive from geometry"));
    ok &= expectNear(lineBounds.value(QStringLiteral("h")).toDouble(), 0.0, QStringLiteral("line bounds height should derive from geometry"));
    ok &= expect(line.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("LineTool"),
                 QStringLiteral("line metadata should record creation provenance"));

    const QJsonObject polyline = firstObjectOfKind(objects, QStringLiteral("polyline"));
    ok &= expect(polyline.value(QStringLiteral("line_variant")).toString() == QStringLiteral("polyline"),
                 QStringLiteral("polyline variant should create polyline object"));
    ok &= expect(polyline.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("points")).toArray().size() == 2,
                 QStringLiteral("polyline geometry should carry explicit points"));
    ok &= expectNear(polyline.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("w")).toDouble(), 96.0,
                     QStringLiteral("polyline bounds should derive from point geometry"));
    ok &= expect(polyline.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("PolylineTool"),
                 QStringLiteral("polyline metadata should record creation provenance"));

    const QJsonObject circle = firstObjectOfKind(objects, QStringLiteral("circle"));
    ok &= expectNear(circle.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("radius")).toDouble(), 64.0,
                     QStringLiteral("circle geometry should carry explicit radius"));
    ok &= expectNear(circle.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("w")).toDouble(), 128.0,
                     QStringLiteral("circle bounds should derive from radius"));
    ok &= expect(circle.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("CircleTool"),
                 QStringLiteral("circle metadata should record creation provenance"));

    const QJsonObject arc = firstObjectOfKind(objects, QStringLiteral("arc"));
    ok &= expect(arc.value(QStringLiteral("start_angle_deg")).toDouble() == 15.0,
                 QStringLiteral("arc start angle should be stamped"));
    ok &= expect(arc.value(QStringLiteral("end_angle_deg")).toDouble() == 120.0,
                 QStringLiteral("arc end angle should be stamped"));
    ok &= expectNear(arc.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("radius")).toDouble(), 64.0,
                     QStringLiteral("arc geometry should carry explicit radius"));
    ok &= expect(arc.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("ArcTool"),
                 QStringLiteral("arc metadata should record creation provenance"));

    const QJsonObject rectangle = firstObjectOfKind(objects, QStringLiteral("rectangle"));
    ok &= expect(rectangle.value(QStringLiteral("stroke_color")).toString() == QStringLiteral("#abcdef"),
                 QStringLiteral("rectangle should stamp stroke color"));
    ok &= expect(rectangle.value(QStringLiteral("fill_color")).toString() == QStringLiteral("#123456"),
                 QStringLiteral("rectangle should stamp fill color"));
    ok &= expect(rectangle.value(QStringLiteral("line_style")).toString() == QStringLiteral("dashed"),
                 QStringLiteral("rectangle should stamp line style"));
    ok &= expect(rectangle.value(QStringLiteral("line_thickness")).toDouble() == 4.0,
                 QStringLiteral("rectangle should stamp line thickness"));
    ok &= expect(rectangle.value(QStringLiteral("stroke_opacity")).toDouble() == 0.5,
                 QStringLiteral("rectangle should stamp stroke opacity"));
    ok &= expectNear(rectangle.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("width")).toDouble(), 96.0,
                     QStringLiteral("rectangle geometry should carry explicit width"));
    ok &= expectNear(rectangle.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("h")).toDouble(), 64.0,
                     QStringLiteral("rectangle bounds should derive from geometry"));
    ok &= expect(rectangle.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("RectangleTool"),
                 QStringLiteral("rectangle metadata should record creation provenance"));

    ok &= expect(!result.svg.contains(QStringLiteral("fill=\"#25232d\"")),
                 QStringLiteral("svg export should not include the canvas background helper"));
    ok &= expect(result.svg.contains(QStringLiteral("id=\"script_rectangle_01\"")),
                 QStringLiteral("svg export should preserve object ids"));
    ok &= expect(result.svg.contains(QStringLiteral("stroke=\"#abcdef\"")),
                 QStringLiteral("svg export should include stamped stroke color"));
    ok &= expect(result.svg.contains(QStringLiteral("fill=\"#123456\"")),
                 QStringLiteral("svg export should include stamped fill color"));
    ok &= expect(result.svg.contains(QStringLiteral("stroke-width=\"4\"")),
                 QStringLiteral("svg export should include stamped stroke width"));
    ok &= expect(result.svg.contains(QStringLiteral("stroke-opacity=\"0.5\"")),
                 QStringLiteral("svg export should include stamped stroke opacity"));
    ok &= expect(result.svg.contains(QStringLiteral("stroke-dasharray=\"12.8 8.4\"")),
                 QStringLiteral("svg export should include stamped dashed line style"));

    const QJsonObject polygon = firstObjectOfKind(objects, QStringLiteral("polygon"));
    ok &= expect(polygon.value(QStringLiteral("sides")).toInt() == 5,
                 QStringLiteral("polygon should respect side count"));
    ok &= expect(polygon.value(QStringLiteral("points")).toArray().size() == 5,
                 QStringLiteral("polygon should generate one point per side"));
    ok &= expect(polygon.value(QStringLiteral("rotation_deg")).toDouble() == 18.0,
                 QStringLiteral("polygon should respect rotation"));
    ok &= expect(polygon.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("points")).toArray().size() == 5,
                 QStringLiteral("polygon geometry should carry explicit generated points"));
    ok &= expect(polygon.value(QStringLiteral("bounds")).toObject().contains(QStringLiteral("w")),
                 QStringLiteral("polygon bounds should derive from generated points"));
    ok &= expect(polygon.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("created_by")).toString() == QStringLiteral("PolygonTool"),
                 QStringLiteral("polygon metadata should record creation provenance"));

    return ok;
}

bool runSelectDeleteSmoke() {
    QJsonArray commands;
    commands.append(selectTool(QStringLiteral("line_polyline")));
    commands.append(click(64, 64));
    commands.append(click(128, 64));

    QJsonObject select = command(QStringLiteral("select_object"));
    select.insert(QStringLiteral("object_id"), QStringLiteral("script_line_01"));
    commands.append(select);

    QJsonObject erase = command(QStringLiteral("delete_object"));
    erase.insert(QStringLiteral("object_id"), QStringLiteral("script_line_01"));
    commands.append(erase);

    const QJsonObject model = DrawingCore::runScript(script(commands)).model;
    bool ok = true;
    ok &= expectDrawingModelInvariants(model);
    ok &= expect(model.value(QStringLiteral("script_status")).toString() == QStringLiteral("pass"),
                 QStringLiteral("select/delete script should pass"));
    ok &= expect(model.value(QStringLiteral("generated_objects")).toArray().isEmpty(),
                 QStringLiteral("delete should remove selected/generated line object"));
    return ok;
}

bool runClickSnapOverrideSmoke() {
    QJsonArray commands;
    commands.append(selectTool(QStringLiteral("anchor_points")));
    commands.append(clickWithSnapStep(20, 20, 8));

    const QJsonObject model = DrawingCore::runScript(script(commands)).model;
    const QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    const QJsonObject point = firstObjectOfKind(objects, QStringLiteral("point"));
    const QJsonArray pointPx = point.value(QStringLiteral("point_px")).toArray();
    const QJsonObject snap = model.value(QStringLiteral("snap")).toObject();

    bool ok = true;
    ok &= expectDrawingModelInvariants(model);
    ok &= expect(model.value(QStringLiteral("script_status")).toString() == QStringLiteral("pass"),
                 QStringLiteral("snap override script should pass"));
    ok &= expect(pointPx.size() >= 2 && pointPx.at(0).toDouble() == 24.0 && pointPx.at(1).toDouble() == 24.0,
                 QStringLiteral("click snap override should use the command grid step"));
    ok &= expect(snap.value(QStringLiteral("grid_step_px")).toInt() == 32,
                 QStringLiteral("click snap override should not mutate stored grid step"));
    return ok;
}

bool runControllerUndoRedoSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("line_polyline"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.125, 8);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    ok &= expect(controller.canUndo(), QStringLiteral("controller should allow undo after creating a line"));
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("line")) == 1,
                 QStringLiteral("controller should create one line before undo"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(!model.value(QStringLiteral("pending_point")).toObject().value(QStringLiteral("ok")).toBool(false),
                 QStringLiteral("undo should not leave a pending two-click point"));
    ok &= expect(model.value(QStringLiteral("generated_objects")).toArray().isEmpty(),
                 QStringLiteral("undo should remove the generated line"));
    ok &= expect(controller.canRedo(), QStringLiteral("controller should allow redo after undo"));

    controller.redo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("line")) == 1,
                 QStringLiteral("redo should restore the generated line"));

    controller.undo();
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);
    ok &= expect(!controller.canRedo(), QStringLiteral("new drawing action should clear redo"));
    return ok;
}

bool runControllerMoveCoalescingSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);

    controller.selectObject(QStringLiteral("script_point_01"));
    controller.beginMoveGesture();
    controller.moveObjectBy(QStringLiteral("script_point_01"), 0.125, 0.0);
    controller.moveObjectBy(QStringLiteral("script_point_01"), 0.125, 0.0);
    controller.endMoveGesture();

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("move_object")) == 1,
                 QStringLiteral("drag move should coalesce into one move command"));

    QJsonObject point = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point"));
    ok &= expect(point.value(QStringLiteral("x")).toDouble() == 0.5,
                 QStringLiteral("coalesced move should preserve total dx"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    point = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point"));
    ok &= expect(point.value(QStringLiteral("x")).toDouble() == 0.25,
                 QStringLiteral("undo should revert the entire drag move at once"));
    return ok;
}

bool runControllerLineStoreLifecycleSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("line_polyline"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.125, 8);

    const QString lineId = QStringLiteral("script_line_01");
    controller.selectObject(lineId);
    controller.updateSelectedObjectMetadataField(QStringLiteral("role"), QStringLiteral("guide"));
    controller.moveObjectBy(lineId, 0.125, 0.0);
    controller.updateObjectField(lineId, QStringLiteral("x2_px"), 320.0);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonObject line = objectById(model.value(QStringLiteral("generated_objects")).toArray(), lineId);
    QJsonObject lineGeometry = line.value(QStringLiteral("geometry")).toObject();
    QJsonObject lineBounds = line.value(QStringLiteral("bounds")).toObject();

    ok &= expect(line.value(QStringLiteral("role")).toString() == QStringLiteral("guide"),
                 QStringLiteral("store-backed line should preserve top-level metadata after projection"));
    ok &= expect(line.value(QStringLiteral("source")).toString() == QStringLiteral("cpp_drawing_core"),
                 QStringLiteral("store-backed line should preserve source attribute after projection"));
    ok &= expectNear(lineGeometry.value(QStringLiteral("a")).toObject().value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("store-backed line move should update typed endpoint a.x"));
    ok &= expectNear(lineGeometry.value(QStringLiteral("b")).toObject().value(QStringLiteral("x")).toDouble(), 320.0,
                     QStringLiteral("store-backed line endpoint edit should update typed endpoint b.x"));
    ok &= expectNear(lineBounds.value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("store-backed line bounds should derive from moved geometry"));
    ok &= expectNear(lineBounds.value(QStringLiteral("w")).toDouble(), 192.0,
                     QStringLiteral("store-backed line bounds should derive from edited endpoint"));
    ok &= expect(controller.exportSvg().contains(QStringLiteral("data-role=\"guide\"")),
                 QStringLiteral("store-backed line svg export should preserve metadata"));

    controller.deleteObject(lineId);
    model = checkedControllerModel(controller, ok);
    ok &= expect(objectById(model.value(QStringLiteral("generated_objects")).toArray(), lineId).isEmpty(),
                 QStringLiteral("store-backed line delete should remove JSON projection"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    line = objectById(model.value(QStringLiteral("generated_objects")).toArray(), lineId);
    lineGeometry = line.value(QStringLiteral("geometry")).toObject();
    ok &= expect(line.value(QStringLiteral("role")).toString() == QStringLiteral("guide"),
                 QStringLiteral("undo after line delete should rebuild store-backed metadata"));
    ok &= expectNear(lineGeometry.value(QStringLiteral("b")).toObject().value(QStringLiteral("x")).toDouble(), 320.0,
                     QStringLiteral("undo after line delete should rebuild typed endpoint edit"));
    return ok;
}

bool runControllerPointStoreLifecycleSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);

    const QString pointId = QStringLiteral("script_point_01");
    controller.selectObject(pointId);
    controller.updateSelectedObjectMetadataField(QStringLiteral("role"), QStringLiteral("anchor"));
    controller.moveObjectBy(pointId, 0.125, 0.0);
    controller.updateObjectField(pointId, QStringLiteral("y_px"), 192.0);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonObject point = objectById(model.value(QStringLiteral("generated_objects")).toArray(), pointId);
    QJsonObject pointGeometry = point.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("point")).toObject();
    QJsonObject pointBounds = point.value(QStringLiteral("bounds")).toObject();

    ok &= expect(point.value(QStringLiteral("role")).toString() == QStringLiteral("anchor"),
                 QStringLiteral("store-backed point should preserve top-level metadata after projection"));
    ok &= expect(point.value(QStringLiteral("source")).toString() == QStringLiteral("cpp_drawing_core"),
                 QStringLiteral("store-backed point should preserve source attribute after projection"));
    ok &= expectNear(pointGeometry.value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("store-backed point move should update typed x"));
    ok &= expectNear(pointGeometry.value(QStringLiteral("y")).toDouble(), 192.0,
                     QStringLiteral("store-backed point field edit should update typed y"));
    ok &= expectNear(pointBounds.value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("store-backed point bounds should derive from moved geometry"));
    ok &= expectNear(pointBounds.value(QStringLiteral("y")).toDouble(), 192.0,
                     QStringLiteral("store-backed point bounds should derive from edited geometry"));
    ok &= expect(controller.exportSvg().contains(QStringLiteral("data-role=\"anchor\"")),
                 QStringLiteral("store-backed point svg export should preserve metadata"));

    controller.deleteObject(pointId);
    model = checkedControllerModel(controller, ok);
    ok &= expect(objectById(model.value(QStringLiteral("generated_objects")).toArray(), pointId).isEmpty(),
                 QStringLiteral("store-backed point delete should remove JSON projection"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    point = objectById(model.value(QStringLiteral("generated_objects")).toArray(), pointId);
    pointGeometry = point.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("point")).toObject();
    ok &= expect(point.value(QStringLiteral("role")).toString() == QStringLiteral("anchor"),
                 QStringLiteral("undo after point delete should rebuild store-backed metadata"));
    ok &= expectNear(pointGeometry.value(QStringLiteral("y")).toDouble(), 192.0,
                     QStringLiteral("undo after point delete should rebuild typed point edit"));
    return ok;
}

bool runControllerCircleArcStoreLifecycleSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("circle_arc"));
    controller.setToolParameter(QStringLiteral("circle_arc_mode"), QStringLiteral("circle"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.375, 0.250, 8);

    const QString circleId = QStringLiteral("script_circle_01");
    controller.selectObject(circleId);
    controller.updateSelectedObjectMetadataField(QStringLiteral("role"), QStringLiteral("orbit"));
    controller.moveObjectBy(circleId, 0.125, 0.0);
    controller.updateObjectField(circleId, QStringLiteral("radius_px"), 96.0);

    controller.setToolParameter(QStringLiteral("circle_arc_mode"), QStringLiteral("arc"));
    controller.setToolParameter(QStringLiteral("circle_arc_start_angle_deg"), 30.0);
    controller.setToolParameter(QStringLiteral("circle_arc_end_angle_deg"), 120.0);
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.625, 0.500, 8);

    const QString arcId = QStringLiteral("script_arc_01");
    controller.selectObject(arcId);
    controller.updateSelectedObjectMetadataField(QStringLiteral("role"), QStringLiteral("curve"));
    controller.moveObjectBy(arcId, 0.0, 0.125);
    controller.updateObjectField(arcId, QStringLiteral("end_angle_deg"), 210.0);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonObject circle = objectById(model.value(QStringLiteral("generated_objects")).toArray(), circleId);
    QJsonObject circleGeometry = circle.value(QStringLiteral("geometry")).toObject();
    QJsonObject circleCenter = circleGeometry.value(QStringLiteral("center")).toObject();
    QJsonObject circleBounds = circle.value(QStringLiteral("bounds")).toObject();
    QJsonObject arc = objectById(model.value(QStringLiteral("generated_objects")).toArray(), arcId);
    QJsonObject arcGeometry = arc.value(QStringLiteral("geometry")).toObject();
    QJsonObject arcCenter = arcGeometry.value(QStringLiteral("center")).toObject();
    QJsonObject arcBounds = arc.value(QStringLiteral("bounds")).toObject();

    ok &= expect(circle.value(QStringLiteral("role")).toString() == QStringLiteral("orbit"),
                 QStringLiteral("store-backed circle should preserve top-level metadata after projection"));
    ok &= expectNear(circleCenter.value(QStringLiteral("x")).toDouble(), 192.0,
                     QStringLiteral("store-backed circle move should update typed center x"));
    ok &= expectNear(circleGeometry.value(QStringLiteral("radius")).toDouble(), 96.0,
                     QStringLiteral("store-backed circle radius edit should update typed radius"));
    ok &= expectNear(circleBounds.value(QStringLiteral("x")).toDouble(), 96.0,
                     QStringLiteral("store-backed circle bounds should derive from center and radius"));
    ok &= expectNear(circleBounds.value(QStringLiteral("w")).toDouble(), 192.0,
                     QStringLiteral("store-backed circle bounds should derive from diameter"));

    ok &= expect(arc.value(QStringLiteral("role")).toString() == QStringLiteral("curve"),
                 QStringLiteral("store-backed arc should preserve top-level metadata after projection"));
    ok &= expectNear(arcCenter.value(QStringLiteral("y")).toDouble(), 320.0,
                     QStringLiteral("store-backed arc move should update typed center y"));
    ok &= expectNear(arcGeometry.value(QStringLiteral("end_angle_deg")).toDouble(), 210.0,
                     QStringLiteral("store-backed arc angle edit should update typed end angle"));
    ok &= expectNear(arcBounds.value(QStringLiteral("y")).toDouble(), 256.0,
                     QStringLiteral("store-backed arc bounds should derive from moved center and radius"));
    ok &= expect(controller.exportSvg().contains(QStringLiteral("data-role=\"orbit\""))
                     && controller.exportSvg().contains(QStringLiteral("data-role=\"curve\"")),
                 QStringLiteral("store-backed circle/arc svg export should preserve metadata"));

    controller.deleteObject(arcId);
    model = checkedControllerModel(controller, ok);
    ok &= expect(objectById(model.value(QStringLiteral("generated_objects")).toArray(), arcId).isEmpty(),
                 QStringLiteral("store-backed arc delete should remove JSON projection"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    arc = objectById(model.value(QStringLiteral("generated_objects")).toArray(), arcId);
    arcGeometry = arc.value(QStringLiteral("geometry")).toObject();
    ok &= expect(arc.value(QStringLiteral("role")).toString() == QStringLiteral("curve"),
                 QStringLiteral("undo after arc delete should rebuild store-backed metadata"));
    ok &= expectNear(arcGeometry.value(QStringLiteral("end_angle_deg")).toDouble(), 210.0,
                     QStringLiteral("undo after arc delete should rebuild typed angle edit"));
    return ok;
}

bool runControllerObjectFieldUpdateSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("rectangle_polygon"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);

    controller.selectObject(QStringLiteral("script_rectangle_01"));
    controller.updateObjectField(QStringLiteral("script_rectangle_01"), QStringLiteral("width_px"), 192.0);
    controller.updateObjectField(QStringLiteral("script_rectangle_01"), QStringLiteral("x_px"), 96.0);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonObject rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    const QJsonArray editedRect = rectangle.value(QStringLiteral("rect_px")).toArray();

    ok &= expect(editedRect.size() == 4, QStringLiteral("edited rectangle should keep rect_px geometry"));
    ok &= expectNear(editedRect.at(0).toDouble(), 96.0, QStringLiteral("right-panel x edit should update rectangle x"));
    ok &= expectNear(editedRect.at(2).toDouble(), 192.0, QStringLiteral("right-panel width edit should update rectangle width"));
    ok &= expect(controller.canUndo(), QStringLiteral("object field edits should be undoable"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    const QJsonArray revertedRect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(revertedRect.at(0).toDouble(), 128.0, QStringLiteral("first undo should revert only the x edit"));
    ok &= expectNear(revertedRect.at(2).toDouble(), 192.0, QStringLiteral("first undo should keep the earlier width edit"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    const QJsonArray fullyRevertedRect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(fullyRevertedRect.at(0).toDouble(), 128.0, QStringLiteral("second undo should revert the x edit"));
    ok &= expectNear(fullyRevertedRect.at(2).toDouble(), 128.0, QStringLiteral("second undo should revert the width edit"));
    return ok;
}

bool runControllerObjectFieldGestureCoalescingSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("rectangle_polygon"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);

    controller.selectObject(QStringLiteral("script_rectangle_01"));
    controller.beginMoveGesture();
    controller.updateObjectField(QStringLiteral("script_rectangle_01"), QStringLiteral("width_px"), 160.0);
    controller.updateObjectField(QStringLiteral("script_rectangle_01"), QStringLiteral("height_px"), 176.0);
    controller.updateObjectField(QStringLiteral("script_rectangle_01"), QStringLiteral("width_px"), 192.0);
    controller.updateObjectField(QStringLiteral("script_rectangle_01"), QStringLiteral("height_px"), 208.0);
    controller.endMoveGesture();

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("update_object")) == 2,
                 QStringLiteral("handle drag should keep only the final update per field"));

    QJsonObject rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    QJsonArray rect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(rect.at(2).toDouble(), 192.0, QStringLiteral("gesture width should use final dragged value"));
    ok &= expectNear(rect.at(3).toDouble(), 208.0, QStringLiteral("gesture height should use final dragged value"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    rect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(rect.at(2).toDouble(), 128.0, QStringLiteral("one undo should revert handle-drag width"));
    ok &= expectNear(rect.at(3).toDouble(), 128.0, QStringLiteral("one undo should revert handle-drag height"));
    return ok;
}

bool runControllerObjectMetadataSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("rectangle_polygon"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);

    controller.selectObject(QStringLiteral("script_rectangle_01"));
    controller.updateSelectedObjectMetadataField(QStringLiteral("role"), QStringLiteral("wall"));
    controller.updateSelectedObjectMetadataField(QStringLiteral("material"), QStringLiteral("stone"));
    controller.updateSelectedObjectMetadataField(QStringLiteral("intent"), QStringLiteral("blocks movement"));
    controller.updateSelectedObjectMetadataField(QStringLiteral("export_group"), QStringLiteral("room_a"));
    controller.updateSelectedObjectMetadataField(QStringLiteral("tags"), QStringLiteral("wall, collision, wall"));

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonObject rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    const QJsonArray tags = rectangle.value(QStringLiteral("tags")).toArray();

    ok &= expect(rectangle.value(QStringLiteral("role")).toString() == QStringLiteral("wall"),
                 QStringLiteral("metadata editor should persist role"));
    ok &= expect(rectangle.value(QStringLiteral("material")).toString() == QStringLiteral("stone"),
                 QStringLiteral("metadata editor should persist material"));
    ok &= expect(rectangle.value(QStringLiteral("intent")).toString() == QStringLiteral("blocks movement"),
                 QStringLiteral("metadata editor should persist intent"));
    ok &= expect(rectangle.value(QStringLiteral("export_group")).toString() == QStringLiteral("room_a"),
                 QStringLiteral("metadata editor should persist export group"));
    ok &= expect(tags.size() == 2 && tags.at(0).toString() == QStringLiteral("wall") && tags.at(1).toString() == QStringLiteral("collision"),
                 QStringLiteral("metadata editor should normalize comma tags"));
    const QString svg = controller.exportSvg();
    ok &= expect(svg.contains(QStringLiteral("data-role=\"wall\""))
                     && svg.contains(QStringLiteral("data-material=\"stone\""))
                     && svg.contains(QStringLiteral("data-intent=\"blocks movement\""))
                     && svg.contains(QStringLiteral("data-export-group=\"room_a\""))
                     && svg.contains(QStringLiteral("data-tags=\"wall,collision\"")),
                 QStringLiteral("svg export should preserve selected object metadata as data attributes"));

    controller.updateSelectedObjectMetadataField(QStringLiteral("material"), QStringLiteral(""));
    model = checkedControllerModel(controller, ok);
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    ok &= expect(!rectangle.contains(QStringLiteral("material")),
                 QStringLiteral("clearing metadata should remove empty material"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    ok &= expect(rectangle.value(QStringLiteral("material")).toString() == QStringLiteral("stone"),
                 QStringLiteral("metadata clear should be undoable"));
    return ok;
}

bool runControllerDuplicateSelectedObjectSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("rectangle_polygon"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);

    controller.selectObject(QStringLiteral("script_rectangle_01"));
    controller.duplicateSelectedObject();

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    ok &= expect(kindCount(objects, QStringLiteral("rectangle")) == 2,
                 QStringLiteral("duplicate selected object should create a second rectangle"));
    ok &= expect(model.value(QStringLiteral("selected_object_id")).toString() == QStringLiteral("script_rectangle_02"),
                 QStringLiteral("duplicate should select the copied object"));
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("duplicate_object")) == 1,
                 QStringLiteral("duplicate should be recorded as one command"));

    QJsonObject duplicate;
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        if (object.value(QStringLiteral("id")).toString() == QStringLiteral("script_rectangle_02")) {
            duplicate = object;
            break;
        }
    }
    const QJsonArray rect = duplicate.value(QStringLiteral("rect_px")).toArray();
    ok &= expect(duplicate.value(QStringLiteral("duplicate_of")).toString() == QStringLiteral("script_rectangle_01"),
                 QStringLiteral("duplicate should record source object"));
    ok &= expectNear(rect.at(0).toDouble(), 144.0, QStringLiteral("duplicate should offset x by 16px"));
    ok &= expectNear(rect.at(1).toDouble(), 144.0, QStringLiteral("duplicate should offset y by 16px"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle")) == 1,
                 QStringLiteral("undo should remove duplicated rectangle"));
    ok &= expect(model.value(QStringLiteral("selected_object_id")).toString() == QStringLiteral("script_rectangle_01"),
                 QStringLiteral("undo should restore original selection"));
    return ok;
}

bool runControllerPasteObjectSnapshotSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("rectangle_polygon"));
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.500, 0.500, 8);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    const QJsonObject source = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    controller.deleteObject(QStringLiteral("script_rectangle_01"));
    controller.pasteObject(source.toVariantMap(), 16.0 / 512.0, 16.0 / 512.0);

    model = checkedControllerModel(controller, ok);
    QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    ok &= expect(kindCount(objects, QStringLiteral("rectangle")) == 1,
                 QStringLiteral("paste should restore a rectangle from copied snapshot after source deletion"));
    ok &= expect(model.value(QStringLiteral("selected_object_id")).toString() == QStringLiteral("script_rectangle_01"),
                 QStringLiteral("paste should select the pasted object"));
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("paste_object")) == 1,
                 QStringLiteral("paste should be recorded as one command"));

    const QJsonObject pasted = firstObjectOfKind(objects, QStringLiteral("rectangle"));
    const QJsonArray rect = pasted.value(QStringLiteral("rect_px")).toArray();
    ok &= expect(pasted.value(QStringLiteral("pasted_from")).toString() == QStringLiteral("script_rectangle_01"),
                 QStringLiteral("pasted object should retain source snapshot id"));
    ok &= expectNear(rect.at(0).toDouble(), 144.0, QStringLiteral("paste should offset x by requested amount"));
    ok &= expectNear(rect.at(1).toDouble(), 144.0, QStringLiteral("paste should offset y by requested amount"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(model.value(QStringLiteral("generated_objects")).toArray().isEmpty(),
                 QStringLiteral("undo should remove pasted object and restore deleted-source state"));
    return ok;
}

bool runControllerSelectObjectsSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.375, 0.375, 8);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    const QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    if (!expect(objects.size() == 3, QStringLiteral("multi-select setup should create three point objects"))) {
        return false;
    }
    const QString firstId = objects.at(0).toObject().value(QStringLiteral("id")).toString();
    const QString secondId = objects.at(1).toObject().value(QStringLiteral("id")).toString();
    const QString thirdId = objects.at(2).toObject().value(QStringLiteral("id")).toString();

    QVariantList ids;
    ids.append(firstId);
    ids.append(thirdId);
    controller.selectObjects(ids);

    model = checkedControllerModel(controller, ok);
    QJsonArray selectedIds = model.value(QStringLiteral("selected_object_ids")).toArray();
    ok &= expect(selectedIds.size() == 2,
                 QStringLiteral("multi-select should expose selected_object_ids"));
    ok &= expect(selectedIds.at(0).toString() == firstId,
                 QStringLiteral("multi-select should preserve selected id order"));
    ok &= expect(selectedIds.at(1).toString() == thirdId,
                 QStringLiteral("multi-select should preserve final selected id"));
    ok &= expect(model.value(QStringLiteral("selected_object_id")).toString() == thirdId,
                 QStringLiteral("multi-select should keep the last selected object as primary"));

    controller.selectObject(secondId);
    model = checkedControllerModel(controller, ok);
    selectedIds = model.value(QStringLiteral("selected_object_ids")).toArray();
    ok &= expect(selectedIds.size() == 1,
                 QStringLiteral("single select should collapse selected_object_ids to one item"));
    ok &= expect(selectedIds.at(0).toString() == secondId,
                 QStringLiteral("single select should expose its selected id"));

    controller.selectObjects({});
    model = checkedControllerModel(controller, ok);
    selectedIds = model.value(QStringLiteral("selected_object_ids")).toArray();
    ok &= expect(selectedIds.isEmpty(),
                 QStringLiteral("empty multi-select should clear selected_object_ids"));
    ok &= expect(model.value(QStringLiteral("selected_object_id")).toString().isEmpty(),
                 QStringLiteral("empty multi-select should clear primary selection"));
    return ok;
}

bool runControllerMultiSelectOperationsSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.375, 0.375, 8);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    QVariantList ids;
    ids.append(objects.at(0).toObject().value(QStringLiteral("id")).toString());
    ids.append(objects.at(2).toObject().value(QStringLiteral("id")).toString());
    controller.selectObjects(ids);
    controller.duplicateSelectedObject();

    model = checkedControllerModel(controller, ok);
    objects = model.value(QStringLiteral("generated_objects")).toArray();
    QJsonArray selectedIds = model.value(QStringLiteral("selected_object_ids")).toArray();
    ok &= expect(kindCount(objects, QStringLiteral("point")) == 5,
                 QStringLiteral("multi-duplicate should create one copy for each selected object"));
    ok &= expect(selectedIds.size() == 2,
                 QStringLiteral("multi-duplicate should select the duplicated set"));
    ok &= expect(model.value(QStringLiteral("selected_object_id")).toString() == selectedIds.at(1).toString(),
                 QStringLiteral("multi-duplicate should keep last duplicate as primary"));
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("duplicate_objects")) == 1,
                 QStringLiteral("multi-duplicate should record one batch command"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point")) == 3,
                 QStringLiteral("one undo should remove the duplicated set"));

    controller.selectObjects(ids);
    controller.deleteSelectedObject();
    model = checkedControllerModel(controller, ok);
    objects = model.value(QStringLiteral("generated_objects")).toArray();
    ok &= expect(kindCount(objects, QStringLiteral("point")) == 1,
                 QStringLiteral("multi-delete should remove all selected objects"));
    ok &= expect(model.value(QStringLiteral("selected_object_ids")).toArray().isEmpty(),
                 QStringLiteral("multi-delete should clear deleted selection"));
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("delete_objects")) == 1,
                 QStringLiteral("multi-delete should record one batch command"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point")) == 3,
                 QStringLiteral("one undo should restore the deleted set"));
    return ok;
}

bool runControllerPasteObjectsSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    QVariantList snapshots;
    snapshots.append(objects.at(0).toObject().toVariantMap());
    snapshots.append(objects.at(1).toObject().toVariantMap());
    controller.pasteObjects(snapshots, 16.0 / 512.0, 16.0 / 512.0);

    model = checkedControllerModel(controller, ok);
    objects = model.value(QStringLiteral("generated_objects")).toArray();
    QJsonArray selectedIds = model.value(QStringLiteral("selected_object_ids")).toArray();
    ok &= expect(kindCount(objects, QStringLiteral("point")) == 4,
                 QStringLiteral("multi-paste should create one pasted object per snapshot"));
    ok &= expect(selectedIds.size() == 2,
                 QStringLiteral("multi-paste should select the pasted set"));
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("paste_objects")) == 1,
                 QStringLiteral("multi-paste should record one batch command"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point")) == 2,
                 QStringLiteral("one undo should remove the pasted set"));
    return ok;
}

bool runControllerMoveSelectedObjectsSmoke() {
    DrawingDocumentController controller;
    controller.selectTool(QStringLiteral("anchor_points"));
    controller.clickCanvasNormalizedWithSnapStep(0.125, 0.125, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.250, 0.250, 8);
    controller.clickCanvasNormalizedWithSnapStep(0.375, 0.375, 8);

    bool ok = true;
    QJsonObject model = checkedControllerModel(controller, ok);
    QJsonArray objects = model.value(QStringLiteral("generated_objects")).toArray();
    const QString firstId = objects.at(0).toObject().value(QStringLiteral("id")).toString();
    const QString thirdId = objects.at(2).toObject().value(QStringLiteral("id")).toString();
    QVariantList ids;
    ids.append(firstId);
    ids.append(thirdId);
    controller.selectObjects(ids);
    controller.beginMoveGesture();
    controller.moveSelectedObjectBy(16.0 / 512.0, 24.0 / 512.0);
    controller.moveSelectedObjectBy(16.0 / 512.0, 8.0 / 512.0);
    controller.endMoveGesture();

    model = checkedControllerModel(controller, ok);
    objects = model.value(QStringLiteral("generated_objects")).toArray();
    QJsonObject first = objectById(objects, firstId);
    QJsonObject third = objectById(objects, thirdId);
    QJsonObject second = objectById(objects, objects.at(1).toObject().value(QStringLiteral("id")).toString());
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("move_objects")) == 1,
                 QStringLiteral("gesture multi-move should coalesce into one move_objects command"));
    ok &= expectNear(first.value(QStringLiteral("point_px")).toArray().at(0).toDouble(), 96.0,
                     QStringLiteral("first selected point should move by total x delta"));
    ok &= expectNear(first.value(QStringLiteral("point_px")).toArray().at(1).toDouble(), 96.0,
                     QStringLiteral("first selected point should move by total y delta"));
    ok &= expectNear(third.value(QStringLiteral("point_px")).toArray().at(0).toDouble(), 224.0,
                     QStringLiteral("third selected point should move by total x delta"));
    ok &= expectNear(third.value(QStringLiteral("point_px")).toArray().at(1).toDouble(), 224.0,
                     QStringLiteral("third selected point should move by total y delta"));
    ok &= expectNear(second.value(QStringLiteral("point_px")).toArray().at(0).toDouble(), 128.0,
                     QStringLiteral("unselected point should not move"));
    ok &= expect(model.value(QStringLiteral("selected_object_ids")).toArray().size() == 2,
                 QStringLiteral("multi-move should preserve selected object ids"));

    controller.undo();
    model = checkedControllerModel(controller, ok);
    objects = model.value(QStringLiteral("generated_objects")).toArray();
    first = objectById(objects, firstId);
    third = objectById(objects, thirdId);
    ok &= expectNear(first.value(QStringLiteral("point_px")).toArray().at(0).toDouble(), 64.0,
                     QStringLiteral("one undo should restore first selected point x"));
    ok &= expectNear(third.value(QStringLiteral("point_px")).toArray().at(0).toDouble(), 192.0,
                     QStringLiteral("one undo should restore third selected point x"));
    return ok;
}

bool runControllerModelRoundTripSmoke() {
    DrawingDocumentController source;
    source.selectTool(QStringLiteral("rectangle_polygon"));
    source.clickCanvasNormalized(0.25, 0.25);
    source.clickCanvasNormalized(0.5, 0.5);
    source.selectTool(QStringLiteral("circle_arc"));
    source.setToolParameter(QStringLiteral("stroke_color"), QStringLiteral("#57add1"));
    source.clickCanvasNormalized(0.75, 0.75);
    source.clickCanvasNormalized(0.875, 0.75);

    DrawingDocumentController restored;
    bool ok = true;
    const QJsonObject savedModel = checkedControllerModel(source, ok);
    ok &= expect(restored.loadModel(savedModel.toVariantMap()),
                 QStringLiteral("saved drawing model should load into a fresh controller"));

    const QJsonObject restoredModel = checkedControllerModel(restored, ok);
    const QJsonArray restoredObjects = restoredModel.value(QStringLiteral("generated_objects")).toArray();
    ok &= expect(restoredModel.value(QStringLiteral("export_kind")).toString() == QStringLiteral("pattern_lab_2d_native_model_v0"),
                 QStringLiteral("round-tripped drawing should keep the native drawing kind"));
    ok &= expectKindCount(restoredObjects, QStringLiteral("rectangle"), 1);
    ok &= expectKindCount(restoredObjects, QStringLiteral("circle"), 1);
    ok &= expect(restoredModel.value(QStringLiteral("selected_tool_id")).toString() == QStringLiteral("circle_arc"),
                 QStringLiteral("round-tripped drawing should keep selected tool"));
    ok &= expect(restored.exportJson().contains(QStringLiteral("\"generated_objects\"")),
                 QStringLiteral("round-tripped drawing should remain exportable as JSON"));
    ok &= expect(restored.exportSvg().contains(QStringLiteral("<svg")),
                 QStringLiteral("round-tripped drawing should remain exportable as SVG"));
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    bool ok = true;
    ok &= runVisibleToolCreationSmoke();
    ok &= runSelectDeleteSmoke();
    ok &= runClickSnapOverrideSmoke();
    ok &= runControllerUndoRedoSmoke();
    ok &= runControllerMoveCoalescingSmoke();
    ok &= runControllerLineStoreLifecycleSmoke();
    ok &= runControllerPointStoreLifecycleSmoke();
    ok &= runControllerCircleArcStoreLifecycleSmoke();
    ok &= runControllerObjectFieldUpdateSmoke();
    ok &= runControllerObjectFieldGestureCoalescingSmoke();
    ok &= runControllerObjectMetadataSmoke();
    ok &= runControllerDuplicateSelectedObjectSmoke();
    ok &= runControllerPasteObjectSnapshotSmoke();
    ok &= runControllerSelectObjectsSmoke();
    ok &= runControllerMultiSelectOperationsSmoke();
    ok &= runControllerPasteObjectsSmoke();
    ok &= runControllerMoveSelectedObjectsSmoke();
    ok &= runControllerModelRoundTripSmoke();
    return ok ? 0 : 1;
}
