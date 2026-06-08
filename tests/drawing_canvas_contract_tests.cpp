#include "canvas/DrawingCanvasGestureState.h"
#include "canvas/DrawingCanvasHandles.h"
#include "canvas/DrawingCanvasHitTest.h"
#include "canvas/DrawingCanvasProjection.h"
#include "canvas/DrawingCanvasSnap.h"
#include "canvas/DrawingCanvasViewport.h"

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

QVariantList pointList(std::initializer_list<QVariantList> points) {
    QVariantList result;
    for (const QVariantList &entry : points) {
        result.push_back(entry);
    }
    return result;
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

bool testViewport() {
    using namespace drawing_canvas;
    bool ok = true;
    const BoardBounds bounds = boardBounds(800, 600, 1.0, 0.0, 0.0);
    ok &= expectNear(bounds.x, 108.0, QStringLiteral("viewport x should center board"));
    ok &= expectNear(bounds.y, 8.0, QStringLiteral("viewport y should center board"));
    ok &= expectNear(bounds.size, 584.0, QStringLiteral("viewport size should leave margin"));
    const CanvasPoint canvas = screenToCanvas(bounds, canvasToScreen(bounds, {0.25, 0.75}));
    ok &= expectNear(canvas.x, 0.25, QStringLiteral("viewport x round trip"));
    ok &= expectNear(canvas.y, 0.75, QStringLiteral("viewport y round trip"));
    return ok;
}

bool testHandles() {
    using namespace drawing_canvas;
    bool ok = true;
    const QVariantMap line {
        {QStringLiteral("id"), QStringLiteral("script_line")},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("x1"), 0.1},
        {QStringLiteral("y1"), 0.2},
        {QStringLiteral("x2"), 0.7},
        {QStringLiteral("y2"), 0.8}
    };
    const std::vector<HandleDescriptor> handles = handlesForObject({line}, settings());
    ok &= expect(handles.size() == 2, QStringLiteral("line should expose two handles"));
    ok &= expect(handles[0].id == QStringLiteral("line_start"), QStringLiteral("line start handle id should be stable"));
    const HandleUpdatePlan plan = handleUpdatePlan({line}, QStringLiteral("line_start"), {0.25, 0.5}, settings());
    ok &= expect(plan.ok, QStringLiteral("line start handle should produce legal update plan"));
    ok &= expect(plan.updates.size() == 2, QStringLiteral("line start plan should update x1/y1"));
    ok &= expect(plan.updates[0].field == QStringLiteral("x1_px"), QStringLiteral("line start x field"));
    ok &= expectNear(plan.updates[0].value, 128.0, QStringLiteral("line start x px"));

    const QVariantMap polygon {
        {QStringLiteral("id"), QStringLiteral("script_polygon")},
        {QStringLiteral("kind"), QStringLiteral("polygon")},
        {QStringLiteral("points"), pointList({QVariantList{0.1, 0.1}, QVariantList{0.5, 0.1}, QVariantList{0.4, 0.7}})}
    };
    const std::vector<HandleDescriptor> vertexHandles = handlesForObject({polygon}, settings());
    ok &= expect(vertexHandles.size() == 3, QStringLiteral("polygon should expose vertex handles"));
    ok &= expect(vertexHandles[0].readOnly, QStringLiteral("polygon first pass handles should be read-only"));
    ok &= expect(!handleUpdatePlan({polygon}, QStringLiteral("vertex_0"), {0.2, 0.2}, settings()).ok,
                 QStringLiteral("read-only polygon handle should not produce updates"));
    return ok;
}

bool testHitTest() {
    using namespace drawing_canvas;
    bool ok = true;
    const QVariantList objects {
        QVariantMap{{QStringLiteral("id"), QStringLiteral("script_low")}, {QStringLiteral("kind"), QStringLiteral("point")}, {QStringLiteral("x"), 0.5}, {QStringLiteral("y"), 0.5}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("script_top")}, {QStringLiteral("kind"), QStringLiteral("point")}, {QStringLiteral("x"), 0.5}, {QStringLiteral("y"), 0.5}}
    };
    const HitResult hit = hitObjectAt(objectsFromVariantList(objects), 0.5, 0.5, 0.02);
    ok &= expect(hit.ok, QStringLiteral("hit test should find point"));
    ok &= expect(hit.objectId == QStringLiteral("script_top"), QStringLiteral("hit test should prefer topmost object"));

    const QVariantMap line {
        {QStringLiteral("id"), QStringLiteral("script_line")},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("x1"), 0.0},
        {QStringLiteral("y1"), 0.0},
        {QStringLiteral("x2"), 1.0},
        {QStringLiteral("y2"), 0.0}
    };
    ok &= expectNear(objectHitScore({line}, 0.5, 0.02), 0.02, QStringLiteral("line hit score should be distance to segment"));
    return ok;
}

bool testSnap() {
    using namespace drawing_canvas;
    bool ok = true;
    const QVariantMap line {
        {QStringLiteral("id"), QStringLiteral("script_line")},
        {QStringLiteral("kind"), QStringLiteral("line")},
        {QStringLiteral("x1"), 0.25},
        {QStringLiteral("y1"), 0.25},
        {QStringLiteral("x2"), 0.75},
        {QStringLiteral("y2"), 0.25}
    };
    const SnapResult objectSnap = resolveSnap({0.252, 0.252}, objectsFromVariantList({line}), settings());
    ok &= expect(objectSnap.kind == QStringLiteral("object"), QStringLiteral("object snap should win before grid"));
    ok &= expect(objectSnap.sourceObjectId == QStringLiteral("script_line"), QStringLiteral("object snap should explain source id"));
    ok &= expect(objectSnap.label == QStringLiteral("endpoint"), QStringLiteral("object snap should explain label"));

    QVariantMap gridSettings = settings();
    gridSettings.insert(QStringLiteral("objectSnapEnabled"), false);
    const SnapResult grid = resolveSnap({0.51, 0.51}, {}, gridSettings);
    ok &= expect(grid.kind == QStringLiteral("grid"), QStringLiteral("grid snap should apply when object snap disabled"));
    ok &= expectNear(grid.x, 0.5, QStringLiteral("grid snap x"));
    ok &= expect(grid.label.startsWith(QStringLiteral("grid ")), QStringLiteral("grid snap should explain step"));
    return ok;
}

bool testProjection() {
    using namespace drawing_canvas;
    bool ok = true;
    const QVariantMap rectangle {
        {QStringLiteral("id"), QStringLiteral("script_rect")},
        {QStringLiteral("kind"), QStringLiteral("rectangle")},
        {QStringLiteral("x"), 0.2},
        {QStringLiteral("y"), 0.3},
        {QStringLiteral("width"), 0.4},
        {QStringLiteral("height"), 0.2},
        {QStringLiteral("rotation_deg"), 0.0}
    };
    const CanvasBounds bounds = normalizedObjectBounds({rectangle});
    ok &= expect(bounds.ok, QStringLiteral("rectangle bounds should be valid"));
    ok &= expectNear(bounds.minX, 0.2, QStringLiteral("rectangle min x"));
    ok &= expectNear(bounds.maxY, 0.5, QStringLiteral("rectangle max y"));
    ok &= expect(objectIntersectsBounds({rectangle}, 0.1, 0.1, 0.25, 0.35), QStringLiteral("rectangle should intersect marquee"));

    const QVariantMap doc {
        {QStringLiteral("selected_object_ids"), QVariantList{QStringLiteral("script_a"), QStringLiteral("script_b")}},
        {QStringLiteral("layers"), QVariantList{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("layer_1")}, {QStringLiteral("visible"), true}, {QStringLiteral("objects"), QVariantList{
                QVariantMap{{QStringLiteral("id"), QStringLiteral("script_a")}, {QStringLiteral("kind"), QStringLiteral("point")}, {QStringLiteral("x"), 0.1}, {QStringLiteral("y"), 0.2}},
                QVariantMap{{QStringLiteral("id"), QStringLiteral("script_b")}, {QStringLiteral("kind"), QStringLiteral("point")}, {QStringLiteral("x"), 0.8}, {QStringLiteral("y"), 0.7}}
            }}}
        }}
    };
    const CanvasBounds selectedBounds = combinedSelectionBounds(doc);
    ok &= expect(selectedBounds.ok, QStringLiteral("multi-selection bounds should be valid"));
    ok &= expectNear(selectedBounds.maxX, 0.8, QStringLiteral("multi-selection max x"));
    return ok;
}

bool testGestureState() {
    using namespace drawing_canvas;
    bool ok = true;
    QVariantMap state = initialGestureState();
    state = beginObjectDrag(state, QStringLiteral("script_line"), {0.1, 0.1}, QVariantList{QStringLiteral("script_line")}, {});
    state = updateGesture(state, {{QStringLiteral("point"), point(0.2, 0.25)}});
    const QVariantMap finish = finishGesture(state, {{QStringLiteral("point"), point(0.2, 0.25)}});
    const QVariantMap intent = finish.value(QStringLiteral("intent")).toMap();
    ok &= expect(intent.value(QStringLiteral("kind")).toString() == QStringLiteral("move_object"), QStringLiteral("object drag should finish as move intent"));
    ok &= expectNear(intent.value(QStringLiteral("dx")).toDouble(), 0.1, QStringLiteral("object drag dx"));
    ok &= expectNear(intent.value(QStringLiteral("dy")).toDouble(), 0.15, QStringLiteral("object drag dy"));

    QVariantMap marquee = beginMarquee(initialGestureState(), {0.1, 0.1}, {});
    marquee = updateGesture(marquee, {{QStringLiteral("point"), point(0.2, 0.2)}, {QStringLiteral("moveTolerance"), 0.01}});
    const QVariantMap action = finishAction(marquee);
    ok &= expect(action.value(QStringLiteral("shouldSelectMarquee")).toBool(), QStringLiteral("moved marquee should select"));
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= testViewport();
    ok &= testHandles();
    ok &= testHitTest();
    ok &= testSnap();
    ok &= testProjection();
    ok &= testGestureState();
    return ok ? 0 : 1;
}
