#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class DrawingCanvasRuntimeAdapter : public QObject {
    Q_OBJECT

public:
    explicit DrawingCanvasRuntimeAdapter(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap boardBounds(double viewWidth, double viewHeight, double zoom, double panX, double panY) const;
    Q_INVOKABLE double canvasToScreenX(const QVariantMap &bounds, double x) const;
    Q_INVOKABLE double canvasToScreenY(const QVariantMap &bounds, double y) const;
    Q_INVOKABLE QVariantMap canvasToScreen(const QVariantMap &bounds, const QVariantMap &point) const;
    Q_INVOKABLE QVariantMap screenToCanvas(const QVariantMap &bounds, double screenX, double screenY) const;

    Q_INVOKABLE bool isRectangleLike(const QString &kind) const;
    Q_INVOKABLE QVariantMap rotatedRectCenter(const QVariantMap &object) const;
    Q_INVOKABLE QVariantList rotatedRectCorners(const QVariantMap &object) const;

    Q_INVOKABLE double objectHitScore(const QVariantMap &object, double x, double y) const;
    Q_INVOKABLE QVariantMap hitObjectAt(const QVariantList &objects, double x, double y, double tolerance) const;

    Q_INVOKABLE double effectiveGridStepPx(const QVariantMap &settings) const;
    Q_INVOKABLE QVariantMap noneSnap(const QVariantMap &rawPoint, const QVariantMap &settings) const;
    Q_INVOKABLE QVariantMap gridSnap(const QVariantMap &rawPoint, const QVariantMap &settings) const;
    Q_INVOKABLE QVariantMap resolveSnap(const QVariantMap &rawPoint, const QVariantList &objects, const QVariantMap &settings) const;

    Q_INVOKABLE QVariantMap emptyBounds() const;
    Q_INVOKABLE bool boundsIntersects(const QVariantMap &bounds, double minX, double minY, double maxX, double maxY) const;
    Q_INVOKABLE QVariantMap normalizedObjectBounds(const QVariantMap &object) const;
    Q_INVOKABLE bool objectIntersectsBounds(const QVariantMap &object, double minX, double minY, double maxX, double maxY) const;
    Q_INVOKABLE QVariantList selectedObjectIds(const QVariantMap &doc) const;
    Q_INVOKABLE bool selectedObject(const QVariantMap &doc, const QString &objectId) const;
    Q_INVOKABLE bool selectedLayer(const QVariantMap &doc, const QString &layerId) const;
    Q_INVOKABLE QVariantMap combinedSelectionBounds(const QVariantMap &doc) const;

    Q_INVOKABLE QVariantMap initialGestureState() const;
    Q_INVOKABLE QVariantMap beginHover(const QVariantMap &state, const QVariantMap &point, const QVariantMap &target) const;
    Q_INVOKABLE QVariantMap beginObjectDrag(const QVariantMap &state, const QString &objectId, const QVariantMap &point, const QVariantList &selectedIds, const QVariantMap &modifiers) const;
    Q_INVOKABLE QVariantMap beginHandleDrag(const QVariantMap &state, const QString &objectId, const QString &handleId, const QVariantMap &point, const QVariantMap &modifiers) const;
    Q_INVOKABLE QVariantMap beginMarquee(const QVariantMap &state, const QVariantMap &point, const QVariantMap &modifiers) const;
    Q_INVOKABLE QVariantMap beginPan(const QVariantMap &state, const QVariantMap &screenPoint, const QVariantMap &modifiers) const;
    Q_INVOKABLE QVariantMap beginDrawingPendingShape(const QVariantMap &state, const QVariantMap &point, const QVariantMap &modifiers) const;
    Q_INVOKABLE QVariantMap updateGesture(const QVariantMap &state, const QVariantMap &payload) const;
    Q_INVOKABLE QVariantMap finishGesture(const QVariantMap &state, const QVariantMap &payload) const;
    Q_INVOKABLE QVariantMap cancelGesture(const QVariantMap &state) const;
    Q_INVOKABLE bool transitionAllowed(const QString &fromMode, const QString &toMode) const;
    Q_INVOKABLE QString finishKind(const QVariantMap &state) const;
    Q_INVOKABLE QVariantMap finishAction(const QVariantMap &state) const;
    Q_INVOKABLE bool isDragging(const QVariantMap &state) const;
    Q_INVOKABLE bool isHandleDrag(const QVariantMap &state) const;
    Q_INVOKABLE bool isObjectDrag(const QVariantMap &state) const;
    Q_INVOKABLE bool isMarquee(const QVariantMap &state) const;
    Q_INVOKABLE QString gestureLabel(const QVariantMap &state) const;
};
