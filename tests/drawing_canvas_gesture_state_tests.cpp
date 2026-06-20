#include "widgets/DrawingCanvasGestureState.h"

#include "EdiAssert.h"

using namespace drawing_canvas;

int main()
{
    const DrawingCanvasGestureState initial = initialGestureState();
    EDI_CHECK(initial.mode == DrawingCanvasGestureMode::Idle);
    EDI_CHECK(!initial.started);
    EDI_CHECK(!initial.moved);

    DrawingCanvasGestureState handle = beginHandleDrag(initial, QStringLiteral("object_1"), QStringLiteral("line_start"), {0.1, 0.2});
    EDI_CHECK(handle.mode == DrawingCanvasGestureMode::DraggingHandle);
    EDI_CHECK(handle.started);
    EDI_CHECK(handle.objectId == QStringLiteral("object_1"));
    EDI_CHECK(handle.handleId == QStringLiteral("line_start"));

    const DrawingCanvasGestureState rejected = beginObjectDrag(handle, QStringLiteral("object_2"), {0.3, 0.4}, {QStringLiteral("object_2")});
    EDI_CHECK(rejected.mode == DrawingCanvasGestureMode::DraggingHandle);
    EDI_CHECK(rejected.rejected);

    handle = updateGesture(handle, {0.2, 0.4});
    EDI_CHECK(handle.moved);
    EDI_CHECK(handle.lastPoint.x == 0.2);
    EDI_CHECK(handle.lastPoint.y == 0.4);

    const DrawingCanvasFinishResult handleFinish = finishGesture(handle);
    EDI_CHECK(handleFinish.state.mode == DrawingCanvasGestureMode::Idle);
    EDI_CHECK(handleFinish.intent.kind == DrawingCanvasGestureIntentKind::UpdateHandle);
    EDI_CHECK(handleFinish.intent.objectId == QStringLiteral("object_1"));
    EDI_CHECK(handleFinish.intent.handleId == QStringLiteral("line_start"));
    EDI_CHECK(handleFinish.intent.point.x == 0.2);
    EDI_CHECK(handleFinish.intent.point.y == 0.4);

    const QStringList selection {QStringLiteral("a"), QStringLiteral("b")};
    DrawingCanvasGestureState objectDrag = beginObjectDrag(initial, QStringLiteral("a"), {0.25, 0.25}, selection);
    objectDrag = updateGesture(objectDrag, {0.35, 0.5});
    const DrawingCanvasFinishResult objectFinish = finishGesture(objectDrag);
    EDI_CHECK(objectFinish.intent.kind == DrawingCanvasGestureIntentKind::MoveObjects);
    EDI_CHECK(objectFinish.intent.objectIds == selection);
    EDI_CHECK(objectFinish.intent.dx > 0.099);
    EDI_CHECK(objectFinish.intent.dy > 0.249);

    DrawingCanvasGestureState marquee = beginMarquee(initial, {0.1, 0.1});
    marquee = updateGesture(marquee, {0.7, 0.8});
    EDI_CHECK(finishKind(marquee) == QStringLiteral("marquee_select"));
    const DrawingCanvasFinishResult marqueeFinish = finishGesture(marquee);
    EDI_CHECK(marqueeFinish.intent.kind == DrawingCanvasGestureIntentKind::SelectObjects);
    EDI_CHECK(marqueeFinish.intent.startPoint.x == 0.1);
    EDI_CHECK(marqueeFinish.intent.endPoint.y == 0.8);

    const DrawingCanvasFinishResult cancelled = cancelGesture();
    EDI_CHECK(cancelled.state.mode == DrawingCanvasGestureMode::Idle);
    EDI_CHECK(cancelled.intent.kind == DrawingCanvasGestureIntentKind::None);

    return 0;
}
