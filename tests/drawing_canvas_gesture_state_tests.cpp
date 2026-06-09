#include "widgets/DrawingCanvasGestureState.h"

#include <cassert>

using namespace drawing_canvas;

int main()
{
    const DrawingCanvasGestureState initial = initialGestureState();
    assert(initial.mode == DrawingCanvasGestureMode::Idle);
    assert(!initial.started);
    assert(!initial.moved);

    DrawingCanvasGestureState handle = beginHandleDrag(initial, QStringLiteral("object_1"), QStringLiteral("line_start"), {0.1, 0.2});
    assert(handle.mode == DrawingCanvasGestureMode::DraggingHandle);
    assert(handle.started);
    assert(handle.objectId == QStringLiteral("object_1"));
    assert(handle.handleId == QStringLiteral("line_start"));

    const DrawingCanvasGestureState rejected = beginObjectDrag(handle, QStringLiteral("object_2"), {0.3, 0.4}, {QStringLiteral("object_2")});
    assert(rejected.mode == DrawingCanvasGestureMode::DraggingHandle);
    assert(rejected.rejected);

    handle = updateGesture(handle, {0.2, 0.4});
    assert(handle.moved);
    assert(handle.lastPoint.x == 0.2);
    assert(handle.lastPoint.y == 0.4);

    const DrawingCanvasFinishResult handleFinish = finishGesture(handle);
    assert(handleFinish.state.mode == DrawingCanvasGestureMode::Idle);
    assert(handleFinish.intent.kind == DrawingCanvasGestureIntentKind::UpdateHandle);
    assert(handleFinish.intent.objectId == QStringLiteral("object_1"));
    assert(handleFinish.intent.handleId == QStringLiteral("line_start"));
    assert(handleFinish.intent.point.x == 0.2);
    assert(handleFinish.intent.point.y == 0.4);

    const QStringList selection {QStringLiteral("a"), QStringLiteral("b")};
    DrawingCanvasGestureState objectDrag = beginObjectDrag(initial, QStringLiteral("a"), {0.25, 0.25}, selection);
    objectDrag = updateGesture(objectDrag, {0.35, 0.5});
    const DrawingCanvasFinishResult objectFinish = finishGesture(objectDrag);
    assert(objectFinish.intent.kind == DrawingCanvasGestureIntentKind::MoveObjects);
    assert(objectFinish.intent.objectIds == selection);
    assert(objectFinish.intent.dx > 0.099);
    assert(objectFinish.intent.dy > 0.249);

    DrawingCanvasGestureState marquee = beginMarquee(initial, {0.1, 0.1});
    marquee = updateGesture(marquee, {0.7, 0.8});
    assert(finishKind(marquee) == QStringLiteral("marquee_select"));
    const DrawingCanvasFinishResult marqueeFinish = finishGesture(marquee);
    assert(marqueeFinish.intent.kind == DrawingCanvasGestureIntentKind::SelectObjects);
    assert(marqueeFinish.intent.startPoint.x == 0.1);
    assert(marqueeFinish.intent.endPoint.y == 0.8);

    const DrawingCanvasFinishResult cancelled = cancelGesture();
    assert(cancelled.state.mode == DrawingCanvasGestureMode::Idle);
    assert(cancelled.intent.kind == DrawingCanvasGestureIntentKind::None);

    return 0;
}
