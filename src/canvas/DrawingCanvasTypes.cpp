#include "DrawingCanvasTypes.h"

#include <algorithm>
#include <cmath>

namespace drawing_canvas {

double finiteNumber(double value, double fallback) {
    return std::isfinite(value) ? value : fallback;
}

double finiteNumber(const QVariant &value, double fallback) {
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && std::isfinite(number) ? number : fallback;
}

double clamp01(double value) {
    return std::clamp(finiteNumber(value, 0.0), 0.0, 1.0);
}

bool isRectangleLike(const QString &kind) {
    return kind == QStringLiteral("rectangle")
        || kind == QStringLiteral("image_reference_frame")
        || kind == QStringLiteral("ascii_crop_frame")
        || kind == QStringLiteral("ascii_cell_region");
}

CanvasPoint rotatedRectCenter(const CanvasObjectView &object) {
    return {
        object.number(QStringLiteral("x")) + object.number(QStringLiteral("width")) / 2.0,
        object.number(QStringLiteral("y")) + object.number(QStringLiteral("height")) / 2.0
    };
}

std::vector<CanvasPoint> rotatedRectCorners(const CanvasObjectView &object) {
    const double x = object.number(QStringLiteral("x"));
    const double y = object.number(QStringLiteral("y"));
    const double width = object.number(QStringLiteral("width"));
    const double height = object.number(QStringLiteral("height"));
    const double cx = x + width / 2.0;
    const double cy = y + height / 2.0;
    const double angle = object.number(QStringLiteral("rotation_deg")) * 3.14159265358979323846 / 180.0;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    std::vector<CanvasPoint> corners = {
        {x, y},
        {x + width, y},
        {x, y + height},
        {x + width, y + height}
    };
    for (CanvasPoint &corner : corners) {
        const double dx = corner.x - cx;
        const double dy = corner.y - cy;
        corner.x = cx + dx * cosA - dy * sinA;
        corner.y = cy + dx * sinA + dy * cosA;
    }
    return corners;
}

CanvasPoint unrotatePointForRect(const CanvasObjectView &object, double x, double y) {
    const CanvasPoint center = rotatedRectCenter(object);
    const double angle = -object.number(QStringLiteral("rotation_deg")) * 3.14159265358979323846 / 180.0;
    const double dx = finiteNumber(x, 0.0) - center.x;
    const double dy = finiteNumber(y, 0.0) - center.y;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    return {
        center.x + dx * cosA - dy * sinA,
        center.y + dx * sinA + dy * cosA
    };
}

QString CanvasObjectView::id() const {
    return values.value(QStringLiteral("id")).toString();
}

QString CanvasObjectView::kind() const {
    return values.value(QStringLiteral("kind")).toString();
}

bool CanvasObjectView::visible() const {
    return values.value(QStringLiteral("visible"), true).toBool();
}

double CanvasObjectView::number(const QString &field, double fallback) const {
    return finiteNumber(values.value(field), fallback);
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

QVariantMap boundsToVariant(const CanvasBounds &bounds) {
    return {
        {QStringLiteral("ok"), bounds.ok},
        {QStringLiteral("minX"), finiteNumber(bounds.minX, 0.0)},
        {QStringLiteral("minY"), finiteNumber(bounds.minY, 0.0)},
        {QStringLiteral("maxX"), finiteNumber(bounds.maxX, 0.0)},
        {QStringLiteral("maxY"), finiteNumber(bounds.maxY, 0.0)}
    };
}

QVariantMap boardBoundsToVariant(const BoardBounds &bounds) {
    return {
        {QStringLiteral("x"), finiteNumber(bounds.x, 0.0)},
        {QStringLiteral("y"), finiteNumber(bounds.y, 0.0)},
        {QStringLiteral("size"), std::max(0.000001, finiteNumber(bounds.size, 1.0))}
    };
}

QVariantMap hitResultToVariant(const HitResult &hit) {
    return {
        {QStringLiteral("ok"), hit.ok},
        {QStringLiteral("objectId"), hit.objectId},
        {QStringLiteral("kind"), hit.kind},
        {QStringLiteral("distance"), finiteNumber(hit.distance, 999.0)}
    };
}

QVariantMap snapResultToVariant(const SnapResult &snap) {
    return {
        {QStringLiteral("x"), finiteNumber(snap.x, 0.0)},
        {QStringLiteral("y"), finiteNumber(snap.y, 0.0)},
        {QStringLiteral("kind"), snap.kind},
        {QStringLiteral("label"), snap.label},
        {QStringLiteral("sourceObjectId"), snap.sourceObjectId},
        {QStringLiteral("sourceKind"), snap.sourceKind},
        {QStringLiteral("stepPx"), finiteNumber(snap.stepPx, 32.0)}
    };
}

QVariantMap objectToMap(const QVariant &value) {
    return value.toMap();
}

std::vector<CanvasObjectView> objectsFromVariantList(const QVariantList &objects) {
    std::vector<CanvasObjectView> result;
    result.reserve(static_cast<std::size_t>(objects.size()));
    for (const QVariant &object : objects) {
        result.push_back({object.toMap()});
    }
    return result;
}

} // namespace drawing_canvas
