#include "widgets/DrawingCanvasProjectedDocument.h"

#include "widgets/DrawingCanvasProjectedObject.h"

#include <QVariantList>

namespace drawing_canvas {

DrawingCanvasProjectedDocumentSurface projectedDocumentSurface(const QVariantMap &model)
{
    DrawingCanvasProjectedDocumentSurface surface;
    surface.drawingObjects = model.value(QStringLiteral("drawing_objects")).toList();
    surface.previewObject = model.value(QStringLiteral("preview_object")).toMap();
    surface.plotSummary = model.value(QStringLiteral("plot_summary")).toMap();
    return surface;
}

QVariantMap projectedObjectById(const QVariantMap &model, const QString &objectId)
{
    if (objectId.isEmpty()) {
        return {};
    }

    for (const QVariant &value : projectedDocumentSurface(model).drawingObjects) {
        const QVariantMap object = value.toMap();
        if (object.isEmpty()) {
            continue;
        }
        if (projectedObjectSummary(object).id == objectId) {
            return object;
        }
    }
    return {};
}

} // namespace drawing_canvas
