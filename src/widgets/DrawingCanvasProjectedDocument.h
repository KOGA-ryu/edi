#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace drawing_canvas {

struct DrawingCanvasProjectedDocumentSurface {
    QVariantList drawingObjects;
    QVariantMap previewObject;
    QVariantMap plotSummary;
};

DrawingCanvasProjectedDocumentSurface projectedDocumentSurface(const QVariantMap &model);
QVariantMap projectedObjectById(const QVariantMap &model, const QString &objectId);

} // namespace drawing_canvas
