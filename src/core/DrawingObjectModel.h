#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <variant>
#include <vector>

namespace drawing_core {

enum class ShapeKind {
    Unknown,
    Point,
    Line,
    Circle,
    Rectangle,
    Polyline,
    Polygon,
};

struct ObjectId {
    QString value;
};

struct LayerId {
    QString value;
};

struct StyleRef {
    QString value;
};

struct ObjectFlags {
    bool locked = false;
    bool hidden = false;
    bool selectable = true;
};

struct Metadata {
    QJsonObject values;
};

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct Bounds2D {
    bool ok = false;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
};

struct LineGeometry {
    Point2D a;
    Point2D b;
};

struct PointGeometry {
    Point2D point;
};

struct CircleGeometry {
    Point2D center;
    double radius = 0.0;
};

struct ArcGeometry {
    Point2D center;
    double radius = 0.0;
    double startAngleDeg = 0.0;
    double endAngleDeg = 90.0;
};

struct RectangleGeometry {
    Point2D origin;
    double width = 0.0;
    double height = 0.0;
    double rotationDeg = 0.0;
};

struct PolylineGeometry {
    std::vector<Point2D> points;
};

struct PolygonGeometry {
    Point2D center;
    double radius = 0.0;
    int sides = 3;
    double rotationDeg = 0.0;
    std::vector<Point2D> points;
};

using Geometry = std::variant<std::monostate, PointGeometry, LineGeometry, CircleGeometry, ArcGeometry, RectangleGeometry, PolylineGeometry, PolygonGeometry>;

struct DrawingObject {
    ObjectId id;
    ShapeKind kind = ShapeKind::Unknown;
    Geometry geometry;
    StyleRef style;
    Bounds2D bounds;
    LayerId layer;
    ObjectFlags flags;
    Metadata metadata;
    QJsonObject attributes;
};

class DrawingStore {
public:
    bool addObject(DrawingObject object);
    bool removeObject(const ObjectId &id);
    bool updateGeometry(const ObjectId &id, Geometry geometry);
    bool translateObject(const ObjectId &id, double dx, double dy);
    bool replaceAttributes(const ObjectId &id, QJsonObject attributes);
    DrawingObject *find(const ObjectId &id);
    const DrawingObject *find(const ObjectId &id) const;
    bool contains(const ObjectId &id) const;
    int size() const;
    QJsonObject serializeObject(const ObjectId &id, int canvasPx) const;
    QJsonArray serializeObjects(int canvasPx) const;

private:
    void rebuildIndex();

    std::vector<DrawingObject> m_objects;
    QHash<QString, int> m_indexById;
};

Bounds2D computeBounds(const LineGeometry &line);
Bounds2D computeBounds(const PointGeometry &point);
Bounds2D computeBounds(const CircleGeometry &circle);
Bounds2D computeBounds(const ArcGeometry &arc);
Bounds2D computeBounds(const RectangleGeometry &rectangle);
Bounds2D computeBounds(const PolylineGeometry &polyline);
Bounds2D computeBounds(const PolygonGeometry &polygon);
PointGeometry scaledPointGeometry(const PointGeometry &point, double scale);
LineGeometry scaledLineGeometry(const LineGeometry &line, double scale);
CircleGeometry scaledCircleGeometry(const CircleGeometry &circle, double scale);
ArcGeometry scaledArcGeometry(const ArcGeometry &arc, double scale);
RectangleGeometry scaledRectangleGeometry(const RectangleGeometry &rectangle, double scale);
PolylineGeometry scaledPolylineGeometry(const PolylineGeometry &polyline, double scale);
PolygonGeometry scaledPolygonGeometry(const PolygonGeometry &polygon, double scale);
PointGeometry translatedPointGeometry(const PointGeometry &point, double dx, double dy);
LineGeometry translatedLineGeometry(const LineGeometry &line, double dx, double dy);
CircleGeometry translatedCircleGeometry(const CircleGeometry &circle, double dx, double dy);
ArcGeometry translatedArcGeometry(const ArcGeometry &arc, double dx, double dy);
RectangleGeometry translatedRectangleGeometry(const RectangleGeometry &rectangle, double dx, double dy);
PolylineGeometry translatedPolylineGeometry(const PolylineGeometry &polyline, double dx, double dy);
PolygonGeometry translatedPolygonGeometry(const PolygonGeometry &polygon, double dx, double dy);
PointGeometry pointGeometryFromObject(const QJsonObject &object);
LineGeometry lineGeometryFromObject(const QJsonObject &object);
CircleGeometry circleGeometryFromObject(const QJsonObject &object);
ArcGeometry arcGeometryFromObject(const QJsonObject &object);
RectangleGeometry rectangleGeometryFromObject(const QJsonObject &object);
PolylineGeometry polylineGeometryFromObject(const QJsonObject &object);
PolygonGeometry polygonGeometryFromObject(const QJsonObject &object);
void writePointGeometry(QJsonObject &object, const PointGeometry &normalizedGeometry, int canvasPx);
void writeLineGeometry(QJsonObject &object, const LineGeometry &normalizedGeometry, int canvasPx);
void writeCircleGeometry(QJsonObject &object, const CircleGeometry &normalizedGeometry, int canvasPx);
void writeArcGeometry(QJsonObject &object, const ArcGeometry &normalizedGeometry, int canvasPx);
void writeRectangleGeometry(QJsonObject &object, const RectangleGeometry &normalizedGeometry, int canvasPx);
void writePolylineGeometry(QJsonObject &object, const PolylineGeometry &normalizedGeometry, int canvasPx);
void writePolygonGeometry(QJsonObject &object, const PolygonGeometry &normalizedGeometry, int canvasPx);
QJsonObject serializePointGeometry(const PointGeometry &point);
QJsonObject serializeLineGeometry(const LineGeometry &line);
QJsonObject serializeCircleGeometry(const CircleGeometry &circle);
QJsonObject serializeArcGeometry(const ArcGeometry &arc);
QJsonObject serializeRectangleGeometry(const RectangleGeometry &rectangle);
QJsonObject serializePolylineGeometry(const PolylineGeometry &polyline);
QJsonObject serializePolygonGeometry(const PolygonGeometry &polygon);
QJsonObject serializeBounds(const Bounds2D &bounds);
QString shapeKindName(ShapeKind kind);
bool geometryMatchesKind(ShapeKind kind, const Geometry &geometry);
Bounds2D computeBounds(const Geometry &geometry);
Geometry translatedGeometry(const Geometry &geometry, double dx, double dy);
void recomputeBounds(DrawingObject &object);
QJsonObject serializeDrawingObject(const DrawingObject &object, int canvasPx);

} // namespace drawing_core
