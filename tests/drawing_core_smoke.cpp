#include "core/DrawingCore.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
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

    const QJsonObject line = firstObjectOfKind(objects, QStringLiteral("line"));
    ok &= expect(line.value(QStringLiteral("line_variant")).toString() == QStringLiteral("straight"),
                 QStringLiteral("straight line should keep straight variant"));

    const QJsonObject polyline = firstObjectOfKind(objects, QStringLiteral("polyline"));
    ok &= expect(polyline.value(QStringLiteral("line_variant")).toString() == QStringLiteral("polyline"),
                 QStringLiteral("polyline variant should create polyline object"));

    const QJsonObject arc = firstObjectOfKind(objects, QStringLiteral("arc"));
    ok &= expect(arc.value(QStringLiteral("start_angle_deg")).toDouble() == 15.0,
                 QStringLiteral("arc start angle should be stamped"));
    ok &= expect(arc.value(QStringLiteral("end_angle_deg")).toDouble() == 120.0,
                 QStringLiteral("arc end angle should be stamped"));

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

    const QJsonObject polygon = firstObjectOfKind(objects, QStringLiteral("polygon"));
    ok &= expect(polygon.value(QStringLiteral("sides")).toInt() == 5,
                 QStringLiteral("polygon should respect side count"));
    ok &= expect(polygon.value(QStringLiteral("points")).toArray().size() == 5,
                 QStringLiteral("polygon should generate one point per side"));
    ok &= expect(polygon.value(QStringLiteral("rotation_deg")).toDouble() == 18.0,
                 QStringLiteral("polygon should respect rotation"));

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

    QJsonObject model = QJsonObject::fromVariantMap(controller.modelDocument());
    bool ok = true;
    ok &= expect(controller.canUndo(), QStringLiteral("controller should allow undo after creating a line"));
    ok &= expect(kindCount(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("line")) == 1,
                 QStringLiteral("controller should create one line before undo"));

    controller.undo();
    model = QJsonObject::fromVariantMap(controller.modelDocument());
    ok &= expect(!model.value(QStringLiteral("pending_point")).toObject().value(QStringLiteral("ok")).toBool(false),
                 QStringLiteral("undo should not leave a pending two-click point"));
    ok &= expect(model.value(QStringLiteral("generated_objects")).toArray().isEmpty(),
                 QStringLiteral("undo should remove the generated line"));
    ok &= expect(controller.canRedo(), QStringLiteral("controller should allow redo after undo"));

    controller.redo();
    model = QJsonObject::fromVariantMap(controller.modelDocument());
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

    QJsonObject model = QJsonObject::fromVariantMap(controller.modelDocument());
    bool ok = true;
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("move_object")) == 1,
                 QStringLiteral("drag move should coalesce into one move command"));

    QJsonObject point = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point"));
    ok &= expect(point.value(QStringLiteral("x")).toDouble() == 0.5,
                 QStringLiteral("coalesced move should preserve total dx"));

    controller.undo();
    model = QJsonObject::fromVariantMap(controller.modelDocument());
    point = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("point"));
    ok &= expect(point.value(QStringLiteral("x")).toDouble() == 0.25,
                 QStringLiteral("undo should revert the entire drag move at once"));
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

    QJsonObject model = QJsonObject::fromVariantMap(controller.modelDocument());
    QJsonObject rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    const QJsonArray editedRect = rectangle.value(QStringLiteral("rect_px")).toArray();

    bool ok = true;
    ok &= expect(editedRect.size() == 4, QStringLiteral("edited rectangle should keep rect_px geometry"));
    ok &= expectNear(editedRect.at(0).toDouble(), 96.0, QStringLiteral("right-panel x edit should update rectangle x"));
    ok &= expectNear(editedRect.at(2).toDouble(), 192.0, QStringLiteral("right-panel width edit should update rectangle width"));
    ok &= expect(controller.canUndo(), QStringLiteral("object field edits should be undoable"));

    controller.undo();
    model = QJsonObject::fromVariantMap(controller.modelDocument());
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    const QJsonArray revertedRect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(revertedRect.at(0).toDouble(), 128.0, QStringLiteral("first undo should revert only the x edit"));
    ok &= expectNear(revertedRect.at(2).toDouble(), 192.0, QStringLiteral("first undo should keep the earlier width edit"));

    controller.undo();
    model = QJsonObject::fromVariantMap(controller.modelDocument());
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

    QJsonObject model = QJsonObject::fromVariantMap(controller.modelDocument());
    bool ok = true;
    ok &= expect(commandCount(model.value(QStringLiteral("command_log")).toArray(), QStringLiteral("update_object")) == 2,
                 QStringLiteral("handle drag should keep only the final update per field"));

    QJsonObject rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    QJsonArray rect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(rect.at(2).toDouble(), 192.0, QStringLiteral("gesture width should use final dragged value"));
    ok &= expectNear(rect.at(3).toDouble(), 208.0, QStringLiteral("gesture height should use final dragged value"));

    controller.undo();
    model = QJsonObject::fromVariantMap(controller.modelDocument());
    rectangle = firstObjectOfKind(model.value(QStringLiteral("generated_objects")).toArray(), QStringLiteral("rectangle"));
    rect = rectangle.value(QStringLiteral("rect_px")).toArray();
    ok &= expectNear(rect.at(2).toDouble(), 128.0, QStringLiteral("one undo should revert handle-drag width"));
    ok &= expectNear(rect.at(3).toDouble(), 128.0, QStringLiteral("one undo should revert handle-drag height"));
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
    ok &= runControllerObjectFieldUpdateSmoke();
    ok &= runControllerObjectFieldGestureCoalescingSmoke();
    return ok ? 0 : 1;
}
