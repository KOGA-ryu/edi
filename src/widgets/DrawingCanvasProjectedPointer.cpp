#include "widgets/DrawingCanvasProjectedPointer.h"

#include <cmath>

namespace drawing_canvas {
namespace {

bool readFinite(const QVariantMap &source, const QString &field, double &target)
{
    bool ok = false;
    const double value = source.value(field).toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        return false;
    }
    target = value;
    return true;
}

} // namespace

DrawingCanvasProjectedPointer projectedPointer(const QVariantMap &model)
{
    const QVariantMap pointer = model.value(QStringLiteral("pointer")).toMap();
    if (pointer.isEmpty()) {
        return {};
    }

    DrawingCanvasProjectedPointer projected;
    const QVariantMap snapped = pointer.value(QStringLiteral("snapped")).toMap();
    projected.visible = readFinite(snapped, QStringLiteral("x"), projected.snappedX)
        && readFinite(snapped, QStringLiteral("y"), projected.snappedY);
    if (!projected.visible) {
        return {};
    }

    projected.kind = pointer.value(QStringLiteral("kind")).toString();
    projected.source = pointer.value(QStringLiteral("source")).toString();
    projected.insideDrawable = pointer.value(QStringLiteral("inside_drawable")).toBool();
    projected.label = pointer.value(QStringLiteral("label")).toString();
    projected.sourceObjectId = pointer.value(QStringLiteral("source_object_id")).toString();
    return projected;
}

} // namespace drawing_canvas
