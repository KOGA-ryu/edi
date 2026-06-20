#include "widgets/DrawingCanvasProjectedObject.h"

#include <QVariantList>

#include "EdiAssert.h"
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

    EDI_CHECK(parsed.size() == 3);
    EDI_CHECK(parsed[0].x == 0.1);
    EDI_CHECK(parsed[0].y == 0.2);
    EDI_CHECK(parsed[1].x == 0.3);
    EDI_CHECK(parsed[1].y == 0.4);
    EDI_CHECK(parsed[2].x == 0.0);
    EDI_CHECK(parsed[2].y == 0.6);

    const std::vector<DrawingCanvasProjectedPoint> missing = projectedObjectPoints({});
    EDI_CHECK(missing.empty());

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
    EDI_CHECK(summary.id == QStringLiteral("object_1"));
    EDI_CHECK(summary.kind == QStringLiteral("line"));
    EDI_CHECK(!summary.visible);
    EDI_CHECK(summary.plotBlocked);
    EDI_CHECK(summary.plotWarningKind == QStringLiteral("outside_drawable"));
    EDI_CHECK(summary.bounds.ok);
    EDI_CHECK(summary.bounds.x == 0.1);
    EDI_CHECK(summary.bounds.y == 0.2);
    EDI_CHECK(summary.bounds.width == 0.3);
    EDI_CHECK(summary.bounds.height == 0.0);

    const DrawingCanvasProjectedObjectSummary defaultSummary = projectedObjectSummary({});
    EDI_CHECK(defaultSummary.visible);
    EDI_CHECK(!defaultSummary.bounds.ok);

    const DrawingCanvasProjectedStyle style = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_color"), QStringLiteral("#123456")},
        {QStringLiteral("effective_stroke_width"), 4.5},
        {QStringLiteral("effective_stroke_opacity"), 0.35}
    });
    EDI_CHECK(style.strokeColor == QStringLiteral("#123456"));
    EDI_CHECK(style.strokeWidth == 4.5);
    EDI_CHECK(style.strokeOpacity == 0.35);

    const DrawingCanvasProjectedStyle defaultStyle = projectedObjectStyle({});
    EDI_CHECK(defaultStyle.strokeColor == QStringLiteral("#d7dde8"));
    EDI_CHECK(defaultStyle.strokeWidth == 2.0);
    EDI_CHECK(defaultStyle.strokeOpacity == 1.0); // absent key = fully opaque

    // Garbage opacity clamps into [0, 1] instead of poisoning the pen alpha.
    const DrawingCanvasProjectedStyle wildOpacity = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_opacity"), 7.0}
    });
    EDI_CHECK(wildOpacity.strokeOpacity == 1.0);
    const DrawingCanvasProjectedStyle negativeOpacity = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_opacity"), -2.0}
    });
    EDI_CHECK(negativeOpacity.strokeOpacity == 0.0);

    const DrawingCanvasProjectedStyle nonFiniteStyle = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_width"), std::numeric_limits<double>::quiet_NaN()}
    });
    EDI_CHECK(nonFiniteStyle.strokeWidth == 2.0);

    const DrawingCanvasProjectedStyle clampedStyle = projectedObjectStyle(QVariantMap{
        {QStringLiteral("effective_stroke_width"), -10.0}
    });
    EDI_CHECK(clampedStyle.strokeWidth == 0.25);

    const DrawingCanvasProjectedPointObject pointObject = projectedPointObject(QVariantMap{
        {QStringLiteral("x"), 0.12},
        {QStringLiteral("y"), 0.34}
    });
    EDI_CHECK(pointObject.ok);
    EDI_CHECK(pointObject.x == 0.12);
    EDI_CHECK(pointObject.y == 0.34);
    EDI_CHECK(!projectedPointObject(QVariantMap{{QStringLiteral("x"), 0.12}}).ok);

    const DrawingCanvasProjectedLine line = projectedLine(QVariantMap{
        {QStringLiteral("x1"), 0.1},
        {QStringLiteral("y1"), 0.2},
        {QStringLiteral("x2"), 0.3},
        {QStringLiteral("y2"), 0.4}
    });
    EDI_CHECK(line.ok);
    EDI_CHECK(line.x1 == 0.1);
    EDI_CHECK(line.y1 == 0.2);
    EDI_CHECK(line.x2 == 0.3);
    EDI_CHECK(line.y2 == 0.4);
    EDI_CHECK(!projectedLine(QVariantMap{
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
    EDI_CHECK(rectangle.ok);
    EDI_CHECK(rectangle.x == 0.2);
    EDI_CHECK(rectangle.y == 0.3);
    EDI_CHECK(rectangle.width == 0.4);
    EDI_CHECK(rectangle.height == 0.5);
    EDI_CHECK(rectangle.rotationDeg == 15.0);
    EDI_CHECK(projectedRectangle(QVariantMap{
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
    EDI_CHECK(circle.ok);
    EDI_CHECK(circle.cx == 0.45);
    EDI_CHECK(circle.cy == 0.55);
    EDI_CHECK(circle.radius == 0.1);
    EDI_CHECK(!projectedCircle(QVariantMap{
        {QStringLiteral("cx"), 0.45},
        {QStringLiteral("cy"), QStringLiteral("nope")},
        {QStringLiteral("radius"), 0.1}
    }).ok);

    const DrawingCanvasProjectedPolygon polygon = projectedPolygon(QVariantMap{
        {QStringLiteral("points"), points}
    });
    EDI_CHECK(polygon.ok);
    EDI_CHECK(polygon.points.size() == 3);
    EDI_CHECK(!projectedPolygon({}).ok);

    const DrawingCanvasProjectedGuide guide = projectedGuide(QVariantMap{
        {QStringLiteral("orientation"), QStringLiteral("horizontal")},
        {QStringLiteral("position"), 0.625},
        {QStringLiteral("locked"), true},
        {QStringLiteral("guide_color"), QStringLiteral("#abcdef")},
        {QStringLiteral("guide_dash_style"), QStringLiteral("dot")},
        {QStringLiteral("guide_show_label"), false},
        {QStringLiteral("guide_label"), QStringLiteral("datum")}
    });
    EDI_CHECK(guide.ok);
    EDI_CHECK(guide.orientation == DrawingCanvasProjectedGuideOrientation::Horizontal);
    EDI_CHECK(guide.position == 0.625);
    EDI_CHECK(guide.locked);
    EDI_CHECK(guide.color == QStringLiteral("#abcdef"));
    EDI_CHECK(guide.dashStyle == QStringLiteral("dot"));
    EDI_CHECK(!guide.showLabel);
    EDI_CHECK(guide.label == QStringLiteral("datum"));
    EDI_CHECK(!projectedGuide({}).ok);

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
    EDI_CHECK(dimension.ok);
    EDI_CHECK(dimension.x1 == 0.1);
    EDI_CHECK(dimension.y1 == 0.2);
    EDI_CHECK(dimension.dimensionX2 == 0.32);
    EDI_CHECK(dimension.dimensionY2 == 0.42);
    EDI_CHECK(dimension.labelX == 0.5);
    EDI_CHECK(dimension.labelY == 0.6);
    EDI_CHECK(!dimension.showLabel);
    EDI_CHECK(dimension.label == QStringLiteral("12 mm"));
    EDI_CHECK(!projectedDimension(QVariantMap{
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

    EDI_CHECK(parsedHandles.size() == 2);
    EDI_CHECK(parsedHandles[0].id == QStringLiteral("move_handle"));
    EDI_CHECK(parsedHandles[0].editable);
    EDI_CHECK(parsedHandles[0].hasAnchor);
    EDI_CHECK(parsedHandles[0].anchorX == 0.1);
    EDI_CHECK(parsedHandles[0].anchorY == 0.2);
    EDI_CHECK(parsedHandles[0].shape == DrawingCanvasProjectedHandleShape::Diamond);
    EDI_CHECK(parsedHandles[0].sizePx == 10.0);
    EDI_CHECK(parsedHandles[0].hitTolerancePx == 18.0);

    EDI_CHECK(parsedHandles[1].id == QStringLiteral("readonly_vertex"));
    EDI_CHECK(!parsedHandles[1].editable);
    EDI_CHECK(!parsedHandles[1].hasAnchor);
    EDI_CHECK(parsedHandles[1].shape == DrawingCanvasProjectedHandleShape::Square);
    EDI_CHECK(parsedHandles[1].sizePx == 2.0);
    EDI_CHECK(parsedHandles[1].hitTolerancePx == 0.0);

    const std::vector<DrawingCanvasProjectedHandle> missingHandles = projectedObjectHandles({});
    EDI_CHECK(missingHandles.empty());

    return 0;
}
