#include "canvas/DrawingCanvasHandles.h"

#include <QVariantMap>

#include <cassert>
#include <cmath>
#include <vector>

using namespace drawing_canvas;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

QVariantMap settings()
{
    return {
        {QStringLiteral("canvasSizePx"), 1000.0},
        {QStringLiteral("handleHitTolerancePx"), 12.0},
        {QStringLiteral("rotateHandleHitTolerancePx"), 18.0},
        {QStringLiteral("rotateHandleOffsetPx"), 30.0},
    };
}

HandleDescriptor handleById(const std::vector<HandleDescriptor> &handles, const QString &id)
{
    for (const HandleDescriptor &handle : handles) {
        if (handle.id == id) {
            return handle;
        }
    }
    return {};
}

} // namespace

int main()
{
    const QVariantMap dimensionMap {
        {QStringLiteral("kind"), QStringLiteral("dimension")},
        {QStringLiteral("x1"), 0.1},
        {QStringLiteral("y1"), 0.2},
        {QStringLiteral("x2"), 0.5},
        {QStringLiteral("y2"), 0.2},
        {QStringLiteral("label_x"), 0.3},
        {QStringLiteral("label_y"), 0.25},
    };
    const CanvasObjectView dimension {dimensionMap};
    const QVariantMap config = settings();

    const std::vector<HandleDescriptor> handles = visibleHandlesForObject(dimension, config);
    assert(handles.size() == 3);

    const HandleDescriptor start = handleById(handles, QStringLiteral("dimension_start"));
    assert(start.id == QStringLiteral("dimension_start"));
    assert(start.role == QStringLiteral("endpoint"));
    assert(start.updateFields.contains(QStringLiteral("x1_px")));
    assert(start.updateFields.contains(QStringLiteral("y1_px")));
    assert(nearlyEqual(start.x, 0.1));
    assert(nearlyEqual(start.y, 0.2));

    const HandleDescriptor end = handleById(handles, QStringLiteral("dimension_end"));
    assert(end.id == QStringLiteral("dimension_end"));
    assert(end.role == QStringLiteral("endpoint"));
    assert(end.updateFields.contains(QStringLiteral("x2_px")));
    assert(end.updateFields.contains(QStringLiteral("y2_px")));
    assert(nearlyEqual(end.x, 0.5));
    assert(nearlyEqual(end.y, 0.2));

    const HandleDescriptor offset = handleById(handles, QStringLiteral("dimension_offset"));
    assert(offset.id == QStringLiteral("dimension_offset"));
    assert(offset.role == QStringLiteral("offset"));
    assert(offset.updateFields.contains(QStringLiteral("offset_px")));
    assert(offset.hasAnchor);
    assert(nearlyEqual(offset.x, 0.3));
    assert(nearlyEqual(offset.y, 0.25));
    assert(nearlyEqual(offset.anchorX, 0.3));
    assert(nearlyEqual(offset.anchorY, 0.2));

    const HandleUpdatePlan startPlan = handleUpdatePlan(dimension, QStringLiteral("dimension_start"), {0.2, 0.4}, config);
    assert(startPlan.ok);
    assert(startPlan.updates.size() == 2);
    assert(startPlan.updates[0].field == QStringLiteral("x1_px"));
    assert(nearlyEqual(startPlan.updates[0].value, 200.0));
    assert(startPlan.updates[1].field == QStringLiteral("y1_px"));
    assert(nearlyEqual(startPlan.updates[1].value, 400.0));

    const HandleUpdatePlan endPlan = handleUpdatePlan(dimension, QStringLiteral("dimension_end"), {0.8, 0.6}, config);
    assert(endPlan.ok);
    assert(endPlan.updates.size() == 2);
    assert(endPlan.updates[0].field == QStringLiteral("x2_px"));
    assert(nearlyEqual(endPlan.updates[0].value, 800.0));
    assert(endPlan.updates[1].field == QStringLiteral("y2_px"));
    assert(nearlyEqual(endPlan.updates[1].value, 600.0));

    const HandleUpdatePlan positiveOffsetPlan = handleUpdatePlan(dimension, QStringLiteral("dimension_offset"), {0.3, 0.35}, config);
    assert(positiveOffsetPlan.ok);
    assert(positiveOffsetPlan.updates.size() == 1);
    assert(positiveOffsetPlan.updates[0].field == QStringLiteral("offset_px"));
    assert(nearlyEqual(positiveOffsetPlan.updates[0].value, 150.0));

    const HandleUpdatePlan negativeOffsetPlan = handleUpdatePlan(dimension, QStringLiteral("dimension_offset"), {0.3, 0.1}, config);
    assert(negativeOffsetPlan.ok);
    assert(negativeOffsetPlan.updates.size() == 1);
    assert(nearlyEqual(negativeOffsetPlan.updates[0].value, -100.0));

    const HitResult hit = hitHandleAt(dimension, 70.0, 70.0, {10.0, 20.0, 200.0}, config);
    assert(hit.ok);
    assert(hit.kind == QStringLiteral("handle"));
    assert(hit.objectId == QStringLiteral("dimension_offset"));

    const HandleUpdatePlan missingPlan = handleUpdatePlan(dimension, QStringLiteral("dimension_missing"), {0.3, 0.3}, config);
    assert(!missingPlan.ok);

    const QVariantMap polygonMap {
        {QStringLiteral("kind"), QStringLiteral("polygon")},
        {QStringLiteral("points"), QVariantList{
            QVariantMap{{QStringLiteral("x"), 0.1}, {QStringLiteral("y"), 0.1}},
            QVariantMap{{QStringLiteral("x"), 0.4}, {QStringLiteral("y"), 0.1}},
            QVariantMap{{QStringLiteral("x"), 0.4}, {QStringLiteral("y"), 0.4}},
        }},
    };
    const CanvasObjectView polygon {polygonMap};
    const std::vector<HandleDescriptor> polygonHandles = visibleHandlesForObject(polygon, config);
    assert(polygonHandles.size() == 3);
    assert(polygonHandles[0].readOnly);
    assert(!handleUpdatePlan(polygon, polygonHandles[0].id, {0.2, 0.2}, config).ok);

    return 0;
}
