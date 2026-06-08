#include "core/DrawingObjectModel.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QTextStream>

#include <cmath>

namespace {

bool expect(bool condition, const QString &message) {
    if (!condition) {
        QTextStream(stderr) << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

bool expectNear(double actual, double expected, const QString &message) {
    return expect(std::abs(actual - expected) < 0.0001,
                  QStringLiteral("%1; expected %2, got %3").arg(message).arg(expected).arg(actual));
}

bool runDrawingStoreLineContract() {
    using namespace drawing_core;

    DrawingStore store;
    DrawingObject line;
    line.id = {QStringLiteral("obj_line_01")};
    line.kind = ShapeKind::Line;
    line.geometry = LineGeometry{{0.125, 0.25}, {0.625, 0.75}};
    line.style = {QStringLiteral("stroke_default")};
    line.layer = {QStringLiteral("layer_test")};
    line.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("store_contract_test"));
    line.metadata.values.insert(QStringLiteral("version"), 1);
    line.attributes.insert(QStringLiteral("label"), QStringLiteral("contract line"));

    bool ok = true;
    ok &= expect(store.addObject(line), QStringLiteral("store should add a valid line object"));
    ok &= expect(!store.addObject(line), QStringLiteral("store should reject duplicate object ids"));
    ok &= expect(store.size() == 1, QStringLiteral("store should keep dense size after duplicate rejection"));

    const DrawingObject *storedLine = store.find({QStringLiteral("obj_line_01")});
    ok &= expect(storedLine != nullptr, QStringLiteral("store should find an object by id"));
    if (storedLine != nullptr) {
        ok &= expectNear(storedLine->bounds.x, 0.125, QStringLiteral("store line bounds x should derive from geometry"));
        ok &= expectNear(storedLine->bounds.w, 0.5, QStringLiteral("store line bounds width should derive from geometry"));
    }

    DrawingObject invalid;
    invalid.id = {QStringLiteral("obj_bad_01")};
    invalid.kind = ShapeKind::Point;
    invalid.geometry = LineGeometry{{0.0, 0.0}, {1.0, 1.0}};
    ok &= expect(!store.addObject(invalid), QStringLiteral("store should reject geometry that does not match shape kind"));

    ok &= expect(store.updateGeometry({QStringLiteral("obj_line_01")}, LineGeometry{{0.5, 0.5}, {0.75, 0.5}}),
                 QStringLiteral("store should update line geometry"));
    storedLine = store.find({QStringLiteral("obj_line_01")});
    if (storedLine != nullptr) {
        ok &= expectNear(storedLine->bounds.x, 0.5, QStringLiteral("updated line bounds x should derive from geometry"));
        ok &= expectNear(storedLine->bounds.w, 0.25, QStringLiteral("updated line bounds width should derive from geometry"));
    }

    QJsonObject attributes;
    attributes.insert(QStringLiteral("label"), QStringLiteral("contract line"));
    attributes.insert(QStringLiteral("role"), QStringLiteral("guide"));
    ok &= expect(store.replaceAttributes({QStringLiteral("obj_line_01")}, attributes),
                 QStringLiteral("store should replace line attributes"));
    ok &= expect(store.translateObject({QStringLiteral("obj_line_01")}, -0.25, 0.125),
                 QStringLiteral("store should translate line geometry"));

    const QJsonObject serialized = store.serializeObject({QStringLiteral("obj_line_01")}, 512);
    ok &= expect(serialized.value(QStringLiteral("id")).toString() == QStringLiteral("obj_line_01"),
                 QStringLiteral("serialized line should preserve id"));
    ok &= expect(serialized.value(QStringLiteral("kind")).toString() == QStringLiteral("line"),
                 QStringLiteral("serialized line should preserve kind"));
    ok &= expect(serialized.value(QStringLiteral("role")).toString() == QStringLiteral("guide"),
                 QStringLiteral("serialized line should preserve replaced attributes"));
    ok &= expectNear(serialized.value(QStringLiteral("x2")).toDouble(), 0.5,
                     QStringLiteral("serialized line should keep normalized legacy fields"));
    ok &= expectNear(serialized.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("b")).toObject().value(QStringLiteral("x")).toDouble(), 256.0,
                     QStringLiteral("serialized line geometry should project pixels"));
    ok &= expectNear(serialized.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("serialized line bounds should project translated x"));
    ok &= expectNear(serialized.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("w")).toDouble(), 128.0,
                     QStringLiteral("serialized line bounds should project width"));

    ok &= expect(store.removeObject({QStringLiteral("obj_line_01")}), QStringLiteral("store should remove existing line"));
    ok &= expect(!store.contains({QStringLiteral("obj_line_01")}), QStringLiteral("store index should not contain removed line"));
    ok &= expect(!store.removeObject({QStringLiteral("obj_line_01")}), QStringLiteral("store should reject removing a missing line"));
    ok &= expect(store.size() == 0, QStringLiteral("store should be empty after line remove"));
    return ok;
}

bool runDrawingStorePointContract() {
    using namespace drawing_core;

    DrawingStore store;
    DrawingObject point;
    point.id = {QStringLiteral("obj_point_01")};
    point.kind = ShapeKind::Point;
    point.geometry = PointGeometry{{0.25, 0.5}};
    point.style = {QStringLiteral("stroke_default")};
    point.layer = {QStringLiteral("layer_test")};
    point.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("point_contract_test"));
    point.metadata.values.insert(QStringLiteral("version"), 1);
    point.attributes.insert(QStringLiteral("kind"), QStringLiteral("point"));
    point.attributes.insert(QStringLiteral("label"), QStringLiteral("contract point"));

    bool ok = true;
    ok &= expect(store.addObject(point), QStringLiteral("store should add a valid point object"));
    const DrawingObject *storedPoint = store.find({QStringLiteral("obj_point_01")});
    ok &= expect(storedPoint != nullptr, QStringLiteral("store should find point by id"));
    if (storedPoint != nullptr) {
        ok &= expectNear(storedPoint->bounds.x, 0.25, QStringLiteral("point bounds x should derive from geometry"));
        ok &= expectNear(storedPoint->bounds.y, 0.5, QStringLiteral("point bounds y should derive from geometry"));
        ok &= expectNear(storedPoint->bounds.w, 0.0, QStringLiteral("point bounds width should be zero"));
        ok &= expectNear(storedPoint->bounds.h, 0.0, QStringLiteral("point bounds height should be zero"));
    }

    ok &= expect(store.translateObject({QStringLiteral("obj_point_01")}, 0.125, -0.25),
                 QStringLiteral("store should translate point geometry"));
    ok &= expect(store.updateGeometry({QStringLiteral("obj_point_01")}, PointGeometry{{0.75, 0.25}}),
                 QStringLiteral("store should update point geometry"));

    QJsonObject attributes;
    attributes.insert(QStringLiteral("kind"), QStringLiteral("point"));
    attributes.insert(QStringLiteral("role"), QStringLiteral("anchor"));
    ok &= expect(store.replaceAttributes({QStringLiteral("obj_point_01")}, attributes),
                 QStringLiteral("store should replace point attributes"));

    const QJsonObject serialized = store.serializeObject({QStringLiteral("obj_point_01")}, 512);
    ok &= expect(serialized.value(QStringLiteral("id")).toString() == QStringLiteral("obj_point_01"),
                 QStringLiteral("serialized point should preserve id"));
    ok &= expect(serialized.value(QStringLiteral("kind")).toString() == QStringLiteral("point"),
                 QStringLiteral("serialized point should preserve kind"));
    ok &= expect(serialized.value(QStringLiteral("role")).toString() == QStringLiteral("anchor"),
                 QStringLiteral("serialized point should preserve attributes"));
    ok &= expectNear(serialized.value(QStringLiteral("x")).toDouble(), 0.75,
                     QStringLiteral("serialized point should keep normalized x"));
    ok &= expectNear(serialized.value(QStringLiteral("point_px")).toArray().at(0).toDouble(), 384.0,
                     QStringLiteral("serialized point should project pixel x"));
    ok &= expectNear(serialized.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble(), 384.0,
                     QStringLiteral("serialized point bounds should project pixel x"));
    return ok;
}

bool runDrawingStoreCircleArcContract() {
    using namespace drawing_core;

    DrawingStore store;
    DrawingObject circle;
    circle.id = {QStringLiteral("obj_circle_01")};
    circle.kind = ShapeKind::Circle;
    circle.geometry = CircleGeometry{{0.5, 0.5}, 0.125};
    circle.style = {QStringLiteral("stroke_default")};
    circle.layer = {QStringLiteral("layer_test")};
    circle.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("circle_contract_test"));
    circle.metadata.values.insert(QStringLiteral("version"), 1);
    circle.attributes.insert(QStringLiteral("kind"), QStringLiteral("circle"));

    DrawingObject arc;
    arc.id = {QStringLiteral("obj_arc_01")};
    arc.kind = ShapeKind::Circle;
    arc.geometry = ArcGeometry{{0.25, 0.25}, 0.125, 15.0, 120.0};
    arc.style = {QStringLiteral("stroke_default")};
    arc.layer = {QStringLiteral("layer_test")};
    arc.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("arc_contract_test"));
    arc.metadata.values.insert(QStringLiteral("version"), 1);
    arc.attributes.insert(QStringLiteral("kind"), QStringLiteral("arc"));

    bool ok = true;
    ok &= expect(store.addObject(circle), QStringLiteral("store should add a valid circle object"));
    ok &= expect(store.addObject(arc), QStringLiteral("store should add a valid arc object"));
    ok &= expect(store.updateGeometry({QStringLiteral("obj_circle_01")}, CircleGeometry{{0.625, 0.5}, 0.25}),
                 QStringLiteral("store should update circle geometry"));
    ok &= expect(store.updateGeometry({QStringLiteral("obj_arc_01")}, ArcGeometry{{0.25, 0.375}, 0.25, 30.0, 180.0}),
                 QStringLiteral("store should update arc geometry"));

    const QJsonObject serializedCircle = store.serializeObject({QStringLiteral("obj_circle_01")}, 512);
    ok &= expect(serializedCircle.value(QStringLiteral("kind")).toString() == QStringLiteral("circle"),
                 QStringLiteral("serialized circle should preserve projected kind"));
    ok &= expectNear(serializedCircle.value(QStringLiteral("cx")).toDouble(), 0.625,
                     QStringLiteral("serialized circle should keep normalized center x"));
    ok &= expectNear(serializedCircle.value(QStringLiteral("radius_px")).toDouble(), 128.0,
                     QStringLiteral("serialized circle should project radius"));
    ok &= expectNear(serializedCircle.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("w")).toDouble(), 256.0,
                     QStringLiteral("serialized circle bounds should project diameter"));

    const QJsonObject serializedArc = store.serializeObject({QStringLiteral("obj_arc_01")}, 512);
    ok &= expect(serializedArc.value(QStringLiteral("kind")).toString() == QStringLiteral("arc"),
                 QStringLiteral("serialized arc should preserve projected kind"));
    ok &= expectNear(serializedArc.value(QStringLiteral("start_angle_deg")).toDouble(), 30.0,
                     QStringLiteral("serialized arc should preserve start angle"));
    ok &= expectNear(serializedArc.value(QStringLiteral("end_angle_deg")).toDouble(), 180.0,
                     QStringLiteral("serialized arc should preserve end angle"));
    ok &= expectNear(serializedArc.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("radius")).toDouble(), 128.0,
                     QStringLiteral("serialized arc should project radius geometry"));
    ok &= expectNear(serializedArc.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("y")).toDouble(), 64.0,
                     QStringLiteral("serialized arc bounds should derive from center and radius"));
    return ok;
}

bool runDrawingStoreRectangleContract() {
    using namespace drawing_core;

    DrawingStore store;
    DrawingObject rectangle;
    rectangle.id = {QStringLiteral("obj_rectangle_01")};
    rectangle.kind = ShapeKind::Rectangle;
    rectangle.geometry = RectangleGeometry{{0.125, 0.25}, 0.5, 0.25, 0.0};
    rectangle.style = {QStringLiteral("stroke_default")};
    rectangle.layer = {QStringLiteral("layer_test")};
    rectangle.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("rectangle_contract_test"));
    rectangle.metadata.values.insert(QStringLiteral("version"), 1);
    rectangle.attributes.insert(QStringLiteral("kind"), QStringLiteral("image_reference_frame"));
    rectangle.attributes.insert(QStringLiteral("label"), QStringLiteral("contract image frame"));

    bool ok = true;
    ok &= expect(store.addObject(rectangle), QStringLiteral("store should add a valid rectangle object"));
    ok &= expect(store.updateGeometry({QStringLiteral("obj_rectangle_01")}, RectangleGeometry{{0.25, 0.375}, 0.25, 0.125, 15.0}),
                 QStringLiteral("store should update rectangle geometry"));

    const DrawingObject *storedRectangle = store.find({QStringLiteral("obj_rectangle_01")});
    ok &= expect(storedRectangle != nullptr, QStringLiteral("store should find rectangle by id"));
    if (storedRectangle != nullptr) {
        ok &= expectNear(storedRectangle->bounds.x, 0.25, QStringLiteral("rectangle bounds x should derive from geometry"));
        ok &= expectNear(storedRectangle->bounds.w, 0.25, QStringLiteral("rectangle bounds width should derive from geometry"));
    }

    const QJsonObject serialized = store.serializeObject({QStringLiteral("obj_rectangle_01")}, 512);
    ok &= expect(serialized.value(QStringLiteral("kind")).toString() == QStringLiteral("image_reference_frame"),
                 QStringLiteral("serialized rectangle should preserve specialized projected kind"));
    ok &= expectNear(serialized.value(QStringLiteral("x")).toDouble(), 0.25,
                     QStringLiteral("serialized rectangle should keep normalized x"));
    ok &= expectNear(serialized.value(QStringLiteral("rect_px")).toArray().at(2).toDouble(), 128.0,
                     QStringLiteral("serialized rectangle should project width"));
    ok &= expectNear(serialized.value(QStringLiteral("rotation_deg")).toDouble(), 15.0,
                     QStringLiteral("serialized rectangle should preserve rotation"));
    ok &= expectNear(serialized.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("serialized rectangle bounds should project x"));
    ok &= expectNear(serialized.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("h")).toDouble(), 64.0,
                     QStringLiteral("serialized rectangle bounds should project height"));
    return ok;
}

bool runDrawingStorePolylinePolygonContract() {
    using namespace drawing_core;

    DrawingStore store;
    DrawingObject polyline;
    polyline.id = {QStringLiteral("obj_polyline_01")};
    polyline.kind = ShapeKind::Polyline;
    polyline.geometry = PolylineGeometry{{{0.125, 0.125}, {0.25, 0.375}, {0.5, 0.25}}};
    polyline.style = {QStringLiteral("stroke_default")};
    polyline.layer = {QStringLiteral("layer_test")};
    polyline.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("polyline_contract_test"));
    polyline.metadata.values.insert(QStringLiteral("version"), 1);
    polyline.attributes.insert(QStringLiteral("kind"), QStringLiteral("polyline"));

    DrawingObject polygon;
    polygon.id = {QStringLiteral("obj_polygon_01")};
    polygon.kind = ShapeKind::Polygon;
    polygon.geometry = PolygonGeometry{{0.5, 0.5}, 0.25, 4, 0.0, {{0.75, 0.5}, {0.5, 0.75}, {0.25, 0.5}, {0.5, 0.25}}};
    polygon.style = {QStringLiteral("stroke_default")};
    polygon.layer = {QStringLiteral("layer_test")};
    polygon.metadata.values.insert(QStringLiteral("created_by"), QStringLiteral("polygon_contract_test"));
    polygon.metadata.values.insert(QStringLiteral("version"), 1);
    polygon.attributes.insert(QStringLiteral("kind"), QStringLiteral("polygon"));

    bool ok = true;
    ok &= expect(store.addObject(polyline), QStringLiteral("store should add a valid polyline object"));
    ok &= expect(store.addObject(polygon), QStringLiteral("store should add a valid polygon object"));
    ok &= expect(store.translateObject({QStringLiteral("obj_polyline_01")}, 0.125, 0.0),
                 QStringLiteral("store should translate polyline geometry"));
    ok &= expect(store.updateGeometry({QStringLiteral("obj_polygon_01")}, PolygonGeometry{{0.25, 0.25}, 0.125, 3, 30.0, {{0.358253, 0.3125}, {0.141747, 0.3125}, {0.25, 0.125}}}),
                 QStringLiteral("store should update polygon geometry"));

    const QJsonObject serializedPolyline = store.serializeObject({QStringLiteral("obj_polyline_01")}, 512);
    ok &= expect(serializedPolyline.value(QStringLiteral("kind")).toString() == QStringLiteral("polyline"),
                 QStringLiteral("serialized polyline should preserve kind"));
    ok &= expect(serializedPolyline.value(QStringLiteral("points")).toArray().size() == 3,
                 QStringLiteral("serialized polyline should preserve normalized points"));
    ok &= expectNear(serializedPolyline.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("points")).toArray().at(0).toArray().at(0).toDouble(), 128.0,
                     QStringLiteral("serialized polyline geometry should project translated x"));
    ok &= expectNear(serializedPolyline.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("x")).toDouble(), 128.0,
                     QStringLiteral("serialized polyline bounds should derive from translated points"));

    const QJsonObject serializedPolygon = store.serializeObject({QStringLiteral("obj_polygon_01")}, 512);
    ok &= expect(serializedPolygon.value(QStringLiteral("kind")).toString() == QStringLiteral("polygon"),
                 QStringLiteral("serialized polygon should preserve kind"));
    ok &= expect(serializedPolygon.value(QStringLiteral("sides")).toInt() == 3,
                 QStringLiteral("serialized polygon should preserve side count"));
    ok &= expectNear(serializedPolygon.value(QStringLiteral("rotation_deg")).toDouble(), 30.0,
                     QStringLiteral("serialized polygon should preserve rotation"));
    ok &= expect(serializedPolygon.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("points")).toArray().size() == 3,
                 QStringLiteral("serialized polygon geometry should project points"));
    ok &= expectNear(serializedPolygon.value(QStringLiteral("bounds")).toObject().value(QStringLiteral("y")).toDouble(), 64.0,
                     QStringLiteral("serialized polygon bounds should derive from points"));
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    bool ok = true;
    ok &= runDrawingStoreLineContract();
    ok &= runDrawingStorePointContract();
    ok &= runDrawingStoreCircleArcContract();
    ok &= runDrawingStoreRectangleContract();
    ok &= runDrawingStorePolylinePolygonContract();
    return ok ? 0 : 1;
}
