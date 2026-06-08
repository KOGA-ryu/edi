#include "DrawingCanvasProjection.h"
#include "DrawingCanvasHandles.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drawing_canvas {

CanvasBounds emptyBounds() {
    return {};
}

CanvasBounds includePointInBounds(CanvasBounds bounds, double x, double y) {
    const double px = finiteNumber(x, std::numeric_limits<double>::quiet_NaN());
    const double py = finiteNumber(y, std::numeric_limits<double>::quiet_NaN());
    if (!std::isfinite(px) || !std::isfinite(py)) {
        return bounds;
    }
    if (!bounds.ok) {
        bounds.ok = true;
        bounds.minX = px;
        bounds.maxX = px;
        bounds.minY = py;
        bounds.maxY = py;
        return bounds;
    }
    bounds.minX = std::min(bounds.minX, px);
    bounds.maxX = std::max(bounds.maxX, px);
    bounds.minY = std::min(bounds.minY, py);
    bounds.maxY = std::max(bounds.maxY, py);
    return bounds;
}

bool boundsIntersects(const CanvasBounds &bounds, double minX, double minY, double maxX, double maxY) {
    if (!bounds.ok) {
        return false;
    }
    return bounds.maxX >= minX && bounds.minX <= maxX && bounds.maxY >= minY && bounds.minY <= maxY;
}

CanvasBounds normalizedObjectBounds(const CanvasObjectView &object) {
    CanvasBounds result = emptyBounds();
    const QString kind = object.kind();
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe") || kind == QStringLiteral("anchor")) {
        return includePointInBounds(result, object.number(QStringLiteral("x")), object.number(QStringLiteral("y")));
    }
    if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        result = includePointInBounds(result, object.number(QStringLiteral("x1")), object.number(QStringLiteral("y1")));
        return includePointInBounds(result, object.number(QStringLiteral("x2")), object.number(QStringLiteral("y2")));
    }
    if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        const double cx = object.number(QStringLiteral("cx"));
        const double cy = object.number(QStringLiteral("cy"));
        const double radius = std::max(0.0, object.number(QStringLiteral("radius")));
        result = includePointInBounds(result, cx - radius, cy - radius);
        return includePointInBounds(result, cx + radius, cy + radius);
    }
    if (isRectangleLike(kind)) {
        for (const HandleDescriptor &corner : rotatedRectCorners(object)) {
            result = includePointInBounds(result, corner.x, corner.y);
        }
        return result;
    }
    if (kind == QStringLiteral("polyline") || kind == QStringLiteral("polygon")) {
        for (const CanvasPoint &point : object.points()) {
            result = includePointInBounds(result, point.x, point.y);
        }
    }
    return result;
}

bool objectIntersectsBounds(const CanvasObjectView &object, double minX, double minY, double maxX, double maxY) {
    return boundsIntersects(normalizedObjectBounds(object), minX, minY, maxX, maxY);
}

QStringList selectedObjectIds(const QVariantMap &doc) {
    QStringList result;
    for (const QVariant &id : doc.value(QStringLiteral("selected_object_ids")).toList()) {
        result.push_back(id.toString());
    }
    return result;
}

bool selectedObject(const QVariantMap &doc, const QString &objectId) {
    const QStringList selectedIds = selectedObjectIds(doc);
    if (!selectedIds.isEmpty()) {
        return selectedIds.contains(objectId);
    }
    return doc.value(QStringLiteral("selected_object_id")).toString() == objectId;
}

bool selectedLayer(const QVariantMap &doc, const QString &layerId) {
    return doc.value(QStringLiteral("selected_layer_id")).toString() == layerId;
}

CanvasBounds combinedSelectionBounds(const QVariantMap &doc) {
    const QStringList selectedIds = selectedObjectIds(doc);
    if (selectedIds.size() <= 1) {
        return emptyBounds();
    }
    CanvasBounds selectedBounds = emptyBounds();
    const QVariantList layers = doc.value(QStringLiteral("layers")).toList();
    for (const QVariant &layerValue : layers) {
        const QVariantMap layer = layerValue.toMap();
        if (layer.value(QStringLiteral("visible"), true).toBool() == false) {
            continue;
        }
        const QVariantList objects = layer.value(QStringLiteral("objects")).toList();
        for (const QVariant &objectValue : objects) {
            const CanvasObjectView object {objectValue.toMap()};
            const QString objectId = object.id();
            if (!objectId.startsWith(QStringLiteral("script_")) || !selectedIds.contains(objectId)) {
                continue;
            }
            const CanvasBounds objectBounds = normalizedObjectBounds(object);
            if (!objectBounds.ok) {
                continue;
            }
            selectedBounds = includePointInBounds(selectedBounds, objectBounds.minX, objectBounds.minY);
            selectedBounds = includePointInBounds(selectedBounds, objectBounds.maxX, objectBounds.maxY);
        }
    }
    return selectedBounds;
}

} // namespace drawing_canvas
