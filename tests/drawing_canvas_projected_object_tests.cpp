#include "widgets/DrawingCanvasProjectedObject.h"

#include <QVariantList>

#include <cassert>
#include <cmath>
#include <limits>

using namespace drawing_canvas;

int main()
{
    QVariantList points;
    points.push_back(QVariantMap{{QStringLiteral("x"), 0.1}, {QStringLiteral("y"), 0.2}});
    points.push_back(QVariantList{0.3, 0.4});
    points.push_back(QVariantMap{{QStringLiteral("x"), std::numeric_limits<double>::quiet_NaN()}, {QStringLiteral("y"), 0.6}});
    points.push_back(QVariantMap{});
    points.push_back(QStringLiteral("ignored"));

    const std::vector<DrawingCanvasProjectedPoint> parsed = projectedObjectPoints(QVariantMap{
        {QStringLiteral("points"), points}
    });

    assert(parsed.size() == 3);
    assert(parsed[0].x == 0.1);
    assert(parsed[0].y == 0.2);
    assert(parsed[1].x == 0.3);
    assert(parsed[1].y == 0.4);
    assert(parsed[2].x == 0.0);
    assert(parsed[2].y == 0.6);

    const std::vector<DrawingCanvasProjectedPoint> missing = projectedObjectPoints({});
    assert(missing.empty());

    const DrawingCanvasProjectedObjectSummary summary = projectedObjectSummary(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("object_1")},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("visible"), true},
        {QStringLiteral("effective_visible"), false},
        {QStringLiteral("plot_blocked"), true},
        {QStringLiteral("plot_warning_kind"), QStringLiteral("outside_drawable")},
        {QStringLiteral("bounds"), QVariantMap{
            {QStringLiteral("x"), 0.1},
            {QStringLiteral("y"), 0.2},
            {QStringLiteral("width"), 0.3},
            {QStringLiteral("height"), std::numeric_limits<double>::quiet_NaN()}
        }}
    });
    assert(summary.id == QStringLiteral("object_1"));
    assert(summary.kind == QStringLiteral("line"));
    assert(!summary.visible);
    assert(summary.plotBlocked);
    assert(summary.plotWarningKind == QStringLiteral("outside_drawable"));
    assert(summary.bounds.ok);
    assert(summary.bounds.x == 0.1);
    assert(summary.bounds.y == 0.2);
    assert(summary.bounds.width == 0.3);
    assert(summary.bounds.height == 0.0);

    const DrawingCanvasProjectedObjectSummary defaultSummary = projectedObjectSummary({});
    assert(defaultSummary.visible);
    assert(!defaultSummary.bounds.ok);

    QVariantList handles;
    handles.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("move_handle")},
        {QStringLiteral("x"), 0.25},
        {QStringLiteral("y"), 0.5},
        {QStringLiteral("editable"), true},
        {QStringLiteral("has_anchor"), true},
        {QStringLiteral("anchor_x"), 0.1},
        {QStringLiteral("anchor_y"), 0.2},
        {QStringLiteral("shape"), QStringLiteral("diamond")},
        {QStringLiteral("size_px"), 10.0},
        {QStringLiteral("hit_tolerance_px"), 18.0}
    });
    handles.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("readonly_vertex")},
        {QStringLiteral("x"), 0.75},
        {QStringLiteral("y"), 0.9},
        {QStringLiteral("read_only"), true},
        {QStringLiteral("shape"), QStringLiteral("square")},
        {QStringLiteral("size_px"), 1.0},
        {QStringLiteral("hit_tolerance_px"), -5.0}
    });
    handles.push_back(QVariantMap{
        {QStringLiteral("x"), 0.4},
        {QStringLiteral("y"), 0.4}
    });
    handles.push_back(QStringLiteral("ignored"));

    const std::vector<DrawingCanvasProjectedHandle> parsedHandles = projectedObjectHandles(QVariantMap{
        {QStringLiteral("edit_handles"), handles}
    });

    assert(parsedHandles.size() == 2);
    assert(parsedHandles[0].id == QStringLiteral("move_handle"));
    assert(parsedHandles[0].editable);
    assert(parsedHandles[0].hasAnchor);
    assert(parsedHandles[0].anchorX == 0.1);
    assert(parsedHandles[0].anchorY == 0.2);
    assert(parsedHandles[0].shape == DrawingCanvasProjectedHandleShape::Diamond);
    assert(parsedHandles[0].sizePx == 10.0);
    assert(parsedHandles[0].hitTolerancePx == 18.0);

    assert(parsedHandles[1].id == QStringLiteral("readonly_vertex"));
    assert(!parsedHandles[1].editable);
    assert(!parsedHandles[1].hasAnchor);
    assert(parsedHandles[1].shape == DrawingCanvasProjectedHandleShape::Square);
    assert(parsedHandles[1].sizePx == 2.0);
    assert(parsedHandles[1].hitTolerancePx == 0.0);

    const std::vector<DrawingCanvasProjectedHandle> missingHandles = projectedObjectHandles({});
    assert(missingHandles.empty());

    return 0;
}
