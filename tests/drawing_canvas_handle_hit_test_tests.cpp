#include "widgets/DrawingCanvasHandleHitTest.h"

#include <QVariantList>
#include <QVariantMap>

#include <cassert>
#include <cmath>

using namespace drawing_canvas;

namespace {

QVariantMap objectWithHandles(const QVariantList &handles)
{
    return QVariantMap{{QStringLiteral("edit_handles"), handles}};
}

QVariantMap handle(
    const QString &id,
    double x,
    double y,
    double tolerance,
    bool editable = true)
{
    return QVariantMap{
        {QStringLiteral("id"), id},
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("hit_tolerance_px"), tolerance},
        {QStringLiteral("editable"), editable},
    };
}

bool near(double a, double b, double epsilon = 0.000001)
{
    return std::abs(a - b) <= epsilon;
}

} // namespace

int main()
{
    const QRectF board(10.0, 20.0, 200.0, 100.0);

    const DrawingCanvasHandleHit hit = hitCanvasHandleAt(
        objectWithHandles(QVariantList{handle(QStringLiteral("center"), 0.5, 0.5, 12.0)}),
        QPointF(113.0, 74.0),
        board);
    assert(hit.hit);
    assert(hit.handleId == QStringLiteral("center"));
    assert(near(hit.distancePx, 5.0));

    const DrawingCanvasHandleHit readOnlyMiss = hitCanvasHandleAt(
        objectWithHandles(QVariantList{handle(QStringLiteral("readonly"), 0.5, 0.5, 20.0, false)}),
        QPointF(110.0, 70.0),
        board);
    assert(!readOnlyMiss.hit);
    assert(readOnlyMiss.handleId.isEmpty());

    const DrawingCanvasHandleHit readOnlyFlagMiss = hitCanvasHandleAt(
        objectWithHandles(QVariantList{
            QVariantMap{
                {QStringLiteral("id"), QStringLiteral("readonly_flag")},
                {QStringLiteral("x"), 0.5},
                {QStringLiteral("y"), 0.5},
                {QStringLiteral("hit_tolerance_px"), 20.0},
                {QStringLiteral("read_only"), true},
            }
        }),
        QPointF(110.0, 70.0),
        board);
    assert(!readOnlyFlagMiss.hit);

    const DrawingCanvasHandleHit nearest = hitCanvasHandleAt(
        objectWithHandles(QVariantList{
            handle(QStringLiteral("far"), 0.45, 0.5, 20.0),
            handle(QStringLiteral("near"), 0.52, 0.5, 20.0),
        }),
        QPointF(113.0, 70.0),
        board);
    assert(nearest.hit);
    assert(nearest.handleId == QStringLiteral("near"));

    const DrawingCanvasHandleHit miss = hitCanvasHandleAt(
        objectWithHandles(QVariantList{handle(QStringLiteral("small"), 0.5, 0.5, 3.0)}),
        QPointF(116.0, 70.0),
        board);
    assert(!miss.hit);
    assert(miss.handleId.isEmpty());

    const DrawingCanvasHandleHit noHandles = hitCanvasHandleAt({}, QPointF(110.0, 70.0), board);
    assert(!noHandles.hit);

    const DrawingCanvasHandleHit malformed = hitCanvasHandleAt(
        objectWithHandles(QVariantList{
            QVariantMap{
                {QStringLiteral("x"), 0.5},
                {QStringLiteral("y"), 0.5},
                {QStringLiteral("hit_tolerance_px"), 20.0},
            },
            QStringLiteral("ignored"),
        }),
        QPointF(110.0, 70.0),
        board);
    assert(!malformed.hit);

    return 0;
}
