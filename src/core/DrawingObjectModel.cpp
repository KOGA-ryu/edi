#include "DrawingObjectModel.h"

#include <QJsonArray>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace drawing_core {

namespace {
QJsonObject pointObject(const Point2D &point) {
    QJsonObject object;
    object.insert(QStringLiteral("x"), point.x);
    object.insert(QStringLiteral("y"), point.y);
    return object;
}

QJsonArray pointArray(const Point2D &point) {
    QJsonArray array;
    array.append(point.x);
    array.append(point.y);
    return array;
}

Bounds2D boundsFromPoints(const std::vector<Point2D> &points) {
    if (points.empty()) {
        return {};
    }
    double minX = points.front().x;
    double minY = points.front().y;
    double maxX = points.front().x;
    double maxY = points.front().y;
    for (const Point2D &point : points) {
        minX = std::min(minX, point.x);
        minY = std::min(minY, point.y);
        maxX = std::max(maxX, point.x);
        maxY = std::max(maxY, point.y);
    }
    return {true, minX, minY, maxX - minX, maxY - minY};
}

std::vector<Point2D> scaledPoints(const std::vector<Point2D> &points, double scale) {
    std::vector<Point2D> scaled;
    scaled.reserve(points.size());
    for (const Point2D &point : points) {
        scaled.push_back({point.x * scale, point.y * scale});
    }
    return scaled;
}

std::vector<Point2D> translatedPoints(const std::vector<Point2D> &points, double dx, double dy) {
    std::vector<Point2D> translated;
    translated.reserve(points.size());
    for (const Point2D &point : points) {
        translated.push_back({point.x + dx, point.y + dy});
    }
    return translated;
}

std::vector<Point2D> pointsFromArray(const QJsonArray &array) {
    std::vector<Point2D> points;
    points.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue value : array) {
        const QJsonArray point = value.toArray();
        if (point.size() >= 2) {
            points.push_back({point.at(0).toDouble(), point.at(1).toDouble()});
        }
    }
    return points;
}

QJsonArray pointsToArray(const std::vector<Point2D> &points) {
    QJsonArray array;
    for (const Point2D &point : points) {
        array.append(pointArray(point));
    }
    return array;
}
} // namespace

Bounds2D computeBounds(const PointGeometry &point) {
    return {true, point.point.x, point.point.y, 0.0, 0.0};
}

Bounds2D computeBounds(const LineGeometry &line) {
    return boundsFromPoints({line.a, line.b});
}

Bounds2D computeBounds(const CircleGeometry &circle) {
    return {
        true,
        circle.center.x - circle.radius,
        circle.center.y - circle.radius,
        circle.radius * 2.0,
        circle.radius * 2.0,
    };
}

Bounds2D computeBounds(const ArcGeometry &arc) {
    return computeBounds(CircleGeometry{arc.center, arc.radius});
}

Bounds2D computeBounds(const RectangleGeometry &rectangle) {
    return {true, rectangle.origin.x, rectangle.origin.y, rectangle.width, rectangle.height};
}

Bounds2D computeBounds(const PolylineGeometry &polyline) {
    return boundsFromPoints(polyline.points);
}

Bounds2D computeBounds(const PolygonGeometry &polygon) {
    return boundsFromPoints(polygon.points);
}

PointGeometry scaledPointGeometry(const PointGeometry &point, double scale) {
    return {{point.point.x * scale, point.point.y * scale}};
}

LineGeometry scaledLineGeometry(const LineGeometry &line, double scale) {
    return {
        {line.a.x * scale, line.a.y * scale},
        {line.b.x * scale, line.b.y * scale},
    };
}

CircleGeometry scaledCircleGeometry(const CircleGeometry &circle, double scale) {
    return {{circle.center.x * scale, circle.center.y * scale}, circle.radius * scale};
}

ArcGeometry scaledArcGeometry(const ArcGeometry &arc, double scale) {
    return {{arc.center.x * scale, arc.center.y * scale}, arc.radius * scale, arc.startAngleDeg, arc.endAngleDeg};
}

RectangleGeometry scaledRectangleGeometry(const RectangleGeometry &rectangle, double scale) {
    return {{rectangle.origin.x * scale, rectangle.origin.y * scale}, rectangle.width * scale, rectangle.height * scale, rectangle.rotationDeg};
}

PolylineGeometry scaledPolylineGeometry(const PolylineGeometry &polyline, double scale) {
    return {scaledPoints(polyline.points, scale)};
}

PolygonGeometry scaledPolygonGeometry(const PolygonGeometry &polygon, double scale) {
    return {{polygon.center.x * scale, polygon.center.y * scale}, polygon.radius * scale, polygon.sides, polygon.rotationDeg, scaledPoints(polygon.points, scale)};
}

PointGeometry translatedPointGeometry(const PointGeometry &point, double dx, double dy) {
    return {{point.point.x + dx, point.point.y + dy}};
}

LineGeometry translatedLineGeometry(const LineGeometry &line, double dx, double dy) {
    return {
        {line.a.x + dx, line.a.y + dy},
        {line.b.x + dx, line.b.y + dy},
    };
}

CircleGeometry translatedCircleGeometry(const CircleGeometry &circle, double dx, double dy) {
    return {{circle.center.x + dx, circle.center.y + dy}, circle.radius};
}

ArcGeometry translatedArcGeometry(const ArcGeometry &arc, double dx, double dy) {
    return {{arc.center.x + dx, arc.center.y + dy}, arc.radius, arc.startAngleDeg, arc.endAngleDeg};
}

RectangleGeometry translatedRectangleGeometry(const RectangleGeometry &rectangle, double dx, double dy) {
    return {{rectangle.origin.x + dx, rectangle.origin.y + dy}, rectangle.width, rectangle.height, rectangle.rotationDeg};
}

PolylineGeometry translatedPolylineGeometry(const PolylineGeometry &polyline, double dx, double dy) {
    return {translatedPoints(polyline.points, dx, dy)};
}

PolygonGeometry translatedPolygonGeometry(const PolygonGeometry &polygon, double dx, double dy) {
    return {{polygon.center.x + dx, polygon.center.y + dy}, polygon.radius, polygon.sides, polygon.rotationDeg, translatedPoints(polygon.points, dx, dy)};
}

PointGeometry pointGeometryFromObject(const QJsonObject &object) {
    return {{object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble()}};
}

LineGeometry lineGeometryFromObject(const QJsonObject &object) {
    return {
        {object.value(QStringLiteral("x1")).toDouble(), object.value(QStringLiteral("y1")).toDouble()},
        {object.value(QStringLiteral("x2")).toDouble(), object.value(QStringLiteral("y2")).toDouble()},
    };
}

CircleGeometry circleGeometryFromObject(const QJsonObject &object) {
    return {
        {object.value(QStringLiteral("cx")).toDouble(), object.value(QStringLiteral("cy")).toDouble()},
        object.value(QStringLiteral("radius")).toDouble(),
    };
}

ArcGeometry arcGeometryFromObject(const QJsonObject &object) {
    return {
        {object.value(QStringLiteral("cx")).toDouble(), object.value(QStringLiteral("cy")).toDouble()},
        object.value(QStringLiteral("radius")).toDouble(),
        object.value(QStringLiteral("start_angle_deg")).toDouble(),
        object.value(QStringLiteral("end_angle_deg")).toDouble(90.0),
    };
}

RectangleGeometry rectangleGeometryFromObject(const QJsonObject &object) {
    return {
        {object.value(QStringLiteral("x")).toDouble(), object.value(QStringLiteral("y")).toDouble()},
        object.value(QStringLiteral("width")).toDouble(),
        object.value(QStringLiteral("height")).toDouble(),
        object.value(QStringLiteral("rotation_deg")).toDouble(),
    };
}

PolylineGeometry polylineGeometryFromObject(const QJsonObject &object) {
    return {pointsFromArray(object.value(QStringLiteral("points")).toArray())};
}

PolygonGeometry polygonGeometryFromObject(const QJsonObject &object) {
    return {
        {object.value(QStringLiteral("cx")).toDouble(), object.value(QStringLiteral("cy")).toDouble()},
        object.value(QStringLiteral("radius")).toDouble(),
        object.value(QStringLiteral("sides")).toInt(3),
        object.value(QStringLiteral("rotation_deg")).toDouble(),
        pointsFromArray(object.value(QStringLiteral("points")).toArray()),
    };
}

QJsonObject serializePointGeometry(const PointGeometry &point) {
    QJsonObject geometry;
    geometry.insert(QStringLiteral("point"), pointObject(point.point));
    return geometry;
}

QJsonObject serializeLineGeometry(const LineGeometry &line) {
    QJsonObject geometry;
    geometry.insert(QStringLiteral("a"), pointObject(line.a));
    geometry.insert(QStringLiteral("b"), pointObject(line.b));
    return geometry;
}

QJsonObject serializeCircleGeometry(const CircleGeometry &circle) {
    QJsonObject geometry;
    geometry.insert(QStringLiteral("center"), pointObject(circle.center));
    geometry.insert(QStringLiteral("radius"), circle.radius);
    return geometry;
}

QJsonObject serializeArcGeometry(const ArcGeometry &arc) {
    QJsonObject geometry = serializeCircleGeometry({arc.center, arc.radius});
    geometry.insert(QStringLiteral("start_angle_deg"), arc.startAngleDeg);
    geometry.insert(QStringLiteral("end_angle_deg"), arc.endAngleDeg);
    return geometry;
}

QJsonObject serializeRectangleGeometry(const RectangleGeometry &rectangle) {
    QJsonObject geometry;
    geometry.insert(QStringLiteral("origin"), pointObject(rectangle.origin));
    geometry.insert(QStringLiteral("width"), rectangle.width);
    geometry.insert(QStringLiteral("height"), rectangle.height);
    geometry.insert(QStringLiteral("rotation_deg"), rectangle.rotationDeg);
    return geometry;
}

QJsonObject serializePolylineGeometry(const PolylineGeometry &polyline) {
    QJsonObject geometry;
    geometry.insert(QStringLiteral("points"), pointsToArray(polyline.points));
    return geometry;
}

QJsonObject serializePolygonGeometry(const PolygonGeometry &polygon) {
    QJsonObject geometry;
    geometry.insert(QStringLiteral("center"), pointObject(polygon.center));
    geometry.insert(QStringLiteral("radius"), polygon.radius);
    geometry.insert(QStringLiteral("sides"), polygon.sides);
    geometry.insert(QStringLiteral("rotation_deg"), polygon.rotationDeg);
    geometry.insert(QStringLiteral("points"), pointsToArray(polygon.points));
    return geometry;
}

QJsonObject serializeBounds(const Bounds2D &bounds) {
    QJsonObject object;
    if (!bounds.ok) {
        return object;
    }
    object.insert(QStringLiteral("x"), bounds.x);
    object.insert(QStringLiteral("y"), bounds.y);
    object.insert(QStringLiteral("w"), bounds.w);
    object.insert(QStringLiteral("h"), bounds.h);
    return object;
}

void writePointGeometry(QJsonObject &object, const PointGeometry &normalizedGeometry, int canvasPx) {
    const double canvas = static_cast<double>(canvasPx);
    const PointGeometry pixelGeometry = scaledPointGeometry(normalizedGeometry, canvas);
    object.insert(QStringLiteral("x"), normalizedGeometry.point.x);
    object.insert(QStringLiteral("y"), normalizedGeometry.point.y);
    object.insert(QStringLiteral("point_px"), pointArray(pixelGeometry.point));
    object.insert(QStringLiteral("geometry"), serializePointGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

void writeLineGeometry(QJsonObject &object, const LineGeometry &normalizedGeometry, int canvasPx) {
    const double canvas = static_cast<double>(canvasPx);
    const LineGeometry pixelGeometry = scaledLineGeometry(normalizedGeometry, canvas);
    object.insert(QStringLiteral("x1"), normalizedGeometry.a.x);
    object.insert(QStringLiteral("y1"), normalizedGeometry.a.y);
    object.insert(QStringLiteral("x2"), normalizedGeometry.b.x);
    object.insert(QStringLiteral("y2"), normalizedGeometry.b.y);
    object.insert(QStringLiteral("from_px"), pointArray(pixelGeometry.a));
    object.insert(QStringLiteral("to_px"), pointArray(pixelGeometry.b));
    object.insert(QStringLiteral("geometry"), serializeLineGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

void writeCircleGeometry(QJsonObject &object, const CircleGeometry &normalizedGeometry, int canvasPx) {
    const double canvas = static_cast<double>(canvasPx);
    const CircleGeometry pixelGeometry = scaledCircleGeometry(normalizedGeometry, canvas);
    object.insert(QStringLiteral("cx"), normalizedGeometry.center.x);
    object.insert(QStringLiteral("cy"), normalizedGeometry.center.y);
    object.insert(QStringLiteral("radius"), normalizedGeometry.radius);
    object.insert(QStringLiteral("center_px"), pointArray(pixelGeometry.center));
    object.insert(QStringLiteral("radius_px"), pixelGeometry.radius);
    object.insert(QStringLiteral("geometry"), serializeCircleGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

void writeArcGeometry(QJsonObject &object, const ArcGeometry &normalizedGeometry, int canvasPx) {
    const double canvas = static_cast<double>(canvasPx);
    const ArcGeometry pixelGeometry = scaledArcGeometry(normalizedGeometry, canvas);
    object.insert(QStringLiteral("cx"), normalizedGeometry.center.x);
    object.insert(QStringLiteral("cy"), normalizedGeometry.center.y);
    object.insert(QStringLiteral("radius"), normalizedGeometry.radius);
    object.insert(QStringLiteral("center_px"), pointArray(pixelGeometry.center));
    object.insert(QStringLiteral("radius_px"), pixelGeometry.radius);
    object.insert(QStringLiteral("start_angle_deg"), normalizedGeometry.startAngleDeg);
    object.insert(QStringLiteral("end_angle_deg"), normalizedGeometry.endAngleDeg);
    object.insert(QStringLiteral("geometry"), serializeArcGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

void writeRectangleGeometry(QJsonObject &object, const RectangleGeometry &normalizedGeometry, int canvasPx) {
    const double canvas = static_cast<double>(canvasPx);
    const RectangleGeometry pixelGeometry = scaledRectangleGeometry(normalizedGeometry, canvas);
    object.insert(QStringLiteral("x"), normalizedGeometry.origin.x);
    object.insert(QStringLiteral("y"), normalizedGeometry.origin.y);
    object.insert(QStringLiteral("width"), normalizedGeometry.width);
    object.insert(QStringLiteral("height"), normalizedGeometry.height);
    object.insert(QStringLiteral("rotation_deg"), normalizedGeometry.rotationDeg);
    object.insert(QStringLiteral("from_px"), pointArray(pixelGeometry.origin));
    object.insert(QStringLiteral("to_px"), pointArray({pixelGeometry.origin.x + pixelGeometry.width, pixelGeometry.origin.y + pixelGeometry.height}));

    QJsonArray rectPx;
    rectPx.append(pixelGeometry.origin.x);
    rectPx.append(pixelGeometry.origin.y);
    rectPx.append(pixelGeometry.width);
    rectPx.append(pixelGeometry.height);
    object.insert(QStringLiteral("rect_px"), rectPx);

    object.insert(QStringLiteral("geometry"), serializeRectangleGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

void writePolylineGeometry(QJsonObject &object, const PolylineGeometry &normalizedGeometry, int canvasPx) {
    const PolylineGeometry pixelGeometry = scaledPolylineGeometry(normalizedGeometry, static_cast<double>(canvasPx));
    object.insert(QStringLiteral("points"), pointsToArray(normalizedGeometry.points));
    object.insert(QStringLiteral("points_px"), pointsToArray(pixelGeometry.points));
    object.insert(QStringLiteral("geometry"), serializePolylineGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

void writePolygonGeometry(QJsonObject &object, const PolygonGeometry &normalizedGeometry, int canvasPx) {
    const double canvas = static_cast<double>(canvasPx);
    const PolygonGeometry pixelGeometry = scaledPolygonGeometry(normalizedGeometry, canvas);
    object.insert(QStringLiteral("cx"), normalizedGeometry.center.x);
    object.insert(QStringLiteral("cy"), normalizedGeometry.center.y);
    object.insert(QStringLiteral("center_px"), pointArray(pixelGeometry.center));
    object.insert(QStringLiteral("radius"), normalizedGeometry.radius);
    object.insert(QStringLiteral("radius_px"), pixelGeometry.radius);
    object.insert(QStringLiteral("sides"), normalizedGeometry.sides);
    object.insert(QStringLiteral("rotation_deg"), normalizedGeometry.rotationDeg);
    object.insert(QStringLiteral("points"), pointsToArray(normalizedGeometry.points));
    object.insert(QStringLiteral("points_px"), pointsToArray(pixelGeometry.points));
    object.insert(QStringLiteral("geometry"), serializePolygonGeometry(pixelGeometry));
    object.insert(QStringLiteral("bounds"), serializeBounds(computeBounds(pixelGeometry)));
}

QString shapeKindName(ShapeKind kind) {
    switch (kind) {
    case ShapeKind::Point:
        return QStringLiteral("point");
    case ShapeKind::Line:
        return QStringLiteral("line");
    case ShapeKind::Circle:
        return QStringLiteral("circle");
    case ShapeKind::Rectangle:
        return QStringLiteral("rectangle");
    case ShapeKind::Polyline:
        return QStringLiteral("polyline");
    case ShapeKind::Polygon:
        return QStringLiteral("polygon");
    case ShapeKind::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

bool geometryMatchesKind(ShapeKind kind, const Geometry &geometry) {
    switch (kind) {
    case ShapeKind::Point:
        return std::holds_alternative<PointGeometry>(geometry);
    case ShapeKind::Line:
        return std::holds_alternative<LineGeometry>(geometry);
    case ShapeKind::Circle:
        return std::holds_alternative<CircleGeometry>(geometry) || std::holds_alternative<ArcGeometry>(geometry);
    case ShapeKind::Rectangle:
        return std::holds_alternative<RectangleGeometry>(geometry);
    case ShapeKind::Polyline:
        return std::holds_alternative<PolylineGeometry>(geometry);
    case ShapeKind::Polygon:
        return std::holds_alternative<PolygonGeometry>(geometry);
    case ShapeKind::Unknown:
        return false;
    }
    return false;
}

Bounds2D computeBounds(const Geometry &geometry) {
    return std::visit(
        [](const auto &value) -> Bounds2D {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return {};
            } else {
                return computeBounds(value);
            }
        },
        geometry);
}

void recomputeBounds(DrawingObject &object) {
    object.bounds = computeBounds(object.geometry);
}

QJsonObject serializeDrawingObject(const DrawingObject &object, int canvasPx) {
    QJsonObject serialized = object.attributes;
    serialized.insert(QStringLiteral("id"), object.id.value);
    serialized.insert(QStringLiteral("kind"), serialized.value(QStringLiteral("kind")).toString(shapeKindName(object.kind)));
    serialized.insert(QStringLiteral("style_id"), object.style.value);
    serialized.insert(QStringLiteral("layer_id"), object.layer.value);
    serialized.insert(QStringLiteral("metadata"), object.metadata.values);
    serialized.insert(QStringLiteral("bounds"), serializeBounds(Bounds2D{
        object.bounds.ok,
        object.bounds.x * canvasPx,
        object.bounds.y * canvasPx,
        object.bounds.w * canvasPx,
        object.bounds.h * canvasPx,
    }));

    std::visit(
        [&](const auto &value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, PointGeometry>) {
                writePointGeometry(serialized, value, canvasPx);
            } else if constexpr (std::is_same_v<T, LineGeometry>) {
                writeLineGeometry(serialized, value, canvasPx);
            } else if constexpr (std::is_same_v<T, CircleGeometry>) {
                writeCircleGeometry(serialized, value, canvasPx);
            } else if constexpr (std::is_same_v<T, ArcGeometry>) {
                writeArcGeometry(serialized, value, canvasPx);
            } else if constexpr (std::is_same_v<T, RectangleGeometry>) {
                writeRectangleGeometry(serialized, value, canvasPx);
            } else if constexpr (std::is_same_v<T, PolylineGeometry>) {
                writePolylineGeometry(serialized, value, canvasPx);
            } else if constexpr (std::is_same_v<T, PolygonGeometry>) {
                writePolygonGeometry(serialized, value, canvasPx);
            }
        },
        object.geometry);
    return serialized;
}

bool DrawingStore::addObject(DrawingObject object) {
    if (object.id.value.isEmpty() || contains(object.id) || !geometryMatchesKind(object.kind, object.geometry)) {
        return false;
    }
    recomputeBounds(object);
    m_indexById.insert(object.id.value, static_cast<int>(m_objects.size()));
    m_objects.push_back(std::move(object));
    return true;
}

bool DrawingStore::removeObject(const ObjectId &id) {
    const auto found = m_indexById.constFind(id.value);
    if (found == m_indexById.constEnd()) {
        return false;
    }
    m_objects.erase(m_objects.begin() + found.value());
    rebuildIndex();
    return true;
}

bool DrawingStore::updateGeometry(const ObjectId &id, Geometry geometry) {
    DrawingObject *object = find(id);
    if (object == nullptr || !geometryMatchesKind(object->kind, geometry)) {
        return false;
    }
    object->geometry = std::move(geometry);
    recomputeBounds(*object);
    return true;
}

DrawingObject *DrawingStore::find(const ObjectId &id) {
    const auto found = m_indexById.constFind(id.value);
    if (found == m_indexById.constEnd()) {
        return nullptr;
    }
    return &m_objects[found.value()];
}

const DrawingObject *DrawingStore::find(const ObjectId &id) const {
    const auto found = m_indexById.constFind(id.value);
    if (found == m_indexById.constEnd()) {
        return nullptr;
    }
    return &m_objects[found.value()];
}

bool DrawingStore::contains(const ObjectId &id) const {
    return m_indexById.contains(id.value);
}

int DrawingStore::size() const {
    return static_cast<int>(m_objects.size());
}

QJsonObject DrawingStore::serializeObject(const ObjectId &id, int canvasPx) const {
    const DrawingObject *object = find(id);
    return object == nullptr ? QJsonObject() : serializeDrawingObject(*object, canvasPx);
}

QJsonArray DrawingStore::serializeObjects(int canvasPx) const {
    QJsonArray objects;
    for (const DrawingObject &object : m_objects) {
        objects.append(serializeDrawingObject(object, canvasPx));
    }
    return objects;
}

void DrawingStore::rebuildIndex() {
    m_indexById.clear();
    for (int index = 0; index < static_cast<int>(m_objects.size()); ++index) {
        m_indexById.insert(m_objects[index].id.value, index);
    }
}

} // namespace drawing_core
