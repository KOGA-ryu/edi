#include "widgets/DrawingCanvasProjectedObject.h"

#include "widgets/DrawingCanvasGestureState.h"

#include <QVariantList>

#include <algorithm>

namespace drawing_canvas {
namespace {

DrawingCanvasProjectedHandleShape handleShapeFromString(const QString &shape)
{
    if (shape == QStringLiteral("square")) {
        return DrawingCanvasProjectedHandleShape::Square;
    }
    if (shape == QStringLiteral("diamond")) {
        return DrawingCanvasProjectedHandleShape::Diamond;
    }
    return DrawingCanvasProjectedHandleShape::Circle;
}

} // namespace

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

std::vector<DrawingCanvasProjectedHandle> projectedObjectHandles(const QVariantMap &object)
{
    std::vector<DrawingCanvasProjectedHandle> result;
    const QVariantList source = object.value(QStringLiteral("edit_handles")).toList();
    result.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        const QVariantMap handle = entry.toMap();
        const QString id = handle.value(QStringLiteral("id")).toString();
        if (handle.isEmpty() || id.isEmpty()) {
            continue;
        }

        DrawingCanvasProjectedHandle projected;
        projected.id = id;
        projected.x = finiteNumber(handle.value(QStringLiteral("x")), 0.0);
        projected.y = finiteNumber(handle.value(QStringLiteral("y")), 0.0);
        projected.editable = handle.value(
            QStringLiteral("editable"),
            !handle.value(QStringLiteral("read_only")).toBool()).toBool();
        projected.hasAnchor = handle.value(QStringLiteral("has_anchor")).toBool();
        if (projected.hasAnchor) {
            projected.anchorX = finiteNumber(handle.value(QStringLiteral("anchor_x")), 0.0);
            projected.anchorY = finiteNumber(handle.value(QStringLiteral("anchor_y")), 0.0);
        }
        projected.sizePx = std::max(2.0, finiteNumber(handle.value(QStringLiteral("size_px")), 8.0));
        projected.hitTolerancePx = std::max(0.0, finiteNumber(handle.value(QStringLiteral("hit_tolerance_px")), 14.0));
        projected.shape = handleShapeFromString(handle.value(QStringLiteral("shape"), QStringLiteral("circle")).toString());
        result.push_back(projected);
    }
    return result;
}

} // namespace drawing_canvas
