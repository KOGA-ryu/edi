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

} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    bool ok = true;
    ok &= runDrawingStoreLineContract();
    ok &= runDrawingStorePointContract();
    return ok ? 0 : 1;
}
