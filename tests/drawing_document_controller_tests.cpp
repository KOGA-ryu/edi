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
    QVariantMap initialGrid = initial.value("grid").toMap();
    assert(initialGrid.value("preset").toString() == "square_art_board");
    assert(initialGrid.value("unit_label").toString() == "in");
    assert(!initialGrid.value("lines").toList().empty());
    assert(!controller.gridSnapEnabled());
    assert(!controller.objectSnapEnabled());

    controller.setSelectedToolId("point_tool");
    controller.clickCanvasNormalized(0.25, 0.5);
    QVariantList objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 1);
    QVariantMap point = objects.front().toMap();
    assert(point.value("kind").toString() == "point");
    assert(point.value("x").toDouble() == 0.25);
    assert(point.value("y").toDouble() == 0.5);
    assert(point.value("layer_id").toString() == "default");
    assert(!point.value("locked").toBool());
    QVariantMap pointBounds = point.value("bounds").toMap();
    assert(pointBounds.value("x").toDouble() == 0.25);
    assert(pointBounds.value("y").toDouble() == 0.5);
    assert(pointBounds.value("width").toDouble() == 0.0);
    assert(pointBounds.value("height").toDouble() == 0.0);
    assert(controller.selectedObjectId() == point.value("id").toString());

    controller.setGridPresetId("letter");
    QVariantMap letterModel = controller.modelDocument();
    QVariantMap letterGrid = letterModel.value("grid").toMap();
    assert(controller.gridPresetId() == "letter");
    assert(letterGrid.value("preset").toString() == "letter");
    assert(letterGrid.value("unit_label").toString() == "in");
    assert(nearlyEqual(letterGrid.value("width").toDouble(), 8.5));
    assert(nearlyEqual(letterGrid.value("height").toDouble(), 11.0));
    QVariantMap letterSnap = letterModel.value("snap").toMap();
    assert(nearlyEqual(letterSnap.value("grid_step_x").toDouble(), 0.25 / 8.5));
    assert(nearlyEqual(letterSnap.value("grid_step_y").toDouble(), 0.25 / 11.0));

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
    QVariantMap lineBounds = line.value("bounds").toMap();
    assert(nearlyEqual(lineBounds.value("x").toDouble(), 0.1));
    assert(nearlyEqual(lineBounds.value("y").toDouble(), 0.2));
    assert(nearlyEqual(lineBounds.value("width").toDouble(), 0.7));
    assert(nearlyEqual(lineBounds.value("height").toDouble(), 0.7));

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

    DrawingDocumentController gridController;
    gridController.setGridSnapEnabled(true);
    assert(gridController.gridSnapEnabled());
    gridController.setSelectedToolId("point_tool");
    gridController.clickCanvasNormalized(0.14, 0.14);
    QVariantMap gridPoint = gridController.modelDocument().value("drawing_objects").toList().front().toMap();
    const double squareQuarterInchStep = 0.25 / 12.0;
    const double snappedSquarePoint = 7.0 * squareQuarterInchStep;
    assert(nearlyEqual(gridPoint.value("x").toDouble(), snappedSquarePoint));
    assert(nearlyEqual(gridPoint.value("y").toDouble(), snappedSquarePoint));

    DrawingDocumentController objectSnapController;
    objectSnapController.setSelectedToolId("point_tool");
    objectSnapController.clickCanvasNormalized(0.25, 0.25);
    objectSnapController.setObjectSnapEnabled(true);
    assert(objectSnapController.objectSnapEnabled());
    objectSnapController.clickCanvasNormalized(0.26, 0.24);
    QVariantList snappedObjects = objectSnapController.modelDocument().value("drawing_objects").toList();
    assert(snappedObjects.size() == 2);
    QVariantMap snappedPoint = snappedObjects.back().toMap();
    assert(nearlyEqual(snappedPoint.value("x").toDouble(), 0.25));
    assert(nearlyEqual(snappedPoint.value("y").toDouble(), 0.25));

    DrawingDocumentController selectionController;
    selectionController.setSelectedToolId("point_tool");
    selectionController.clickCanvasNormalized(0.1, 0.1);
    selectionController.clickCanvasNormalized(0.4, 0.4);
    selectionController.clickCanvasNormalized(0.8, 0.8);
    assert(selectionController.selectObjectsInBoundsNormalized(0.0, 0.0, 0.5, 0.5));
    QVariantMap selectionModel = selectionController.modelDocument();
    QVariantList selectedIds = selectionModel.value("selected_object_ids").toList();
    assert(selectedIds.size() == 2);
    assert(selectionModel.value("active_object_id").toString() == selectedIds.back().toString());

    DrawingDocumentController previewController;
    previewController.setSelectedToolId("line_tool");
    previewController.clickCanvasNormalized(0.2, 0.2);
    QVariantMap pendingModel = previewController.modelDocument();
    assert(pendingModel.value("drawing_objects").toList().empty());
    assert(!pendingModel.contains("preview_object"));

    previewController.updateCreationPreviewNormalized(0.6, 0.7);
    QVariantMap previewModel = previewController.modelDocument();
    assert(previewModel.value("drawing_objects").toList().empty());
    assert(previewModel.contains("preview_object"));
    QVariantMap previewLine = previewModel.value("preview_object").toMap();
    assert(previewLine.value("kind").toString() == "line");
    assert(nearlyEqual(previewLine.value("x1").toDouble(), 0.2));
    assert(nearlyEqual(previewLine.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(previewLine.value("x2").toDouble(), 0.6));
    assert(nearlyEqual(previewLine.value("y2").toDouble(), 0.7));

    previewController.clickCanvasNormalized(0.6, 0.7);
    QVariantMap committedPreviewModel = previewController.modelDocument();
    assert(committedPreviewModel.value("drawing_objects").toList().size() == 1);
    assert(!committedPreviewModel.contains("preview_object"));

    DrawingDocumentController cancelPreviewController;
    cancelPreviewController.setSelectedToolId("rectangle_tool");
    cancelPreviewController.clickCanvasNormalized(0.1, 0.1);
    cancelPreviewController.updateCreationPreviewNormalized(0.4, 0.4);
    assert(cancelPreviewController.modelDocument().contains("preview_object"));
    cancelPreviewController.setSelectedToolId("select_move");
    assert(!cancelPreviewController.modelDocument().contains("preview_object"));
    assert(cancelPreviewController.modelDocument().value("drawing_objects").toList().empty());

    DrawingDocumentController snappedPreviewController;
    snappedPreviewController.setGridSnapEnabled(true);
    snappedPreviewController.setSelectedToolId("rectangle_tool");
    snappedPreviewController.clickCanvasNormalized(0.14, 0.14);
    snappedPreviewController.updateCreationPreviewNormalized(0.36, 0.36);
    QVariantMap snappedPreview = snappedPreviewController.modelDocument().value("preview_object").toMap();
    assert(snappedPreview.value("kind").toString() == "rectangle");
    assert(nearlyEqual(snappedPreview.value("x").toDouble(), snappedSquarePoint));
    assert(nearlyEqual(snappedPreview.value("y").toDouble(), snappedSquarePoint));
    const double snappedSquareExtent = 10.0 * squareQuarterInchStep;
    assert(nearlyEqual(snappedPreview.value("width").toDouble(), snappedSquareExtent));
    assert(nearlyEqual(snappedPreview.value("height").toDouble(), snappedSquareExtent));
    assert(snappedPreviewController.modelDocument().value("drawing_objects").toList().empty());
    snappedPreviewController.clickCanvasNormalized(0.36, 0.36);
    QVariantMap snappedCommitted = snappedPreviewController.modelDocument();
    assert(snappedCommitted.value("drawing_objects").toList().size() == 1);
    assert(!snappedCommitted.contains("preview_object"));

    DrawingDocumentController zeroSizeController;
    zeroSizeController.setSelectedToolId("circle_tool");
    zeroSizeController.clickCanvasNormalized(0.5, 0.5);
    zeroSizeController.updateCreationPreviewNormalized(0.5, 0.5);
    QVariantMap zeroCirclePreview = zeroSizeController.modelDocument().value("preview_object").toMap();
    assert(zeroCirclePreview.value("kind").toString() == "circle");
    assert(nearlyEqual(zeroCirclePreview.value("radius").toDouble(), 0.0));
    zeroSizeController.clickCanvasNormalized(0.5, 0.5);
    QVariantMap zeroCircleCommitted = zeroSizeController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(zeroCircleCommitted.value("radius").toDouble(), 0.0));

    DrawingDocumentController selectionIsolationController;
    selectionIsolationController.setSelectedToolId("point_tool");
    selectionIsolationController.clickCanvasNormalized(0.2, 0.2);
    QString selectedBeforePreview = selectionIsolationController.selectedObjectId();
    selectionIsolationController.setSelectedToolId("line_tool");
    selectionIsolationController.clickCanvasNormalized(0.4, 0.4);
    selectionIsolationController.updateCreationPreviewNormalized(0.8, 0.8);
    assert(selectionIsolationController.modelDocument().contains("preview_object"));
    assert(selectionIsolationController.selectedObjectId() == selectedBeforePreview);
    selectionIsolationController.setSelectedToolId("select_move");
    selectionIsolationController.clickCanvasNormalized(0.8, 0.8);
    assert(selectionIsolationController.selectedObjectId().isEmpty());

    return 0;
}
