#include "DrawingCoreInternal.h"

#include <algorithm>
#include <cmath>

namespace drawing_core {

ShapeKind objectShapeKind(const QString &kind) {
    if (kind == QStringLiteral("point") || kind == QStringLiteral("tone_probe")) {
        return ShapeKind::Point;
    }
    if (kind == QStringLiteral("line") || kind == QStringLiteral("glyph_baseline")) {
        return ShapeKind::Line;
    }
    if (kind == QStringLiteral("circle") || kind == QStringLiteral("arc")) {
        return ShapeKind::Circle;
    }
    if (kind == QStringLiteral("rectangle")
        || kind == QStringLiteral("image_reference_frame")
        || kind == QStringLiteral("ascii_crop_frame")
        || kind == QStringLiteral("ascii_cell_region")) {
        return ShapeKind::Rectangle;
    }
    if (kind == QStringLiteral("polyline")) {
        return ShapeKind::Polyline;
    }
    if (kind == QStringLiteral("polygon")) {
        return ShapeKind::Polygon;
    }
    return ShapeKind::Unknown;
}

ShapeKind objectShapeKind(const QJsonObject &object) {
    return objectShapeKind(object.value("kind").toString());
}

QJsonArray pointArray(double x, double y) {
    QJsonArray array;
    array.append(x);
    array.append(y);
    return array;
}

QJsonArray translatedPointArray(const QJsonArray &point, double dx, double dy) {
    if (point.size() < 2) {
        return point;
    }
    return pointArray(point.at(0).toDouble() + dx, point.at(1).toDouble() + dy);
}

QJsonArray translatedPointList(const QJsonArray &points, double dx, double dy) {
    QJsonArray translated;
    for (const QJsonValue value : points) {
        translated.append(translatedPointArray(value.toArray(), dx, dy));
    }
    return translated;
}

void includePoint(Bounds &bounds, double x, double y) {
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }
    if (!bounds.ok) {
        bounds.ok = true;
        bounds.minX = bounds.maxX = x;
        bounds.minY = bounds.maxY = y;
        return;
    }
    bounds.minX = std::min(bounds.minX, x);
    bounds.minY = std::min(bounds.minY, y);
    bounds.maxX = std::max(bounds.maxX, x);
    bounds.maxY = std::max(bounds.maxY, y);
}

void includePointList(Bounds &bounds, const QJsonArray &points) {
    for (const QJsonValue value : points) {
        const QJsonArray point = value.toArray();
        if (point.size() >= 2) {
            includePoint(bounds, point.at(0).toDouble(), point.at(1).toDouble());
        }
    }
}

Bounds normalizedBounds(const QJsonObject &object) {
    Bounds bounds;
    const ShapeKind shapeKind = objectShapeKind(object);
    switch (shapeKind) {
    case ShapeKind::Point:
        includePoint(bounds, object.value("x").toDouble(), object.value("y").toDouble());
        break;
    case ShapeKind::Line:
        includePoint(bounds, object.value("x1").toDouble(), object.value("y1").toDouble());
        includePoint(bounds, object.value("x2").toDouble(), object.value("y2").toDouble());
        break;
    case ShapeKind::Circle: {
        const double cx = object.value("cx").toDouble();
        const double cy = object.value("cy").toDouble();
        const double radius = object.value("radius").toDouble();
        includePoint(bounds, cx - radius, cy - radius);
        includePoint(bounds, cx + radius, cy + radius);
        break;
    }
    case ShapeKind::Rectangle: {
        const double x = object.value("x").toDouble();
        const double y = object.value("y").toDouble();
        includePoint(bounds, x, y);
        includePoint(bounds, x + object.value("width").toDouble(), y + object.value("height").toDouble());
        break;
    }
    case ShapeKind::Polyline:
        includePointList(bounds, object.value("points").toArray());
        break;
    case ShapeKind::Polygon:
        includePointList(bounds, object.value("points").toArray());
        break;
    case ShapeKind::Unknown:
        break;
    }
    return bounds;
}

Bounds normalizedBoundsForObjects(const QJsonArray &objects, const QStringList &objectIds) {
    Bounds bounds;
    for (const QJsonValue value : objects) {
        const QJsonObject object = value.toObject();
        if (!objectIds.contains(object.value("id").toString())) {
            continue;
        }
        const Bounds objectBounds = normalizedBounds(object);
        if (!objectBounds.ok) {
            continue;
        }
        includePoint(bounds, objectBounds.minX, objectBounds.minY);
        includePoint(bounds, objectBounds.maxX, objectBounds.maxY);
    }
    return bounds;
}

double clampedMoveDelta(double delta, double minValue, double maxValue) {
    if (!std::isfinite(delta)) {
        return 0.0;
    }
    const double minDelta = 0.0 - minValue;
    const double maxDelta = 1.0 - maxValue;
    return std::clamp(delta, minDelta, maxDelta);
}

double normalizedToPixels(double value, int canvasPx) {
    return value * static_cast<double>(canvasPx);
}

double degreesToRadians(double degrees) {
    return degrees * kPi / 180.0;
}

double commandDeltaFromCommand(const State &state, const QJsonObject &command, const QString &axis, double fallback) {
    const QString pxKey = axis + QStringLiteral("_px");
    if (command.contains(pxKey)) {
        return numberAt(command, pxKey) / state.canvasPx;
    }
    return numberAt(command, axis, fallback);
}

void translateObject(QJsonObject &object, double dxN, double dyN, double dxPx, double dyPx) {
    const ShapeKind shapeKind = objectShapeKind(object);
    switch (shapeKind) {
    case ShapeKind::Point:
        object.insert("x", object.value("x").toDouble() + dxN);
        object.insert("y", object.value("y").toDouble() + dyN);
        object.insert("point_px", translatedPointArray(object.value("point_px").toArray(), dxPx, dyPx));
        break;
    case ShapeKind::Line:
        object.insert("x1", object.value("x1").toDouble() + dxN);
        object.insert("y1", object.value("y1").toDouble() + dyN);
        object.insert("x2", object.value("x2").toDouble() + dxN);
        object.insert("y2", object.value("y2").toDouble() + dyN);
        object.insert("from_px", translatedPointArray(object.value("from_px").toArray(), dxPx, dyPx));
        object.insert("to_px", translatedPointArray(object.value("to_px").toArray(), dxPx, dyPx));
        break;
    case ShapeKind::Circle:
        object.insert("cx", object.value("cx").toDouble() + dxN);
        object.insert("cy", object.value("cy").toDouble() + dyN);
        object.insert("center_px", translatedPointArray(object.value("center_px").toArray(), dxPx, dyPx));
        break;
    case ShapeKind::Rectangle: {
        object.insert("x", object.value("x").toDouble() + dxN);
        object.insert("y", object.value("y").toDouble() + dyN);
        object.insert("from_px", translatedPointArray(object.value("from_px").toArray(), dxPx, dyPx));
        object.insert("to_px", translatedPointArray(object.value("to_px").toArray(), dxPx, dyPx));
        QJsonArray rect = object.value("rect_px").toArray();
        if (rect.size() >= 4) {
            rect[0] = rect.at(0).toDouble() + dxPx;
            rect[1] = rect.at(1).toDouble() + dyPx;
            object.insert("rect_px", rect);
        }
        break;
    }
    case ShapeKind::Polyline:
        object.insert("points", translatedPointList(object.value("points").toArray(), dxN, dyN));
        object.insert("points_px", translatedPointList(object.value("points_px").toArray(), dxPx, dyPx));
        break;
    case ShapeKind::Polygon:
        object.insert("cx", object.value("cx").toDouble() + dxN);
        object.insert("cy", object.value("cy").toDouble() + dyN);
        object.insert("center_px", translatedPointArray(object.value("center_px").toArray(), dxPx, dyPx));
        object.insert("points", translatedPointList(object.value("points").toArray(), dxN, dyN));
        object.insert("points_px", translatedPointList(object.value("points_px").toArray(), dxPx, dyPx));
        break;
    case ShapeKind::Unknown:
        break;
    }
}

void translateObjectWithState(QJsonObject &object, const State &state, double dxN, double dyN) {
    translateObject(object, dxN, dyN, normalizedToPixels(dxN, state.canvasPx), normalizedToPixels(dyN, state.canvasPx));
}

Point snapPoint(State &state, double x, double y) {
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return {};
    }
    double px = x;
    double py = y;
    if (state.gridSnap) {
        const double step = std::max(1, state.gridStepPx);
        px = std::round(px / step) * step;
        py = std::round(py / step) * step;
    }
    px = std::clamp(px, 0.0, static_cast<double>(state.canvasPx));
    py = std::clamp(py, 0.0, static_cast<double>(state.canvasPx));
    return {true, px, py, px / state.canvasPx, py / state.canvasPx};
}

Point pointFromArray(State &state, const QJsonArray &array) {
    if (array.size() < 2) {
        return {};
    }
    return snapPoint(state, array.at(0).toDouble(), array.at(1).toDouble());
}

Point pointFromCommand(State &state, const QJsonObject &command) {
    if (command.contains("point")) {
        return pointFromArray(state, arrayAt(command, "point"));
    }
    if (command.contains("center")) {
        return pointFromArray(state, arrayAt(command, "center"));
    }
    return snapPoint(state, numberAt(command, "x"), numberAt(command, "y"));
}

double distancePx(const Point &a, const Point &b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

double clampedPx(double value, int canvasPx) {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, static_cast<double>(canvasPx));
}

double positivePx(double value, double fallback) {
    if (!std::isfinite(value) || value <= 0.0) {
        return fallback;
    }
    return value;
}

void rebuildRectangle(QJsonObject &object, int canvasPx, double xPx, double yPx, double widthPx, double heightPx) {
    const double width = std::clamp(positivePx(widthPx), 1.0, static_cast<double>(canvasPx));
    const double height = std::clamp(positivePx(heightPx), 1.0, static_cast<double>(canvasPx));
    const double x = std::clamp(clampedPx(xPx, canvasPx), 0.0, static_cast<double>(canvasPx) - width);
    const double y = std::clamp(clampedPx(yPx, canvasPx), 0.0, static_cast<double>(canvasPx) - height);
    object.insert("x", x / canvasPx);
    object.insert("y", y / canvasPx);
    object.insert("width", width / canvasPx);
    object.insert("height", height / canvasPx);
    object.insert("from_px", pointArray(x, y));
    object.insert("to_px", pointArray(x + width, y + height));
    QJsonArray rectPx;
    rectPx.append(x);
    rectPx.append(y);
    rectPx.append(width);
    rectPx.append(height);
    object.insert("rect_px", rectPx);
}

void rebuildPolygon(QJsonObject &object, int canvasPx, double cxPx, double cyPx, double radiusPx, int sides, double rotationDeg) {
    const double cx = clampedPx(cxPx, canvasPx);
    const double cy = clampedPx(cyPx, canvasPx);
    const int sideCount = std::clamp(sides, 3, 64);
    const double maxRadius = std::max(1.0, std::min({cx, cy, static_cast<double>(canvasPx) - cx, static_cast<double>(canvasPx) - cy}));
    const double radius = std::clamp(positivePx(radiusPx), 1.0, maxRadius);
    QJsonArray points;
    QJsonArray pointsPx;
    for (int index = 0; index < sideCount; ++index) {
        const double angle = degreesToRadians(rotationDeg + 360.0 * index / sideCount);
        const double px = cx + std::cos(angle) * radius;
        const double py = cy + std::sin(angle) * radius;
        points.append(pointArray(px / canvasPx, py / canvasPx));
        pointsPx.append(pointArray(px, py));
    }
    object.insert("cx", cx / canvasPx);
    object.insert("cy", cy / canvasPx);
    object.insert("center_px", pointArray(cx, cy));
    object.insert("radius", radius / canvasPx);
    object.insert("radius_px", radius);
    object.insert("sides", sideCount);
    object.insert("rotation_deg", rotationDeg);
    object.insert("points", points);
    object.insert("points_px", pointsPx);
}

} // namespace drawing_core
