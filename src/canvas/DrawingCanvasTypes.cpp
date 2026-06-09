#include "DrawingCanvasTypes.h"

#include <cmath>
#include <QVariantList>

namespace drawing_canvas {

double finiteNumber(double value, double fallback) {
    return std::isfinite(value) ? value : fallback;
}

double finiteNumber(const QVariant &value, double fallback) {
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && std::isfinite(number) ? number : fallback;
}

std::vector<CanvasPoint> CanvasObjectView::points() const {
    std::vector<CanvasPoint> result;
    const QVariantList source = values.value(QStringLiteral("points")).toList();
    result.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        if (entry.typeId() == QMetaType::QVariantList) {
            const QVariantList list = entry.toList();
            if (list.size() >= 2) {
                result.push_back({finiteNumber(list.at(0), 0.0), finiteNumber(list.at(1), 0.0)});
            }
            continue;
        }
        const QVariantMap map = entry.toMap();
        if (!map.isEmpty()) {
            result.push_back({finiteNumber(map.value(QStringLiteral("x")), 0.0),
                              finiteNumber(map.value(QStringLiteral("y")), 0.0)});
        }
    }
    return result;
}

CanvasPoint pointFromVariant(const QVariant &value) {
    const QVariantMap map = value.toMap();
    return {finiteNumber(map.value(QStringLiteral("x")), 0.0),
            finiteNumber(map.value(QStringLiteral("y")), 0.0)};
}

QVariantMap pointToVariant(const CanvasPoint &point) {
    return {
        {QStringLiteral("x"), finiteNumber(point.x, 0.0)},
        {QStringLiteral("y"), finiteNumber(point.y, 0.0)}
    };
}

} // namespace drawing_canvas
