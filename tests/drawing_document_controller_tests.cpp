#include "core/DrawingCore.h"

#include <QCoreApplication>
#include <QVariantList>
#include <QVariantMap>

#include <cassert>
#include <cmath>
#include <limits>

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    DrawingDocumentController controller;
    QVariantMap initial = controller.modelDocument();
    assert(initial.value("engine").toString() == "cpp_drafting_document");
    assert(initial.value("drawing_objects").toList().empty());

    controller.setSelectedToolId("point_tool");
    controller.clickCanvasNormalized(0.25, 0.5);
    QVariantList objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 1);
    QVariantMap point = objects.front().toMap();
    assert(point.value("kind").toString() == "point");
    assert(point.value("x").toDouble() == 0.25);
    assert(point.value("y").toDouble() == 0.5);
    assert(controller.selectedObjectId() == point.value("id").toString());

    controller.setSelectedToolId("line_tool");
    controller.clickCanvasNormalized(0.1, 0.2);
    assert(controller.modelDocument().value("drawing_objects").toList().size() == 1);
    controller.clickCanvasNormalized(0.8, 0.9);
    objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 2);
    QVariantMap line = objects.back().toMap();
    assert(line.value("kind").toString() == "line");
    assert(line.value("x1").toDouble() == 0.1);
    assert(line.value("y1").toDouble() == 0.2);
    assert(line.value("x2").toDouble() == 0.8);
    assert(line.value("y2").toDouble() == 0.9);

    controller.setSelectedToolId("select_move");
    controller.clickCanvasNormalized(0.25, 0.5);
    assert(controller.selectedObjectId() == point.value("id").toString());

    controller.clickCanvasNormalized(0.8, 0.9);
    assert(controller.selectedObjectId() == line.value("id").toString());
    assert(controller.editSelectedHandleNormalized("line_end", 0.4, 0.6));
    objects = controller.modelDocument().value("drawing_objects").toList();
    QVariantMap editedLine;
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == line.value("id").toString()) {
            editedLine = object;
        }
    }
    assert(!editedLine.isEmpty());
    assert(editedLine.value("x2").toDouble() == 0.4);
    assert(editedLine.value("y2").toDouble() == 0.6);
    assert(!controller.editSelectedHandleNormalized("missing_handle", 0.1, 0.1));

    assert(controller.moveSelectionNormalized(0.1, -0.2));
    objects = controller.modelDocument().value("drawing_objects").toList();
    QVariantMap movedLine;
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == line.value("id").toString()) {
            movedLine = object;
        }
    }
    assert(!movedLine.isEmpty());
    assert(nearlyEqual(movedLine.value("x1").toDouble(), 0.2));
    assert(nearlyEqual(movedLine.value("y1").toDouble(), 0.0));
    assert(nearlyEqual(movedLine.value("x2").toDouble(), 0.5));
    assert(nearlyEqual(movedLine.value("y2").toDouble(), 0.4));
    assert(!controller.moveSelectionNormalized(std::numeric_limits<double>::infinity(), 0.0));

    return 0;
}
