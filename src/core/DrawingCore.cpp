#include "DrawingCore.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace {
constexpr int kDefaultCanvasPx = 512;
constexpr const char *kScriptLayer = "layer_09_script_geometry";

struct Point {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
    double nx = 0.0;
    double ny = 0.0;
};

struct State {
    QString scriptId = "unnamed_script";
    QString selectedTool = "anchor_points";
    QString selectedLayer = kScriptLayer;
    QString selectedObject;
    QStringList selectedObjects;
    bool gridSnap = true;
    int gridStepPx = 32;
    int canvasPx = kDefaultCanvasPx;
    QString circleArcMode = "circle";
    double circleArcStartAngleDeg = 0.0;
    double circleArcEndAngleDeg = 90.0;
    int regularPolygonSides = 6;
    double regularPolygonRotationDeg = 30.0;
    QString lineVariant = "straight";
    QString lineStyle = "solid";
    QString strokeColor = "#f4d46f";
    QString fillColor;
    double lineThickness = 2.0;
    double strokeOpacity = 1.0;
    Point pendingPoint;
    QJsonArray commandLog;
    QJsonArray generatedObjects;
    QJsonArray errors;
};

struct Bounds {
    bool ok = false;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
};

double numberAt(const QJsonObject &object, const QString &key, double fallback = 0.0) {
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toDouble() : fallback;
}

QString stringAt(const QJsonObject &object, const QString &key, const QString &fallback = QString()) {
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

QJsonArray pointArray(double x, double y) {
    QJsonArray array;
    array.append(x);
    array.append(y);
    return array;
}

QJsonArray translatedPointArray(const QJsonArray &point, double dx, double dy) {
    if (point.size() < 2) {
        return point;
    }
    return pointArray(point.at(0).toDouble() + dx, point.at(1).toDouble() + dy);
}

QJsonArray translatedPointList(const QJsonArray &points, double dx, double dy) {
    QJsonArray translated;
    for (const QJsonValue value : points) {
        translated.append(translatedPointArray(value.toArray(), dx, dy));
    }
    return translated;
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

bool generatedObjectExists(const State &state, const QString &objectId) {
    for (const QJsonValue value : state.generatedObjects) {
        if (value.toObject().value("id").toString() == objectId) {
            return true;
        }
    }
    return false;
}

void selectObject(State &state, const QString &objectId) {
    state.selectedObject = objectId;
    state.selectedObjects = objectId.isEmpty() ? QStringList{} : QStringList{objectId};
}

void selectObjects(State &state, const QStringList &objectIds) {
    QStringList kept;
    for (const QString &objectId : objectIds) {
        if (objectId.startsWith(QStringLiteral("script_")) && generatedObjectExists(state, objectId)) {
            kept.append(objectId);
        }
    }
    state.selectedObjects = kept;
    state.selectedObject = kept.isEmpty() ? QString() : kept.last();
    state.selectedLayer = kept.isEmpty() ? state.selectedLayer : QString::fromLatin1(kScriptLayer);
    state.pendingPoint = {};
}

void includePoint(Bounds &bounds, double x, double y) {
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }
    if (!bounds.ok) {
        bounds.ok = true;
        bounds.minX = bounds.maxX = x;
        bounds.minY = bounds.maxY = y;
        return;
    }
    bounds.minX = std::min(bounds.minX, x);
    bounds.minY = std::min(bounds.minY, y);
    bounds.maxX = std::max(bounds.maxX, x);
    bounds.maxY = std::max(bounds.maxY, y);
}

void includePointList(Bounds &bounds, const QJsonArray &points) {
    for (const QJsonValue value : points) {
        const QJsonArray point = value.toArray();
        if (point.size() >= 2) {
            includePoint(bounds, point.at(0).toDouble(), point.at(1).toDouble());
        }
    }
}

Bounds normalizedBounds(const QJsonObject &object) {
    Bounds bounds;
    const QString kind = object.value("kind").toString();
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        includePoint(bounds, object.value("x").toDouble(), object.value("y").toDouble());
    } else if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        includePoint(bounds, object.value("x1").toDouble(), object.value("y1").toDouble());
        includePoint(bounds, object.value("x2").toDouble(), object.value("y2").toDouble());
    } else if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        const double cx = object.value("cx").toDouble();
        const double cy = object.value("cy").toDouble();
        const double radius = object.value("radius").toDouble();
        includePoint(bounds, cx - radius, cy - radius);
        includePoint(bounds, cx + radius, cy + radius);
    } else if (kind == QStringLiteral("rectangle")
               || kind == QStringLiteral("image_reference_frame")
               || kind == QStringLiteral("ascii_crop_frame")
               || kind == QStringLiteral("ascii_cell_region")) {
        const double x = object.value("x").toDouble();
        const double y = object.value("y").toDouble();
        includePoint(bounds, x, y);
        includePoint(bounds, x + object.value("width").toDouble(), y + object.value("height").toDouble());
    } else if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon")) {
        includePointList(bounds, object.value("points").toArray());
    }
    return bounds;
}

Bounds normalizedBoundsForObjects(const QJsonArray &objects, const QStringList &objectIds) {
    Bounds bounds;
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        if (!objectIds.contains(object.value("id").toString())) {
            continue;
        }
        const Bounds objectBounds = normalizedBounds(object);
        if (!objectBounds.ok) {
            continue;
        }
        includePoint(bounds, objectBounds.minX, objectBounds.minY);
        includePoint(bounds, objectBounds.maxX, objectBounds.maxY);
    }
    return bounds;
}

double clampedMoveDelta(double delta, double minValue, double maxValue) {
    if (!std::isfinite(delta)) {
        return 0.0;
    }
    const double minDelta = 0.0 - minValue;
    const double maxDelta = 1.0 - maxValue;
    return std::clamp(delta, minDelta, maxDelta);
}

void translateObject(QJsonObject &object, double dxN, double dyN, double dxPx, double dyPx) {
    const QString kind = object.value("kind").toString();
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        object.insert("x", object.value("x").toDouble() + dxN);
        object.insert("y", object.value("y").toDouble() + dyN);
        object.insert("point_px", translatedPointArray(object.value("point_px").toArray(), dxPx, dyPx));
    } else if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        object.insert("x1", object.value("x1").toDouble() + dxN);
        object.insert("y1", object.value("y1").toDouble() + dyN);
        object.insert("x2", object.value("x2").toDouble() + dxN);
        object.insert("y2", object.value("y2").toDouble() + dyN);
        object.insert("from_px", translatedPointArray(object.value("from_px").toArray(), dxPx, dyPx));
        object.insert("to_px", translatedPointArray(object.value("to_px").toArray(), dxPx, dyPx));
    } else if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        object.insert("cx", object.value("cx").toDouble() + dxN);
        object.insert("cy", object.value("cy").toDouble() + dyN);
        object.insert("center_px", translatedPointArray(object.value("center_px").toArray(), dxPx, dyPx));
    } else if (kind == QStringLiteral("rectangle")
               || kind == QStringLiteral("image_reference_frame")
               || kind == QStringLiteral("ascii_crop_frame")
               || kind == QStringLiteral("ascii_cell_region")) {
        object.insert("x", object.value("x").toDouble() + dxN);
        object.insert("y", object.value("y").toDouble() + dyN);
        object.insert("from_px", translatedPointArray(object.value("from_px").toArray(), dxPx, dyPx));
        object.insert("to_px", translatedPointArray(object.value("to_px").toArray(), dxPx, dyPx));
        QJsonArray rect = object.value("rect_px").toArray();
        if (rect.size() >= 4) {
            rect[0] = rect.at(0).toDouble() + dxPx;
            rect[1] = rect.at(1).toDouble() + dyPx;
            object.insert("rect_px", rect);
        }
    } else if (kind == QStringLiteral("polyline")) {
        object.insert("points", translatedPointList(object.value("points").toArray(), dxN, dyN));
        object.insert("points_px", translatedPointList(object.value("points_px").toArray(), dxPx, dyPx));
    } else if (kind == QStringLiteral("polygon")) {
        object.insert("cx", object.value("cx").toDouble() + dxN);
        object.insert("cy", object.value("cy").toDouble() + dyN);
        object.insert("center_px", translatedPointArray(object.value("center_px").toArray(), dxPx, dyPx));
        object.insert("points", translatedPointList(object.value("points").toArray(), dxN, dyN));
        object.insert("points_px", translatedPointList(object.value("points_px").toArray(), dxPx, dyPx));
    }
}

Point snapPoint(State &state, double x, double y) {
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return {};
    }
    double px = x;
    double py = y;
    if (state.gridSnap) {
        const double step = std::max(1, state.gridStepPx);
        px = std::round(px / step) * step;
        py = std::round(py / step) * step;
    }
    px = std::clamp(px, 0.0, static_cast<double>(state.canvasPx));
    py = std::clamp(py, 0.0, static_cast<double>(state.canvasPx));
    return {true, px, py, px / state.canvasPx, py / state.canvasPx};
}

Point pointFromArray(State &state, const QJsonArray &array) {
    if (array.size() < 2) {
        return {};
    }
    return snapPoint(state, array.at(0).toDouble(), array.at(1).toDouble());
}

Point pointFromCommand(State &state, const QJsonObject &command) {
    if (command.contains("point")) {
        return pointFromArray(state, arrayAt(command, "point"));
    }
    if (command.contains("center")) {
        return pointFromArray(state, arrayAt(command, "center"));
    }
    return snapPoint(state, numberAt(command, "x"), numberAt(command, "y"));
}

double distancePx(const Point &a, const Point &b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

double clampedPx(double value, int canvasPx) {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, static_cast<double>(canvasPx));
}

double positivePx(double value, double fallback = 1.0) {
    if (!std::isfinite(value) || value <= 0.0) {
        return fallback;
    }
    return value;
}

void rebuildRectangle(QJsonObject &object, int canvasPx, double xPx, double yPx, double widthPx, double heightPx) {
    const double width = std::clamp(positivePx(widthPx), 1.0, static_cast<double>(canvasPx));
    const double height = std::clamp(positivePx(heightPx), 1.0, static_cast<double>(canvasPx));
    const double x = std::clamp(clampedPx(xPx, canvasPx), 0.0, static_cast<double>(canvasPx) - width);
    const double y = std::clamp(clampedPx(yPx, canvasPx), 0.0, static_cast<double>(canvasPx) - height);
    object.insert("x", x / canvasPx);
    object.insert("y", y / canvasPx);
    object.insert("width", width / canvasPx);
    object.insert("height", height / canvasPx);
    object.insert("from_px", pointArray(x, y));
    object.insert("to_px", pointArray(x + width, y + height));
    QJsonArray rectPx;
    rectPx.append(x);
    rectPx.append(y);
    rectPx.append(width);
    rectPx.append(height);
    object.insert("rect_px", rectPx);
}

void rebuildPolygon(QJsonObject &object, int canvasPx, double cxPx, double cyPx, double radiusPx, int sides, double rotationDeg) {
    const double cx = clampedPx(cxPx, canvasPx);
    const double cy = clampedPx(cyPx, canvasPx);
    const int sideCount = std::clamp(sides, 3, 64);
    const double maxRadius = std::max(1.0, std::min({cx, cy, static_cast<double>(canvasPx) - cx, static_cast<double>(canvasPx) - cy}));
    const double radius = std::clamp(positivePx(radiusPx), 1.0, maxRadius);
    QJsonArray points;
    QJsonArray pointsPx;
    for (int index = 0; index < sideCount; ++index) {
        const double angle = (rotationDeg + 360.0 * index / sideCount) * M_PI / 180.0;
        const double px = cx + std::cos(angle) * radius;
        const double py = cy + std::sin(angle) * radius;
        points.append(pointArray(px / canvasPx, py / canvasPx));
        pointsPx.append(pointArray(px, py));
    }
    object.insert("cx", cx / canvasPx);
    object.insert("cy", cy / canvasPx);
    object.insert("center_px", pointArray(cx, cy));
    object.insert("radius", radius / canvasPx);
    object.insert("radius_px", radius);
    object.insert("sides", sideCount);
    object.insert("rotation_deg", rotationDeg);
    object.insert("points", points);
    object.insert("points_px", pointsPx);
}

QString nextId(const State &state, const QString &kind) {
    int count = 0;
    for (const QJsonValue value : state.generatedObjects) {
        const QJsonObject object = value.toObject();
        if (object.value("kind").toString() == kind) {
            ++count;
        }
    }
    return QStringLiteral("script_%1_%2").arg(kind, QStringLiteral("%1").arg(count + 1, 2, 10, QLatin1Char('0')));
}

void pushObject(State &state, QJsonObject object) {
    object.insert("layer_id", kScriptLayer);
    object.insert("source", "cpp_drawing_core");
    state.generatedObjects.append(object);
    state.selectedLayer = kScriptLayer;
    selectObject(state, object.value("id").toString());
}

void deleteObject(State &state, const QString &objectId) {
    if (objectId.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    QJsonArray keptObjects;
    bool deleted = false;
    for (const QJsonValue value : state.generatedObjects) {
        const QJsonObject object = value.toObject();
        if (object.value("id").toString() == objectId) {
            deleted = true;
            continue;
        }
        keptObjects.append(object);
    }
    if (!deleted) {
        state.errors.append("delete_object could not find generated object: " + objectId);
        return;
    }
    state.generatedObjects = keptObjects;
    if (state.selectedObject == objectId) {
        state.selectedObject.clear();
    }
    state.selectedObjects.removeAll(objectId);
    if (state.selectedObject.isEmpty() && !state.selectedObjects.isEmpty()) {
        state.selectedObject = state.selectedObjects.last();
    }
    state.pendingPoint = {};
}

void deleteObjects(State &state, const QStringList &objectIds) {
    if (objectIds.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    QStringList ids;
    for (const QString &objectId : objectIds) {
        if (!objectId.isEmpty() && !ids.contains(objectId)) {
            ids.append(objectId);
        }
    }
    QJsonArray keptObjects;
    int deleted = 0;
    for (const QJsonValue value : state.generatedObjects) {
        const QJsonObject object = value.toObject();
        const QString objectId = object.value("id").toString();
        if (ids.contains(objectId)) {
            ++deleted;
            continue;
        }
        keptObjects.append(object);
    }
    if (deleted == 0) {
        state.errors.append("delete_objects could not find selected generated objects");
        return;
    }
    state.generatedObjects = keptObjects;
    for (const QString &objectId : ids) {
        state.selectedObjects.removeAll(objectId);
        if (state.selectedObject == objectId) {
            state.selectedObject.clear();
        }
    }
    if (state.selectedObject.isEmpty() && !state.selectedObjects.isEmpty()) {
        state.selectedObject = state.selectedObjects.last();
    }
    state.pendingPoint = {};
}

QString pushClonedObject(State &state, QJsonObject object, const QString &sourceId, double dxN, double dyN, const QString &sourceKey) {
    const QString kind = object.value("kind").toString();
    if (kind.isEmpty()) {
        state.errors.append(sourceKey + " missing object kind");
        return {};
    }
    const Bounds bounds = normalizedBounds(object);
    if (!bounds.ok) {
        state.errors.append(sourceKey + " could not compute bounds");
        return {};
    }
    double clampedDxN = clampedMoveDelta(dxN, bounds.minX, bounds.maxX);
    double clampedDyN = clampedMoveDelta(dyN, bounds.minY, bounds.maxY);
    if (std::abs(clampedDxN) < 0.000001 && std::abs(clampedDyN) < 0.000001) {
        clampedDxN = clampedMoveDelta(-dxN, bounds.minX, bounds.maxX);
        clampedDyN = clampedMoveDelta(-dyN, bounds.minY, bounds.maxY);
    }
    const QString nextObjectId = nextId(state, kind);
    object.insert("id", nextObjectId);
    if (!sourceId.isEmpty()) {
        object.insert(sourceKey, sourceId);
    }
    translateObject(object, clampedDxN, clampedDyN, clampedDxN * state.canvasPx, clampedDyN * state.canvasPx);
    pushObject(state, object);
    state.pendingPoint = {};
    return nextObjectId;
}

void duplicateObject(State &state, const QString &objectId, double dxN, double dyN) {
    if (objectId.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    for (const QJsonValue value : state.generatedObjects) {
        QJsonObject object = value.toObject();
        if (object.value("id").toString() != objectId) {
            continue;
        }
        pushClonedObject(state, object, objectId, dxN, dyN, QStringLiteral("duplicate_of"));
        return;
    }
    state.errors.append("duplicate_object could not find generated object: " + objectId);
}

void duplicateObjects(State &state, const QStringList &objectIds, double dxN, double dyN) {
    if (objectIds.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    const QJsonArray sourceObjects = state.generatedObjects;
    QStringList duplicateIds;
    for (const QString &objectId : objectIds) {
        bool found = false;
        for (const QJsonValue value : sourceObjects) {
            QJsonObject object = value.toObject();
            if (object.value("id").toString() != objectId) {
                continue;
            }
            const QString duplicateId = pushClonedObject(state, object, objectId, dxN, dyN, QStringLiteral("duplicate_of"));
            if (!duplicateId.isEmpty()) {
                duplicateIds.append(duplicateId);
            }
            found = true;
            break;
        }
        if (!found) {
            state.errors.append("duplicate_objects could not find generated object: " + objectId);
        }
    }
    if (!duplicateIds.isEmpty()) {
        selectObjects(state, duplicateIds);
    }
}

void pasteObject(State &state, const QJsonObject &snapshot, double dxN, double dyN) {
    const QString sourceId = snapshot.value("id").toString();
    if (sourceId.isEmpty() || sourceId.indexOf(QStringLiteral("script_")) != 0) {
        state.errors.append("paste_object requires a generated object snapshot");
        return;
    }
    pushClonedObject(state, snapshot, sourceId, dxN, dyN, QStringLiteral("pasted_from"));
}

void pasteObjects(State &state, const QJsonArray &snapshots, double dxN, double dyN) {
    if (snapshots.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    QStringList pastedIds;
    for (const QJsonValue value : snapshots) {
        const QJsonObject snapshot = value.toObject();
        const QString sourceId = snapshot.value("id").toString();
        if (sourceId.isEmpty() || sourceId.indexOf(QStringLiteral("script_")) != 0) {
            state.errors.append("paste_objects skipped invalid generated object snapshot");
            continue;
        }
        const QString pastedId = pushClonedObject(state, snapshot, sourceId, dxN, dyN, QStringLiteral("pasted_from"));
        if (!pastedId.isEmpty()) {
            pastedIds.append(pastedId);
        }
    }
    if (!pastedIds.isEmpty()) {
        selectObjects(state, pastedIds);
    }
}

void moveObject(State &state, const QString &objectId, double dxN, double dyN) {
    if (objectId.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    bool moved = false;
    QJsonArray movedObjects;
    for (const QJsonValue value : state.generatedObjects) {
        QJsonObject object = value.toObject();
        if (object.value("id").toString() == objectId) {
            const Bounds bounds = normalizedBounds(object);
            if (!bounds.ok) {
                state.errors.append("move_object could not compute bounds for: " + objectId);
                return;
            }
            const double clampedDxN = clampedMoveDelta(dxN, bounds.minX, bounds.maxX);
            const double clampedDyN = clampedMoveDelta(dyN, bounds.minY, bounds.maxY);
            translateObject(object, clampedDxN, clampedDyN, clampedDxN * state.canvasPx, clampedDyN * state.canvasPx);
            moved = true;
        }
        movedObjects.append(object);
    }
    if (!moved) {
        state.errors.append("move_object could not find generated object: " + objectId);
        return;
    }
    state.generatedObjects = movedObjects;
    state.selectedLayer = kScriptLayer;
    selectObject(state, objectId);
    state.pendingPoint = {};
}

void moveObjects(State &state, const QStringList &objectIds, double dxN, double dyN) {
    if (objectIds.isEmpty()) {
        state.pendingPoint = {};
        return;
    }
    QStringList ids;
    for (const QString &objectId : objectIds) {
        if (!objectId.isEmpty() && !ids.contains(objectId)) {
            ids.append(objectId);
        }
    }
    const Bounds bounds = normalizedBoundsForObjects(state.generatedObjects, ids);
    if (!bounds.ok) {
        state.errors.append("move_objects could not compute selected object bounds");
        return;
    }
    const double clampedDxN = clampedMoveDelta(dxN, bounds.minX, bounds.maxX);
    const double clampedDyN = clampedMoveDelta(dyN, bounds.minY, bounds.maxY);
    const double dxPx = clampedDxN * state.canvasPx;
    const double dyPx = clampedDyN * state.canvasPx;
    bool moved = false;
    QJsonArray movedObjects;
    for (const QJsonValue value : state.generatedObjects) {
        QJsonObject object = value.toObject();
        if (ids.contains(object.value("id").toString())) {
            translateObject(object, clampedDxN, clampedDyN, dxPx, dyPx);
            moved = true;
        }
        movedObjects.append(object);
    }
    if (!moved) {
        state.errors.append("move_objects could not find selected generated objects");
        return;
    }
    state.generatedObjects = movedObjects;
    state.selectedLayer = kScriptLayer;
    selectObjects(state, ids);
    state.pendingPoint = {};
}

void updateObjectField(State &state, const QString &objectId, const QString &field, double value) {
    if (objectId.isEmpty() || field.isEmpty() || !std::isfinite(value)) {
        state.pendingPoint = {};
        return;
    }
    bool updated = false;
    QJsonArray updatedObjects;
    for (const QJsonValue objectValue : state.generatedObjects) {
        QJsonObject object = objectValue.toObject();
        if (object.value("id").toString() == objectId) {
            const QString kind = object.value("kind").toString();
            const double canvas = static_cast<double>(state.canvasPx);
            if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
                const double x = field == QStringLiteral("x_px") ? clampedPx(value, state.canvasPx) : object.value("x").toDouble() * canvas;
                const double y = field == QStringLiteral("y_px") ? clampedPx(value, state.canvasPx) : object.value("y").toDouble() * canvas;
                if (field != QStringLiteral("x_px") && field != QStringLiteral("y_px")) {
                    state.errors.append("update_object unsupported point field: " + field);
                    return;
                }
                object.insert("x", x / canvas);
                object.insert("y", y / canvas);
                object.insert("point_px", pointArray(x, y));
                updated = true;
            } else if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
                double x1 = object.value("x1").toDouble() * canvas;
                double y1 = object.value("y1").toDouble() * canvas;
                double x2 = object.value("x2").toDouble() * canvas;
                double y2 = object.value("y2").toDouble() * canvas;
                if (field == QStringLiteral("x1_px")) {
                    x1 = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("y1_px")) {
                    y1 = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("x2_px")) {
                    x2 = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("y2_px")) {
                    y2 = clampedPx(value, state.canvasPx);
                } else {
                    state.errors.append("update_object unsupported line field: " + field);
                    return;
                }
                object.insert("x1", x1 / canvas);
                object.insert("y1", y1 / canvas);
                object.insert("x2", x2 / canvas);
                object.insert("y2", y2 / canvas);
                object.insert("from_px", pointArray(x1, y1));
                object.insert("to_px", pointArray(x2, y2));
                updated = true;
            } else if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
                double cx = object.value("cx").toDouble() * canvas;
                double cy = object.value("cy").toDouble() * canvas;
                double radius = object.value("radius_px").toDouble();
                if (field == QStringLiteral("cx_px")) {
                    cx = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("cy_px")) {
                    cy = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("radius_px")) {
                    radius = std::clamp(positivePx(value), 1.0, canvas);
                } else if (kind == QStringLiteral("arc") && field == QStringLiteral("start_angle_deg")) {
                    object.insert("start_angle_deg", value);
                    updated = true;
                } else if (kind == QStringLiteral("arc") && field == QStringLiteral("end_angle_deg")) {
                    object.insert("end_angle_deg", value);
                    updated = true;
                } else {
                    state.errors.append("update_object unsupported circle field: " + field);
                    return;
                }
                object.insert("cx", cx / canvas);
                object.insert("cy", cy / canvas);
                object.insert("center_px", pointArray(cx, cy));
                object.insert("radius", radius / canvas);
                object.insert("radius_px", radius);
                updated = true;
            } else if (kind == QStringLiteral("rectangle")
                       || kind == QStringLiteral("image_reference_frame")
                       || kind == QStringLiteral("ascii_crop_frame")
                       || kind == QStringLiteral("ascii_cell_region")) {
                double x = object.value("x").toDouble() * canvas;
                double y = object.value("y").toDouble() * canvas;
                double width = object.value("width").toDouble() * canvas;
                double height = object.value("height").toDouble() * canvas;
                if (field == QStringLiteral("x_px")) {
                    x = value;
                } else if (field == QStringLiteral("y_px")) {
                    y = value;
                } else if (field == QStringLiteral("width_px")) {
                    width = value;
                } else if (field == QStringLiteral("height_px")) {
                    height = value;
                } else {
                    state.errors.append("update_object unsupported rectangle field: " + field);
                    return;
                }
                rebuildRectangle(object, state.canvasPx, x, y, width, height);
                updated = true;
            } else if (kind == QStringLiteral("polygon")) {
                double cx = object.value("cx").toDouble() * canvas;
                double cy = object.value("cy").toDouble() * canvas;
                double radius = object.value("radius_px").toDouble();
                int sides = object.value("sides").toInt(6);
                double rotation = object.value("rotation_deg").toDouble();
                if (field == QStringLiteral("cx_px")) {
                    cx = value;
                } else if (field == QStringLiteral("cy_px")) {
                    cy = value;
                } else if (field == QStringLiteral("radius_px")) {
                    radius = value;
                } else if (field == QStringLiteral("sides")) {
                    sides = static_cast<int>(std::round(value));
                } else if (field == QStringLiteral("rotation_deg")) {
                    rotation = value;
                } else {
                    state.errors.append("update_object unsupported polygon field: " + field);
                    return;
                }
                rebuildPolygon(object, state.canvasPx, cx, cy, radius, sides, rotation);
                updated = true;
            } else {
                state.errors.append("update_object unsupported kind: " + kind);
                return;
            }
        }
        updatedObjects.append(object);
    }
    if (!updated) {
        state.errors.append("update_object could not find generated object: " + objectId);
        return;
    }
    state.generatedObjects = updatedObjects;
    state.selectedLayer = kScriptLayer;
    selectObject(state, objectId);
    state.pendingPoint = {};
}

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
        const double angle = (rotationDeg + 360.0 * index / sides) * M_PI / 180.0;
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

QString svgNumber(double value) {
    return QString::number(value, 'f', 3).replace(QRegularExpression("\\.?0+$"), "");
}

QString pointsToSvg(const QJsonArray &points) {
    QStringList values;
    for (const QJsonValue value : points) {
        const QJsonArray point = value.toArray();
        if (point.size() >= 2) {
            values.append(svgNumber(point.at(0).toDouble()) + "," + svgNumber(point.at(1).toDouble()));
        }
    }
    return values.join(" ");
}
} // namespace

DrawingCoreResult DrawingCore::runScript(const QJsonObject &script) {
    State state;
    state.scriptId = stringAt(script, "script_id", "unnamed_script");
    const QJsonArray canvas = arrayAt(script, "canvas_px");
    if (canvas.size() >= 2 && canvas.at(0).toInt() == canvas.at(1).toInt() && canvas.at(0).toInt() > 0) {
        state.canvasPx = canvas.at(0).toInt();
    }

    const QJsonArray commands = arrayAt(script, "commands");
    for (const QJsonValue value : commands) {
        const QJsonObject command = value.toObject();
        state.commandLog.append(command);
        const QString name = stringAt(command, "cmd");

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
                 const double dxN = command.contains("dx_px")
                     ? numberAt(command, "dx_px") / state.canvasPx
                     : numberAt(command, "dx", 16.0 / state.canvasPx);
                 const double dyN = command.contains("dy_px")
                     ? numberAt(command, "dy_px") / state.canvasPx
                     : numberAt(command, "dy", 16.0 / state.canvasPx);
                 duplicateObject(state, stringAt(command, "object_id", state.selectedObject), dxN, dyN);
             }},
            {"duplicate_objects", [&]() {
                 const double dxN = command.contains("dx_px")
                     ? numberAt(command, "dx_px") / state.canvasPx
                     : numberAt(command, "dx", 16.0 / state.canvasPx);
                 const double dyN = command.contains("dy_px")
                     ? numberAt(command, "dy_px") / state.canvasPx
                     : numberAt(command, "dy", 16.0 / state.canvasPx);
                 duplicateObjects(state, stringListFromArray(arrayAt(command, "object_ids")), dxN, dyN);
             }},
            {"paste_object", [&]() {
                 const double dxN = command.contains("dx_px")
                     ? numberAt(command, "dx_px") / state.canvasPx
                     : numberAt(command, "dx", 16.0 / state.canvasPx);
                 const double dyN = command.contains("dy_px")
                     ? numberAt(command, "dy_px") / state.canvasPx
                     : numberAt(command, "dy", 16.0 / state.canvasPx);
                 pasteObject(state, command.value("object").toObject(), dxN, dyN);
             }},
            {"paste_objects", [&]() {
                 const double dxN = command.contains("dx_px")
                     ? numberAt(command, "dx_px") / state.canvasPx
                     : numberAt(command, "dx", 16.0 / state.canvasPx);
                 const double dyN = command.contains("dy_px")
                     ? numberAt(command, "dy_px") / state.canvasPx
                     : numberAt(command, "dy", 16.0 / state.canvasPx);
                 pasteObjects(state, arrayAt(command, "objects"), dxN, dyN);
             }},
            {"move_object", [&]() {
                 const double dxN = command.contains("dx_px")
                     ? numberAt(command, "dx_px") / state.canvasPx
                     : numberAt(command, "dx");
                 const double dyN = command.contains("dy_px")
                     ? numberAt(command, "dy_px") / state.canvasPx
                     : numberAt(command, "dy");
                 moveObject(state, stringAt(command, "object_id", state.selectedObject), dxN, dyN);
             }},
            {"move_objects", [&]() {
                 const double dxN = command.contains("dx_px")
                     ? numberAt(command, "dx_px") / state.canvasPx
                     : numberAt(command, "dx");
                 const double dyN = command.contains("dy_px")
                     ? numberAt(command, "dy_px") / state.canvasPx
                     : numberAt(command, "dy");
                 moveObjects(state, stringListFromArray(arrayAt(command, "object_ids")), dxN, dyN);
             }},
            {"update_object", [&]() {
                 updateObjectField(
                     state,
                     stringAt(command, "object_id", state.selectedObject),
                     stringAt(command, "field"),
                     numberAt(command, "value"));
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
        const auto handler = std::find_if(handlers.begin(), handlers.end(), [&](const CommandHandler &entry) {
            return name == QString::fromLatin1(entry.commandName);
        });
        if (handler == handlers.end()) {
            state.errors.append("unsupported command: " + name);
            continue;
        }
        handler->run();
    }
    if (state.pendingPoint.ok && !script.value("allow_pending").toBool(false)) {
        state.errors.append("line_polyline ended with an unmatched pending point");
    }

    DrawingCoreResult result;
    result.model = buildModel(state);
    result.svg = modelToSvg(result.model);
    return result;
}

QString DrawingCore::modelToJson(const QJsonObject &model) {
    return QString::fromUtf8(QJsonDocument(model).toJson(QJsonDocument::Indented));
}

QString DrawingCore::modelToSvg(const QJsonObject &model) {
    const int canvas = model.value("canvas_px").toArray().at(0).toInt(kDefaultCanvasPx);
    QString svg;
    svg += QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %1 %1\" width=\"%1\" height=\"%1\">\n").arg(canvas);
    svg += QStringLiteral("  <rect width=\"100%\" height=\"100%\" fill=\"#25232d\"/>\n");
    svg += QStringLiteral("  <g id=\"script_geometry\" fill=\"none\" stroke=\"#f4d46f\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\">\n");
    const QJsonArray objects = model.value("generated_objects").toArray();
    struct ObjectRenderer {
        const char *kind;
        std::function<void(const QJsonObject &)> render;
    };
    const std::vector<ObjectRenderer> renderers = {
        {"point", [&](const QJsonObject &object) {
             const QJsonArray point = object.value("point_px").toArray();
             svg += QStringLiteral("    <circle id=\"%1\" cx=\"%2\" cy=\"%3\" r=\"5\" fill=\"#f4d46f\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(point.at(0).toDouble()), svgNumber(point.at(1).toDouble()));
         }},
        {"tone_probe", [&](const QJsonObject &object) {
             const QJsonArray point = object.value("point_px").toArray();
             svg += QStringLiteral("    <circle id=\"%1\" cx=\"%2\" cy=\"%3\" r=\"7\" fill=\"#70d6ff\" stroke=\"#f4d46f\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(point.at(0).toDouble()), svgNumber(point.at(1).toDouble()));
         }},
        {"line", [&](const QJsonObject &object) {
             const QJsonArray from = object.value("from_px").toArray();
             const QJsonArray to = object.value("to_px").toArray();
             svg += QStringLiteral("    <line id=\"%1\" x1=\"%2\" y1=\"%3\" x2=\"%4\" y2=\"%5\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(from.at(0).toDouble()), svgNumber(from.at(1).toDouble()), svgNumber(to.at(0).toDouble()), svgNumber(to.at(1).toDouble()));
         }},
        {"glyph_baseline", [&](const QJsonObject &object) {
             const QJsonArray from = object.value("from_px").toArray();
             const QJsonArray to = object.value("to_px").toArray();
             svg += QStringLiteral("    <line id=\"%1\" x1=\"%2\" y1=\"%3\" x2=\"%4\" y2=\"%5\" stroke=\"#70d6ff\" stroke-dasharray=\"8 5\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(from.at(0).toDouble()), svgNumber(from.at(1).toDouble()), svgNumber(to.at(0).toDouble()), svgNumber(to.at(1).toDouble()));
         }},
        {"polyline", [&](const QJsonObject &object) {
             svg += QStringLiteral("    <polyline id=\"%1\" points=\"%2\"/>\n")
                 .arg(object.value("id").toString(), pointsToSvg(object.value("points_px").toArray()));
         }},
        {"circle", [&](const QJsonObject &object) {
             const QJsonArray center = object.value("center_px").toArray();
             svg += QStringLiteral("    <circle id=\"%1\" cx=\"%2\" cy=\"%3\" r=\"%4\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(center.at(0).toDouble()), svgNumber(center.at(1).toDouble()), svgNumber(object.value("radius_px").toDouble()));
         }},
        {"arc", [&](const QJsonObject &object) {
             const QJsonArray center = object.value("center_px").toArray();
             const double radius = object.value("radius_px").toDouble();
             const double start = object.value("start_angle_deg").toDouble() * M_PI / 180.0;
             const double end = object.value("end_angle_deg").toDouble() * M_PI / 180.0;
             const double x1 = center.at(0).toDouble() + std::cos(start) * radius;
             const double y1 = center.at(1).toDouble() + std::sin(start) * radius;
             const double x2 = center.at(0).toDouble() + std::cos(end) * radius;
             const double y2 = center.at(1).toDouble() + std::sin(end) * radius;
             const int largeArc = std::abs(object.value("end_angle_deg").toDouble() - object.value("start_angle_deg").toDouble()) > 180.0 ? 1 : 0;
             svg += QStringLiteral("    <path id=\"%1\" d=\"M %2 %3 A %4 %4 0 %5 1 %6 %7\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(x1), svgNumber(y1), svgNumber(radius), QString::number(largeArc), svgNumber(x2), svgNumber(y2));
         }},
        {"rectangle", [&](const QJsonObject &object) {
             const QJsonArray rect = object.value("rect_px").toArray();
             svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()));
         }},
        {"image_reference_frame", [&](const QJsonObject &object) {
             const QJsonArray rect = object.value("rect_px").toArray();
             svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\" stroke=\"#70d6ff\" stroke-dasharray=\"10 6\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()));
         }},
        {"ascii_crop_frame", [&](const QJsonObject &object) {
             const QJsonArray rect = object.value("rect_px").toArray();
             svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\" stroke=\"#ffcc66\" stroke-width=\"4\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()));
         }},
        {"ascii_cell_region", [&](const QJsonObject &object) {
             const QJsonArray rect = object.value("rect_px").toArray();
             svg += QStringLiteral("    <rect id=\"%1\" x=\"%2\" y=\"%3\" width=\"%4\" height=\"%5\" stroke=\"#c084fc\" stroke-dasharray=\"5 5\"/>\n")
                 .arg(object.value("id").toString(), svgNumber(rect.at(0).toDouble()), svgNumber(rect.at(1).toDouble()), svgNumber(rect.at(2).toDouble()), svgNumber(rect.at(3).toDouble()));
         }},
        {"polygon", [&](const QJsonObject &object) {
             svg += QStringLiteral("    <polygon id=\"%1\" points=\"%2\"/>\n")
                 .arg(object.value("id").toString(), pointsToSvg(object.value("points_px").toArray()));
         }},
    };
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        const QString kind = object.value("kind").toString();
        const auto renderer = std::find_if(renderers.begin(), renderers.end(), [&](const ObjectRenderer &entry) {
            return kind == QString::fromLatin1(entry.kind);
        });
        if (renderer != renderers.end()) {
            renderer->render(object);
        }
    }
    svg += QStringLiteral("  </g>\n</svg>\n");
    return svg;
}

DrawingDocumentController::DrawingDocumentController(QObject *parent)
    : QObject(parent) {
    reset();
}

int DrawingDocumentController::revision() const {
    return m_revision;
}

QVariantMap DrawingDocumentController::modelDocument() const {
    return m_model.toVariantMap();
}

QString DrawingDocumentController::exportJson() const {
    return DrawingCore::modelToJson(m_model);
}

QString DrawingDocumentController::exportSvg() const {
    return DrawingCore::modelToSvg(m_model);
}

bool DrawingDocumentController::loadModel(const QVariantMap &model) {
    const QJsonObject object = QJsonObject::fromVariantMap(model);
    if (object.value("export_kind").toString() != QStringLiteral("pattern_lab_2d_native_model_v0")) {
        return false;
    }
    m_commands = object.value("command_log").toArray();
    m_undoSnapshots.clear();
    m_redoSnapshots.clear();
    m_lastStableCommands = pendingPointActive(object) ? QJsonArray{} : m_commands;
    m_model = object;
    ++m_revision;
    emit modelChanged();
    return true;
}

void DrawingDocumentController::reset() {
    m_commands = {};
    m_undoSnapshots.clear();
    m_redoSnapshots.clear();
    m_lastStableCommands = {};
    publish();
}

void DrawingDocumentController::selectTool(const QString &toolId) {
    QJsonObject command;
    command.insert("cmd", "select_tool");
    command.insert("tool", toolId);
    applyCommand(command);
}

void DrawingDocumentController::selectObject(const QString &objectId) {
    QJsonObject command;
    command.insert("cmd", "select_object");
    command.insert("object_id", objectId);
    applyCommand(command);
}

void DrawingDocumentController::selectObjects(const QVariantList &objectIds) {
    QJsonArray ids;
    for (const QVariant &value : objectIds) {
        const QString objectId = value.toString();
        if (!objectId.isEmpty()) {
            ids.append(objectId);
        }
    }
    QJsonObject command;
    command.insert("cmd", "select_objects");
    command.insert("object_ids", ids);
    applyCommand(command);
}

void DrawingDocumentController::deleteObject(const QString &objectId) {
    if (objectId.isEmpty()) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "delete_object");
    command.insert("object_id", objectId);
    applyCommand(command);
}

void DrawingDocumentController::deleteObjects(const QVariantList &objectIds) {
    QJsonArray ids;
    for (const QVariant &value : objectIds) {
        const QString objectId = value.toString();
        if (!objectId.isEmpty()) {
            ids.append(objectId);
        }
    }
    if (ids.isEmpty()) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "delete_objects");
    command.insert("object_ids", ids);
    applyCommand(command);
}

void DrawingDocumentController::deleteSelectedObject() {
    const QVariantList ids = selectedObjectIds();
    if (ids.size() > 1) {
        deleteObjects(ids);
        return;
    }
    deleteObject(selectedObjectId());
}

void DrawingDocumentController::duplicateObject(const QString &objectId, double dx, double dy) {
    if (objectId.isEmpty() || (!std::isfinite(dx) && !std::isfinite(dy))) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "duplicate_object");
    command.insert("object_id", objectId);
    command.insert("dx", std::isfinite(dx) ? dx : 0.03125);
    command.insert("dy", std::isfinite(dy) ? dy : 0.03125);
    applyCommand(command);
}

void DrawingDocumentController::duplicateObjects(const QVariantList &objectIds, double dx, double dy) {
    QJsonArray ids;
    for (const QVariant &value : objectIds) {
        const QString objectId = value.toString();
        if (!objectId.isEmpty()) {
            ids.append(objectId);
        }
    }
    if (ids.isEmpty() || (!std::isfinite(dx) && !std::isfinite(dy))) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "duplicate_objects");
    command.insert("object_ids", ids);
    command.insert("dx", std::isfinite(dx) ? dx : 0.03125);
    command.insert("dy", std::isfinite(dy) ? dy : 0.03125);
    applyCommand(command);
}

void DrawingDocumentController::duplicateSelectedObject() {
    const QVariantList ids = selectedObjectIds();
    if (ids.size() > 1) {
        duplicateObjects(ids);
        return;
    }
    duplicateObject(selectedObjectId());
}

void DrawingDocumentController::pasteObject(const QVariantMap &object, double dx, double dy) {
    const QJsonObject snapshot = QJsonObject::fromVariantMap(object);
    if (snapshot.isEmpty() || (!std::isfinite(dx) && !std::isfinite(dy))) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "paste_object");
    command.insert("object", snapshot);
    command.insert("dx", std::isfinite(dx) ? dx : 0.03125);
    command.insert("dy", std::isfinite(dy) ? dy : 0.03125);
    applyCommand(command);
}

void DrawingDocumentController::pasteObjects(const QVariantList &objects, double dx, double dy) {
    QJsonArray snapshots;
    for (const QVariant &value : objects) {
        const QJsonObject snapshot = QJsonObject::fromVariantMap(value.toMap());
        if (!snapshot.isEmpty()) {
            snapshots.append(snapshot);
        }
    }
    if (snapshots.isEmpty() || (!std::isfinite(dx) && !std::isfinite(dy))) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "paste_objects");
    command.insert("objects", snapshots);
    command.insert("dx", std::isfinite(dx) ? dx : 0.03125);
    command.insert("dy", std::isfinite(dy) ? dy : 0.03125);
    applyCommand(command);
}

void DrawingDocumentController::beginMoveGesture() {
    m_moveGestureActive = true;
    m_moveGestureUndoCaptured = false;
    m_moveGestureStartCommandCount = m_commands.size();
}

void DrawingDocumentController::endMoveGesture() {
    m_moveGestureActive = false;
    m_moveGestureUndoCaptured = false;
    m_moveGestureStartCommandCount = m_commands.size();
}

void DrawingDocumentController::moveObjectBy(const QString &objectId, double dx, double dy) {
    if (objectId.isEmpty() || (!std::isfinite(dx) && !std::isfinite(dy))) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "move_object");
    command.insert("object_id", objectId);
    command.insert("dx", std::isfinite(dx) ? dx : 0.0);
    command.insert("dy", std::isfinite(dy) ? dy : 0.0);
    applyCommand(command);
}

void DrawingDocumentController::moveObjectsBy(const QVariantList &objectIds, double dx, double dy) {
    QJsonArray ids;
    for (const QVariant &value : objectIds) {
        const QString objectId = value.toString();
        if (!objectId.isEmpty()) {
            ids.append(objectId);
        }
    }
    if (ids.isEmpty() || (!std::isfinite(dx) && !std::isfinite(dy))) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "move_objects");
    command.insert("object_ids", ids);
    command.insert("dx", std::isfinite(dx) ? dx : 0.0);
    command.insert("dy", std::isfinite(dy) ? dy : 0.0);
    applyCommand(command);
}

void DrawingDocumentController::moveSelectedObjectBy(double dx, double dy) {
    const QVariantList ids = selectedObjectIds();
    if (ids.size() > 1) {
        moveObjectsBy(ids, dx, dy);
        return;
    }
    moveObjectBy(selectedObjectId(), dx, dy);
}

void DrawingDocumentController::updateObjectField(const QString &objectId, const QString &field, double value) {
    if (objectId.isEmpty() || field.isEmpty() || !std::isfinite(value)) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "update_object");
    command.insert("object_id", objectId);
    command.insert("field", field);
    command.insert("value", value);
    applyCommand(command);
}

void DrawingDocumentController::updateSelectedObjectField(const QString &field, double value) {
    updateObjectField(selectedObjectId(), field, value);
}

void DrawingDocumentController::setToolParameter(const QString &parameter, const QVariant &value) {
    if (parameter.isEmpty() || !value.isValid()) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "set_tool_parameter");
    command.insert("parameter", parameter);
    command.insert("value", QJsonValue::fromVariant(value));
    applyCommand(command);
}

void DrawingDocumentController::cancelPending() {
    QJsonObject command;
    command.insert("cmd", "cancel_pending");
    applyCommand(command);
}

void DrawingDocumentController::setSnap(bool enabled, int gridStepPx) {
    QJsonObject command;
    command.insert("cmd", "set_snap");
    command.insert("grid", enabled);
    command.insert("grid_step_px", std::max(1, gridStepPx));
    applyCommand(command);
}

void DrawingDocumentController::clickCanvasNormalized(double x, double y) {
    const QJsonArray canvas = m_model.value("canvas_px").toArray();
    const double canvasPx = canvas.size() >= 2 ? canvas.at(0).toDouble(kDefaultCanvasPx) : kDefaultCanvasPx;
    const double px = std::clamp(x, 0.0, 1.0) * canvasPx;
    const double py = std::clamp(y, 0.0, 1.0) * canvasPx;
    QJsonObject command;
    command.insert("cmd", "click_canvas");
    command.insert("x", px);
    command.insert("y", py);
    applyCommand(command);
}

void DrawingDocumentController::clickCanvasNormalizedWithSnapStep(double x, double y, int gridStepPx) {
    const QJsonArray canvas = m_model.value("canvas_px").toArray();
    const double canvasPx = canvas.size() >= 2 ? canvas.at(0).toDouble(kDefaultCanvasPx) : kDefaultCanvasPx;
    const double px = std::clamp(x, 0.0, 1.0) * canvasPx;
    const double py = std::clamp(y, 0.0, 1.0) * canvasPx;
    QJsonObject command;
    command.insert("cmd", "click_canvas");
    command.insert("x", px);
    command.insert("y", py);
    command.insert("grid_step_px", std::max(1, gridStepPx));
    applyCommand(command);
}

bool DrawingDocumentController::canUndo() const {
    return !m_undoSnapshots.isEmpty();
}

bool DrawingDocumentController::canRedo() const {
    return !m_redoSnapshots.isEmpty();
}

void DrawingDocumentController::undo() {
    if (!canUndo()) {
        return;
    }
    m_redoSnapshots.append(m_commands);
    m_commands = m_undoSnapshots.takeLast();
    publish();
}

void DrawingDocumentController::redo() {
    if (!canRedo()) {
        return;
    }
    m_undoSnapshots.append(m_commands);
    m_commands = m_redoSnapshots.takeLast();
    publish();
}

void DrawingDocumentController::runScript(const QVariantMap &script) {
    const DrawingCoreResult result = DrawingCore::runScript(QJsonObject::fromVariantMap(script));
    m_commands = QJsonObject::fromVariantMap(script).value("commands").toArray();
    m_undoSnapshots.clear();
    m_redoSnapshots.clear();
    m_lastStableCommands = pendingPointActive(result.model) ? QJsonArray{} : m_commands;
    m_model = result.model;
    ++m_revision;
    emit modelChanged();
}

QString DrawingDocumentController::selectedToolId() const {
    return m_model.value("selected_tool_id").toString("anchor_points");
}

QString DrawingDocumentController::selectedObjectId() const {
    return m_model.value("selected_object_id").toString();
}

QVariantList DrawingDocumentController::selectedObjectIds() const {
    QVariantList ids;
    const QJsonArray selectedIds = m_model.value("selected_object_ids").toArray();
    for (const QJsonValue value : selectedIds) {
        const QString objectId = value.toString();
        if (!objectId.isEmpty() && !ids.contains(objectId)) {
            ids.append(objectId);
        }
    }
    const QString primaryId = selectedObjectId();
    if (ids.isEmpty() && !primaryId.isEmpty()) {
        ids.append(primaryId);
    }
    return ids;
}

void DrawingDocumentController::applyCommand(const QJsonObject &command) {
    const bool undoable = undoableInteractiveCommand(command);
    const bool wasPending = pendingPointActive(m_model);
    const int beforeObjectCount = generatedObjectCount(m_model);
    const QJsonArray beforeCommands = m_commands;
    const QJsonArray undoSnapshot = wasPending ? m_lastStableCommands : beforeCommands;
    const QString name = stringAt(command, QStringLiteral("cmd"));

    if (m_moveGestureActive && (name == QStringLiteral("move_object") || name == QStringLiteral("move_objects")) && !m_commands.isEmpty()) {
        const int lastIndex = m_commands.size() - 1;
        QJsonObject previous = m_commands.at(lastIndex).toObject();
        const bool sameMoveObject = name == QStringLiteral("move_object")
            && stringAt(previous, QStringLiteral("cmd")) == QStringLiteral("move_object")
            && stringAt(previous, QStringLiteral("object_id")) == stringAt(command, QStringLiteral("object_id"));
        const bool sameMoveObjects = name == QStringLiteral("move_objects")
            && stringAt(previous, QStringLiteral("cmd")) == QStringLiteral("move_objects")
            && previous.value(QStringLiteral("object_ids")).toArray() == command.value(QStringLiteral("object_ids")).toArray();
        if (sameMoveObject || sameMoveObjects) {
            previous.insert(QStringLiteral("dx"), numberAt(previous, QStringLiteral("dx")) + numberAt(command, QStringLiteral("dx")));
            previous.insert(QStringLiteral("dy"), numberAt(previous, QStringLiteral("dy")) + numberAt(command, QStringLiteral("dy")));
            m_commands[lastIndex] = previous;
            m_redoSnapshots.clear();
            publish();
            return;
        }
    }

    const bool gestureFieldUpdate = m_moveGestureActive && name == QStringLiteral("update_object");
    if (gestureFieldUpdate) {
        const int firstGestureIndex = std::clamp(m_moveGestureStartCommandCount, 0, static_cast<int>(m_commands.size()));
        for (int index = m_commands.size() - 1; index >= firstGestureIndex; --index) {
            QJsonObject previous = m_commands.at(index).toObject();
            if (stringAt(previous, QStringLiteral("cmd")) == QStringLiteral("update_object")
                    && stringAt(previous, QStringLiteral("object_id")) == stringAt(command, QStringLiteral("object_id"))
                    && stringAt(previous, QStringLiteral("field")) == stringAt(command, QStringLiteral("field"))) {
                previous.insert(QStringLiteral("value"), numberAt(command, QStringLiteral("value")));
                m_commands[index] = previous;
                m_redoSnapshots.clear();
                publish();
                return;
            }
        }
    }

    m_commands.append(command);
    publish();

    if (!undoable) {
        return;
    }

    m_redoSnapshots.clear();
    const bool nowPending = pendingPointActive(m_model);
    const int afterObjectCount = generatedObjectCount(m_model);
    const bool pendingOnlyClick = name == QStringLiteral("click_canvas")
        && nowPending
        && afterObjectCount == beforeObjectCount;
    if (!pendingOnlyClick) {
        if (gestureFieldUpdate && m_moveGestureUndoCaptured) {
            return;
        }
        m_undoSnapshots.append(undoSnapshot);
        if (gestureFieldUpdate) {
            m_moveGestureUndoCaptured = true;
        }
    }
}

void DrawingDocumentController::publish() {
    const DrawingCoreResult result = DrawingCore::runScript(scriptEnvelope());
    m_model = result.model;
    if (!pendingPointActive(m_model)) {
        m_lastStableCommands = m_commands;
    }
    ++m_revision;
    emit modelChanged();
}

QJsonObject DrawingDocumentController::scriptEnvelope() const {
    QJsonObject script;
    script.insert("script_id", "interactive_drawing_session_v0");
    script.insert("canvas_px", pointArray(kDefaultCanvasPx, kDefaultCanvasPx));
    script.insert("commands", m_commands);
    script.insert("allow_pending", true);
    return script;
}
