#include "widgets/DrawingCanvasProjectedDocument.h"

#include "widgets/DrawingCanvasProjectedObject.h"

#include <QMetaType>
#include <QVariantList>

namespace drawing_canvas {
namespace {

QVariantList strictList(const QVariant &value)
{
    return value.metaType().id() == QMetaType::QVariantList ? value.toList() : QVariantList{};
}

QVariantMap strictMap(const QVariant &value)
{
    return value.metaType().id() == QMetaType::QVariantMap ? value.toMap() : QVariantMap{};
}

} // namespace

DrawingCanvasProjectedDocumentSurface projectedDocumentSurface(const QVariantMap &model)
{
    DrawingCanvasProjectedDocumentSurface surface;
    surface.drawingObjects = strictList(model.value(QStringLiteral("drawing_objects")));
    surface.previewObject = strictMap(model.value(QStringLiteral("preview_object")));
    surface.plotSummary = strictMap(model.value(QStringLiteral("plot_summary")));
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
