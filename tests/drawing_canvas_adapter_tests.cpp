#include "canvas/DrawingCanvasRuntimeAdapter.h"

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

QVariantMap point(double x, double y) {
    return {{QStringLiteral("x"), x}, {QStringLiteral("y"), y}};
}

QVariantMap settings() {
    return {
        {QStringLiteral("canvasSizePx"), 512.0},
        {QStringLiteral("boardSizePx"), 512.0},
        {QStringLiteral("zoom"), 1.0},
        {QStringLiteral("gridEnabled"), true},
        {QStringLiteral("gridStepPx"), 32.0},
        {QStringLiteral("objectSnapEnabled"), true},
        {QStringLiteral("objectSnapTolerancePx"), 14.0},
        {QStringLiteral("endpointEnabled"), true},
        {QStringLiteral("midpointEnabled"), true},
        {QStringLiteral("centerEnabled"), true},
        {QStringLiteral("vertexEnabled"), true},
        {QStringLiteral("objectPriority"), QStringLiteral("before_grid")},
        {QStringLiteral("handleHitTolerancePx"), 14.0},
        {QStringLiteral("rotateHandleHitTolerancePx"), 18.0},
        {QStringLiteral("rotateHandleOffsetPx"), 28.0}
    };
}

bool testViewportAdapter(DrawingCanvasRuntimeAdapter &adapter) {
    bool ok = true;
    const QVariantMap bounds = adapter.boardBounds(800.0, 600.0, 1.5, 12.0, -8.0);
    ok &= expectNear(bounds.value(QStringLiteral("size")).toDouble(), 876.0, QStringLiteral("adapter boardBounds size"));
    ok &= expectNear(bounds.value(QStringLiteral("x")).toDouble(), -26.0, QStringLiteral("adapter boardBounds x"));

    const QVariantMap screen = adapter.canvasToScreen(bounds, point(0.375, 0.625));
    const QVariantMap normalized = adapter.screenToCanvas(bounds, screen.value(QStringLiteral("x")).toDouble(), screen.value(QStringLiteral("y")).toDouble());
    ok &= expectNear(normalized.value(QStringLiteral("x")).toDouble(), 0.375, QStringLiteral("adapter viewport x round trip"));
    ok &= expectNear(normalized.value(QStringLiteral("y")).toDouble(), 0.625, QStringLiteral("adapter viewport y round trip"));
    ok &= expectNear(adapter.canvasToScreenX(bounds, 0.375), screen.value(QStringLiteral("x")).toDouble(), QStringLiteral("adapter canvasToScreenX"));
    ok &= expectNear(adapter.canvasToScreenY(bounds, 0.625), screen.value(QStringLiteral("y")).toDouble(), QStringLiteral("adapter canvasToScreenY"));
    return ok;
}

bool testHandleAdapter(DrawingCanvasRuntimeAdapter &adapter) {
    bool ok = true;
    const QVariantMap line {
        {QStringLiteral("id"), QStringLiteral("script_line")},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("x1"), 0.25},
        {QStringLiteral("y1"), 0.125},
        {QStringLiteral("x2"), 0.75},
        {QStringLiteral("y2"), 0.875}
    };
    const QVariantList handles = adapter.handlesForObject(line, settings());
    ok &= expect(handles.size() == 2, QStringLiteral("adapter should return line handles as QVariantList"));
    ok &= expect(handles.at(0).toMap().value(QStringLiteral("id")).toString() == QStringLiteral("line_start"), QStringLiteral("adapter handle id"));

    const QVariantMap plan = adapter.handleUpdatePlan(line, QStringLiteral("line_start"), point(0.5, 0.25), settings());
    ok &= expect(plan.value(QStringLiteral("ok")).toBool(), QStringLiteral("adapter handle plan ok"));
    const QVariantList updates = plan.value(QStringLiteral("updates")).toList();
    ok &= expect(updates.size() == 2, QStringLiteral("adapter handle plan updates list"));
    ok &= expect(updates.at(0).toMap().value(QStringLiteral("field")).toString() == QStringLiteral("x1_px"), QStringLiteral("adapter handle plan field"));
    ok &= expectNear(updates.at(0).toMap().value(QStringLiteral("value")).toDouble(), 256.0, QStringLiteral("adapter handle plan value"));

    const QVariantMap viewport {{QStringLiteral("x"), 0.0}, {QStringLiteral("y"), 0.0}, {QStringLiteral("size"), 512.0}};
    const QVariantMap hit = adapter.hitHandleAt(line, 129.0, 65.0, viewport, settings());
    ok &= expect(hit.value(QStringLiteral("ok")).toBool(), QStringLiteral("adapter should hit handle"));
    ok &= expect(hit.value(QStringLiteral("id")).toString() == QStringLiteral("line_start"), QStringLiteral("adapter hit handle id"));
    ok &= expect(hit.value(QStringLiteral("handle")).toMap().value(QStringLiteral("id")).toString() == QStringLiteral("line_start"), QStringLiteral("adapter hit should include handle map"));
    return ok;
}

bool testHitSnapProjectionGestureAdapter(DrawingCanvasRuntimeAdapter &adapter) {
    bool ok = true;
    const QVariantMap line {
        {QStringLiteral("id"), QStringLiteral("script_line")},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("x1"), 0.25},
        {QStringLiteral("y1"), 0.25},
        {QStringLiteral("x2"), 0.75},
        {QStringLiteral("y2"), 0.25}
    };
    const QVariantList objects {line};
    const QVariantMap hit = adapter.hitObjectAt(objects, 0.5, 0.25, 0.025);
    ok &= expect(hit.value(QStringLiteral("kind")).toString() == QStringLiteral("object"), QStringLiteral("adapter hit object kind"));
    ok &= expect(hit.value(QStringLiteral("objectId")).toString() == QStringLiteral("script_line"), QStringLiteral("adapter hit object id"));

    const QVariantMap snap = adapter.resolveSnap(point(0.252, 0.252), objects, settings());
    ok &= expect(snap.value(QStringLiteral("kind")).toString() == QStringLiteral("object"), QStringLiteral("adapter object snap kind"));
    ok &= expect(snap.value(QStringLiteral("sourceObjectId")).toString() == QStringLiteral("script_line"), QStringLiteral("adapter object snap source id"));

    QVariantMap gridSettings = settings();
    gridSettings.insert(QStringLiteral("objectSnapEnabled"), false);
    ok &= expectNear(adapter.effectiveGridStepPx(gridSettings), 32.0, QStringLiteral("adapter effective grid step"));
    const QVariantMap grid = adapter.gridSnap(point(0.52, -0.1), gridSettings);
    ok &= expect(grid.value(QStringLiteral("kind")).toString() == QStringLiteral("grid"), QStringLiteral("adapter grid snap kind"));
    ok &= expectNear(grid.value(QStringLiteral("x")).toDouble(), 0.5, QStringLiteral("adapter grid snap x"));
    ok &= expectNear(grid.value(QStringLiteral("y")).toDouble(), 0.0, QStringLiteral("adapter grid snap y clamp"));

    gridSettings.insert(QStringLiteral("gridEnabled"), false);
    const QVariantMap none = adapter.noneSnap(point(1.2, 0.25), gridSettings);
    ok &= expect(none.value(QStringLiteral("kind")).toString() == QStringLiteral("none"), QStringLiteral("adapter none snap kind"));
    ok &= expectNear(none.value(QStringLiteral("x")).toDouble(), 1.0, QStringLiteral("adapter none snap clamps x"));

    const QVariantMap bounds = adapter.normalizedObjectBounds(line);
    ok &= expect(bounds.value(QStringLiteral("ok")).toBool(), QStringLiteral("adapter object bounds ok"));
    ok &= expectNear(bounds.value(QStringLiteral("minX")).toDouble(), 0.25, QStringLiteral("adapter object bounds min x"));

    QVariantMap gesture = adapter.beginObjectDrag(adapter.initialGestureState(), QStringLiteral("script_line"), point(0.1, 0.1), QVariantList{QStringLiteral("script_line")}, {});
    gesture = adapter.updateGesture(gesture, {{QStringLiteral("point"), point(0.2, 0.25)}});
    const QVariantMap finish = adapter.finishGesture(gesture, {{QStringLiteral("point"), point(0.2, 0.25)}}).value(QStringLiteral("intent")).toMap();
    ok &= expect(finish.value(QStringLiteral("kind")).toString() == QStringLiteral("move_object"), QStringLiteral("adapter gesture finish kind"));
    ok &= expectNear(finish.value(QStringLiteral("dx")).toDouble(), 0.1, QStringLiteral("adapter gesture dx"));
    return ok;
}

} // namespace

int main() {
    DrawingCanvasRuntimeAdapter adapter;
    bool ok = true;
    ok &= testViewportAdapter(adapter);
    ok &= testHandleAdapter(adapter);
    ok &= testHitSnapProjectionGestureAdapter(adapter);
    return ok ? 0 : 1;
}
