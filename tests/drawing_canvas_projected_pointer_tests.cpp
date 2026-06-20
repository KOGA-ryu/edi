#include "widgets/DrawingCanvasProjectedPointer.h"

#include <QVariantMap>

#include "EdiAssert.h"
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
    EDI_CHECK(!missing.visible);

    const DrawingCanvasProjectedPointer valid = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), snapped(0.25, 0.75)},
        {QStringLiteral("kind"), QStringLiteral("object")},
        {QStringLiteral("source"), QStringLiteral("endpoint")},
        {QStringLiteral("inside_drawable"), true},
        {QStringLiteral("label"), QStringLiteral("endpoint")},
        {QStringLiteral("source_object_id"), QStringLiteral("line_1")},
    }));
    EDI_CHECK(valid.visible);
    EDI_CHECK(valid.snappedX == 0.25);
    EDI_CHECK(valid.snappedY == 0.75);
    EDI_CHECK(valid.kind == QStringLiteral("object"));
    EDI_CHECK(valid.source == QStringLiteral("endpoint"));
    EDI_CHECK(valid.insideDrawable);
    EDI_CHECK(valid.label == QStringLiteral("endpoint"));
    EDI_CHECK(valid.sourceObjectId == QStringLiteral("line_1"));

    const DrawingCanvasProjectedPointer badX = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), QVariantMap{
            {QStringLiteral("x"), std::numeric_limits<double>::quiet_NaN()},
            {QStringLiteral("y"), 0.5},
        }},
    }));
    EDI_CHECK(!badX.visible);

    const DrawingCanvasProjectedPointer badY = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), QVariantMap{
            {QStringLiteral("x"), 0.5},
            {QStringLiteral("y"), QStringLiteral("bad")},
        }},
    }));
    EDI_CHECK(!badY.visible);

    const DrawingCanvasProjectedPointer missingInside = projectedPointer(modelWithPointer(QVariantMap{
        {QStringLiteral("snapped"), snapped(0.1, 0.2)},
    }));
    EDI_CHECK(missingInside.visible);
    EDI_CHECK(!missingInside.insideDrawable);
    EDI_CHECK(missingInside.kind.isEmpty());
    EDI_CHECK(missingInside.source.isEmpty());
    EDI_CHECK(missingInside.label.isEmpty());
    EDI_CHECK(missingInside.sourceObjectId.isEmpty());

    const DrawingCanvasProjectedGuideDragSnapIntent missingGuide = projectedGuideDragSnapIntent({});
    EDI_CHECK(!missingGuide.visible);

    const DrawingCanvasProjectedGuideDragSnapIntent validGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), snapped(0.34, 0.74)},
            {QStringLiteral("snapped_anchor"), snapped(0.33, 0.75)},
            {QStringLiteral("anchor_label"), QStringLiteral("point")},
            {QStringLiteral("source_object_id"), QStringLiteral("point_1")},
            {QStringLiteral("intersection"), true},
        }},
    });
    EDI_CHECK(validGuide.visible);
    EDI_CHECK(validGuide.rawX == 0.34);
    EDI_CHECK(validGuide.rawY == 0.74);
    EDI_CHECK(validGuide.snappedX == 0.33);
    EDI_CHECK(validGuide.snappedY == 0.75);
    EDI_CHECK(validGuide.label == QStringLiteral("point"));
    EDI_CHECK(validGuide.sourceObjectId == QStringLiteral("point_1"));
    EDI_CHECK(validGuide.intersection);

    const DrawingCanvasProjectedGuideDragSnapIntent defaultGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), snapped(0.1, 0.2)},
            {QStringLiteral("snapped_anchor"), snapped(0.3, 0.4)},
            {QStringLiteral("anchor_label"), QString()},
        }},
    });
    EDI_CHECK(defaultGuide.visible);
    EDI_CHECK(defaultGuide.label == QStringLiteral("anchor"));
    EDI_CHECK(defaultGuide.sourceObjectId.isEmpty());
    EDI_CHECK(!defaultGuide.intersection);

    const DrawingCanvasProjectedGuideDragSnapIntent badRawGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), QVariantMap{
                {QStringLiteral("x"), 0.1},
                {QStringLiteral("y"), std::numeric_limits<double>::infinity()},
            }},
            {QStringLiteral("snapped_anchor"), snapped(0.3, 0.4)},
        }},
    });
    EDI_CHECK(!badRawGuide.visible);

    const DrawingCanvasProjectedGuideDragSnapIntent badSnappedGuide = projectedGuideDragSnapIntent(QVariantMap{
        {QStringLiteral("guide_drag_snap"), QVariantMap{
            {QStringLiteral("raw_anchor"), snapped(0.1, 0.2)},
            {QStringLiteral("snapped_anchor"), QVariantMap{
                {QStringLiteral("x"), 0.3},
                {QStringLiteral("y"), QStringLiteral("bad")},
            }},
        }},
    });
    EDI_CHECK(!badSnappedGuide.visible);

    return 0;
}
