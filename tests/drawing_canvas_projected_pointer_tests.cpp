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

    return 0;
}
