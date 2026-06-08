#include "DrawingCanvasRuntimeAdapter.h"

#include "DrawingCanvasGestureState.h"
#include "DrawingCanvasHandles.h"
#include "DrawingCanvasHitTest.h"
#include "DrawingCanvasProjection.h"
#include "DrawingCanvasSnap.h"
#include "DrawingCanvasViewport.h"

using namespace drawing_canvas;

namespace {

CanvasBounds boundsFromVariant(const QVariantMap &bounds) {
    return {
        bounds.value(QStringLiteral("ok")).toBool(),
        finiteNumber(bounds.value(QStringLiteral("minX")), 0.0),
        finiteNumber(bounds.value(QStringLiteral("minY")), 0.0),
        finiteNumber(bounds.value(QStringLiteral("maxX")), 0.0),
        finiteNumber(bounds.value(QStringLiteral("maxY")), 0.0)
    };
}

QVariantMap hitHandleResultToVariant(const HitResult &hit, const QVariantMap &handle) {
    QVariantMap result = hitResultToVariant(hit);
    result.insert(QStringLiteral("id"), hit.objectId);
    result.insert(QStringLiteral("handle"), handle);
    return result;
}

} // namespace

DrawingCanvasRuntimeAdapter::DrawingCanvasRuntimeAdapter(QObject *parent)
    : QObject(parent) {
}

QVariantMap DrawingCanvasRuntimeAdapter::boardBounds(double viewWidth, double viewHeight, double zoom, double panX, double panY) const {
    return boardBoundsToVariant(drawing_canvas::boardBounds(viewWidth, viewHeight, zoom, panX, panY));
}

double DrawingCanvasRuntimeAdapter::canvasToScreenX(const QVariantMap &bounds, double x) const {
    return drawing_canvas::canvasToScreenX(boardBoundsFromVariant(bounds), x);
}

double DrawingCanvasRuntimeAdapter::canvasToScreenY(const QVariantMap &bounds, double y) const {
    return drawing_canvas::canvasToScreenY(boardBoundsFromVariant(bounds), y);
}

QVariantMap DrawingCanvasRuntimeAdapter::canvasToScreen(const QVariantMap &bounds, const QVariantMap &point) const {
    const ScreenPoint screen = drawing_canvas::canvasToScreen(boardBoundsFromVariant(bounds), pointFromVariant(point));
    return {{QStringLiteral("x"), screen.x}, {QStringLiteral("y"), screen.y}};
}

QVariantMap DrawingCanvasRuntimeAdapter::screenToCanvas(const QVariantMap &bounds, double screenX, double screenY) const {
    return pointToVariant(drawing_canvas::screenToCanvas(boardBoundsFromVariant(bounds), {screenX, screenY}));
}

bool DrawingCanvasRuntimeAdapter::isRectangleLike(const QString &kind) const {
    return drawing_canvas::isRectangleLike(kind);
}

QVariantMap DrawingCanvasRuntimeAdapter::rotatedRectCenter(const QVariantMap &object) const {
    return pointToVariant(drawing_canvas::rotatedRectCenter({object}));
}

QVariantList DrawingCanvasRuntimeAdapter::rotatedRectCorners(const QVariantMap &object) const {
    return handlesToVariant(drawing_canvas::rotatedRectCorners({object}));
}

QVariantMap DrawingCanvasRuntimeAdapter::rotatedRectRotationHandle(const QVariantMap &object, const QVariantMap &settings) const {
    return handleToVariant(drawing_canvas::rotatedRectRotationHandle({object}, settings));
}

QVariantList DrawingCanvasRuntimeAdapter::handlesForObject(const QVariantMap &object, const QVariantMap &settings) const {
    return handlesToVariant(drawing_canvas::handlesForObject({object}, settings));
}

QVariantList DrawingCanvasRuntimeAdapter::visibleHandlesForObject(const QVariantMap &object, const QVariantMap &settings) const {
    return handlesToVariant(drawing_canvas::visibleHandlesForObject({object}, settings));
}

QVariantMap DrawingCanvasRuntimeAdapter::handleById(const QVariantMap &object, const QString &handleId, const QVariantMap &settings) const {
    const HandleDescriptor handle = drawing_canvas::handleById({object}, handleId, settings);
    return handle.id.isEmpty() ? QVariantMap() : handleToVariant(handle);
}

QVariantMap DrawingCanvasRuntimeAdapter::hitHandleAt(const QVariantMap &object, double screenX, double screenY, const QVariantMap &viewportBounds, const QVariantMap &settings) const {
    const CanvasObjectView view {object};
    const HitResult hit = drawing_canvas::hitHandleAt(view, screenX, screenY, boardBoundsFromVariant(viewportBounds), settings);
    const QVariantMap handle = hit.ok ? handleToVariant(drawing_canvas::handleById(view, hit.objectId, settings)) : QVariantMap();
    return hitHandleResultToVariant(hit, handle);
}

QVariantMap DrawingCanvasRuntimeAdapter::handleUpdatePlan(const QVariantMap &object, const QString &handleId, const QVariantMap &point, const QVariantMap &settings) const {
    return updatePlanToVariant(drawing_canvas::handleUpdatePlan({object}, handleId, pointFromVariant(point), settings));
}

double DrawingCanvasRuntimeAdapter::objectHitScore(const QVariantMap &object, double x, double y) const {
    return drawing_canvas::objectHitScore({object}, x, y);
}

QVariantMap DrawingCanvasRuntimeAdapter::hitObjectAt(const QVariantList &objects, double x, double y, double tolerance) const {
    return hitResultToVariant(drawing_canvas::hitObjectAt(objectsFromVariantList(objects), x, y, tolerance));
}

double DrawingCanvasRuntimeAdapter::effectiveGridStepPx(const QVariantMap &settings) const {
    return drawing_canvas::effectiveGridStepPx(settings);
}

QVariantMap DrawingCanvasRuntimeAdapter::noneSnap(const QVariantMap &rawPoint, const QVariantMap &settings) const {
    return snapResultToVariant(drawing_canvas::noneSnap(pointFromVariant(rawPoint), settings));
}

QVariantMap DrawingCanvasRuntimeAdapter::gridSnap(const QVariantMap &rawPoint, const QVariantMap &settings) const {
    return snapResultToVariant(drawing_canvas::gridSnap(pointFromVariant(rawPoint), settings));
}

QVariantMap DrawingCanvasRuntimeAdapter::resolveSnap(const QVariantMap &rawPoint, const QVariantList &objects, const QVariantMap &settings) const {
    return snapResultToVariant(drawing_canvas::resolveSnap(pointFromVariant(rawPoint), objectsFromVariantList(objects), settings));
}

QVariantMap DrawingCanvasRuntimeAdapter::emptyBounds() const {
    return boundsToVariant(drawing_canvas::emptyBounds());
}

bool DrawingCanvasRuntimeAdapter::boundsIntersects(const QVariantMap &bounds, double minX, double minY, double maxX, double maxY) const {
    return drawing_canvas::boundsIntersects(boundsFromVariant(bounds), minX, minY, maxX, maxY);
}

QVariantMap DrawingCanvasRuntimeAdapter::normalizedObjectBounds(const QVariantMap &object) const {
    return boundsToVariant(drawing_canvas::normalizedObjectBounds({object}));
}

bool DrawingCanvasRuntimeAdapter::objectIntersectsBounds(const QVariantMap &object, double minX, double minY, double maxX, double maxY) const {
    return drawing_canvas::objectIntersectsBounds({object}, minX, minY, maxX, maxY);
}

QVariantList DrawingCanvasRuntimeAdapter::selectedObjectIds(const QVariantMap &doc) const {
    QVariantList result;
    for (const QString &id : drawing_canvas::selectedObjectIds(doc)) {
        result.push_back(id);
    }
    return result;
}

bool DrawingCanvasRuntimeAdapter::selectedObject(const QVariantMap &doc, const QString &objectId) const {
    return drawing_canvas::selectedObject(doc, objectId);
}

bool DrawingCanvasRuntimeAdapter::selectedLayer(const QVariantMap &doc, const QString &layerId) const {
    return drawing_canvas::selectedLayer(doc, layerId);
}

QVariantMap DrawingCanvasRuntimeAdapter::combinedSelectionBounds(const QVariantMap &doc) const {
    return boundsToVariant(drawing_canvas::combinedSelectionBounds(doc));
}

QVariantMap DrawingCanvasRuntimeAdapter::initialGestureState() const {
    return drawing_canvas::initialGestureState();
}

QVariantMap DrawingCanvasRuntimeAdapter::beginHover(const QVariantMap &state, const QVariantMap &point, const QVariantMap &target) const {
    return drawing_canvas::beginHover(state, pointFromVariant(point), target);
}

QVariantMap DrawingCanvasRuntimeAdapter::beginObjectDrag(const QVariantMap &state, const QString &objectId, const QVariantMap &point, const QVariantList &selectedIds, const QVariantMap &modifiers) const {
    return drawing_canvas::beginObjectDrag(state, objectId, pointFromVariant(point), selectedIds, modifiers);
}

QVariantMap DrawingCanvasRuntimeAdapter::beginHandleDrag(const QVariantMap &state, const QString &objectId, const QString &handleId, const QVariantMap &point, const QVariantMap &modifiers) const {
    return drawing_canvas::beginHandleDrag(state, objectId, handleId, pointFromVariant(point), modifiers);
}

QVariantMap DrawingCanvasRuntimeAdapter::beginMarquee(const QVariantMap &state, const QVariantMap &point, const QVariantMap &modifiers) const {
    return drawing_canvas::beginMarquee(state, pointFromVariant(point), modifiers);
}

QVariantMap DrawingCanvasRuntimeAdapter::beginPan(const QVariantMap &state, const QVariantMap &screenPoint, const QVariantMap &modifiers) const {
    return drawing_canvas::beginPan(state, {finiteNumber(screenPoint.value(QStringLiteral("x")), 0.0), finiteNumber(screenPoint.value(QStringLiteral("y")), 0.0)}, modifiers);
}

QVariantMap DrawingCanvasRuntimeAdapter::beginDrawingPendingShape(const QVariantMap &state, const QVariantMap &point, const QVariantMap &modifiers) const {
    return drawing_canvas::beginDrawingPendingShape(state, pointFromVariant(point), modifiers);
}

QVariantMap DrawingCanvasRuntimeAdapter::updateGesture(const QVariantMap &state, const QVariantMap &payload) const {
    return drawing_canvas::updateGesture(state, payload);
}

QVariantMap DrawingCanvasRuntimeAdapter::finishGesture(const QVariantMap &state, const QVariantMap &payload) const {
    return drawing_canvas::finishGesture(state, payload);
}

QVariantMap DrawingCanvasRuntimeAdapter::cancelGesture(const QVariantMap &state) const {
    return drawing_canvas::cancelGesture(state);
}

QVariantMap DrawingCanvasRuntimeAdapter::finishAction(const QVariantMap &state) const {
    return drawing_canvas::finishAction(state);
}

bool DrawingCanvasRuntimeAdapter::isDragging(const QVariantMap &state) const {
    return drawing_canvas::isDragging(state);
}

bool DrawingCanvasRuntimeAdapter::isHandleDrag(const QVariantMap &state) const {
    return drawing_canvas::isHandleDrag(state);
}

bool DrawingCanvasRuntimeAdapter::isObjectDrag(const QVariantMap &state) const {
    return drawing_canvas::isObjectDrag(state);
}

bool DrawingCanvasRuntimeAdapter::isMarquee(const QVariantMap &state) const {
    return drawing_canvas::isMarquee(state);
}

QString DrawingCanvasRuntimeAdapter::gestureLabel(const QVariantMap &state) const {
    return drawing_canvas::gestureLabel(state);
}
