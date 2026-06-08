#include "canvas/DrawingCanvasGestureState.h"
#include "canvas/DrawingCanvasHandles.h"
#include "canvas/DrawingCanvasHitTest.h"
#include "canvas/DrawingCanvasProjection.h"
#include "canvas/DrawingCanvasSnap.h"
#include "canvas/DrawingCanvasViewport.h"

#include <QTextStream>

#include <cmath>
#include <limits>

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

double updateValue(const drawing_canvas::HandleUpdatePlan &plan, const QString &field) {
    for (const drawing_canvas::FieldUpdate &update : plan.updates) {
        if (update.field == field) {
            return update.value;
        }
    }
    return std::numeric_limits<double>::quiet_NaN();
}

bool hasCandidate(const std::vector<drawing_canvas::SnapCandidate> &candidates, const QString &sourceKind) {
    for (const drawing_canvas::SnapCandidate &candidate : candidates) {
        if (candidate.sourceKind == sourceKind) {
            return true;
        }
    }
    return false;
}

int candidateCount(const std::vector<drawing_canvas::SnapCandidate> &candidates, const QString &sourceKind) {
    int count = 0;
    for (const drawing_canvas::SnapCandidate &candidate : candidates) {
        if (candidate.sourceKind == sourceKind) {
            ++count;
        }
    }
    return count;
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

    const BoardBounds zoomed = boardBounds(800, 600, 1.5, 12.0, -8.0);
    ok &= expectNear(zoomed.size, 876.0, QStringLiteral("viewport should scale min dimension by zoom"));
    ok &= expectNear(zoomed.x, -26.0, QStringLiteral("viewport should center and pan x deterministically"));
    ok &= expectNear(zoomed.y, -146.0, QStringLiteral("viewport should center and pan y deterministically"));

    const BoardBounds tiny = boardBounds(4.0, 3.0, 0.0, 0.0, 0.0);
    ok &= expectNear(tiny.size, 0.0032, QStringLiteral("viewport should clamp invalid zoom while preserving minimum board rule"));
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

    const QVariantMap pointObject {
        {QStringLiteral("id"), QStringLiteral("script_point")},
        {QStringLiteral("kind"), QStringLiteral("point")},
        {QStringLiteral("x"), 0.25},
        {QStringLiteral("y"), 0.5}
    };
    const std::vector<HandleDescriptor> pointHandles = handlesForObject({pointObject}, settings());
    ok &= expect(pointHandles.size() == 1, QStringLiteral("point should expose one handle"));
    ok &= expect(pointHandles[0].updateFields.join(QStringLiteral(",")) == QStringLiteral("x_px,y_px"), QStringLiteral("point handle should target x/y pixel fields"));
    const HandleUpdatePlan pointPlan = handleUpdatePlan({pointObject}, QStringLiteral("point_position"), {0.75, 0.25}, settings());
    ok &= expectNear(updateValue(pointPlan, QStringLiteral("x_px")), 384.0, QStringLiteral("point x update should use canvas pixels"));
    ok &= expectNear(updateValue(pointPlan, QStringLiteral("y_px")), 128.0, QStringLiteral("point y update should use canvas pixels"));

    const QVariantMap circle {
        {QStringLiteral("id"), QStringLiteral("script_circle")},
        {QStringLiteral("kind"), QStringLiteral("circle")},
        {QStringLiteral("cx"), 0.4},
        {QStringLiteral("cy"), 0.5},
        {QStringLiteral("radius"), 0.2}
    };
    const std::vector<HandleDescriptor> circleHandles = handlesForObject({circle}, settings());
    ok &= expect(circleHandles.size() == 2, QStringLiteral("circle should expose center and radius handles"));
    ok &= expectNear(circleHandles[1].x, 0.6, QStringLiteral("circle radius handle should sit at cx + radius"));
    const HandleUpdatePlan radiusPlan = handleUpdatePlan({circle}, QStringLiteral("circle_radius"), {0.4, 0.7}, settings());
    ok &= expectNear(updateValue(radiusPlan, QStringLiteral("radius_px")), 102.4, QStringLiteral("circle radius should derive pixel distance from center"));

    const QVariantMap rectangle {
        {QStringLiteral("id"), QStringLiteral("script_rect")},
        {QStringLiteral("kind"), QStringLiteral("rectangle")},
        {QStringLiteral("x"), 0.25},
        {QStringLiteral("y"), 0.25},
        {QStringLiteral("width"), 0.25},
        {QStringLiteral("height"), 0.125},
        {QStringLiteral("rotation_deg"), 90.0}
    };
    const std::vector<HandleDescriptor> corners = rotatedRectCorners({rectangle});
    ok &= expect(corners.size() == 4, QStringLiteral("rectangle should return four rotated corners"));
    ok &= expectNear(corners[0].x, 0.4375, QStringLiteral("rotated rectangle nw x should be deterministic"));
    ok &= expectNear(corners[0].y, 0.1875, QStringLiteral("rotated rectangle nw y should be deterministic"));

    QVariantMap constrained = settings();
    constrained.insert(QStringLiteral("shiftConstrain"), true);
    constrained.insert(QStringLiteral("angleSnapDeg"), 15.0);
    const HandleUpdatePlan rotatePlan = handleUpdatePlan({rectangle}, QStringLiteral("rect_rotate"), {0.6, 0.3125}, constrained);
    ok &= expectNear(updateValue(rotatePlan, QStringLiteral("rotation_deg")), 90.0, QStringLiteral("rectangle rotate plan should snap degrees when constrained"));

    const HitResult handleHit = hitHandleAt({line}, 52.0, 103.0, {0.0, 0.0, 512.0}, settings());
    ok &= expect(handleHit.ok && handleHit.objectId == QStringLiteral("line_start"), QStringLiteral("hitHandleAt should hit nearest handle within tolerance"));
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

    const QVariantMap arc {
        {QStringLiteral("id"), QStringLiteral("script_arc")},
        {QStringLiteral("kind"), QStringLiteral("arc")},
        {QStringLiteral("cx"), 0.5},
        {QStringLiteral("cy"), 0.5},
        {QStringLiteral("radius"), 0.25},
        {QStringLiteral("start_angle_deg"), 0.0},
        {QStringLiteral("end_angle_deg"), 90.0}
    };
    ok &= expect(objectHitScore({arc}, 0.5, 0.25) > 0.9, QStringLiteral("arc hit score should reject points outside the angle span"));

    const QVariantMap rectangle {
        {QStringLiteral("id"), QStringLiteral("script_rect")},
        {QStringLiteral("kind"), QStringLiteral("rectangle")},
        {QStringLiteral("x"), 0.2},
        {QStringLiteral("y"), 0.2},
        {QStringLiteral("width"), 0.2},
        {QStringLiteral("height"), 0.2}
    };
    ok &= expectNear(objectHitScore({rectangle}, 0.3, 0.3), 0.0, QStringLiteral("rectangle hit score should be zero inside the rectangle"));

    const QVariantMap polygon {
        {QStringLiteral("id"), QStringLiteral("script_polygon")},
        {QStringLiteral("kind"), QStringLiteral("polygon")},
        {QStringLiteral("points"), pointList({QVariantList{0.2, 0.2}, QVariantList{0.4, 0.2}, QVariantList{0.3, 0.4}})}
    };
    ok &= expectNear(objectHitScore({polygon}, 0.3, 0.25), 0.0, QStringLiteral("polygon hit score should be zero inside polygon"));

    const HitResult miss = hitObjectAt(objectsFromVariantList({line}), 0.5, 0.5, 0.025);
    ok &= expect(!miss.ok && miss.objectId.isEmpty() && miss.kind == QStringLiteral("none"), QStringLiteral("hitObjectAt miss should report none"));
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

    QVariantMap disabledSettings = settings();
    disabledSettings.insert(QStringLiteral("gridEnabled"), false);
    disabledSettings.insert(QStringLiteral("objectSnapEnabled"), false);
    const SnapResult invalid = resolveSnap({std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}, {}, disabledSettings);
    ok &= expect(invalid.kind == QStringLiteral("none"), QStringLiteral("disabled snap should report none"));
    ok &= expect(std::isfinite(invalid.x) && std::isfinite(invalid.y), QStringLiteral("disabled snap should return finite coordinates"));
    ok &= expectNear(invalid.x, 0.0, QStringLiteral("invalid snap x should clamp through finite fallback"));
    ok &= expectNear(invalid.y, 0.0, QStringLiteral("invalid snap y should clamp through finite fallback"));

    QVariantMap afterGrid = settings();
    afterGrid.insert(QStringLiteral("objectPriority"), QStringLiteral("after_grid"));
    const SnapResult gridFirst = resolveSnap({0.252, 0.252}, objectsFromVariantList({line}), afterGrid);
    ok &= expect(gridFirst.kind == QStringLiteral("grid"), QStringLiteral("grid should win when object priority is after_grid"));

    QVariantMap outside = settings();
    outside.insert(QStringLiteral("gridEnabled"), false);
    outside.insert(QStringLiteral("objectSnapTolerancePx"), 4.0);
    const SnapResult outsideTolerance = resolveSnap({0.1, 0.9}, objectsFromVariantList({line}), outside);
    ok &= expect(outsideTolerance.kind == QStringLiteral("none"), QStringLiteral("object candidate outside tolerance should not snap"));

    const QVariantMap snapRectangle {
        {QStringLiteral("id"), QStringLiteral("rect_a")},
        {QStringLiteral("kind"), QStringLiteral("rectangle")},
        {QStringLiteral("x"), 0.2},
        {QStringLiteral("y"), 0.3},
        {QStringLiteral("width"), 0.2},
        {QStringLiteral("height"), 0.4}
    };
    const std::vector<SnapCandidate> rectangleCandidates = snapCandidatesForObject({snapRectangle}, settings());
    ok &= expect(hasCandidate(rectangleCandidates, QStringLiteral("vertex")), QStringLiteral("rectangle should expose vertex snap candidates"));
    ok &= expect(hasCandidate(rectangleCandidates, QStringLiteral("midpoint")), QStringLiteral("rectangle should expose midpoint snap candidates"));
    ok &= expect(hasCandidate(rectangleCandidates, QStringLiteral("center")), QStringLiteral("rectangle should expose center snap candidate"));

    const QVariantMap polyline {
        {QStringLiteral("id"), QStringLiteral("polyline_a")},
        {QStringLiteral("kind"), QStringLiteral("polyline")},
        {QStringLiteral("points"), pointList({QVariantList{0.1, 0.1}, QVariantList{0.3, 0.1}, QVariantList{0.2, 0.3}})}
    };
    const std::vector<SnapCandidate> polylineCandidates = snapCandidatesForObject({polyline}, settings());
    ok &= expect(candidateCount(polylineCandidates, QStringLiteral("endpoint")) == 2, QStringLiteral("polyline should expose endpoint snap candidates"));
    ok &= expect(hasCandidate(polylineCandidates, QStringLiteral("vertex")), QStringLiteral("polyline should expose interior vertex snap candidates"));

    const QVariantMap snapPolygon {
        {QStringLiteral("id"), QStringLiteral("polygon_a")},
        {QStringLiteral("kind"), QStringLiteral("polygon")},
        {QStringLiteral("points"), pointList({QVariantList{0.1, 0.1}, QVariantList{0.3, 0.1}, QVariantList{0.2, 0.3}})}
    };
    const std::vector<SnapCandidate> polygonCandidates = snapCandidatesForObject({snapPolygon}, settings());
    ok &= expect(hasCandidate(polygonCandidates, QStringLiteral("vertex")), QStringLiteral("polygon should expose vertex snap candidates"));
    ok &= expect(hasCandidate(polygonCandidates, QStringLiteral("midpoint")), QStringLiteral("polygon should expose midpoint snap candidates"));
    ok &= expect(hasCandidate(polygonCandidates, QStringLiteral("center")), QStringLiteral("polygon should expose center snap candidate"));
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

    const QVariantMap rotated {
        {QStringLiteral("id"), QStringLiteral("script_rotated")},
        {QStringLiteral("kind"), QStringLiteral("rectangle")},
        {QStringLiteral("x"), 0.25},
        {QStringLiteral("y"), 0.25},
        {QStringLiteral("width"), 0.25},
        {QStringLiteral("height"), 0.125},
        {QStringLiteral("rotation_deg"), 90.0}
    };
    const CanvasBounds rotatedBounds = normalizedObjectBounds({rotated});
    ok &= expectNear(rotatedBounds.minX, 0.3125, QStringLiteral("rotated rectangle bounds min x"));
    ok &= expectNear(rotatedBounds.minY, 0.1875, QStringLiteral("rotated rectangle bounds min y"));
    ok &= expectNear(rotatedBounds.maxX, 0.4375, QStringLiteral("rotated rectangle bounds max x"));
    ok &= expectNear(rotatedBounds.maxY, 0.4375, QStringLiteral("rotated rectangle bounds max y"));

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

    const QVariantMap selectionDoc {
        {QStringLiteral("selected_object_id"), QStringLiteral("script_line_01")},
        {QStringLiteral("selected_object_ids"), QVariantList{}},
        {QStringLiteral("selected_layer_id"), QStringLiteral("layer_02")}
    };
    ok &= expect(selectedObject(selectionDoc, QStringLiteral("script_line_01")), QStringLiteral("selectedObject should use single selection fallback"));
    ok &= expect(!selectedObject(selectionDoc, QStringLiteral("script_line_02")), QStringLiteral("selectedObject should reject unselected object"));
    ok &= expect(selectedLayer(selectionDoc, QStringLiteral("layer_02")), QStringLiteral("selectedLayer should identify selected layer"));

    const QVariantMap multiSelectionDoc {
        {QStringLiteral("selected_object_id"), QStringLiteral("script_line_01")},
        {QStringLiteral("selected_object_ids"), QVariantList{QStringLiteral("script_line_02"), QStringLiteral("script_rect_01")}}
    };
    ok &= expect(!selectedObject(multiSelectionDoc, QStringLiteral("script_line_01")), QStringLiteral("selected ids should override single selected id"));
    ok &= expect(selectedObject(multiSelectionDoc, QStringLiteral("script_line_02")), QStringLiteral("selectedObject should use selected id set"));
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

    QVariantMap handle = beginHandleDrag(initialGestureState(), QStringLiteral("script_rect"), QStringLiteral("rect_se"), {0.25, 0.25}, {});
    ok &= expect(handle.value(QStringLiteral("mode")).toString() == QStringLiteral("dragging_handle"), QStringLiteral("beginHandleDrag from idle should enter dragging_handle"));
    const QVariantMap rejected = beginObjectDrag(handle, QStringLiteral("script_rect"), {0.5, 0.5}, QVariantList{QStringLiteral("script_rect")}, {});
    ok &= expect(rejected.value(QStringLiteral("mode")).toString() == QStringLiteral("dragging_handle"), QStringLiteral("new active gesture should not replace current gesture implicitly"));
    ok &= expect(rejected.value(QStringLiteral("rejected")).toBool(), QStringLiteral("rejected transition should be explicit"));
    ok &= expect(!transitionAllowed(QStringLiteral("dragging_handle"), QStringLiteral("dragging_object")), QStringLiteral("active drag to active drag should be rejected"));
    ok &= expect(transitionAllowed(QStringLiteral("dragging_handle"), QStringLiteral("idle")), QStringLiteral("active drag can transition to idle"));

    const QVariantMap hover = beginHover(initialGestureState(), {0.3, 0.4}, {{QStringLiteral("kind"), QStringLiteral("object")}, {QStringLiteral("objectId"), QStringLiteral("script_line")}});
    ok &= expect(hover.value(QStringLiteral("mode")).toString() == QStringLiteral("hovering"), QStringLiteral("hover should enter hovering mode"));
    ok &= expect(finishGesture(hover, {}).value(QStringLiteral("intent")).toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("none"), QStringLiteral("hover finish should emit no mutation intent"));

    QVariantMap pan = beginPan(initialGestureState(), {100.0, 200.0}, {{QStringLiteral("meta"), true}});
    pan = updateGesture(pan, {{QStringLiteral("screenPoint"), point(124.0, 176.0)}});
    const QVariantMap panIntent = finishGesture(pan, {{QStringLiteral("screenPoint"), point(124.0, 176.0)}}).value(QStringLiteral("intent")).toMap();
    ok &= expect(panIntent.value(QStringLiteral("kind")).toString() == QStringLiteral("pan"), QStringLiteral("pan finish should emit viewport intent"));
    ok &= expectNear(panIntent.value(QStringLiteral("dxPx")).toDouble(), 24.0, QStringLiteral("pan should compute dx pixels"));
    ok &= expectNear(panIntent.value(QStringLiteral("dyPx")).toDouble(), -24.0, QStringLiteral("pan should compute dy pixels"));

    const QVariantMap marqueeClick = beginMarquee(initialGestureState(), {0.2, 0.2}, {});
    ok &= expect(finishKind(marqueeClick) == QStringLiteral("marquee_click"), QStringLiteral("unmoved marquee finish kind should be marquee click"));
    ok &= expect(finishAction(marqueeClick).value(QStringLiteral("shouldSuppressClick")).toBool() == false, QStringLiteral("unmoved marquee should not suppress click"));

    const QVariantMap draw = beginDrawingPendingShape(initialGestureState(), {0.3, 0.4}, {});
    ok &= expect(finishKind(draw) == QStringLiteral("draw_click"), QStringLiteral("pending draw finish kind should be draw click"));
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
