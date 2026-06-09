#include "widgets/DrawingCanvasProjectedPointer.h"

#include <QVariantMap>

#include <cassert>
#include <limits>

using namespace drawing_canvas;

namespace {

QVariantMap modelWithPointer(const QVariantMap &pointer)
{
    return QVariantMap{{QStringLiteral("pointer"), pointer}};
}

QVariantMap snapped(double x, double y)
{
    return QVariantMap{
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
    };
}

} // namespace

int main()
{
    const DrawingCanvasProjectedPointer missing = projectedPointer({});
    assert(!missing.visible);

    const DrawingCanvasProjectedPointer valid = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), snapped(0.25, 0.75)},
        {QStringLiteral("kind"), QStringLiteral("object")},
        {QStringLiteral("source"), QStringLiteral("endpoint")},
        {QStringLiteral("inside_drawable"), true},
        {QStringLiteral("label"), QStringLiteral("endpoint")},
        {QStringLiteral("source_object_id"), QStringLiteral("line_1")},
    }));
    assert(valid.visible);
    assert(valid.snappedX == 0.25);
    assert(valid.snappedY == 0.75);
    assert(valid.kind == QStringLiteral("object"));
    assert(valid.source == QStringLiteral("endpoint"));
    assert(valid.insideDrawable);
    assert(valid.label == QStringLiteral("endpoint"));
    assert(valid.sourceObjectId == QStringLiteral("line_1"));

    const DrawingCanvasProjectedPointer badX = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), QVariantMap{
            {QStringLiteral("x"), std::numeric_limits<double>::quiet_NaN()},
            {QStringLiteral("y"), 0.5},
        }},
    }));
    assert(!badX.visible);

    const DrawingCanvasProjectedPointer badY = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), QVariantMap{
            {QStringLiteral("x"), 0.5},
            {QStringLiteral("y"), QStringLiteral("bad")},
        }},
    }));
    assert(!badY.visible);

    const DrawingCanvasProjectedPointer missingInside = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), snapped(0.1, 0.2)},
    }));
    assert(missingInside.visible);
    assert(!missingInside.insideDrawable);
    assert(missingInside.kind.isEmpty());
    assert(missingInside.source.isEmpty());
    assert(missingInside.label.isEmpty());
    assert(missingInside.sourceObjectId.isEmpty());

    const DrawingCanvasProjectedGuideDragSnapIntent missingGuide = projectedGuideDragSnapIntent({});
    assert(!missingGuide.visible);

    const DrawingCanvasProjectedGuideDragSnapIntent validGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), snapped(0.34, 0.74)},
            {QStringLiteral("snapped_anchor"), snapped(0.33, 0.75)},
            {QStringLiteral("anchor_label"), QStringLiteral("point")},
            {QStringLiteral("source_object_id"), QStringLiteral("point_1")},
            {QStringLiteral("intersection"), true},
        }},
    });
    assert(validGuide.visible);
    assert(validGuide.rawX == 0.34);
    assert(validGuide.rawY == 0.74);
    assert(validGuide.snappedX == 0.33);
    assert(validGuide.snappedY == 0.75);
    assert(validGuide.label == QStringLiteral("point"));
    assert(validGuide.sourceObjectId == QStringLiteral("point_1"));
    assert(validGuide.intersection);

    const DrawingCanvasProjectedGuideDragSnapIntent defaultGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), snapped(0.1, 0.2)},
            {QStringLiteral("snapped_anchor"), snapped(0.3, 0.4)},
            {QStringLiteral("anchor_label"), QString()},
        }},
    });
    assert(defaultGuide.visible);
    assert(defaultGuide.label == QStringLiteral("anchor"));
    assert(defaultGuide.sourceObjectId.isEmpty());
    assert(!defaultGuide.intersection);

    const DrawingCanvasProjectedGuideDragSnapIntent badRawGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), QVariantMap{
                {QStringLiteral("x"), 0.1},
                {QStringLiteral("y"), std::numeric_limits<double>::infinity()},
            }},
            {QStringLiteral("snapped_anchor"), snapped(0.3, 0.4)},
        }},
    });
    assert(!badRawGuide.visible);

    const DrawingCanvasProjectedGuideDragSnapIntent badSnappedGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), snapped(0.1, 0.2)},
            {QStringLiteral("snapped_anchor"), QVariantMap{
                {QStringLiteral("x"), 0.3},
                {QStringLiteral("y"), QStringLiteral("bad")},
            }},
        }},
    });
    assert(!badSnappedGuide.visible);

    return 0;
}
