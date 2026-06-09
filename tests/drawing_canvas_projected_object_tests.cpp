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

    const DrawingCanvasProjectedStyle style = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_color"), QStringLiteral("#123456")},
        {QStringLiteral("effective_stroke_width"), 4.5}
    });
    assert(style.strokeColor == QStringLiteral("#123456"));
    assert(style.strokeWidth == 4.5);

    const DrawingCanvasProjectedStyle defaultStyle = projectedObjectStyle({});
    assert(defaultStyle.strokeColor == QStringLiteral("#d7dde8"));
    assert(defaultStyle.strokeWidth == 2.0);

    const DrawingCanvasProjectedStyle nonFiniteStyle = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_width"), std::numeric_limits<double>::quiet_NaN()}
    });
    assert(nonFiniteStyle.strokeWidth == 2.0);

    const DrawingCanvasProjectedStyle clampedStyle = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_width"), -10.0}
    });
    assert(clampedStyle.strokeWidth == 0.25);

    const DrawingCanvasProjectedPointObject pointObject = projectedPointObject(QVariantMap{
        {QStringLiteral("x"), 0.12},
        {QStringLiteral("y"), 0.34}
    });
    assert(pointObject.ok);
    assert(pointObject.x == 0.12);
    assert(pointObject.y == 0.34);
    assert(!projectedPointObject(QVariantMap{{QStringLiteral("x"), 0.12}}).ok);

    const DrawingCanvasProjectedLine line = projectedLine(QVariantMap{
        {QStringLiteral("x1"), 0.1},
        {QStringLiteral("y1"), 0.2},
        {QStringLiteral("x2"), 0.3},
        {QStringLiteral("y2"), 0.4}
    });
    assert(line.ok);
    assert(line.x1 == 0.1);
    assert(line.y1 == 0.2);
    assert(line.x2 == 0.3);
    assert(line.y2 == 0.4);
    assert(!projectedLine(QVariantMap{
        {QStringLiteral("x1"), 0.1},
        {QStringLiteral("y1"), 0.2},
        {QStringLiteral("x2"), std::numeric_limits<double>::quiet_NaN()},
        {QStringLiteral("y2"), 0.4}
    }).ok);

    const DrawingCanvasProjectedRectangle rectangle = projectedRectangle(QVariantMap{
        {QStringLiteral("x"), 0.2},
        {QStringLiteral("y"), 0.3},
        {QStringLiteral("width"), 0.4},
        {QStringLiteral("height"), 0.5},
        {QStringLiteral("rotation_deg"), 15.0}
    });
    assert(rectangle.ok);
    assert(rectangle.x == 0.2);
    assert(rectangle.y == 0.3);
    assert(rectangle.width == 0.4);
    assert(rectangle.height == 0.5);
    assert(rectangle.rotationDeg == 15.0);
    assert(projectedRectangle(QVariantMap{
        {QStringLiteral("x"), 0.2},
        {QStringLiteral("y"), 0.3},
        {QStringLiteral("width"), 0.4},
        {QStringLiteral("height"), 0.5},
        {QStringLiteral("rotation_deg"), std::numeric_limits<double>::quiet_NaN()}
    }).rotationDeg == 0.0);

    const DrawingCanvasProjectedCircle circle = projectedCircle(QVariantMap{
        {QStringLiteral("cx"), 0.45},
        {QStringLiteral("cy"), 0.55},
        {QStringLiteral("radius"), 0.1}
    });
    assert(circle.ok);
    assert(circle.cx == 0.45);
    assert(circle.cy == 0.55);
    assert(circle.radius == 0.1);
    assert(!projectedCircle(QVariantMap{
        {QStringLiteral("cx"), 0.45},
        {QStringLiteral("cy"), QStringLiteral("nope")},
        {QStringLiteral("radius"), 0.1}
    }).ok);

    const DrawingCanvasProjectedPolygon polygon = projectedPolygon(QVariantMap{
        {QStringLiteral("points"), points}
    });
    assert(polygon.ok);
    assert(polygon.points.size() == 3);
    assert(!projectedPolygon({}).ok);

    const DrawingCanvasProjectedGuide guide = projectedGuide(QVariantMap{
        {QStringLiteral("orientation"), QStringLiteral("horizontal")},
        {QStringLiteral("position"), 0.625},
        {QStringLiteral("locked"), true},
        {QStringLiteral("guide_color"), QStringLiteral("#abcdef")},
        {QStringLiteral("guide_dash_style"), QStringLiteral("dot")},
        {QStringLiteral("guide_show_label"), false},
        {QStringLiteral("guide_label"), QStringLiteral("datum")}
    });
    assert(guide.ok);
    assert(guide.orientation == DrawingCanvasProjectedGuideOrientation::Horizontal);
    assert(guide.position == 0.625);
    assert(guide.locked);
    assert(guide.color == QStringLiteral("#abcdef"));
    assert(guide.dashStyle == QStringLiteral("dot"));
    assert(!guide.showLabel);
    assert(guide.label == QStringLiteral("datum"));
    assert(!projectedGuide({}).ok);

    const DrawingCanvasProjectedDimension dimension = projectedDimension(QVariantMap{
        {QStringLiteral("x1"), 0.1},
        {QStringLiteral("y1"), 0.2},
        {QStringLiteral("x2"), 0.3},
        {QStringLiteral("y2"), 0.4},
        {QStringLiteral("dimension_x1"), 0.12},
        {QStringLiteral("dimension_y1"), 0.22},
        {QStringLiteral("dimension_x2"), 0.32},
        {QStringLiteral("dimension_y2"), 0.42},
        {QStringLiteral("label_x"), 0.5},
        {QStringLiteral("label_y"), 0.6},
        {QStringLiteral("dimension_show_label"), false},
        {QStringLiteral("label"), QStringLiteral("12 mm")}
    });
    assert(dimension.ok);
    assert(dimension.x1 == 0.1);
    assert(dimension.y1 == 0.2);
    assert(dimension.dimensionX2 == 0.32);
    assert(dimension.dimensionY2 == 0.42);
    assert(dimension.labelX == 0.5);
    assert(dimension.labelY == 0.6);
    assert(!dimension.showLabel);
    assert(dimension.label == QStringLiteral("12 mm"));
    assert(!projectedDimension(QVariantMap{
        {QStringLiteral("x1"), 0.1}
    }).ok);

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
