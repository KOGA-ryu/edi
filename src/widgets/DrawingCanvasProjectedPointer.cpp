#include "widgets/DrawingCanvasProjectedPointer.h"

#include "widgets/DrawingCanvasValues.h"

namespace drawing_canvas {
namespace {

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

DrawingCanvasProjectedGuideDragSnapIntent projectedGuideDragSnapIntent(const QVariantMap &model)
{
    const QVariantMap snap = model.value(QStringLiteral("guide_drag_snap")).toMap();
    if (snap.isEmpty()) {
        return {};
    }

    const QVariantMap rawAnchor = snap.value(QStringLiteral("raw_anchor")).toMap();
    const QVariantMap snappedAnchor = snap.value(QStringLiteral("snapped_anchor")).toMap();

    DrawingCanvasProjectedGuideDragSnapIntent projected;
    projected.visible = readFinite(rawAnchor, QStringLiteral("x"), projected.rawX)
        && readFinite(rawAnchor, QStringLiteral("y"), projected.rawY)
        && readFinite(snappedAnchor, QStringLiteral("x"), projected.snappedX)
        && readFinite(snappedAnchor, QStringLiteral("y"), projected.snappedY);
    if (!projected.visible) {
        return {};
    }

    projected.label = snap.value(QStringLiteral("anchor_label"), QStringLiteral("anchor")).toString();
    if (projected.label.isEmpty()) {
        projected.label = QStringLiteral("anchor");
    }
    projected.sourceObjectId = snap.value(QStringLiteral("source_object_id")).toString();
    projected.intersection = snap.value(QStringLiteral("intersection")).toBool();
    return projected;
}

} // namespace drawing_canvas
