#include "widgets/DrawingCanvasProjectedObject.h"

#include "widgets/DrawingCanvasGestureState.h"

#include <QVariantList>

namespace drawing_canvas {

std::vector<DrawingCanvasProjectedPoint> projectedObjectPoints(const QVariantMap &object)
{
    std::vector<DrawingCanvasProjectedPoint> result;
    const QVariantList source = object.value(QStringLiteral("points")).toList();
    result.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        if (entry.typeId() == QMetaType::QVariantList) {
            const QVariantList list = entry.toList();
            if (list.size() >= 2) {
                result.push_back({finiteNumber(list.at(0), 0.0), finiteNumber(list.at(1), 0.0)});
            }
            continue;
        }
        const QVariantMap point = entry.toMap();
        if (!point.isEmpty()) {
            result.push_back({
                finiteNumber(point.value(QStringLiteral("x")), 0.0),
                finiteNumber(point.value(QStringLiteral("y")), 0.0)
            });
        }
    }
    return result;
}

} // namespace drawing_canvas
