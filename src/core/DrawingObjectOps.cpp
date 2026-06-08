#include "DrawingCoreInternal.h"

#include <algorithm>
#include <cmath>

namespace drawing_core {

namespace {
Bounds boundsFromStoreObject(const DrawingObject &object) {
    if (!object.bounds.ok) {
        return {};
    }
    return {true, object.bounds.x, object.bounds.y, object.bounds.x + object.bounds.w, object.bounds.y + object.bounds.h};
}

bool refreshStoreProjection(State &state, QJsonObject &object) {
    const QString objectId = object.value(QStringLiteral("id")).toString();
    if (!state.store.contains({objectId})) {
        return true;
    }
    const QJsonObject projected = state.store.serializeObject({objectId}, state.canvasPx);
    if (projected.isEmpty()) {
        return false;
    }
    object = projected;
    return true;
}

bool replaceStoreAttributesFromJson(State &state, QJsonObject &object) {
    const QString objectId = object.value(QStringLiteral("id")).toString();
    if (!state.store.contains({objectId})) {
        return true;
    }
    if (!state.store.replaceAttributes({objectId}, object)) {
        return false;
    }
    return refreshStoreProjection(state, object);
}

bool rectangleLikeKind(const QString &kind) {
    return kind == QStringLiteral("rectangle")
        || kind == QStringLiteral("image_reference_frame")
        || kind == QStringLiteral("ascii_crop_frame")
        || kind == QStringLiteral("ascii_cell_region");
}

bool addStoreObjectFromJson(State &state, const QJsonObject &object) {
    const QString kind = object.value(QStringLiteral("kind")).toString();
    if (kind != QStringLiteral("point")
        && kind != QStringLiteral("tone_probe")
        && kind != QStringLiteral("line")
        && kind != QStringLiteral("glyph_baseline")
        && kind != QStringLiteral("circle")
        && kind != QStringLiteral("arc")
        && !rectangleLikeKind(kind)) {
        return true;
    }
    const QString objectId = object.value(QStringLiteral("id")).toString();
    if (objectId.isEmpty() || state.store.contains({objectId})) {
        return false;
    }

    DrawingObject typed;
    typed.id = {objectId};
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        typed.kind = ShapeKind::Point;
        typed.geometry = pointGeometryFromObject(object);
    } else if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        typed.kind = ShapeKind::Line;
        typed.geometry = lineGeometryFromObject(object);
    } else if (kind == QStringLiteral("arc")) {
        typed.kind = ShapeKind::Circle;
        typed.geometry = arcGeometryFromObject(object);
    } else if (kind == QStringLiteral("circle")) {
        typed.kind = ShapeKind::Circle;
        typed.geometry = circleGeometryFromObject(object);
    } else {
        typed.kind = ShapeKind::Rectangle;
        typed.geometry = rectangleGeometryFromObject(object);
    }
    typed.style = {object.value(QStringLiteral("style_id")).toString(QStringLiteral("inline_active_stroke"))};
    typed.layer = {object.value(QStringLiteral("layer_id")).toString(QString::fromLatin1(kScriptLayer))};
    typed.metadata.values = object.value(QStringLiteral("metadata")).toObject();
    typed.attributes = object;
    return state.store.addObject(typed);
}
} // namespace

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
    const QString objectId = object.value(QStringLiteral("id")).toString();
    if (state.store.contains({objectId})) {
        state.store.replaceAttributes({objectId}, object);
        object = state.store.serializeObject({objectId}, state.canvasPx);
    }
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
    if (state.store.contains({objectId})) {
        state.store.removeObject({objectId});
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
    for (const QString &objectId : ids) {
        if (state.store.contains({objectId})) {
            state.store.removeObject({objectId});
        }
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
    translateObjectWithState(object, state, clampedDxN, clampedDyN);
    if (!addStoreObjectFromJson(state, object)) {
        state.errors.append(sourceKey + " could not add typed object: " + nextObjectId);
        return {};
    }
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
            const DrawingObject *storeObject = state.store.find({objectId});
            const Bounds bounds = storeObject == nullptr ? normalizedBounds(object) : boundsFromStoreObject(*storeObject);
            if (!bounds.ok) {
                state.errors.append("move_object could not compute bounds for: " + objectId);
                return;
            }
            const double clampedDxN = clampedMoveDelta(dxN, bounds.minX, bounds.maxX);
            const double clampedDyN = clampedMoveDelta(dyN, bounds.minY, bounds.maxY);
            if (storeObject != nullptr) {
                if (!state.store.translateObject({objectId}, clampedDxN, clampedDyN)) {
                    state.errors.append("move_object could not update typed object: " + objectId);
                    return;
                }
                object = state.store.serializeObject({objectId}, state.canvasPx);
            } else {
                translateObjectWithState(object, state, clampedDxN, clampedDyN);
            }
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
    bool moved = false;
    QJsonArray movedObjects;
    for (const QJsonValue value : state.generatedObjects) {
        QJsonObject object = value.toObject();
        const QString objectId = object.value("id").toString();
        if (ids.contains(objectId)) {
            if (state.store.contains({objectId})) {
                if (!state.store.translateObject({objectId}, clampedDxN, clampedDyN)) {
                    state.errors.append("move_objects could not update typed object: " + objectId);
                    return;
                }
                object = state.store.serializeObject({objectId}, state.canvasPx);
            } else {
                translateObjectWithState(object, state, clampedDxN, clampedDyN);
            }
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
                const DrawingObject *storeObject = state.store.find({objectId});
                const PointGeometry *storePoint = storeObject == nullptr ? nullptr : std::get_if<PointGeometry>(&storeObject->geometry);
                const double x = field == QStringLiteral("x_px") ? clampedPx(value, state.canvasPx) : (storePoint == nullptr ? object.value("x").toDouble() : storePoint->point.x) * canvas;
                const double y = field == QStringLiteral("y_px") ? clampedPx(value, state.canvasPx) : (storePoint == nullptr ? object.value("y").toDouble() : storePoint->point.y) * canvas;
                if (field != QStringLiteral("x_px") && field != QStringLiteral("y_px")) {
                    state.errors.append("update_object unsupported point field: " + field);
                    return;
                }
                const PointGeometry updatedPoint{{x / canvas, y / canvas}};
                if (storeObject != nullptr) {
                    if (storePoint == nullptr || !state.store.updateGeometry({objectId}, updatedPoint)) {
                        state.errors.append("update_object could not update typed point: " + objectId);
                        return;
                    }
                    object = state.store.serializeObject({objectId}, state.canvasPx);
                } else {
                    writePointGeometry(object, updatedPoint, state.canvasPx);
                }
                updated = true;
            } else if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
                const DrawingObject *storeObject = state.store.find({objectId});
                const LineGeometry *storeLine = storeObject == nullptr ? nullptr : std::get_if<LineGeometry>(&storeObject->geometry);
                double x1 = storeLine == nullptr ? object.value("x1").toDouble() * canvas : storeLine->a.x * canvas;
                double y1 = storeLine == nullptr ? object.value("y1").toDouble() * canvas : storeLine->a.y * canvas;
                double x2 = storeLine == nullptr ? object.value("x2").toDouble() * canvas : storeLine->b.x * canvas;
                double y2 = storeLine == nullptr ? object.value("y2").toDouble() * canvas : storeLine->b.y * canvas;
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
                const LineGeometry updatedLine{{x1 / canvas, y1 / canvas}, {x2 / canvas, y2 / canvas}};
                if (storeObject != nullptr) {
                    if (storeLine == nullptr || !state.store.updateGeometry({objectId}, updatedLine)) {
                        state.errors.append("update_object could not update typed line: " + objectId);
                        return;
                    }
                    object = state.store.serializeObject({objectId}, state.canvasPx);
                } else {
                    writeLineGeometry(object, updatedLine, state.canvasPx);
                }
                updated = true;
            } else if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
                const DrawingObject *storeObject = state.store.find({objectId});
                const CircleGeometry *storeCircle = storeObject == nullptr ? nullptr : std::get_if<CircleGeometry>(&storeObject->geometry);
                const ArcGeometry *storeArc = storeObject == nullptr ? nullptr : std::get_if<ArcGeometry>(&storeObject->geometry);
                double cx = object.value("cx").toDouble() * canvas;
                double cy = object.value("cy").toDouble() * canvas;
                double radius = object.value("radius_px").toDouble();
                if (storeCircle != nullptr) {
                    cx = storeCircle->center.x * canvas;
                    cy = storeCircle->center.y * canvas;
                    radius = storeCircle->radius * canvas;
                } else if (storeArc != nullptr) {
                    cx = storeArc->center.x * canvas;
                    cy = storeArc->center.y * canvas;
                    radius = storeArc->radius * canvas;
                }
                double startAngleDeg = storeArc != nullptr ? storeArc->startAngleDeg : object.value(QStringLiteral("start_angle_deg")).toDouble();
                double endAngleDeg = storeArc != nullptr ? storeArc->endAngleDeg : object.value(QStringLiteral("end_angle_deg")).toDouble(90.0);
                if (field == QStringLiteral("cx_px")) {
                    cx = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("cy_px")) {
                    cy = clampedPx(value, state.canvasPx);
                } else if (field == QStringLiteral("radius_px")) {
                    radius = std::clamp(positivePx(value), 1.0, canvas);
                } else if (kind == QStringLiteral("arc") && field == QStringLiteral("start_angle_deg")) {
                    startAngleDeg = value;
                } else if (kind == QStringLiteral("arc") && field == QStringLiteral("end_angle_deg")) {
                    endAngleDeg = value;
                } else {
                    state.errors.append("update_object unsupported circle field: " + field);
                    return;
                }
                if (kind == QStringLiteral("arc")) {
                    const ArcGeometry updatedArc{{cx / canvas, cy / canvas}, radius / canvas, startAngleDeg, endAngleDeg};
                    if (storeObject != nullptr) {
                        if (storeArc == nullptr || !state.store.updateGeometry({objectId}, updatedArc)) {
                            state.errors.append("update_object could not update typed arc: " + objectId);
                            return;
                        }
                        object = state.store.serializeObject({objectId}, state.canvasPx);
                    } else {
                        writeArcGeometry(object, updatedArc, state.canvasPx);
                    }
                } else {
                    const CircleGeometry updatedCircle{{cx / canvas, cy / canvas}, radius / canvas};
                    if (storeObject != nullptr) {
                        if (storeCircle == nullptr || !state.store.updateGeometry({objectId}, updatedCircle)) {
                            state.errors.append("update_object could not update typed circle: " + objectId);
                            return;
                        }
                        object = state.store.serializeObject({objectId}, state.canvasPx);
                    } else {
                        writeCircleGeometry(object, updatedCircle, state.canvasPx);
                    }
                }
                updated = true;
            } else if (rectangleLikeKind(kind)) {
                const DrawingObject *storeObject = state.store.find({objectId});
                const RectangleGeometry *storeRectangle = storeObject == nullptr ? nullptr : std::get_if<RectangleGeometry>(&storeObject->geometry);
                double x = storeRectangle == nullptr ? object.value("x").toDouble() * canvas : storeRectangle->origin.x * canvas;
                double y = storeRectangle == nullptr ? object.value("y").toDouble() * canvas : storeRectangle->origin.y * canvas;
                double width = storeRectangle == nullptr ? object.value("width").toDouble() * canvas : storeRectangle->width * canvas;
                double height = storeRectangle == nullptr ? object.value("height").toDouble() * canvas : storeRectangle->height * canvas;
                double rotation = storeRectangle == nullptr ? object.value("rotation_deg").toDouble() : storeRectangle->rotationDeg;
                if (field == QStringLiteral("x_px")) {
                    x = value;
                } else if (field == QStringLiteral("y_px")) {
                    y = value;
                } else if (field == QStringLiteral("width_px")) {
                    width = value;
                } else if (field == QStringLiteral("height_px")) {
                    height = value;
                } else if (field == QStringLiteral("rotation_deg")) {
                    rotation = value;
                } else {
                    state.errors.append("update_object unsupported rectangle field: " + field);
                    return;
                }
                const RectangleGeometry updatedRectangle{{x / canvas, y / canvas}, width / canvas, height / canvas, rotation};
                if (storeObject != nullptr) {
                    if (storeRectangle == nullptr || !state.store.updateGeometry({objectId}, updatedRectangle)) {
                        state.errors.append("update_object could not update typed rectangle: " + objectId);
                        return;
                    }
                    object = state.store.serializeObject({objectId}, state.canvasPx);
                } else {
                    rebuildRectangle(object, state.canvasPx, x, y, width, height);
                    object.insert("rotation_deg", rotation);
                    writeRectangleGeometry(object, rectangleGeometryFromObject(object), state.canvasPx);
                }
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

bool isEditableObjectMetadataField(const QString &field) {
    return field == QStringLiteral("role")
        || field == QStringLiteral("material")
        || field == QStringLiteral("intent")
        || field == QStringLiteral("export_group")
        || field == QStringLiteral("tags");
}

QJsonArray normalizedMetadataTags(const QJsonValue &value) {
    QJsonArray tags;
    if (value.isArray()) {
        for (const QJsonValue tagValue : value.toArray()) {
            const QString tag = tagValue.toString().trimmed();
            if (!tag.isEmpty() && !tags.contains(tag)) {
                tags.append(tag);
            }
        }
        return tags;
    }
    const QStringList parts = value.toString().split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString tag = part.trimmed();
        if (!tag.isEmpty() && !tags.contains(tag)) {
            tags.append(tag);
        }
    }
    return tags;
}

void setObjectMetadataField(State &state, const QString &objectId, const QString &field, const QJsonValue &value) {
    if (objectId.isEmpty() || !isEditableObjectMetadataField(field)) {
        state.pendingPoint = {};
        return;
    }

    bool updated = false;
    QJsonArray updatedObjects;
    for (const QJsonValue objectValue : state.generatedObjects) {
        QJsonObject object = objectValue.toObject();
        if (object.value(QStringLiteral("id")).toString() == objectId) {
            if (field == QStringLiteral("tags")) {
                const QJsonArray tags = normalizedMetadataTags(value);
                if (tags.isEmpty()) {
                    object.remove(field);
                } else {
                    object.insert(field, tags);
                }
            } else {
                const QString text = value.toString().trimmed();
                if (text.isEmpty()) {
                    object.remove(field);
                } else {
                    object.insert(field, text);
                }
            }
            if (!replaceStoreAttributesFromJson(state, object)) {
                state.errors.append(QStringLiteral("set_object_metadata could not update typed attributes: ") + objectId);
                return;
            }
            updated = true;
        }
        updatedObjects.append(object);
    }
    if (!updated) {
        state.errors.append(QStringLiteral("set_object_metadata could not find generated object: ") + objectId);
        return;
    }
    state.generatedObjects = updatedObjects;
    state.selectedLayer = kScriptLayer;
    selectObject(state, objectId);
    state.pendingPoint = {};
}

} // namespace drawing_core
