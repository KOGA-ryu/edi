#include "widgets/DrawingCanvasWidget.h"

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasViewport.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPointF>
#include <QVariantList>
#include <QVariantMap>

#include <cassert>
#include <cmath>

namespace {

QPointF screenPointFor(const DrawingDocumentController &controller, const QWidget &widget, double x, double y)
{
    const drawing_canvas::DrawingCanvasViewportInput input =
        drawing_canvas::viewportInputFromModel(controller.modelDocument(), widget.width(), widget.height());
    return drawing_canvas::canvasToScreen(drawing_canvas::viewportBoardRect(input), x, y);
}

void sendMouse(QWidget &widget, QEvent::Type type, const QPointF &pos, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, pos, widget.mapToGlobal(pos), button, buttons, Qt::NoModifier);
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

    return 0;
}
