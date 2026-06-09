#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>

namespace drawing_canvas {

struct CanvasPoint {
    double x = 0.0;
    double y = 0.0;
};

struct ScreenPoint {
    double x = 0.0;
    double y = 0.0;
};

struct DrawingCanvasModifiers {
    bool shift = false;
    bool alt = false;
    bool control = false;
    bool meta = false;
};

enum class DrawingCanvasGestureMode {
    Idle,
    Hovering,
    DrawingPendingShape,
    DraggingObject,
    DraggingHandle,
    MarqueeSelect,
    Panning
};

enum class DrawingCanvasGestureIntentKind {
    None,
    SelectObjects,
    MoveObject,
    MoveObjects,
    UpdateHandle,
    DrawClick,
    Pan
};

struct DrawingCanvasGestureState {
    DrawingCanvasGestureMode mode = DrawingCanvasGestureMode::Idle;
    bool started = false;
    QString objectId;
    QString handleId;
    QStringList selectedIds;
    CanvasPoint startPoint;
    CanvasPoint lastPoint;
    ScreenPoint startScreenPoint;
    ScreenPoint lastScreenPoint;
    bool moved = false;
    DrawingCanvasModifiers modifiers;
    QString targetKind = QStringLiteral("none");
    QString targetObjectId;
    QString targetHandleId;
    bool rejected = false;
};

struct DrawingCanvasGestureIntent {
    DrawingCanvasGestureIntentKind kind = DrawingCanvasGestureIntentKind::None;
    QString objectId;
    QString handleId;
    QStringList objectIds;
    CanvasPoint point;
    CanvasPoint startPoint;
    CanvasPoint endPoint;
    double dx = 0.0;
    double dy = 0.0;
    double dxPx = 0.0;
    double dyPx = 0.0;
};

struct DrawingCanvasFinishOptions {
    bool incremental = false;
    QStringList objectIds;
    CanvasPoint point;
    bool hasPoint = false;
    ScreenPoint screenPoint;
    bool hasScreenPoint = false;
};

struct DrawingCanvasFinishResult {
    DrawingCanvasGestureState state;
    DrawingCanvasGestureIntent intent;
};

double finiteNumber(double value, double fallback);
double finiteNumber(const QVariant &value, double fallback);

DrawingCanvasGestureState initialGestureState();
DrawingCanvasGestureState beginHover(const DrawingCanvasGestureState &state, CanvasPoint point, const QString &targetKind = QStringLiteral("none"), const QString &targetObjectId = {}, const QString &targetHandleId = {}, DrawingCanvasModifiers modifiers = {});
DrawingCanvasGestureState beginObjectDrag(const DrawingCanvasGestureState &state, const QString &objectId, CanvasPoint point, const QStringList &selectedIds, DrawingCanvasModifiers modifiers = {});
DrawingCanvasGestureState beginHandleDrag(const DrawingCanvasGestureState &state, const QString &objectId, const QString &handleId, CanvasPoint point, DrawingCanvasModifiers modifiers = {});
DrawingCanvasGestureState beginMarquee(const DrawingCanvasGestureState &state, CanvasPoint point, DrawingCanvasModifiers modifiers = {});
DrawingCanvasGestureState beginPan(const DrawingCanvasGestureState &state, ScreenPoint screenPoint, DrawingCanvasModifiers modifiers = {});
DrawingCanvasGestureState beginDrawingPendingShape(const DrawingCanvasGestureState &state, CanvasPoint point, DrawingCanvasModifiers modifiers = {});
DrawingCanvasGestureState updateGesture(const DrawingCanvasGestureState &state, CanvasPoint point, DrawingCanvasModifiers modifiers = {}, double moveTolerance = 0.000001);
DrawingCanvasGestureState updateGestureScreenPoint(const DrawingCanvasGestureState &state, ScreenPoint screenPoint, DrawingCanvasModifiers modifiers = {}, double screenMoveTolerancePx = 0.0);
DrawingCanvasFinishResult finishGesture(const DrawingCanvasGestureState &state, DrawingCanvasFinishOptions options = {});
DrawingCanvasFinishResult cancelGesture();
bool transitionAllowed(DrawingCanvasGestureMode fromMode, DrawingCanvasGestureMode toMode);
QString finishKind(const DrawingCanvasGestureState &state);
bool isDragging(const DrawingCanvasGestureState &state);
bool isHandleDrag(const DrawingCanvasGestureState &state);
bool isObjectDrag(const DrawingCanvasGestureState &state);
bool isMarquee(const DrawingCanvasGestureState &state);
QString gestureLabel(const DrawingCanvasGestureState &state);

} // namespace drawing_canvas
