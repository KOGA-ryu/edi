#include "widgets/DrawingCanvasWidget.h"

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasViewport.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointF>
#include <QVariantList>
#include <QVariantMap>
#include <QWheelEvent>

#include <cassert>
#include <cmath>

namespace {

QPointF screenPointFor(const DrawingDocumentController &controller, const QWidget &widget, double x, double y)
{
    const drawing_canvas::DrawingCanvasViewportInput input =
        drawing_canvas::viewportInputFromModel(controller.modelDocument(), widget.width(), widget.height());
    return drawing_canvas::canvasToScreen(drawing_canvas::viewportBoardRect(input), x, y);
}

void sendMouse(QWidget &widget, QEvent::Type type, const QPointF &pos, Qt::MouseButton button, Qt::MouseButtons buttons,
               Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QMouseEvent event(type, pos, widget.mapToGlobal(pos), button, buttons, modifiers);
    QApplication::sendEvent(&widget, &event);
}

void sendWheel(QWidget &widget, const QPointF &pos, int angleDeltaY, Qt::KeyboardModifiers modifiers)
{
    QWheelEvent event(pos, widget.mapToGlobal(pos), QPoint(0, 0), QPoint(0, angleDeltaY),
                      Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&widget, &event);
}

void sendKey(QWidget &widget, int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    QApplication::sendEvent(&widget, &event);
}

void clickCanvas(DrawingDocumentController &controller, QWidget &widget, double x, double y)
{
    const QPointF pos = screenPointFor(controller, widget, x, y);
    sendMouse(widget, QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton);
    sendMouse(widget, QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::NoButton);
}

QVariantMap activeObject(const DrawingDocumentController &controller)
{
    const QVariantMap model = controller.modelDocument();
    const QString activeId = model.value(QStringLiteral("active_object_id")).toString();
    for (const QVariant &value : model.value(QStringLiteral("drawing_objects")).toList()) {
        const QVariantMap object = value.toMap();
        if (object.value(QStringLiteral("id")).toString() == activeId) {
            return object;
        }
    }
    return {};
}

bool near(double a, double b, double tolerance = 0.01)
{
    return std::abs(a - b) <= tolerance;
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    DrawingDocumentController controller;
    DrawingCanvasWidget canvas(&controller);
    canvas.resize(600, 450);

    // Click-create: a point tool click on the canvas creates and selects a point.
    controller.setSelectedToolId(QStringLiteral("point_tool"));
    clickCanvas(controller, canvas, 0.5, 0.5);
    QVariantMap point = activeObject(controller);
    assert(!point.isEmpty());
    assert(point.value(QStringLiteral("kind")).toString() == QStringLiteral("point"));
    assert(near(point.value(QStringLiteral("x")).toDouble(), 0.5));
    assert(near(point.value(QStringLiteral("y")).toDouble(), 0.5));

    // Drag-move: clear the selection first so the press takes the object-drag
    // path (a selected point's edit handle sits exactly on it and would route
    // the press to handle-drag instead).
    controller.setSelectedToolId(QStringLiteral("select_move"));
    clickCanvas(controller, canvas, 0.9, 0.9);
    assert(controller.selectedObjectId().isEmpty());
    {
        const QPointF start = screenPointFor(controller, canvas, 0.5, 0.5);
        sendMouse(canvas, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        assert(!controller.selectedObjectId().isEmpty());
        const QPointF mid = screenPointFor(controller, canvas, 0.6, 0.55);
        sendMouse(canvas, QEvent::MouseMove, mid, Qt::NoButton, Qt::LeftButton);
        sendMouse(canvas, QEvent::MouseButtonRelease, mid, Qt::LeftButton, Qt::NoButton);
    }
    point = activeObject(controller);
    assert(near(point.value(QStringLiteral("x")).toDouble(), 0.6));
    assert(near(point.value(QStringLiteral("y")).toDouble(), 0.55));

    // Marquee select: drag on empty canvas around the point; it gets selected.
    {
        const QPointF start = screenPointFor(controller, canvas, 0.2, 0.2);
        sendMouse(canvas, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        // Press on empty space clears the selection before the marquee begins.
        assert(controller.selectedObjectId().isEmpty());
        const QPointF end = screenPointFor(controller, canvas, 0.8, 0.8);
        sendMouse(canvas, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        sendMouse(canvas, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    }
    {
        const QVariantList selected = controller.modelDocument().value(QStringLiteral("selected_object_ids")).toList();
        assert(selected.size() == 1);
        assert(selected.first().toString() == point.value(QStringLiteral("id")).toString());
    }

    // Two-click creation: a line tool needs press at both endpoints.
    controller.setSelectedToolId(QStringLiteral("line_tool"));
    clickCanvas(controller, canvas, 0.2, 0.3);
    clickCanvas(controller, canvas, 0.4, 0.3);
    QVariantMap line = activeObject(controller);
    assert(line.value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
    assert(near(line.value(QStringLiteral("x1")).toDouble(), 0.2));
    assert(near(line.value(QStringLiteral("x2")).toDouble(), 0.4));

    // Handle drag: press on the line's endpoint handle and drag it.
    controller.setSelectedToolId(QStringLiteral("select_move"));
    {
        const QPointF endpoint = screenPointFor(controller, canvas, 0.4, 0.3);
        sendMouse(canvas, QEvent::MouseButtonPress, endpoint, Qt::LeftButton, Qt::LeftButton);
        const QPointF target = screenPointFor(controller, canvas, 0.45, 0.4);
        sendMouse(canvas, QEvent::MouseMove, target, Qt::NoButton, Qt::LeftButton);
        sendMouse(canvas, QEvent::MouseButtonRelease, target, Qt::LeftButton, Qt::NoButton);
    }
    line = activeObject(controller);
    assert(near(line.value(QStringLiteral("x2")).toDouble(), 0.45));
    assert(near(line.value(QStringLiteral("y2")).toDouble(), 0.4));
    assert(near(line.value(QStringLiteral("x1")).toDouble(), 0.2));

    // Ctrl + wheel zoom is anchored at the cursor: the canvas point under the
    // anchor stays at the same screen pixel; other points move. The anchor is
    // deliberately off-centre (the board centre is invariant under scaling
    // regardless of pan, so it could not detect a missing pan compensation).
    {
        const QPointF anchor = canvas.mapCanvasToScreen(0.3, 0.7);
        const QPointF otherBefore = canvas.mapCanvasToScreen(0.6, 0.2);
        const double zoomBefore = canvas.viewportZoom();
        sendWheel(canvas, anchor, 120, Qt::ControlModifier);
        assert(canvas.viewportZoom() > zoomBefore); // zoomed in
        const QPointF anchorAfter = canvas.mapCanvasToScreen(0.3, 0.7);
        assert(near(anchorAfter.x(), anchor.x()));
        assert(near(anchorAfter.y(), anchor.y()));
        const QPointF otherAfter = canvas.mapCanvasToScreen(0.6, 0.2);
        assert(!near(otherAfter.x(), otherBefore.x()) || !near(otherAfter.y(), otherBefore.y()));
    }

    // Plain wheel pans the view (the anchor point moves on screen).
    {
        const QPointF before = canvas.mapCanvasToScreen(0.5, 0.5);
        sendWheel(canvas, before, 120, Qt::NoModifier);
        const QPointF after = canvas.mapCanvasToScreen(0.5, 0.5);
        assert(!near(after.x(), before.x()) || !near(after.y(), before.y()));
    }

    // Keyboard map. Use a fresh controller/canvas at zoom 1 so screenPointFor
    // (which assumes the default board) lines up with the widget's mapping.
    {
        DrawingDocumentController keyController;
        DrawingCanvasWidget keyCanvas(&keyController);
        keyCanvas.resize(600, 450);

        // Escape cancels a pending two-click creation and clears the preview.
        keyController.setSelectedToolId(QStringLiteral("line_tool"));
        clickCanvas(keyController, keyCanvas, 0.3, 0.3); // first click: pending
        const QPointF mid = screenPointFor(keyController, keyCanvas, 0.6, 0.6);
        sendMouse(keyCanvas, QEvent::MouseMove, mid, Qt::NoButton, Qt::NoButton);
        assert(keyController.modelDocument().contains(QStringLiteral("preview_object")));
        sendKey(keyCanvas, Qt::Key_Escape);
        assert(!keyController.modelDocument().contains(QStringLiteral("preview_object")));
        assert(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());

        // Create a point, then Delete removes it.
        keyController.setSelectedToolId(QStringLiteral("point_tool"));
        clickCanvas(keyController, keyCanvas, 0.5, 0.5);
        assert(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 1);
        sendKey(keyCanvas, Qt::Key_Delete);
        assert(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());

        // Create a point, Ctrl+D duplicates it.
        clickCanvas(keyController, keyCanvas, 0.4, 0.4);
        assert(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 1);
        sendKey(keyCanvas, Qt::Key_D, Qt::ControlModifier);
        assert(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 2);

        // Arrow key nudges the selection by the grid step.
        keyController.setSelectedToolId(QStringLiteral("select_move"));
        const QString dupId = keyController.selectedObjectId();
        const double beforeX = activeObject(keyController).value(QStringLiteral("x")).toDouble();
        sendKey(keyCanvas, Qt::Key_Right);
        const double afterX = activeObject(keyController).value(QStringLiteral("x")).toDouble();
        assert(afterX > beforeX);
        assert(keyController.selectedObjectId() == dupId);
    }

    // Two-click polygon creation honours the controller's side count. A fresh
    // controller/canvas keeps the board at zoom 1 so screenPointFor lines up.
    {
        DrawingDocumentController polyController;
        DrawingCanvasWidget polyCanvas(&polyController);
        polyCanvas.resize(600, 450);
        polyController.setPolygonSides(5);
        polyController.setSelectedToolId(QStringLiteral("regular_polygon_tool"));
        clickCanvas(polyController, polyCanvas, 0.5, 0.5); // centre
        clickCanvas(polyController, polyCanvas, 0.7, 0.5); // radius point
        const QVariantMap polygon = activeObject(polyController);
        assert(polygon.value(QStringLiteral("kind")).toString() == QStringLiteral("polygon"));
        assert(polygon.value(QStringLiteral("points")).toList().size() == 5);
    }

    // A multi-move object drag is a single undo step (regression: each move used
    // to push its own snapshot).
    {
        DrawingDocumentController dragController;
        DrawingCanvasWidget dragCanvas(&dragController);
        dragCanvas.resize(600, 450);
        dragController.setSelectedToolId(QStringLiteral("point_tool"));
        clickCanvas(dragController, dragCanvas, 0.3, 0.3);
        const QString id = dragController.selectedObjectId();
        const double startX = activeObject(dragController).value(QStringLiteral("x")).toDouble();

        dragController.setSelectedToolId(QStringLiteral("select_move"));
        const QPointF press = screenPointFor(dragController, dragCanvas, 0.3, 0.3);
        sendMouse(dragCanvas, QEvent::MouseButtonPress, press, Qt::LeftButton, Qt::LeftButton);
        // Several intermediate moves, as a real drag produces.
        for (double t : {0.4, 0.5, 0.6, 0.7}) {
            const QPointF move = screenPointFor(dragController, dragCanvas, t, 0.3);
            sendMouse(dragCanvas, QEvent::MouseMove, move, Qt::NoButton, Qt::LeftButton);
        }
        const QPointF release = screenPointFor(dragController, dragCanvas, 0.7, 0.3);
        sendMouse(dragCanvas, QEvent::MouseButtonRelease, release, Qt::LeftButton, Qt::NoButton);

        const double movedX = activeObject(dragController).value(QStringLiteral("x")).toDouble();
        assert(movedX > startX + 0.2); // the drag moved the point substantially
        // One undo returns the point all the way to the start (proving the whole
        // drag is a single step, not one step per intermediate move).
        assert(dragController.canUndo());
        assert(dragController.undo());
        const double undoneX = activeObject(dragController).value(QStringLiteral("x")).toDouble();
        assert(near(undoneX, startX));
        // Only the original point-creation step remains; a second undo empties it.
        assert(dragController.canUndo());
        dragController.undo();
        assert(dragController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());
        assert(!dragController.canUndo());
    }

    // Two-click arc creation, then drag its radius handle to grow the radius.
    {
        DrawingDocumentController arcController;
        DrawingCanvasWidget arcCanvas(&arcController);
        arcCanvas.resize(600, 450);
        arcController.setSelectedToolId(QStringLiteral("arc_tool"));
        clickCanvas(arcController, arcCanvas, 0.5, 0.5); // centre
        clickCanvas(arcController, arcCanvas, 0.7, 0.5); // radius 0.2, start angle 0
        QVariantMap arc = activeObject(arcController);
        assert(arc.value(QStringLiteral("kind")).toString() == QStringLiteral("arc"));
        assert(near(arc.value(QStringLiteral("radius")).toDouble(), 0.2));
        const double startRadius = arc.value(QStringLiteral("radius")).toDouble();

        // The radius handle sits at the mid angle ((0 + 105)/2 = 52.5deg).
        const double midRad = 52.5 * 3.14159265358979323846 / 180.0;
        const double handleX = 0.5 + startRadius * std::cos(midRad);
        const double handleY = 0.5 + startRadius * std::sin(midRad);

        arcController.setSelectedToolId(QStringLiteral("select_move"));
        const QPointF grabAt = screenPointFor(arcController, arcCanvas, handleX, handleY);
        sendMouse(arcCanvas, QEvent::MouseButtonPress, grabAt, Qt::LeftButton, Qt::LeftButton);
        // Drag the handle outward along the same mid-angle ray to a larger radius.
        const double newRadius = 0.32;
        const QPointF target = screenPointFor(arcController, arcCanvas,
            0.5 + newRadius * std::cos(midRad), 0.5 + newRadius * std::sin(midRad));
        sendMouse(arcCanvas, QEvent::MouseMove, target, Qt::NoButton, Qt::LeftButton);
        sendMouse(arcCanvas, QEvent::MouseButtonRelease, target, Qt::LeftButton, Qt::NoButton);
        arc = activeObject(arcController);
        assert(arc.value(QStringLiteral("radius")).toDouble() > startRadius);
    }

    return 0;
}
