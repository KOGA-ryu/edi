#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include <algorithm>
#include <cmath>

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
    m_lastStableCommands = drawing_core::pendingPointActive(object) ? QJsonArray{} : m_commands;
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

void DrawingDocumentController::updateObjectMetadataField(const QString &objectId, const QString &field, const QVariant &value) {
    if (objectId.isEmpty() || field.isEmpty()) {
        return;
    }
    QJsonObject command;
    command.insert("cmd", "set_object_metadata");
    command.insert("object_id", objectId);
    command.insert("field", field);
    command.insert("value", QJsonValue::fromVariant(value));
    applyCommand(command);
}

void DrawingDocumentController::updateSelectedObjectMetadataField(const QString &field, const QVariant &value) {
    updateObjectMetadataField(selectedObjectId(), field, value);
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
    const double canvasPx = canvas.size() >= 2 ? canvas.at(0).toDouble(drawing_core::kDefaultCanvasPx) : drawing_core::kDefaultCanvasPx;
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
    const double canvasPx = canvas.size() >= 2 ? canvas.at(0).toDouble(drawing_core::kDefaultCanvasPx) : drawing_core::kDefaultCanvasPx;
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
    m_lastStableCommands = drawing_core::pendingPointActive(result.model) ? QJsonArray{} : m_commands;
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
    const bool undoable = drawing_core::undoableInteractiveCommand(command);
    const bool wasPending = drawing_core::pendingPointActive(m_model);
    const int beforeObjectCount = drawing_core::generatedObjectCount(m_model);
    const QJsonArray beforeCommands = m_commands;
    const QJsonArray undoSnapshot = wasPending ? m_lastStableCommands : beforeCommands;
    const QString name = drawing_core::stringAt(command, QStringLiteral("cmd"));

    if (m_moveGestureActive && (name == QStringLiteral("move_object") || name == QStringLiteral("move_objects")) && !m_commands.isEmpty()) {
        const int lastIndex = m_commands.size() - 1;
        QJsonObject previous = m_commands.at(lastIndex).toObject();
        const bool sameMoveObject = name == QStringLiteral("move_object")
            && drawing_core::stringAt(previous, QStringLiteral("cmd")) == QStringLiteral("move_object")
            && drawing_core::stringAt(previous, QStringLiteral("object_id")) == drawing_core::stringAt(command, QStringLiteral("object_id"));
        const bool sameMoveObjects = name == QStringLiteral("move_objects")
            && drawing_core::stringAt(previous, QStringLiteral("cmd")) == QStringLiteral("move_objects")
            && previous.value(QStringLiteral("object_ids")).toArray() == command.value(QStringLiteral("object_ids")).toArray();
        if (sameMoveObject || sameMoveObjects) {
            previous.insert(QStringLiteral("dx"), drawing_core::numberAt(previous, QStringLiteral("dx")) + drawing_core::numberAt(command, QStringLiteral("dx")));
            previous.insert(QStringLiteral("dy"), drawing_core::numberAt(previous, QStringLiteral("dy")) + drawing_core::numberAt(command, QStringLiteral("dy")));
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
            if (drawing_core::stringAt(previous, QStringLiteral("cmd")) == QStringLiteral("update_object")
                    && drawing_core::stringAt(previous, QStringLiteral("object_id")) == drawing_core::stringAt(command, QStringLiteral("object_id"))
                    && drawing_core::stringAt(previous, QStringLiteral("field")) == drawing_core::stringAt(command, QStringLiteral("field"))) {
                previous.insert(QStringLiteral("value"), drawing_core::numberAt(command, QStringLiteral("value")));
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
    const bool nowPending = drawing_core::pendingPointActive(m_model);
    const int afterObjectCount = drawing_core::generatedObjectCount(m_model);
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
    if (!drawing_core::pendingPointActive(m_model)) {
        m_lastStableCommands = m_commands;
    }
    ++m_revision;
    emit modelChanged();
}

QJsonObject DrawingDocumentController::scriptEnvelope() const {
    QJsonObject script;
    script.insert("script_id", "interactive_drawing_session_v0");
    script.insert("canvas_px", drawing_core::pointArray(drawing_core::kDefaultCanvasPx, drawing_core::kDefaultCanvasPx));
    script.insert("commands", m_commands);
    script.insert("allow_pending", true);
    return script;
}
