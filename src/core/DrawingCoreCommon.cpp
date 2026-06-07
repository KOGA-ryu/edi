#include "DrawingCoreInternal.h"

namespace drawing_core {

double numberAt(const QJsonObject &object, const QString &key, double fallback) {
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toDouble() : fallback;
}

QString stringAt(const QJsonObject &object, const QString &key, const QString &fallback) {
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

bool pendingPointActive(const QJsonObject &model) {
    const QJsonObject pending = model.value(QStringLiteral("pending_point")).toObject();
    return pending.value(QStringLiteral("ok")).toBool(false);
}

int generatedObjectCount(const QJsonObject &model) {
    return model.value(QStringLiteral("generated_objects")).toArray().size();
}

bool undoableInteractiveCommand(const QJsonObject &command) {
    const QString name = stringAt(command, QStringLiteral("cmd"));
    return name == QStringLiteral("click_canvas")
        || name == QStringLiteral("delete_object")
        || name == QStringLiteral("delete_objects")
        || name == QStringLiteral("duplicate_object")
        || name == QStringLiteral("duplicate_objects")
        || name == QStringLiteral("paste_object")
        || name == QStringLiteral("paste_objects")
        || name == QStringLiteral("move_object")
        || name == QStringLiteral("move_objects")
        || name == QStringLiteral("update_object")
        || name == QStringLiteral("set_object_metadata")
        || name == QStringLiteral("set_tool_parameter")
        || name == QStringLiteral("add_point")
        || name == QStringLiteral("add_line")
        || name == QStringLiteral("add_polyline")
        || name == QStringLiteral("add_circle")
        || name == QStringLiteral("add_arc")
        || name == QStringLiteral("add_rectangle")
        || name == QStringLiteral("add_polygon");
}

QJsonArray arrayAt(const QJsonObject &object, const QString &key) {
    const QJsonValue value = object.value(key);
    return value.isArray() ? value.toArray() : QJsonArray();
}

QJsonArray stringListToJsonArray(const QStringList &values) {
    QJsonArray array;
    for (const QString &value : values) {
        if (!value.isEmpty()) {
            array.append(value);
        }
    }
    return array;
}

QStringList stringListFromArray(const QJsonArray &values) {
    QStringList result;
    for (const QJsonValue value : values) {
        const QString text = value.toString();
        if (!text.isEmpty() && !result.contains(text)) {
            result.append(text);
        }
    }
    return result;
}

} // namespace drawing_core
