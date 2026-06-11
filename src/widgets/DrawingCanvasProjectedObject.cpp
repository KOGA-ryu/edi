#include "widgets/DrawingCanvasProjectedObject.h"

#include "widgets/DrawingCanvasValues.h"

#include <QVariantList>

#include <algorithm>
#include <cmath>

namespace drawing_canvas {
namespace {

DrawingCanvasProjectedHandleShape handleShapeFromString(const QString &shape)
{
    if (shape == QStringLiteral("square")) {
        return DrawingCanvasProjectedHandleShape::Square;
    }
    if (shape == QStringLiteral("diamond")) {
        return DrawingCanvasProjectedHandleShape::Diamond;
    }
    return DrawingCanvasProjectedHandleShape::Circle;
}

} // namespace

DrawingCanvasProjectedObjectSummary projectedObjectSummary(const QVariantMap &object)
{
    DrawingCanvasProjectedObjectSummary summary;
    summary.id = object.value(QStringLiteral("id")).toString();
    summary.kind = object.value(QStringLiteral("kind")).toString();
    summary.visible = object.value(
        QStringLiteral("effective_visible"),
        object.value(QStringLiteral("visible"), true)).toBool();
    summary.plotBlocked = object.value(QStringLiteral("plot_blocked")).toBool();
    summary.plotWarningKind = object.value(QStringLiteral("plot_warning_kind")).toString();

    const QVariantMap bounds = object.value(QStringLiteral("bounds")).toMap();
    if (!bounds.isEmpty()) {
        summary.bounds.ok = true;
        summary.bounds.x = finiteNumber(bounds.value(QStringLiteral("x")), 0.0);
        summary.bounds.y = finiteNumber(bounds.value(QStringLiteral("y")), 0.0);
        summary.bounds.width = finiteNumber(bounds.value(QStringLiteral("width")), 0.0);
        summary.bounds.height = finiteNumber(bounds.value(QStringLiteral("height")), 0.0);
    }
    return summary;
}

DrawingCanvasProjectedStyle projectedObjectStyle(const QVariantMap &object)
{
    DrawingCanvasProjectedStyle style;
    style.strokeColor = object.value(QStringLiteral("effective_stroke_color"), style.strokeColor).toString();
    style.strokeWidth = std::max(0.25, finiteNumber(object.value(QStringLiteral("effective_stroke_width")), style.strokeWidth));
    const QString lineStyle = object.value(QStringLiteral("effective_line_style")).toString();
    if (!lineStyle.isEmpty()) {
        style.lineStyle = lineStyle;
    }
    return style;
}

DrawingCanvasProjectedPointObject projectedPointObject(const QVariantMap &object)
{
    DrawingCanvasProjectedPointObject point;
    point.ok = readFinite(object, QStringLiteral("x"), point.x)
        && readFinite(object, QStringLiteral("y"), point.y);
    return point;
}

DrawingCanvasProjectedLine projectedLine(const QVariantMap &object)
{
    DrawingCanvasProjectedLine line;
    line.ok = readFinite(object, QStringLiteral("x1"), line.x1)
        && readFinite(object, QStringLiteral("y1"), line.y1)
        && readFinite(object, QStringLiteral("x2"), line.x2)
        && readFinite(object, QStringLiteral("y2"), line.y2);
    return line;
}

DrawingCanvasProjectedRectangle projectedRectangle(const QVariantMap &object)
{
    DrawingCanvasProjectedRectangle rectangle;
    rectangle.ok = readFinite(object, QStringLiteral("x"), rectangle.x)
        && readFinite(object, QStringLiteral("y"), rectangle.y)
        && readFinite(object, QStringLiteral("width"), rectangle.width)
        && readFinite(object, QStringLiteral("height"), rectangle.height);
    rectangle.rotationDeg = finiteNumber(object.value(QStringLiteral("rotation_deg")), 0.0);
    rectangle.cornerRadius = finiteNumber(object.value(QStringLiteral("corner_radius")), 0.0);
    rectangle.inset = finiteNumber(object.value(QStringLiteral("inset")), 0.0);
    return rectangle;
}

DrawingCanvasProjectedCircle projectedCircle(const QVariantMap &object)
{
    DrawingCanvasProjectedCircle circle;
    circle.ok = readFinite(object, QStringLiteral("cx"), circle.cx)
        && readFinite(object, QStringLiteral("cy"), circle.cy)
        && readFinite(object, QStringLiteral("radius"), circle.radius);
    return circle;
}

DrawingCanvasProjectedPolygon projectedPolygon(const QVariantMap &object)
{
    DrawingCanvasProjectedPolygon polygon;
    polygon.points = projectedObjectPoints(object);
    polygon.ok = !polygon.points.empty();
    return polygon;
}

DrawingCanvasProjectedGuide projectedGuide(const QVariantMap &object)
{
    DrawingCanvasProjectedGuide guide;
    guide.ok = readFinite(object, QStringLiteral("position"), guide.position);
    guide.orientation = object.value(QStringLiteral("orientation")).toString() == QStringLiteral("horizontal")
        ? DrawingCanvasProjectedGuideOrientation::Horizontal
        : DrawingCanvasProjectedGuideOrientation::Vertical;
    guide.locked = object.value(QStringLiteral("locked")).toBool();
    guide.color = object.value(QStringLiteral("guide_color"), guide.color).toString();
    guide.dashStyle = object.value(QStringLiteral("guide_dash_style"), guide.dashStyle).toString();
    guide.showLabel = object.value(QStringLiteral("guide_show_label"), true).toBool();
    guide.label = object.value(QStringLiteral("guide_label")).toString();
    return guide;
}

DrawingCanvasProjectedDimension projectedDimension(const QVariantMap &object)
{
    DrawingCanvasProjectedDimension dimension;
    dimension.ok = readFinite(object, QStringLiteral("x1"), dimension.x1)
        && readFinite(object, QStringLiteral("y1"), dimension.y1)
        && readFinite(object, QStringLiteral("x2"), dimension.x2)
        && readFinite(object, QStringLiteral("y2"), dimension.y2)
        && readFinite(object, QStringLiteral("dimension_x1"), dimension.dimensionX1)
        && readFinite(object, QStringLiteral("dimension_y1"), dimension.dimensionY1)
        && readFinite(object, QStringLiteral("dimension_x2"), dimension.dimensionX2)
        && readFinite(object, QStringLiteral("dimension_y2"), dimension.dimensionY2)
        && readFinite(object, QStringLiteral("label_x"), dimension.labelX)
        && readFinite(object, QStringLiteral("label_y"), dimension.labelY);
    dimension.showLabel = object.value(QStringLiteral("dimension_show_label"), true).toBool();
    dimension.label = object.value(QStringLiteral("label")).toString();
    return dimension;
}

std::vector<DrawingCanvasProjectedPoint> projectedObjectPoints(const QVariantMap &object)
{
    std::vector<DrawingCanvasProjectedPoint> result;
    const QVariantList source = object.value(QStringLiteral("points")).toList();
    result.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        if (entry.typeId() == QMetaType::QVariantList) {
            const QVariantList list = entry.toList();
            if (list.size() >= 2) {
                result.push_back({finiteNumber(list.at(0), 0.0), finiteNumber(list.at(1), 0.0)});
            }
            continue;
        }
        const QVariantMap point = entry.toMap();
        if (!point.isEmpty()) {
            result.push_back({
                finiteNumber(point.value(QStringLiteral("x")), 0.0),
                finiteNumber(point.value(QStringLiteral("y")), 0.0)
            });
        }
    }
    return result;
}

std::vector<DrawingCanvasProjectedHandle> projectedObjectHandles(const QVariantMap &object)
{
    std::vector<DrawingCanvasProjectedHandle> result;
    const QVariantList source = object.value(QStringLiteral("edit_handles")).toList();
    result.reserve(static_cast<std::size_t>(source.size()));
    for (const QVariant &entry : source) {
        const QVariantMap handle = entry.toMap();
        const QString id = handle.value(QStringLiteral("id")).toString();
        if (handle.isEmpty() || id.isEmpty()) {
            continue;
        }

        DrawingCanvasProjectedHandle projected;
        projected.id = id;
        projected.x = finiteNumber(handle.value(QStringLiteral("x")), 0.0);
        projected.y = finiteNumber(handle.value(QStringLiteral("y")), 0.0);
        projected.editable = handle.value(
            QStringLiteral("editable"),
            !handle.value(QStringLiteral("read_only")).toBool()).toBool();
        projected.hasAnchor = handle.value(QStringLiteral("has_anchor")).toBool();
        if (projected.hasAnchor) {
            projected.anchorX = finiteNumber(handle.value(QStringLiteral("anchor_x")), 0.0);
            projected.anchorY = finiteNumber(handle.value(QStringLiteral("anchor_y")), 0.0);
        }
        projected.sizePx = std::max(2.0, finiteNumber(handle.value(QStringLiteral("size_px")), 8.0));
        projected.hitTolerancePx = std::max(0.0, finiteNumber(handle.value(QStringLiteral("hit_tolerance_px")), 14.0));
        projected.shape = handleShapeFromString(handle.value(QStringLiteral("shape"), QStringLiteral("circle")).toString());
        result.push_back(projected);
    }
    return result;
}

} // namespace drawing_canvas
