#include "widgets/DrawingCanvasProjectedDocument.h"

#include "widgets/DrawingCanvasProjectedObject.h"

#include <QVariantList>

namespace drawing_canvas {

QVariantMap projectedObjectById(const QVariantMap &model, const QString &objectId)
{
    if (objectId.isEmpty()) {
        return {};
    }

    for (const QVariant &value : model.value(QStringLiteral("drawing_objects")).toList()) {
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
