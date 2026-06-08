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

QVariantMap handleToVariant(const HandleDescriptor &handle) {
    QVariantList updateFields;
    for (const QString &field : handle.updateFields) {
        updateFields.push_back(field);
    }

    QVariantMap result {
        {QStringLiteral("id"), handle.id},
        {QStringLiteral("role"), handle.role},
        {QStringLiteral("cursor"), handle.cursor},
        {QStringLiteral("field"), handle.field},
        {QStringLiteral("updateFields"), updateFields},
        {QStringLiteral("x"), finiteNumber(handle.x, 0.0)},
        {QStringLiteral("y"), finiteNumber(handle.y, 0.0)}
    };
    if (handle.readOnly) {
        result.insert(QStringLiteral("readOnly"), true);
    }
    if (!handle.visible) {
        result.insert(QStringLiteral("visible"), false);
    }
    if (handle.hasAnchor) {
        result.insert(QStringLiteral("anchorX"), finiteNumber(handle.anchorX, 0.0));
        result.insert(QStringLiteral("anchorY"), finiteNumber(handle.anchorY, 0.0));
    }
    return result;
}

QVariantList handlesToVariant(const std::vector<HandleDescriptor> &handles) {
    QVariantList result;
    result.reserve(static_cast<qsizetype>(handles.size()));
    for (const HandleDescriptor &handle : handles) {
        result.push_back(handleToVariant(handle));
    }
    return result;
}

QVariantMap updatePlanToVariant(const HandleUpdatePlan &plan) {
    QVariantList updates;
    for (const FieldUpdate &update : plan.updates) {
        updates.push_back(QVariantMap {
            {QStringLiteral("field"), update.field},
            {QStringLiteral("value"), finiteNumber(update.value, 0.0)}
        });
    }
    return {
        {QStringLiteral("ok"), plan.ok},
        {QStringLiteral("updates"), updates}
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
