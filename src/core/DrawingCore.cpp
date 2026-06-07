#include "DrawingCore.h"
#include "DrawingCoreInternal.h"

#include <QJsonDocument>

// Refactor notes:
// - Geometry helpers, object mutation, command dispatch, model building, SVG export,
//   and document-controller glue now live in focused core implementation files.

DrawingCoreResult DrawingCore::runScript(const QJsonObject &script) {
    drawing_core::State state;
    state.scriptId = drawing_core::stringAt(script, "script_id", "unnamed_script");
    const QJsonArray canvas = drawing_core::arrayAt(script, "canvas_px");
    if (canvas.size() >= 2 && canvas.at(0).toInt() == canvas.at(1).toInt() && canvas.at(0).toInt() > 0) {
        state.canvasPx = canvas.at(0).toInt();
    }

    const QJsonArray commands = drawing_core::arrayAt(script, "commands");
    for (const QJsonValue value : commands) {
        const QJsonObject command = value.toObject();
        state.commandLog.append(command);
        if (!drawing_core::runCommand(state, command)) {
            state.errors.append("unsupported command: " + drawing_core::stringAt(command, "cmd"));
        }
    }
    if (state.pendingPoint.ok && !script.value("allow_pending").toBool(false)) {
        state.errors.append("line_polyline ended with an unmatched pending point");
    }

    DrawingCoreResult result;
    result.model = drawing_core::buildModel(state);
    result.svg = modelToSvg(result.model);
    return result;
}

QString DrawingCore::modelToJson(const QJsonObject &model) {
    return QString::fromUtf8(QJsonDocument(model).toJson(QJsonDocument::Indented));
}
