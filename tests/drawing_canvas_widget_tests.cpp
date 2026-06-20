#include "widgets/DrawingCanvasWidget.h"

#include "core/DrawingCore.h"
#include "widgets/DrawingCanvasObjectPainter.h"
#include "widgets/DrawingCanvasViewport.h"

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointF>
#include <QRect>
#include <QVariantList>
#include <QVariantMap>
#include <QWheelEvent>

#include "EdiAssert.h"
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

// --- Painter render-contract helpers ---
// The render-contract block at the end strokes each shape #ff0000 and reads
// the grabbed QImage back. `reddish` is "this pixel is the shape's ink": a red
// channel that clearly dominates green and blue. The board is a dark blue-grey
// and the grid a faint blend — neither is reddish — so this separates ink from
// background without pinning the exact sub-pixel the rasteriser chose, and the
// generous margins survive the anti-aliasing on curved/glyph edges.
bool reddish(const QColor &c)
{
    return c.red() > 0x90 && c.red() > c.green() + 0x40 && c.red() > c.blue() + 0x40;
}

// "Did the stroke land near here?" — any reddish pixel within `radius` of
// `center`. Used for rim/curve points where the stroke width and sampling
// leave the exact pixel a little fuzzy, but the neighbourhood is unambiguous.
bool anyReddishNear(const QImage &image, const QPoint &center, int radius)
{
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const QPoint p = center + QPoint(dx, dy);
            if (p.x() < 0 || p.y() < 0 || p.x() >= image.width() || p.y() >= image.height()) {
                continue;
            }
            if (reddish(QColor(image.pixel(p)))) {
                return true;
            }
        }
    }
    return false;
}

// How many reddish pixels fall inside `box` — the text test needs a CLUSTER
// of glyph ink, not a single stray pixel, and needs the count to be zero
// outside the glyph box.
int reddishCount(const QImage &image, const QRect &box)
{
    int count = 0;
    for (int y = box.top(); y <= box.bottom(); ++y) {
        for (int x = box.left(); x <= box.right(); ++x) {
            if (x < 0 || y < 0 || x >= image.width() || y >= image.height()) {
                continue;
            }
            if (reddish(QColor(image.pixel(x, y)))) {
                ++count;
            }
        }
    }
    return count;
}

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // M1.2 wall corner miter — pure geometry (no widget). Two bands meeting at
    // the screen origin: this wall runs left (into-dir (-1,0)), the neighbour
    // runs down (into-dir (0,1)), both half-thickness 10px. The outer notch is
    // the unit square scaled to 10 in the +x/-y quadrant; the miter quad fills
    // it exactly, apex at (10,-10).
    {
        const std::vector<QPointF> quad = drawing_canvas::wallCornerMiterFill(
            QPointF(0.0, 0.0), QPointF(-1.0, 0.0), QPointF(0.0, 1.0), 10.0, 10.0, 4.0);
        EDI_CHECK(quad.size() == 4);
        EDI_CHECK(near(quad[0].x(), 0.0) && near(quad[0].y(), -10.0)); // this band's outer corner
        EDI_CHECK(near(quad[1].x(), 10.0) && near(quad[1].y(), -10.0)); // miter apex
        EDI_CHECK(near(quad[2].x(), 10.0) && near(quad[2].y(), 0.0));   // neighbour's outer corner
        EDI_CHECK(near(quad[3].x(), 0.0) && near(quad[3].y(), 0.0));    // the shared point

        // Collinear walls (straight through) have no corner -> no fill.
        const std::vector<QPointF> straight = drawing_canvas::wallCornerMiterFill(
            QPointF(0.0, 0.0), QPointF(-1.0, 0.0), QPointF(1.0, 0.0), 10.0, 10.0, 4.0);
        EDI_CHECK(straight.empty());

        // A sharp ~10deg V sends the apex past the miter limit -> 3-pt bevel.
        const double s = std::sin(10.0 * 3.14159265358979323846 / 180.0);
        const double c = std::cos(10.0 * 3.14159265358979323846 / 180.0);
        const std::vector<QPointF> bevel = drawing_canvas::wallCornerMiterFill(
            QPointF(0.0, 0.0), QPointF(0.0, 1.0), QPointF(s, c), 10.0, 10.0, 4.0);
        EDI_CHECK(bevel.size() == 3);

        // The returned wedge must ALWAYS be a simple convex polygon (or empty) —
        // never a self-intersecting bow-tie. Unequal band thicknesses at an
        // obtuse corner used to fold the quad, which OddEven fill would leave
        // holed; sweep angles x thickness ratios to pin that it never folds.
        const auto convexOrEmpty = [](const std::vector<QPointF> &poly) {
            if (poly.empty()) {
                return true;
            }
            if (poly.size() < 3) {
                return false;
            }
            bool pos = false, neg = false;
            for (std::size_t i = 0; i < poly.size(); ++i) {
                const QPointF a = poly[i];
                const QPointF b = poly[(i + 1) % poly.size()];
                const QPointF d = poly[(i + 2) % poly.size()];
                const double turn = (b.x() - a.x()) * (d.y() - b.y()) - (b.y() - a.y()) * (d.x() - b.x());
                if (turn > 1e-9) {
                    pos = true;
                } else if (turn < -1e-9) {
                    neg = true;
                }
            }
            return !(pos && neg);
        };
        const double pi = 3.14159265358979323846;
        for (int deg = 10; deg <= 350; deg += 10) {
            const QPointF tNbr(std::cos(deg * pi / 180.0), std::sin(deg * pi / 180.0));
            for (double hNbr : {4.0, 10.0, 18.0}) {
                EDI_CHECK(convexOrEmpty(drawing_canvas::wallCornerMiterFill(
                    QPointF(0.0, 0.0), QPointF(-1.0, 0.0), tNbr, 10.0, hNbr, 4.0)));
            }
        }
        // The specific unequal-thickness obtuse corner that previously folded
        // (this=10, neighbour=4 at 45deg) now degrades to the 3-pt bevel.
        const QPointF foldDir(std::cos(45.0 * pi / 180.0), std::sin(45.0 * pi / 180.0));
        EDI_CHECK(drawing_canvas::wallCornerMiterFill(
                   QPointF(0.0, 0.0), QPointF(-1.0, 0.0), foldDir, 10.0, 4.0, 4.0).size() == 3);
    }

    // M1.2 join detection: annotateWallJoins flags a corner only where EXACTLY
    // two walls share an endpoint; the lower scene index draws it, and a
    // T-junction (3 walls) keeps square caps (no conflicting partial miters).
    {
        using drawing_canvas::DrawingCanvasSceneItem;
        const auto wallItem = [](double ax, double ay, double bx, double by) {
            DrawingCanvasSceneItem item;
            item.wall.ok = true;
            item.wall.ax = ax;
            item.wall.ay = ay;
            item.wall.bx = bx;
            item.wall.by = by;
            item.wall.thickness = 0.1;
            return item;
        };
        // Two walls sharing (0.6,0.5): wall0's end b joins, drawn by the lower index.
        {
            std::vector<DrawingCanvasSceneItem> scene{
                wallItem(0.3, 0.5, 0.6, 0.5),
                wallItem(0.6, 0.5, 0.6, 0.8)};
            drawing_canvas::annotateWallJoins(scene);
            EDI_CHECK(scene[0].wall.joinB.drawCorner);   // lower index paints the corner
            EDI_CHECK(!scene[1].wall.joinA.drawCorner);   // neighbour does not double-draw
            EDI_CHECK(near(scene[0].wall.joinB.neighborFarX, 0.6));
            EDI_CHECK(near(scene[0].wall.joinB.neighborFarY, 0.8)); // neighbour's far endpoint
        }
        // Three walls at one point (T-junction): no miter anywhere.
        {
            std::vector<DrawingCanvasSceneItem> scene{
                wallItem(0.3, 0.5, 0.6, 0.5),
                wallItem(0.6, 0.5, 0.6, 0.8),
                wallItem(0.6, 0.5, 0.9, 0.5)};
            drawing_canvas::annotateWallJoins(scene);
            EDI_CHECK(!scene[0].wall.joinB.drawCorner);
            EDI_CHECK(!scene[1].wall.joinA.drawCorner);
            EDI_CHECK(!scene[2].wall.joinA.drawCorner);
        }
    }

    DrawingDocumentController controller;
    DrawingCanvasWidget canvas(&controller);
    canvas.resize(600, 450);

    // Click-create: a point tool click on the canvas creates and selects a point.
    controller.setSelectedToolId(QStringLiteral("point_tool"));
    clickCanvas(controller, canvas, 0.5, 0.5);
    QVariantMap point = activeObject(controller);
    EDI_CHECK(!point.isEmpty());
    EDI_CHECK(point.value(QStringLiteral("kind")).toString() == QStringLiteral("point"));
    EDI_CHECK(near(point.value(QStringLiteral("x")).toDouble(), 0.5));
    EDI_CHECK(near(point.value(QStringLiteral("y")).toDouble(), 0.5));

    // Drag-move: clear the selection first so the press takes the object-drag
    // path (a selected point's edit handle sits exactly on it and would route
    // the press to handle-drag instead).
    controller.setSelectedToolId(QStringLiteral("select_move"));
    clickCanvas(controller, canvas, 0.9, 0.9);
    EDI_CHECK(controller.selectedObjectId().isEmpty());
    {
        const QPointF start = screenPointFor(controller, canvas, 0.5, 0.5);
        sendMouse(canvas, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        EDI_CHECK(!controller.selectedObjectId().isEmpty());
        const QPointF mid = screenPointFor(controller, canvas, 0.6, 0.55);
        sendMouse(canvas, QEvent::MouseMove, mid, Qt::NoButton, Qt::LeftButton);
        sendMouse(canvas, QEvent::MouseButtonRelease, mid, Qt::LeftButton, Qt::NoButton);
    }
    point = activeObject(controller);
    EDI_CHECK(near(point.value(QStringLiteral("x")).toDouble(), 0.6));
    EDI_CHECK(near(point.value(QStringLiteral("y")).toDouble(), 0.55));

    // Marquee select: drag on empty canvas around the point; it gets selected.
    {
        const QPointF start = screenPointFor(controller, canvas, 0.2, 0.2);
        sendMouse(canvas, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        // Press on empty space clears the selection before the marquee begins.
        EDI_CHECK(controller.selectedObjectId().isEmpty());
        const QPointF end = screenPointFor(controller, canvas, 0.8, 0.8);
        sendMouse(canvas, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        sendMouse(canvas, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    }
    {
        const QVariantList selected = controller.modelDocument().value(QStringLiteral("selected_object_ids")).toList();
        EDI_CHECK(selected.size() == 1);
        EDI_CHECK(selected.first().toString() == point.value(QStringLiteral("id")).toString());
    }

    // Two-click creation: a line tool needs press at both endpoints.
    controller.setSelectedToolId(QStringLiteral("line_tool"));
    clickCanvas(controller, canvas, 0.2, 0.3);
    clickCanvas(controller, canvas, 0.4, 0.3);
    QVariantMap line = activeObject(controller);
    EDI_CHECK(line.value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
    EDI_CHECK(near(line.value(QStringLiteral("x1")).toDouble(), 0.2));
    EDI_CHECK(near(line.value(QStringLiteral("x2")).toDouble(), 0.4));

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
    EDI_CHECK(near(line.value(QStringLiteral("x2")).toDouble(), 0.45));
    EDI_CHECK(near(line.value(QStringLiteral("y2")).toDouble(), 0.4));
    EDI_CHECK(near(line.value(QStringLiteral("x1")).toDouble(), 0.2));

    // Ctrl + wheel zoom is anchored at the cursor: the canvas point under the
    // anchor stays at the same screen pixel; other points move. The anchor is
    // deliberately off-centre (the board centre is invariant under scaling
    // regardless of pan, so it could not detect a missing pan compensation).
    {
        const QPointF anchor = canvas.mapCanvasToScreen(0.3, 0.7);
        const QPointF otherBefore = canvas.mapCanvasToScreen(0.6, 0.2);
        const double zoomBefore = canvas.viewportZoom();
        sendWheel(canvas, anchor, 120, Qt::ControlModifier);
        EDI_CHECK(canvas.viewportZoom() > zoomBefore); // zoomed in
        const QPointF anchorAfter = canvas.mapCanvasToScreen(0.3, 0.7);
        EDI_CHECK(near(anchorAfter.x(), anchor.x()));
        EDI_CHECK(near(anchorAfter.y(), anchor.y()));
        const QPointF otherAfter = canvas.mapCanvasToScreen(0.6, 0.2);
        EDI_CHECK(!near(otherAfter.x(), otherBefore.x()) || !near(otherAfter.y(), otherBefore.y()));
    }

    // Plain wheel pans the view (the anchor point moves on screen).
    {
        const QPointF before = canvas.mapCanvasToScreen(0.5, 0.5);
        sendWheel(canvas, before, 120, Qt::NoModifier);
        const QPointF after = canvas.mapCanvasToScreen(0.5, 0.5);
        EDI_CHECK(!near(after.x(), before.x()) || !near(after.y(), before.y()));
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
        EDI_CHECK(keyController.modelDocument().contains(QStringLiteral("preview_object")));
        sendKey(keyCanvas, Qt::Key_Escape);
        EDI_CHECK(!keyController.modelDocument().contains(QStringLiteral("preview_object")));
        EDI_CHECK(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());

        // Create a point, then Delete removes it.
        keyController.setSelectedToolId(QStringLiteral("point_tool"));
        clickCanvas(keyController, keyCanvas, 0.5, 0.5);
        EDI_CHECK(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 1);
        sendKey(keyCanvas, Qt::Key_Delete);
        EDI_CHECK(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());

        // Create a point, Ctrl+D duplicates it.
        clickCanvas(keyController, keyCanvas, 0.4, 0.4);
        EDI_CHECK(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 1);
        sendKey(keyCanvas, Qt::Key_D, Qt::ControlModifier);
        EDI_CHECK(keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 2);

        // Arrow key nudges the selection by the grid step.
        keyController.setSelectedToolId(QStringLiteral("select_move"));
        const QString dupId = keyController.selectedObjectId();
        const double beforeX = activeObject(keyController).value(QStringLiteral("x")).toDouble();
        sendKey(keyCanvas, Qt::Key_Right);
        const double afterX = activeObject(keyController).value(QStringLiteral("x")).toDouble();
        EDI_CHECK(afterX > beforeX);
        EDI_CHECK(keyController.selectedObjectId() == dupId);

        // N1: Ctrl+C / Ctrl+V / Ctrl+X drive the controller clipboard from the
        // canvas. Copy+paste the duplicate (still selected) to add one more.
        auto objectCount = [&]() {
            return keyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        };
        EDI_CHECK(objectCount() == 2);
        sendKey(keyCanvas, Qt::Key_C, Qt::ControlModifier);
        sendKey(keyCanvas, Qt::Key_V, Qt::ControlModifier);
        EDI_CHECK(objectCount() == 3);

        // Cut everything, then paste it all back.
        keyController.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        sendKey(keyCanvas, Qt::Key_X, Qt::ControlModifier);
        EDI_CHECK(objectCount() == 0);
        sendKey(keyCanvas, Qt::Key_V, Qt::ControlModifier);
        EDI_CHECK(objectCount() == 3); // all three cut objects pasted back
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
        EDI_CHECK(polygon.value(QStringLiteral("kind")).toString() == QStringLiteral("polygon"));
        EDI_CHECK(polygon.value(QStringLiteral("points")).toList().size() == 5);
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
        EDI_CHECK(movedX > startX + 0.2); // the drag moved the point substantially
        // One undo returns the point all the way to the start (proving the whole
        // drag is a single step, not one step per intermediate move).
        EDI_CHECK(dragController.canUndo());
        EDI_CHECK(dragController.undo());
        const double undoneX = activeObject(dragController).value(QStringLiteral("x")).toDouble();
        EDI_CHECK(near(undoneX, startX));
        // Only the original point-creation step remains; a second undo empties it.
        EDI_CHECK(dragController.canUndo());
        dragController.undo();
        EDI_CHECK(dragController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());
        EDI_CHECK(!dragController.canUndo());
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
        EDI_CHECK(arc.value(QStringLiteral("kind")).toString() == QStringLiteral("arc"));
        EDI_CHECK(near(arc.value(QStringLiteral("radius")).toDouble(), 0.2));
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
        EDI_CHECK(arc.value(QStringLiteral("radius")).toDouble() > startRadius);
    }

    // Two-click wall creation (a thick line), then drag its start endpoint
    // handle. The default band thickness 0.1 makes it render; the projection
    // emits ax/ay/bx/by + thickness for the inspector and the band painter.
    {
        DrawingDocumentController wallController;
        DrawingCanvasWidget wallCanvas(&wallController);
        wallCanvas.resize(600, 450);
        wallController.setSelectedToolId(QStringLiteral("wall_tool"));
        clickCanvas(wallController, wallCanvas, 0.3, 0.5); // a
        clickCanvas(wallController, wallCanvas, 0.7, 0.5); // b
        QVariantMap wall = activeObject(wallController);
        EDI_CHECK(wall.value(QStringLiteral("kind")).toString() == QStringLiteral("wall"));
        EDI_CHECK(near(wall.value(QStringLiteral("ax")).toDouble(), 0.3));
        EDI_CHECK(near(wall.value(QStringLiteral("bx")).toDouble(), 0.7));
        EDI_CHECK(near(wall.value(QStringLiteral("thickness")).toDouble(), 0.1));

        // Grab the start endpoint handle at (0.3,0.5) and drag it to (0.2,0.4).
        wallController.setSelectedToolId(QStringLiteral("select_move"));
        const QPointF grabAt = screenPointFor(wallController, wallCanvas, 0.3, 0.5);
        sendMouse(wallCanvas, QEvent::MouseButtonPress, grabAt, Qt::LeftButton, Qt::LeftButton);
        const QPointF target = screenPointFor(wallController, wallCanvas, 0.2, 0.4);
        sendMouse(wallCanvas, QEvent::MouseMove, target, Qt::NoButton, Qt::LeftButton);
        sendMouse(wallCanvas, QEvent::MouseButtonRelease, target, Qt::LeftButton, Qt::NoButton);
        wall = activeObject(wallController);
        EDI_CHECK(near(wall.value(QStringLiteral("ax")).toDouble(), 0.2));
        EDI_CHECK(near(wall.value(QStringLiteral("ay")).toDouble(), 0.4));
        EDI_CHECK(near(wall.value(QStringLiteral("bx")).toDouble(), 0.7)); // b end untouched
    }

    // N2 arrow: a two-click line whose projection carries end_arrow; a plain
    // line does not. The DoD's "distinct projected flag".
    {
        DrawingDocumentController arrowController;
        DrawingCanvasWidget arrowCanvas(&arrowController);
        arrowCanvas.resize(640, 480);
        arrowCanvas.show();

        arrowController.setSelectedToolId(QStringLiteral("arrow_tool"));
        clickCanvas(arrowController, arrowCanvas, 0.2, 0.2);
        clickCanvas(arrowController, arrowCanvas, 0.6, 0.4);
        const QVariantMap arrow = activeObject(arrowController);
        EDI_CHECK(arrow.value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
        EDI_CHECK(arrow.value(QStringLiteral("end_arrow")).toBool());

        arrowController.setSelectedToolId(QStringLiteral("line_tool"));
        clickCanvas(arrowController, arrowCanvas, 0.3, 0.7);
        clickCanvas(arrowController, arrowCanvas, 0.7, 0.8);
        const QVariantMap plainLine = activeObject(arrowController);
        EDI_CHECK(plainLine.value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
        EDI_CHECK(!plainLine.value(QStringLiteral("end_arrow")).toBool());
    }

    // Polyline: clicks anchor vertices, a double-click finishes the trail.
    {
        DrawingDocumentController polyController;
        DrawingCanvasWidget polyCanvas(&polyController);
        polyCanvas.resize(640, 480);
        polyCanvas.show();
        polyController.setSelectedToolId(QStringLiteral("polyline_tool"));

        const QPointF a = screenPointFor(polyController, polyCanvas, 0.2, 0.2);
        const QPointF b = screenPointFor(polyController, polyCanvas, 0.5, 0.3);
        const QPointF c = screenPointFor(polyController, polyCanvas, 0.7, 0.6);
        sendMouse(polyCanvas, QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton);
        sendMouse(polyCanvas, QEvent::MouseButtonRelease, a, Qt::LeftButton, Qt::NoButton);
        sendMouse(polyCanvas, QEvent::MouseButtonPress, b, Qt::LeftButton, Qt::LeftButton);
        sendMouse(polyCanvas, QEvent::MouseButtonRelease, b, Qt::LeftButton, Qt::NoButton);
        EDI_CHECK(polyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().isEmpty());

        // The double-click pair: its press anchors the final vertex, the
        // dblclick event closes the trail there.
        sendMouse(polyCanvas, QEvent::MouseButtonPress, c, Qt::LeftButton, Qt::LeftButton);
        sendMouse(polyCanvas, QEvent::MouseButtonRelease, c, Qt::LeftButton, Qt::NoButton);
        sendMouse(polyCanvas, QEvent::MouseButtonDblClick, c, Qt::LeftButton, Qt::LeftButton);
        const QVariantList objects = polyController.modelDocument().value(QStringLiteral("drawing_objects")).toList();
        EDI_CHECK(objects.size() == 1);
        EDI_CHECK(objects.front().toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("polyline"));

        // Escape mid-trail leaves the document alone.
        polyController.setSelectedToolId(QStringLiteral("polyline_tool"));
        sendMouse(polyCanvas, QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton);
        sendMouse(polyCanvas, QEvent::MouseButtonRelease, a, Qt::LeftButton, Qt::NoButton);
        sendKey(polyCanvas, Qt::Key_Escape);
        sendMouse(polyCanvas, QEvent::MouseButtonDblClick, a, Qt::LeftButton, Qt::LeftButton);
        EDI_CHECK(polyController.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 1);
    }

    // Overlay gating: during normal drafting a fresh shape adds NO wash to
    // the board around it — no green plot-safety rect, no amber selection
    // plot-bounds rect (the user's 'square highlighted box' report). The
    // plot preview brings the diagnostics back. Measured as patch diffs
    // around a point INSIDE the shape's bbox but off its stroke and handles.
    {
        DrawingDocumentController overlayController;
        DrawingCanvasWidget overlayCanvas(&overlayController);
        overlayCanvas.resize(600, 450);

        const auto patchAt = [&overlayCanvas](const QPoint &center) {
            const QImage frame = overlayCanvas.grab().toImage();
            return frame.copy(center.x() - 10, center.y() - 10, 20, 20);
        };
        const auto diffCount = [](const QImage &a, const QImage &b) {
            int differing = 0;
            for (int y = 0; y < a.height(); ++y) {
                for (int x = 0; x < a.width(); ++x) {
                    if (a.pixel(x, y) != b.pixel(x, y)) {
                        ++differing;
                    }
                }
            }
            return differing;
        };

        const QPoint probe = screenPointFor(overlayController, overlayCanvas, 0.45, 0.45).toPoint();
        const QImage emptyPatch = patchAt(probe);

        overlayController.setSelectedToolId(QStringLiteral("circle_tool"));
        clickCanvas(overlayController, overlayCanvas, 0.4, 0.4);
        clickCanvas(overlayController, overlayCanvas, 0.6, 0.6); // circle, auto-selected
        EDI_CHECK(!overlayController.selectedObjectId().isEmpty());

        // Normal drafting: the patch sits inside the new circle's bbox, off
        // the stroke — a box wash would tint all 400 pixels; without it the
        // patch stays (nearly) what the empty board showed.
        const QImage draftingPatch = patchAt(probe);
        EDI_CHECK(diffCount(emptyPatch, draftingPatch) <= 40);

        // Plot preview: selection bounds + machine bounds return as washes.
        overlayCanvas.setPlotPreviewVisible(true);
        const QImage previewPatch = patchAt(probe);
        EDI_CHECK(diffCount(draftingPatch, previewPatch) > 200);

        // And toggling OFF removes them again — the review caught warning
        // boxes burned into the static layer because the toggle changed no
        // cache key; this round-trip fails without the invalidation.
        overlayCanvas.setPlotPreviewVisible(false);
        const QImage offAgainPatch = patchAt(probe);
        EDI_CHECK(diffCount(draftingPatch, offAgainPatch) <= 10);
    }

    // View commands: Cmd/Ctrl +/-/0 step and reset the zoom through the
    // same viewport math as the wheel; the widget reports the factor and
    // emits zoomChanged for the status readout.
    {
        DrawingDocumentController viewController;
        DrawingCanvasWidget viewCanvas(&viewController);
        viewCanvas.resize(600, 450);
        int zoomSignals = 0;
        QObject::connect(&viewCanvas, &DrawingCanvasWidget::zoomChanged, [&zoomSignals]() { ++zoomSignals; });

        EDI_CHECK(near(viewCanvas.viewportZoom(), 1.0));
        sendKey(viewCanvas, Qt::Key_Equal, Qt::ControlModifier);
        EDI_CHECK(viewCanvas.viewportZoom() > 1.2);
        sendKey(viewCanvas, Qt::Key_Minus, Qt::ControlModifier);
        sendKey(viewCanvas, Qt::Key_0, Qt::ControlModifier);
        EDI_CHECK(near(viewCanvas.viewportZoom(), 1.0));
        EDI_CHECK(zoomSignals == 3);
    }

    // Per-object styling paints: a line given its own color renders that
    // color on canvas (scan across the stroke's midpoint for the ink).
    {
        DrawingDocumentController styleController;
        DrawingCanvasWidget styleCanvas(&styleController);
        styleCanvas.resize(600, 450);
        styleController.setSelectedToolId(QStringLiteral("line_tool"));
        clickCanvas(styleController, styleCanvas, 0.2, 0.5);
        clickCanvas(styleController, styleCanvas, 0.8, 0.5);
        EDI_CHECK(styleController.setSelectedObjectStrokeColor(QStringLiteral("#ff2200")));
        styleController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(styleController, styleCanvas, 0.05, 0.05); // deselect: selection restroke would mask the color

        const QImage frame = styleCanvas.grab().toImage();
        const QPoint mid = screenPointFor(styleController, styleCanvas, 0.5, 0.5).toPoint();
        const auto scanForInk = [&](const QImage &image) {
            for (int dy = -4; dy <= 4; ++dy) {
                const QColor pixel(image.pixel(mid + QPoint(0, dy)));
                if (pixel.red() > 0xc0 && pixel.green() < 0x60 && pixel.blue() < 0x60) {
                    return true;
                }
            }
            return false;
        };
        EDI_CHECK(scanForInk(frame));

        // Opacity is PEN ALPHA at the pixel level — the one hop the
        // projection tests cannot see. Fade the line to 0 and the red ink
        // must vanish from the same scan column (the edit bumps
        // modelGeneration, so the static layer re-renders on grab).
        clickCanvas(styleController, styleCanvas, 0.5, 0.5); // re-select the line
        EDI_CHECK(styleController.setSelectedObjectStrokeOpacity(0.0));
        clickCanvas(styleController, styleCanvas, 0.05, 0.05); // deselect again
        EDI_CHECK(!scanForInk(styleCanvas.grab().toImage()));

        // Selected objects stay fully visible regardless of opacity — a
        // transparent object must still light up when picked, or it can
        // never be found to edit.
        clickCanvas(styleController, styleCanvas, 0.5, 0.5);
        EDI_CHECK(styleController.selectedObjectId() != QString());
        const QImage selectedFrame = styleCanvas.grab().toImage();
        bool sawSelection = false;
        for (int dy = -4; dy <= 4 && !sawSelection; ++dy) {
            const QColor pixel(selectedFrame.pixel(mid + QPoint(0, dy)));
            // Any clearly non-background ink counts: the selection restroke
            // paints at full alpha in the selection color.
            sawSelection = pixel.value() > 0x80 && pixel != QColor(selectedFrame.pixel(QPoint(2, 2)));
        }
        EDI_CHECK(sawSelection);
    }

    // Rulers paint: the band separators sit at the 20px lines in the
    // board-outline color, and a major tick labels unit 0 at the board's
    // left edge — pixels, because the golden's budget cannot see a band
    // this small (the review's recurring lesson).
    {
        DrawingDocumentController rulerController;
        DrawingCanvasWidget rulerCanvas(&rulerController);
        rulerCanvas.resize(600, 450);
        const QImage frame = rulerCanvas.grab().toImage();
        const QColor separator(frame.pixel(300, 20));
        EDI_CHECK(separator != QColor(frame.pixel(300, 40))); // band edge differs from board
        bool sawTick = false;
        for (int x = 21; x < 600 && !sawTick; ++x) {
            sawTick = QColor(frame.pixel(x, 16)) != QColor(frame.pixel(x, 2));
        }
        EDI_CHECK(sawTick); // at least one tick reaches into the band
    }

    // ===== Painter render contracts =====
    // The recently-shipped shapes (fill, ellipse, spline, text, arc, trim) all
    // had logic/projection/serialize coverage, but nothing pinned that their
    // PAINTER paths put ink on the canvas. The painter splits extraction
    // (buildCanvasSceneItem) from drawing (drawSceneItem): a shape dropped from
    // EITHER half draws nothing, with no compile error (tool-change-log
    // friction #3). Each block builds a known document offscreen, strokes the
    // shape #ff0000, deselects (so the full-alpha gold selection restroke
    // cannot stand in for the object's own ink), grabs the QImage, and asserts
    // on pixels. screenPointFor maps canvas coords to the same board pixels the
    // painter drew through, so the probes track the real geometry.

    // Fill brush: a closed shape with fillOpacity > 0 paints its INTERIOR, not
    // just its outline. Fill was added as a dormant data model (default opacity
    // 0); this is the only proof the opacity actually reaches a QBrush.
    {
        DrawingDocumentController fillController;
        DrawingCanvasWidget fillCanvas(&fillController);
        fillCanvas.resize(600, 450);

        const QPoint interior = screenPointFor(fillController, fillCanvas, 0.5, 0.5).toPoint();
        const QColor emptyInterior(fillCanvas.grab().toImage().pixel(interior));

        fillController.setSelectedToolId(QStringLiteral("rectangle_tool"));
        clickCanvas(fillController, fillCanvas, 0.3, 0.3);
        clickCanvas(fillController, fillCanvas, 0.7, 0.7);
        EDI_CHECK(activeObject(fillController).value(QStringLiteral("kind")).toString() == QStringLiteral("rectangle"));
        EDI_CHECK(fillController.setSelectedObjectFillColor(QStringLiteral("#2277ee")));
        EDI_CHECK(fillController.setSelectedObjectFillOpacity(1.0));
        fillController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(fillController, fillCanvas, 0.02, 0.02); // deselect

        const QColor filled(fillCanvas.grab().toImage().pixel(interior));
        // An opaque fill covers the board it sits on: the interior pixel is now
        // the fill colour (within rounding) and it changed from the bare board.
        EDI_CHECK(filled != emptyInterior);
        EDI_CHECK(std::abs(filled.red() - 0x22) < 0x18
            && std::abs(filled.green() - 0x77) < 0x18
            && std::abs(filled.blue() - 0xee) < 0x18);
    }

    // Ellipse: the projection flattens it to a closed polygon (sampleEllipse,
    // 64 segments) that the painter draws with drawPolygon. Pin that the
    // outline is an ELLIPSE, not its bounding box — the rim crosses each
    // bbox-edge midpoint (an exact sample vertex at 64 segments) but the
    // corners stay bare. That bare corner is what tells an ellipse from a rect.
    {
        DrawingDocumentController ellipseController;
        DrawingCanvasWidget ellipseCanvas(&ellipseController);
        ellipseCanvas.resize(600, 450);
        ellipseController.setSelectedToolId(QStringLiteral("ellipse_tool"));
        clickCanvas(ellipseController, ellipseCanvas, 0.5, 0.5);  // centre
        clickCanvas(ellipseController, ellipseCanvas, 0.8, 0.75); // rx=0.3, ry=0.25
        EDI_CHECK(activeObject(ellipseController).value(QStringLiteral("kind")).toString() == QStringLiteral("ellipse"));
        EDI_CHECK(ellipseController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        ellipseController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(ellipseController, ellipseCanvas, 0.02, 0.02); // deselect

        const QImage frame = ellipseCanvas.grab().toImage();
        // Rightmost (0.8,0.5) and topmost (0.5,0.25) rim points are inked...
        EDI_CHECK(anyReddishNear(frame, screenPointFor(ellipseController, ellipseCanvas, 0.8, 0.5).toPoint(), 4));
        EDI_CHECK(anyReddishNear(frame, screenPointFor(ellipseController, ellipseCanvas, 0.5, 0.25).toPoint(), 4));
        // ...but the bbox CORNER (0.8,0.25) is bare — ~30px from the nearest rim.
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(ellipseController, ellipseCanvas, 0.8, 0.25).toPoint(), 6));
    }

    // Spline: the projection samples the Catmull-Rom curve through the control
    // points (sampleSpline) and the painter draws that as an OPEN polyline. The
    // regression this guards: drawing a straight chord between the endpoints
    // (or nothing). Control points (0.2,0.5)-(0.5,0.2)-(0.8,0.5) make an arch;
    // the curve interpolates the middle knot but never touches the chord.
    {
        DrawingDocumentController splineController;
        DrawingCanvasWidget splineCanvas(&splineController);
        splineCanvas.resize(600, 450);
        splineController.setSelectedToolId(QStringLiteral("spline_tool"));
        const QPointF a = screenPointFor(splineController, splineCanvas, 0.2, 0.5);
        const QPointF b = screenPointFor(splineController, splineCanvas, 0.5, 0.2);
        const QPointF c = screenPointFor(splineController, splineCanvas, 0.8, 0.5);
        sendMouse(splineCanvas, QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton);
        sendMouse(splineCanvas, QEvent::MouseButtonRelease, a, Qt::LeftButton, Qt::NoButton);
        sendMouse(splineCanvas, QEvent::MouseButtonPress, b, Qt::LeftButton, Qt::LeftButton);
        sendMouse(splineCanvas, QEvent::MouseButtonRelease, b, Qt::LeftButton, Qt::NoButton);
        sendMouse(splineCanvas, QEvent::MouseButtonPress, c, Qt::LeftButton, Qt::LeftButton);
        sendMouse(splineCanvas, QEvent::MouseButtonRelease, c, Qt::LeftButton, Qt::NoButton);
        sendMouse(splineCanvas, QEvent::MouseButtonDblClick, c, Qt::LeftButton, Qt::LeftButton);
        EDI_CHECK(activeObject(splineController).value(QStringLiteral("kind")).toString() == QStringLiteral("spline"));
        EDI_CHECK(splineController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        splineController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(splineController, splineCanvas, 0.02, 0.02); // deselect

        const QImage frame = splineCanvas.grab().toImage();
        // The curve passes through the middle control point (it interpolates)...
        EDI_CHECK(anyReddishNear(frame, screenPointFor(splineController, splineCanvas, 0.5, 0.2).toPoint(), 4));
        // ...but the midpoint of the straight chord between the first and last
        // control points (0.5,0.5) is bare — the curve arched ~120px away from
        // it. A straight-line bug would ink exactly this pixel; a draws-nothing
        // bug would fail the assert above. Together they pin "smooth curve".
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(splineController, splineCanvas, 0.5, 0.5).toPoint(), 6));
    }

    // Text: the painter sizes a QFont to height x board and drawText()s the
    // content, dropping the pen by the font ascent so the glyph box TOP sits at
    // the projected position. Pin that glyphs land inside that box, none spill
    // left of the anchor, and none rise above it (the ascent offset is right).
    {
        DrawingDocumentController textController;
        DrawingCanvasWidget textCanvas(&textController);
        textCanvas.resize(600, 450);
        textController.setSelectedToolId(QStringLiteral("text_tool"));
        clickCanvas(textController, textCanvas, 0.5, 0.5); // single click places "Text"
        EDI_CHECK(activeObject(textController).value(QStringLiteral("kind")).toString() == QStringLiteral("text"));
        EDI_CHECK(activeObject(textController).value(QStringLiteral("content")).toString() == QStringLiteral("Text"));
        EDI_CHECK(textController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        textController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(textController, textCanvas, 0.02, 0.02); // deselect

        const QImage frame = textCanvas.grab().toImage();
        const QPoint anchor = screenPointFor(textController, textCanvas, 0.5, 0.5).toPoint(); // glyph-box top-left
        // Glyph ink clusters in the box that opens down-and-right of the anchor.
        EDI_CHECK(reddishCount(frame, QRect(anchor.x(), anchor.y(), 44, 24)) >= 4);
        // Nothing left of the anchor: the text is left-anchored at its position.
        EDI_CHECK(reddishCount(frame, QRect(anchor.x() - 40, anchor.y(), 36, 24)) == 0);
        // Nothing above the anchor: the box top is the position, glyphs sit
        // below it — the ascent baseline-offset, the one subtle bit of drawText.
        EDI_CHECK(reddishCount(frame, QRect(anchor.x(), anchor.y() - 28, 44, 24)) == 0);
    }

    // Arc: flattened to an OPEN polyline (sampleArc) and drawn as a chain (not
    // closed). Pin that the rim is inked only across the swept angles — the
    // centre is bare (no fill, no spokes) and the unswept side of the circle is
    // bare (it is an arc, not a circle).
    {
        DrawingDocumentController arcController;
        DrawingCanvasWidget arcCanvas(&arcController);
        arcCanvas.resize(600, 450);
        arcController.setSelectedToolId(QStringLiteral("arc_tool"));
        clickCanvas(arcController, arcCanvas, 0.5, 0.5); // centre
        clickCanvas(arcController, arcCanvas, 0.7, 0.5); // radius 0.2, start angle 0 -> sweep 105deg
        EDI_CHECK(activeObject(arcController).value(QStringLiteral("kind")).toString() == QStringLiteral("arc"));
        EDI_CHECK(arcController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        arcController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(arcController, arcCanvas, 0.02, 0.02); // deselect

        const QImage frame = arcCanvas.grab().toImage();
        const double deg = 3.14159265358979323846 / 180.0;
        const double mid = 52.5 * deg; // inside the 0..105 sweep
        EDI_CHECK(anyReddishNear(frame, screenPointFor(arcController, arcCanvas,
            0.5 + 0.2 * std::cos(mid), 0.5 + 0.2 * std::sin(mid)).toPoint(), 4));
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(arcController, arcCanvas, 0.5, 0.5).toPoint(), 4));
        const double outside = 230.0 * deg; // well outside the sweep
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(arcController, arcCanvas,
            0.5 + 0.2 * std::cos(outside), 0.5 + 0.2 * std::sin(outside)).toPoint(), 4));
    }

    // Wall: a thick line drawn as a SOLID oriented band, NOT a hairline. The
    // centreline (0.2,0.6)->(0.8,0.6) with the default 0.1 thickness fills the
    // strip y in [0.55,0.65]. Pin that ink covers the centreline AND a point
    // offset within the half-thickness (a hairline would miss it), and that the
    // strip stops: bare well beyond the band and bare past the square-capped end.
    {
        DrawingDocumentController wallController;
        DrawingCanvasWidget wallCanvas(&wallController);
        wallCanvas.resize(600, 450);
        wallController.setSelectedToolId(QStringLiteral("wall_tool"));
        clickCanvas(wallController, wallCanvas, 0.2, 0.6); // a
        clickCanvas(wallController, wallCanvas, 0.8, 0.6); // b
        EDI_CHECK(activeObject(wallController).value(QStringLiteral("kind")).toString() == QStringLiteral("wall"));
        EDI_CHECK(wallController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        wallController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(wallController, wallCanvas, 0.02, 0.02); // deselect

        const QImage frame = wallCanvas.grab().toImage();
        // Inked on the centreline (the solid fill)...
        EDI_CHECK(anyReddishNear(frame, screenPointFor(wallController, wallCanvas, 0.5, 0.6).toPoint(), 3));
        // ...and inked a third of the way to the band edge (0.035 off the
        // centreline, inside the 0.05 half-thickness) — THIS is what a hairline
        // would miss, so it pins "band, not line".
        EDI_CHECK(anyReddishNear(frame, screenPointFor(wallController, wallCanvas, 0.5, 0.635).toPoint(), 3));
        // Bare well beyond the band (0.2 off >> half-thickness)...
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(wallController, wallCanvas, 0.5, 0.8).toPoint(), 5));
        // ...and bare past the square-capped end (the band does not overshoot b).
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(wallController, wallCanvas, 0.92, 0.6).toPoint(), 5));
    }

    // Wall TYPE (M1.3): a window draws the band HOLLOW — outline stroked, interior
    // NOT filled — vs the solid band above. Same geometry, different render: the
    // type is a drawing concern only. (mutation-pin: drop the type branch and the
    // interior fills, failing the bare-interior assert.)
    {
        DrawingDocumentController winController;
        DrawingCanvasWidget winCanvas(&winController);
        winCanvas.resize(600, 450);
        winController.setSelectedToolId(QStringLiteral("wall_tool"));
        clickCanvas(winController, winCanvas, 0.2, 0.6); // a
        clickCanvas(winController, winCanvas, 0.8, 0.6); // b
        EDI_CHECK(winController.setSelectedWallType(QStringLiteral("window")));
        EDI_CHECK(winController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        winController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(winController, winCanvas, 0.02, 0.02); // deselect

        const QImage frame = winCanvas.grab().toImage();
        // The window IS drawn — its outline cap at the a end is inked...
        EDI_CHECK(anyReddishNear(frame, screenPointFor(winController, winCanvas, 0.2, 0.6).toPoint(), 3));
        // ...but the interior is BARE, where the solid band above was filled.
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(winController, winCanvas, 0.5, 0.6).toPoint(), 3));
    }

    // Wall corner MITER (M1.2): two walls sharing an endpoint join into one band.
    // An L from (0.3,0.5)->(0.6,0.5)->(0.6,0.8) turns at the shared corner; with
    // bare square caps the OUTER notch (top-right of the corner, past wall1's end
    // and before wall2's start) would be empty. The miter corner-fill covers it.
    {
        DrawingDocumentController joinController;
        DrawingCanvasWidget joinCanvas(&joinController);
        joinCanvas.resize(600, 450);
        joinController.setSelectedToolId(QStringLiteral("wall_tool"));
        clickCanvas(joinController, joinCanvas, 0.3, 0.5); // wall1 a
        clickCanvas(joinController, joinCanvas, 0.6, 0.5); // wall1 b == shared corner
        EDI_CHECK(joinController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        joinController.setSelectedToolId(QStringLiteral("wall_tool"));
        clickCanvas(joinController, joinCanvas, 0.6, 0.5); // wall2 a (snaps to the corner)
        clickCanvas(joinController, joinCanvas, 0.6, 0.8); // wall2 b
        EDI_CHECK(joinController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        joinController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(joinController, joinCanvas, 0.02, 0.02); // deselect

        const QImage frame = joinCanvas.grab().toImage();
        // The outer-notch pixel: past wall1's end (x>0.6) and above wall2's start
        // (y<0.5) — NOT covered by either square-capped band, only by the miter.
        EDI_CHECK(anyReddishNear(frame, screenPointFor(joinController, joinCanvas, 0.62, 0.47).toPoint(), 3));
        // The inner overlap is solid (both bands present).
        EDI_CHECK(anyReddishNear(frame, screenPointFor(joinController, joinCanvas, 0.58, 0.52).toPoint(), 3));
        // Beyond the miter apex (apex ~ (0.65,0.45)) the corner is bare — the
        // fill is a bounded wedge, not an unbounded overhang.
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(joinController, joinCanvas, 0.72, 0.40).toPoint(), 4));
    }

    // Trim: the verb shortens a line back to a crossing boundary. The painter
    // path is the ordinary line stroke, but this pins that it draws the MUTATED
    // (shorter) geometry — ink on the kept half, bare where the removed stub
    // used to be.
    {
        DrawingDocumentController trimController;
        DrawingCanvasWidget trimCanvas(&trimController);
        trimCanvas.resize(600, 450);

        // Target: horizontal line 0.2..0.8 at y=0.5, stroked red to find it
        // specifically. Boundary: a vertical line crossing at x=0.5 (default
        // colour, so it never trips the red probes).
        trimController.setSelectedToolId(QStringLiteral("line_tool"));
        clickCanvas(trimController, trimCanvas, 0.2, 0.5);
        clickCanvas(trimController, trimCanvas, 0.8, 0.5);
        EDI_CHECK(trimController.setSelectedObjectStrokeColor(QStringLiteral("#ff0000")));
        const QString targetId = trimController.selectedObjectId();
        clickCanvas(trimController, trimCanvas, 0.5, 0.2);
        clickCanvas(trimController, trimCanvas, 0.5, 0.8);

        // Re-select the target off the crossing, arm trim, click the right stub:
        // the b end pulls back to the cut at x=0.5.
        trimController.setSelectedToolId(QStringLiteral("select_move"));
        clickCanvas(trimController, trimCanvas, 0.3, 0.5);
        EDI_CHECK(trimController.selectedObjectId() == targetId);
        EDI_CHECK(trimController.beginTrimSelectedLine());
        clickCanvas(trimController, trimCanvas, 0.72, 0.5);
        EDI_CHECK(near(activeObject(trimController).value(QStringLiteral("x2")).toDouble(), 0.5));

        clickCanvas(trimController, trimCanvas, 0.02, 0.02); // deselect
        const QImage frame = trimCanvas.grab().toImage();
        // Kept half (x=0.35) still carries the red target...
        EDI_CHECK(anyReddishNear(frame, screenPointFor(trimController, trimCanvas, 0.35, 0.5).toPoint(), 3));
        // ...the removed stub (x=0.65) is bare of the target's red.
        EDI_CHECK(!anyReddishNear(frame, screenPointFor(trimController, trimCanvas, 0.65, 0.5).toPoint(), 3));
    }

    return 0;
}
