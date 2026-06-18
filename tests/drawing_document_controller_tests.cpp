#include "core/DrawingCore.h"
#include "core/DrawingDocumentProjection.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingRoom.h"
#include "drafting/DraftingSerialize.h"

#include <QCoreApplication>
#include <QSet>
#include <QStringList>
#include <QFile>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <variant>

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

// Trailing numeric suffix of an "<prefix>_NNNN" object id.
int trailingSerial(const QString &id)
{
    int begin = id.size();
    while (begin > 0 && id.at(begin - 1).isDigit()) {
        --begin;
    }
    if (begin == id.size()) {
        return 0;
    }
    return id.mid(begin).toInt();
}

QStringList numericFieldIds(const QVariantMap &object)
{
    QStringList ids;
    for (const QVariant &field : object.value("numeric_fields").toList()) {
        ids.push_back(field.toMap().value("id").toString());
    }
    return ids;
}

QVariantMap numericField(const QVariantMap &object, const QString &id)
{
    for (const QVariant &field : object.value("numeric_fields").toList()) {
        const QVariantMap map = field.toMap();
        if (map.value("id").toString() == id) {
            return map;
        }
    }
    return {};
}

QVariantMap editStatus(const DrawingDocumentController &controller)
{
    return controller.modelDocument().value("edit_status").toMap();
}

QStringList editHandleIds(const QVariantMap &object)
{
    QStringList ids;
    for (const QVariant &handle : object.value("edit_handles").toList()) {
        ids.push_back(handle.toMap().value("id").toString());
    }
    return ids;
}

QVariantMap lastObjectOfKind(const QVariantMap &model, const QString &kind)
{
    QVariantMap result;
    for (const QVariant &objectValue : model.value("drawing_objects").toList()) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("kind").toString() == kind) {
            result = object;
        }
    }
    return result;
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
    QVariantMap initialSnap = initial.value("snap").toMap();
    assert(initialSnap.value("endpoint_enabled").toBool());
    assert(initialSnap.value("vertex_enabled").toBool());
    assert(initialSnap.value("midpoint_enabled").toBool());
    assert(initialSnap.value("center_enabled").toBool());
    assert(initialSnap.value("intersection_enabled").toBool());
    assert(initialSnap.value("guide_enabled").toBool());
    assert(initialSnap.value("guide_move_enabled").toBool());
    assert(initialSnap.value("object_priority_before_grid").toBool());
    assert(controller.objectSnapTolerancePresetId() == "normal");

    controller.setEndpointSnapEnabled(false);
    controller.setVertexSnapEnabled(false);
    controller.setMidpointSnapEnabled(false);
    controller.setCenterSnapEnabled(false);
    controller.setIntersectionSnapEnabled(false);
    controller.setGuideSnapEnabled(false);
    controller.setGuideMoveSnapEnabled(false);
    controller.setObjectSnapPriorityBeforeGrid(false);
    controller.setObjectSnapTolerancePreset("tight");
    QVariantMap changedSnap = controller.modelDocument().value("snap").toMap();
    assert(!controller.endpointSnapEnabled());
    assert(!controller.vertexSnapEnabled());
    assert(!controller.midpointSnapEnabled());
    assert(!controller.centerSnapEnabled());
    assert(!controller.intersectionSnapEnabled());
    assert(!controller.guideSnapEnabled());
    assert(!controller.guideMoveSnapEnabled());
    assert(!controller.objectSnapPriorityBeforeGrid());
    assert(controller.objectSnapTolerancePresetId() == "tight");
    assert(!changedSnap.value("endpoint_enabled").toBool());
    assert(!changedSnap.value("vertex_enabled").toBool());
    assert(!changedSnap.value("midpoint_enabled").toBool());
    assert(!changedSnap.value("center_enabled").toBool());
    assert(!changedSnap.value("intersection_enabled").toBool());
    assert(!changedSnap.value("guide_enabled").toBool());
    assert(!changedSnap.value("guide_move_enabled").toBool());
    assert(!changedSnap.value("object_priority_before_grid").toBool());
    assert(nearlyEqual(changedSnap.value("object_tolerance").toDouble(), 0.015));
    controller.setEndpointSnapEnabled(true);
    controller.setVertexSnapEnabled(true);
    controller.setMidpointSnapEnabled(true);
    controller.setCenterSnapEnabled(true);
    controller.setIntersectionSnapEnabled(true);
    controller.setGuideSnapEnabled(true);
    controller.setGuideMoveSnapEnabled(true);
    controller.setObjectSnapPriorityBeforeGrid(true);
    controller.setObjectSnapTolerancePreset("normal");

    controller.setSelectedToolId("point_tool");
    controller.clickCanvasNormalized(0.25, 0.5);
    QVariantList objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 1);
    QVariantMap point = objects.front().toMap();
    assert(point.value("kind").toString() == "point");
    assert(point.value("x").toDouble() == 0.25);
    assert(point.value("y").toDouble() == 0.5);
    assert(numericFieldIds(point) == QStringList({QStringLiteral("x"), QStringLiteral("y")}));
    QVariantMap pointXField = numericField(point, "x");
    assert(pointXField.value("physical_editable").toBool());
    assert(pointXField.value("physical_unit_kind").toString() == "length");
    assert(pointXField.value("physical_unit_label").toString() == "in");
    assert(nearlyEqual(pointXField.value("physical_minimum").toDouble(), -100000.0));
    assert(nearlyEqual(pointXField.value("physical_maximum").toDouble(), 100000.0));
    QVariantMap pointPhysical = point.value("physical_geometry").toMap();
    assert(pointPhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(pointPhysical.value("x").toDouble(), 3.0));
    assert(nearlyEqual(pointPhysical.value("y").toDouble(), 6.0));
    assert(point.value("layer_id").toString() == "default");
    assert(!point.value("locked").toBool());
    QVariantMap pointBounds = point.value("bounds").toMap();
    assert(pointBounds.value("x").toDouble() == 0.25);
    assert(pointBounds.value("y").toDouble() == 0.5);
    assert(pointBounds.value("width").toDouble() == 0.0);
    assert(pointBounds.value("height").toDouble() == 0.0);
    QVariantList pointMeasurement = point.value("measurement_lines").toList();
    assert(pointMeasurement.size() == 2);
    assert(pointMeasurement[0].toString() == "width: 0 canvas_unit");
    assert(pointMeasurement[1].toString() == "height: 0 canvas_unit");
    assert(controller.selectedObjectId() == point.value("id").toString());
    assert(controller.updateSelectedObjectGeometryField("x", 0.3));
    assert(controller.updateSelectedObjectGeometryField("y", 0.35));
    QVariantMap editedPoint = controller.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(editedPoint.value("x").toDouble(), 0.3));
    assert(nearlyEqual(editedPoint.value("y").toDouble(), 0.35));
    assert(!controller.updateSelectedObjectGeometryField("missing_field", 0.1));

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
    QVariantMap letterPoint = letterModel.value("drawing_objects").toList().front().toMap();
    QVariantMap letterPointPhysical = letterPoint.value("physical_geometry").toMap();
    assert(letterPointPhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(letterPointPhysical.value("x").toDouble(), 0.3 * 8.5));
    assert(nearlyEqual(letterPointPhysical.value("y").toDouble(), 0.35 * 11.0));
    DrawingDocumentController physicalPointController;
    physicalPointController.setSelectedToolId("point_tool");
    physicalPointController.clickCanvasNormalized(0.25, 0.5);
    physicalPointController.setGridPresetId("letter");
    assert(physicalPointController.updateSelectedObjectPhysicalGeometryField("x", 4.25));
    assert(physicalPointController.updateSelectedObjectPhysicalGeometryField("y", 5.5));
    QVariantMap physicalEditedPoint = physicalPointController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(physicalEditedPoint.value("x").toDouble(), 0.5));
    assert(nearlyEqual(physicalEditedPoint.value("y").toDouble(), 0.5));

    DrawingDocumentController customGridController;
    customGridController.setSelectedToolId("point_tool");
    customGridController.clickCanvasNormalized(0.5, 0.25);
    const int customGridRevision = customGridController.modelDocument().value("revision").toInt();
    customGridController.setGridUnitId("centimeter");
    customGridController.setGridSize(20.0, 10.0);
    customGridController.setGridMargins(1.0, 2.0, 3.0, 4.0);
    customGridController.setGridStep(0.5);
    customGridController.setGridMajorLineEvery(5);
    customGridController.setGridVisible(false);
    QVariantMap customGridModel = customGridController.modelDocument();
    QVariantMap customGrid = customGridModel.value("grid").toMap();
    assert(customGrid.value("preset").toString() == "custom");
    assert(customGrid.value("unit").toString() == "centimeter");
    assert(customGrid.value("unit_label").toString() == "cm");
    assert(nearlyEqual(customGrid.value("width").toDouble(), 20.0));
    assert(nearlyEqual(customGrid.value("height").toDouble(), 10.0));
    assert(nearlyEqual(customGrid.value("margin_left").toDouble(), 1.0));
    assert(nearlyEqual(customGrid.value("margin_top").toDouble(), 2.0));
    assert(nearlyEqual(customGrid.value("margin_right").toDouble(), 3.0));
    assert(nearlyEqual(customGrid.value("margin_bottom").toDouble(), 4.0));
    assert(nearlyEqual(customGrid.value("minor_step").toDouble(), 0.5));
    assert(customGrid.value("major_line_every").toInt() == 5);
    assert(!customGrid.value("visible").toBool());
    assert(customGrid.value("lines").toList().empty());
    QVariantMap customSnap = customGridModel.value("snap").toMap();
    assert(nearlyEqual(customSnap.value("grid_step_x").toDouble(), 0.5 / 20.0));
    assert(nearlyEqual(customSnap.value("grid_step_y").toDouble(), 0.5 / 10.0));
    QVariantMap customPoint = customGridModel.value("drawing_objects").toList().front().toMap();
    QVariantMap customPointPhysical = customPoint.value("physical_geometry").toMap();
    assert(nearlyEqual(customPointPhysical.value("x").toDouble(), 10.0));
    assert(nearlyEqual(customPointPhysical.value("y").toDouble(), 2.5));
    assert(customGridModel.value("revision").toInt() == customGridRevision);

    customGridController.setGridSize(-5.0, std::numeric_limits<double>::infinity());
    customGridController.setGridMargins(-1.0, -2.0, 1000.0, 1000.0);
    customGridController.setGridStep(-1.0);
    customGridController.setGridMajorLineEvery(-4);
    QVariantMap sanitizedGrid = customGridController.modelDocument().value("grid").toMap();
    assert(sanitizedGrid.value("width").toDouble() > 0.0);
    assert(sanitizedGrid.value("height").toDouble() > 0.0);
    assert(sanitizedGrid.value("margin_left").toDouble() >= 0.0);
    assert(sanitizedGrid.value("margin_top").toDouble() >= 0.0);
    assert(sanitizedGrid.value("margin_right").toDouble() >= 0.0);
    assert(sanitizedGrid.value("margin_bottom").toDouble() >= 0.0);
    assert(sanitizedGrid.value("minor_step").toDouble() > 0.0);
    assert(sanitizedGrid.value("major_line_every").toInt() == 1);
    assert(customGridController.modelDocument().value("revision").toInt() == customGridRevision);

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
    QVariantMap lineLengthField = numericField(line, "line_length");
    assert(lineLengthField.value("physical_editable").toBool());
    assert(lineLengthField.value("physical_unit_kind").toString() == "length");
    assert(lineLengthField.value("physical_unit_label").toString() == "in");
    assert(nearlyEqual(lineLengthField.value("physical_minimum").toDouble(), 0.0));
    QVariantMap lineAngleField = numericField(line, "line_angle_deg");
    assert(lineAngleField.value("physical_editable").toBool());
    assert(lineAngleField.value("physical_unit_kind").toString() == "angle");
    assert(lineAngleField.value("physical_unit_label").toString() == "deg");
    assert(nearlyEqual(lineAngleField.value("physical_step").toDouble(), 1.0));
    QVariantMap lineBounds = line.value("bounds").toMap();
    assert(nearlyEqual(lineBounds.value("x").toDouble(), 0.1));
    assert(nearlyEqual(lineBounds.value("y").toDouble(), 0.2));
    assert(nearlyEqual(lineBounds.value("width").toDouble(), 0.7));
    assert(nearlyEqual(lineBounds.value("height").toDouble(), 0.7));
    QVariantList lineMeasurement = line.value("measurement_lines").toList();
    assert(lineMeasurement.size() == 3);
    assert(lineMeasurement[0].toString().startsWith("distance: "));
    assert(lineMeasurement[1].toString().startsWith("width: "));
    assert(lineMeasurement[2].toString().startsWith("height: "));

    controller.setSelectedToolId("select_move");
    controller.clickCanvasNormalized(0.3, 0.35);
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
    assert(controller.updateSelectedObjectGeometryField("x2", 0.7));
    assert(controller.updateSelectedObjectGeometryField("y2", 0.8));
    objects = controller.modelDocument().value("drawing_objects").toList();
    QVariantMap numericallyEditedLine;
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == line.value("id").toString()) {
            numericallyEditedLine = object;
        }
    }
    assert(!numericallyEditedLine.isEmpty());
    assert(nearlyEqual(numericallyEditedLine.value("x2").toDouble(), 0.7));
    assert(nearlyEqual(numericallyEditedLine.value("y2").toDouble(), 0.8));
    QVariantMap numericLineBounds = numericallyEditedLine.value("bounds").toMap();
    assert(nearlyEqual(numericLineBounds.value("width").toDouble(), 0.6));
    assert(!controller.updateSelectedObjectGeometryField("missing_field", 0.1));
    assert(!controller.updateSelectedObjectGeometryField("x2", std::numeric_limits<double>::infinity()));

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
    assert(nearlyEqual(movedLine.value("x2").toDouble(), 0.8));
    assert(nearlyEqual(movedLine.value("y2").toDouble(), 0.6));

    assert(controller.offsetSelectedObject("left"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 3);
    QVariantMap offsetLine = objects.back().toMap();
    assert(offsetLine.value("kind").toString() == "line");
    assert(controller.selectedObjectId() == offsetLine.value("id").toString());
    assert(nearlyEqual(offsetLine.value("x1").toDouble(), 0.1646446609));
    assert(nearlyEqual(offsetLine.value("y1").toDouble(), 0.0353553391));
    assert(nearlyEqual(offsetLine.value("x2").toDouble(), 0.7646446609));
    assert(nearlyEqual(offsetLine.value("y2").toDouble(), 0.6353553391));

    assert(controller.mirrorSelectedObject("vertical"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 4);
    QVariantMap mirroredLine = objects.back().toMap();
    assert(mirroredLine.value("kind").toString() == "line");
    assert(controller.selectedObjectId() == mirroredLine.value("id").toString());
    assert(nearlyEqual(mirroredLine.value("x1").toDouble(), 0.7646446609));
    assert(nearlyEqual(mirroredLine.value("y1").toDouble(), 0.0353553391));
    assert(nearlyEqual(mirroredLine.value("x2").toDouble(), 0.1646446609));
    assert(nearlyEqual(mirroredLine.value("y2").toDouble(), 0.6353553391));

    assert(controller.repeatSelectedObject("x"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == 7);
    QVariantMap repeatedLine1 = objects[4].toMap();
    QVariantMap repeatedLine3 = objects[6].toMap();
    assert(repeatedLine1.value("kind").toString() == "line");
    assert(nearlyEqual(repeatedLine1.value("x1").toDouble(), 0.8646446609));
    assert(nearlyEqual(repeatedLine1.value("y1").toDouble(), 0.0353553391));
    assert(nearlyEqual(repeatedLine1.value("x2").toDouble(), 0.2646446609));
    assert(nearlyEqual(repeatedLine3.value("x1").toDouble(), 1.0646446609));
    assert(nearlyEqual(repeatedLine3.value("x2").toDouble(), 0.4646446609));
    QVariantList repeatedSelection = controller.modelDocument().value("selected_object_ids").toList();
    assert(repeatedSelection.size() == 3);
    assert(!controller.repeatSelectedObject("diagonal"));
    assert(controller.modelDocument().value("drawing_objects").toList().size() == 7);

    controller.clickCanvasNormalized(0.3, 0.35);
    const int beforeUnsupportedOffsetCount = controller.modelDocument().value("drawing_objects").toList().size();
    assert(!controller.offsetSelectedObject("right"));
    assert(controller.modelDocument().value("drawing_objects").toList().size() == beforeUnsupportedOffsetCount);
    assert(controller.mirrorSelectedObject("horizontal"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    assert(objects.size() == beforeUnsupportedOffsetCount + 1);
    QVariantMap mirroredPoint = objects.back().toMap();
    assert(mirroredPoint.value("kind").toString() == "point");
    assert(nearlyEqual(mirroredPoint.value("x").toDouble(), 0.3));
    assert(nearlyEqual(mirroredPoint.value("y").toDouble(), 0.35));
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

    DrawingDocumentController guideController;
    guideController.setSelectedToolId("horizontal_guide_tool");
    guideController.clickCanvasNormalized(0.2, 0.3);
    guideController.setSelectedToolId("vertical_guide_tool");
    guideController.clickCanvasNormalized(0.6, 0.7);
    QVariantList guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    assert(guideObjects.size() == 2);
    QVariantMap horizontalGuide = guideObjects[0].toMap();
    QVariantMap verticalGuide = guideObjects[1].toMap();
    assert(horizontalGuide.value("kind").toString() == "guide");
    assert(horizontalGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(horizontalGuide.value("position").toDouble(), 0.3));
    assert(!horizontalGuide.value("plot_ready").toBool());
    assert(horizontalGuide.value("guide_visual_controls").toBool());
    assert(horizontalGuide.value("guide_label").toString() == "H guide 0.300");
    assert(horizontalGuide.value("guide_custom_label").toString().isEmpty());
    assert(horizontalGuide.value("guide_color").toString() == "#83aeca");
    assert(horizontalGuide.value("guide_dash_style").toString() == "dash");
    assert(horizontalGuide.value("guide_show_label").toBool());
    assert(verticalGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 0.6));
    assert(verticalGuide.value("guide_drawable_controls").toBool());
    assert(guideController.setSelectedGuideLabel("material edge"));
    assert(guideController.setSelectedGuideColor("#54d2c6"));
    assert(guideController.setSelectedGuideDashStyle("solid"));
    assert(guideController.setSelectedGuideLabelVisible(false));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(verticalGuide.value("guide_label").toString() == "material edge");
    assert(verticalGuide.value("guide_custom_label").toString() == "material edge");
    assert(verticalGuide.value("guide_color").toString() == "#54d2c6");
    assert(verticalGuide.value("guide_dash_style").toString() == "solid");
    assert(!verticalGuide.value("guide_show_label").toBool());
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 0.6));
    const int guideVisualRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    assert(!guideController.setSelectedGuideColor("teal"));
    assert(!guideController.setSelectedGuideDashStyle("stripe"));
    assert(guideController.modelDocument().value("revision").toInt() == guideVisualRevisionBeforeInvalid);
    DrawingDocumentController nonGuideVisualController;
    nonGuideVisualController.setSelectedToolId("point_tool");
    nonGuideVisualController.clickCanvasNormalized(0.2, 0.3);
    assert(!nonGuideVisualController.setSelectedGuideLabel("not a guide"));
    QVariantMap verticalGuidePhysical = verticalGuide.value("physical_geometry").toMap();
    assert(verticalGuidePhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(verticalGuidePhysical.value("position").toDouble(), 7.2));
    assert(guideController.updateSelectedObjectPhysicalGeometryField("position", 3.0));
    QVariantMap guideEditStatus = editStatus(guideController);
    assert(guideEditStatus.value("ok").toBool());
    assert(guideEditStatus.value("mode").toString() == "physical");
    assert(guideEditStatus.value("field_id").toString() == "position");
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 0.25));
    verticalGuidePhysical = verticalGuide.value("physical_geometry").toMap();
    assert(nearlyEqual(verticalGuidePhysical.value("position").toDouble(), 3.0));
    const int guidePhysicalRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    assert(!guideController.updateSelectedObjectPhysicalGeometryField("position", 13.0));
    guideEditStatus = editStatus(guideController);
    assert(!guideEditStatus.value("ok").toBool());
    assert(guideEditStatus.value("mode").toString() == "physical");
    assert(guideEditStatus.value("field_id").toString() == "position");
    assert(guideEditStatus.value("code").toString() == "invalid_geometry");
    assert(guideEditStatus.value("message").toString() == "guide position must be normalized");
    assert(guideController.modelDocument().value("revision").toInt() == guidePhysicalRevisionBeforeInvalid);
    assert(guideController.moveSelectedGuideToDrawableOrigin());
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), squareQuarterInchStep));
    assert(guideController.centerSelectedGuideInDrawable());
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 0.5));
    assert(guideController.moveSelectedGuideToDrawableMax());
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep));
    assert(guideController.offsetSelectedGuide("negative", "fine"));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep - squareQuarterInchStep * 0.25));
    assert(guideController.offsetSelectedGuide("positive", "fine"));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep));
    assert(guideController.offsetSelectedGuide("negative", "coarse"));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep - squareQuarterInchStep * 4.0));
    const int guideOffsetRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    assert(!guideController.offsetSelectedGuide("sideways", "grid"));
    assert(guideController.modelDocument().value("revision").toInt() == guideOffsetRevisionBeforeInvalid);
    assert(guideController.updateSelectedObjectGeometryField("position", 0.8));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    assert(nearlyEqual(verticalGuide.value("position").toDouble(), 0.8));
    const int guideRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    assert(!guideController.updateSelectedObjectGeometryField("position", 1.2));
    assert(guideController.modelDocument().value("revision").toInt() == guideRevisionBeforeInvalid);

    DrawingDocumentController horizontalGuidePlacementController;
    horizontalGuidePlacementController.setSelectedToolId("horizontal_guide_tool");
    horizontalGuidePlacementController.clickCanvasNormalized(0.2, 0.3);
    assert(horizontalGuidePlacementController.moveSelectedGuideToDrawableOrigin());
    QVariantMap horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), squareQuarterInchStep));
    assert(horizontalGuidePlacementController.centerSelectedGuideInDrawable());
    horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), 0.5));
    assert(horizontalGuidePlacementController.moveSelectedGuideToDrawableMax());
    horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep));
    assert(horizontalGuidePlacementController.updateSelectedObjectPhysicalGeometryField("position", 6.0));
    horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), 0.5));

    DrawingDocumentController lockedGuidePlacementController;
    lockedGuidePlacementController.setSelectedToolId("horizontal_guide_tool");
    lockedGuidePlacementController.clickCanvasNormalized(0.2, 0.3);
    assert(lockedGuidePlacementController.setSelectedObjectLocked(true));
    const int lockedGuideRevision = lockedGuidePlacementController.modelDocument().value("revision").toInt();
    assert(!lockedGuidePlacementController.moveSelectedGuideToDrawableOrigin());
    assert(lockedGuidePlacementController.modelDocument().value("revision").toInt() == lockedGuideRevision);

    DrawingDocumentController layerLockedGuidePlacementController;
    layerLockedGuidePlacementController.setSelectedToolId("vertical_guide_tool");
    layerLockedGuidePlacementController.clickCanvasNormalized(0.6, 0.7);
    assert(layerLockedGuidePlacementController.setActiveLayerLocked(true));
    const int layerLockedGuideRevision = layerLockedGuidePlacementController.modelDocument().value("revision").toInt();
    assert(!layerLockedGuidePlacementController.centerSelectedGuideInDrawable());
    assert(layerLockedGuidePlacementController.modelDocument().value("revision").toInt() == layerLockedGuideRevision);

    DrawingDocumentController boundsGuideController;
    boundsGuideController.setSelectedToolId("rectangle_tool");
    boundsGuideController.clickCanvasNormalized(0.2, 0.3);
    boundsGuideController.clickCanvasNormalized(0.6, 0.7);
    QVariantMap boundsGuideModel = boundsGuideController.modelDocument();
    QVariantList boundsGuideObjects = boundsGuideModel.value("drawing_objects").toList();
    assert(boundsGuideObjects.size() == 1);
    const QString boundsGuideSourceId = boundsGuideObjects.front().toMap().value("id").toString();
    assert(boundsGuideObjects.front().toMap().value("bounds_guide_controls").toBool());
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    boundsGuideModel = boundsGuideController.modelDocument();
    boundsGuideObjects = boundsGuideModel.value("drawing_objects").toList();
    assert(boundsGuideObjects.size() == 2);
    assert(boundsGuideModel.value("guide_count").toInt() == 1);
    assert(boundsGuideModel.value("visible_guide_count").toInt() == 1);
    assert(boundsGuideModel.value("duplicate_guide_count").toInt() == 0);
    QVariantMap boundsGuide = boundsGuideObjects.back().toMap();
    assert(boundsGuide.value("kind").toString() == "guide");
    assert(boundsGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(boundsGuide.value("position").toDouble(), 0.2));
    assert(!boundsGuide.value("plot_ready").toBool());
    assert(boundsGuideController.selectedObjectId() == boundsGuideSourceId);
    const int duplicateBoundsGuideRevision = boundsGuideController.modelDocument().value("revision").toInt();
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    boundsGuideModel = boundsGuideController.modelDocument();
    assert(boundsGuideModel.value("revision").toInt() == duplicateBoundsGuideRevision);
    assert(boundsGuideModel.value("drawing_objects").toList().size() == 2);
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("right")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(boundsGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(boundsGuide.value("position").toDouble(), 0.6));
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("vertical_center")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(boundsGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(boundsGuide.value("position").toDouble(), 0.4));
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("top")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(boundsGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(boundsGuide.value("position").toDouble(), 0.3));
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("bottom")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(boundsGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(boundsGuide.value("position").toDouble(), 0.7));
    assert(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("horizontal_center")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(boundsGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(boundsGuide.value("position").toDouble(), 0.5));
    assert(boundsGuideController.modelDocument().value("drawing_objects").toList().size() == 7);
    const int boundsGuideRevisionBeforeInvalid = boundsGuideController.modelDocument().value("revision").toInt();
    assert(!boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("diagonal")));
    assert(boundsGuideController.modelDocument().value("revision").toInt() == boundsGuideRevisionBeforeInvalid);

    DrawingDocumentController guidePresetController;
    guidePresetController.setSelectedToolId("point_tool");
    guidePresetController.clickCanvasNormalized(0.4, 0.4);
    const QString guidePresetSelectedId = guidePresetController.selectedObjectId();
    assert(guidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    QVariantMap guidePresetModel = guidePresetController.modelDocument();
    QVariantList guidePresetObjects = guidePresetModel.value("drawing_objects").toList();
    assert(guidePresetObjects.size() == 5);
    assert(guidePresetModel.value("guide_count").toInt() == 4);
    assert(guidePresetModel.value("visible_guide_count").toInt() == 4);
    assert(guidePresetController.selectedObjectId() == guidePresetSelectedId);
    const int guidePresetDuplicateRevision = guidePresetModel.value("revision").toInt();
    assert(guidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    guidePresetModel = guidePresetController.modelDocument();
    assert(guidePresetModel.value("revision").toInt() == guidePresetDuplicateRevision);
    assert(guidePresetModel.value("guide_count").toInt() == 4);

    bool foundDrawableLeft = false;
    bool foundDrawableBottom = false;
    for (const QVariant &objectValue : guidePresetModel.value("drawing_objects").toList()) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("kind").toString() != "guide") {
            continue;
        }
        if (object.value("guide_label").toString() == "drawable left") {
            foundDrawableLeft = true;
            assert(object.value("orientation").toString() == "vertical");
            assert(nearlyEqual(object.value("position").toDouble(), squareQuarterInchStep));
            assert(object.value("guide_color").toString() == "#f6c65b");
        } else if (object.value("guide_label").toString() == "drawable bottom") {
            foundDrawableBottom = true;
            assert(object.value("orientation").toString() == "horizontal");
            assert(nearlyEqual(object.value("position").toDouble(), 1.0 - squareQuarterInchStep));
            assert(object.value("guide_color").toString() == "#f6c65b");
        }
    }
    assert(foundDrawableLeft);
    assert(foundDrawableBottom);
    assert(guidePresetController.applyGuidePreset(QStringLiteral("drawable_centerlines")));
    guidePresetModel = guidePresetController.modelDocument();
    assert(guidePresetModel.value("guide_count").toInt() == 6);
    assert(guidePresetController.applyGuidePreset(QStringLiteral("thirds")));
    guidePresetModel = guidePresetController.modelDocument();
    assert(guidePresetModel.value("guide_count").toInt() == 10);
    assert(guidePresetController.applyGuidePreset(QStringLiteral("quarters")));
    guidePresetModel = guidePresetController.modelDocument();
    assert(guidePresetModel.value("guide_count").toInt() == 14);
    assert(guidePresetController.applyGuidePreset(QStringLiteral("margin_safe")));
    guidePresetModel = guidePresetController.modelDocument();
    assert(guidePresetModel.value("guide_count").toInt() == 14);
    assert(!guidePresetController.applyGuidePreset(QStringLiteral("unknown_preset")));

    DrawingDocumentController hiddenGuidePresetController;
    assert(hiddenGuidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    assert(hiddenGuidePresetController.setAllGuidesVisible(false));
    QVariantMap hiddenGuidePresetModel = hiddenGuidePresetController.modelDocument();
    assert(hiddenGuidePresetModel.value("guide_count").toInt() == 4);
    assert(hiddenGuidePresetModel.value("visible_guide_count").toInt() == 0);

    DrawingDocumentController lockedLayerGuidePresetController;
    assert(lockedLayerGuidePresetController.setActiveLayerLocked(true));
    const int lockedLayerGuidePresetRevision = lockedLayerGuidePresetController.modelDocument().value("revision").toInt();
    assert(!lockedLayerGuidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    assert(lockedLayerGuidePresetController.modelDocument().value("revision").toInt() == lockedLayerGuidePresetRevision);
    assert(lockedLayerGuidePresetController.modelDocument().value("guide_count").toInt() == 0);

    DrawingDocumentController offsetGuideController;
    offsetGuideController.setSelectedToolId("rectangle_tool");
    offsetGuideController.clickCanvasNormalized(0.2, 0.3);
    offsetGuideController.clickCanvasNormalized(0.6, 0.7);
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    QVariantMap offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(offsetGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(offsetGuide.value("position").toDouble(), 0.2 - squareQuarterInchStep));
    const int duplicateOffsetGuideRevision = offsetGuideController.modelDocument().value("revision").toInt();
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    assert(offsetGuideController.modelDocument().value("revision").toInt() == duplicateOffsetGuideRevision);
    assert(offsetGuideController.modelDocument().value("drawing_objects").toList().size() == 2);
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("right"), QStringLiteral("grid")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(offsetGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(offsetGuide.value("position").toDouble(), 0.6 + squareQuarterInchStep));
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("top"), QStringLiteral("grid")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(offsetGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(offsetGuide.value("position").toDouble(), 0.3 - squareQuarterInchStep));
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("bottom"), QStringLiteral("grid")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(offsetGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(offsetGuide.value("position").toDouble(), 0.7 + squareQuarterInchStep));
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("center_x_plus"), QStringLiteral("fine")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(offsetGuide.value("orientation").toString() == "vertical");
    assert(nearlyEqual(offsetGuide.value("position").toDouble(), 0.4 + squareQuarterInchStep * 0.25));
    assert(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("center_y_minus"), QStringLiteral("coarse")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(offsetGuide.value("orientation").toString() == "horizontal");
    assert(nearlyEqual(offsetGuide.value("position").toDouble(), 0.5 - squareQuarterInchStep * 4.0));
    const int offsetGuideRevisionBeforeInvalid = offsetGuideController.modelDocument().value("revision").toInt();
    assert(!offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("diagonal"), QStringLiteral("grid")));
    assert(offsetGuideController.modelDocument().value("revision").toInt() == offsetGuideRevisionBeforeInvalid);

    DrawingDocumentController lockedBoundsGuideController;
    lockedBoundsGuideController.setSelectedToolId("line_tool");
    lockedBoundsGuideController.clickCanvasNormalized(0.2, 0.3);
    lockedBoundsGuideController.clickCanvasNormalized(0.6, 0.7);
    assert(lockedBoundsGuideController.setSelectedObjectLocked(true));
    const int lockedBoundsGuideRevision = lockedBoundsGuideController.modelDocument().value("revision").toInt();
    assert(!lockedBoundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    assert(!lockedBoundsGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    assert(lockedBoundsGuideController.modelDocument().value("revision").toInt() == lockedBoundsGuideRevision);
    assert(lockedBoundsGuideController.modelDocument().value("drawing_objects").toList().size() == 1);

    DrawingDocumentController layerLockedBoundsGuideController;
    layerLockedBoundsGuideController.setSelectedToolId("circle_tool");
    layerLockedBoundsGuideController.clickCanvasNormalized(0.5, 0.5);
    layerLockedBoundsGuideController.clickCanvasNormalized(0.6, 0.5);
    assert(layerLockedBoundsGuideController.setActiveLayerLocked(true));
    const int layerLockedBoundsGuideRevision = layerLockedBoundsGuideController.modelDocument().value("revision").toInt();
    assert(!layerLockedBoundsGuideController.createGuideFromSelectedBounds(QStringLiteral("top")));
    assert(!layerLockedBoundsGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("top"), QStringLiteral("grid")));
    assert(layerLockedBoundsGuideController.modelDocument().value("revision").toInt() == layerLockedBoundsGuideRevision);
    assert(layerLockedBoundsGuideController.modelDocument().value("drawing_objects").toList().size() == 1);

    DrawingDocumentController unsupportedBoundsGuideController;
    unsupportedBoundsGuideController.setSelectedToolId("horizontal_guide_tool");
    unsupportedBoundsGuideController.clickCanvasNormalized(0.2, 0.3);
    QVariantMap unsupportedGuide = unsupportedBoundsGuideController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(!unsupportedGuide.value("bounds_guide_controls").toBool());
    const int unsupportedBoundsGuideRevision = unsupportedBoundsGuideController.modelDocument().value("revision").toInt();
    assert(!unsupportedBoundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    assert(!unsupportedBoundsGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    assert(unsupportedBoundsGuideController.modelDocument().value("revision").toInt() == unsupportedBoundsGuideRevision);

    DrawingDocumentController directDuplicateGuideController;
    directDuplicateGuideController.setSelectedToolId("horizontal_guide_tool");
    directDuplicateGuideController.clickCanvasNormalized(0.2, 0.3);
    const QString directGuideId = directDuplicateGuideController.selectedObjectId();
    directDuplicateGuideController.clickCanvasNormalized(0.8, 0.3);
    QVariantMap directDuplicateGuideModel = directDuplicateGuideController.modelDocument();
    assert(directDuplicateGuideModel.value("drawing_objects").toList().size() == 1);
    assert(directDuplicateGuideModel.value("guide_count").toInt() == 1);
    assert(directDuplicateGuideModel.value("duplicate_guide_count").toInt() == 0);
    assert(directDuplicateGuideController.selectedObjectId() == directGuideId);

    DrawingDocumentController mergeDuplicateGuideController;
    mergeDuplicateGuideController.setSelectedToolId("horizontal_guide_tool");
    mergeDuplicateGuideController.clickCanvasNormalized(0.2, 0.3);
    mergeDuplicateGuideController.clickCanvasNormalized(0.2, 0.4);
    assert(mergeDuplicateGuideController.updateSelectedObjectGeometryField(QStringLiteral("position"), 0.3));
    assert(mergeDuplicateGuideController.setSelectedObjectLocked(true));
    QVariantMap duplicateGuideModel = mergeDuplicateGuideController.modelDocument();
    assert(duplicateGuideModel.value("drawing_objects").toList().size() == 2);
    assert(duplicateGuideModel.value("guide_count").toInt() == 2);
    assert(duplicateGuideModel.value("duplicate_guide_count").toInt() == 1);
    assert(mergeDuplicateGuideController.mergeDuplicateGuides());
    duplicateGuideModel = mergeDuplicateGuideController.modelDocument();
    QVariantList mergedGuides = duplicateGuideModel.value("drawing_objects").toList();
    assert(mergedGuides.size() == 1);
    assert(duplicateGuideModel.value("guide_count").toInt() == 1);
    assert(duplicateGuideModel.value("duplicate_guide_count").toInt() == 0);
    assert(mergedGuides.front().toMap().value("kind").toString() == "guide");
    assert(!mergedGuides.front().toMap().value("locked").toBool());

    DrawingDocumentController guideAlignLeftController;
    guideAlignLeftController.setSelectedToolId("vertical_guide_tool");
    guideAlignLeftController.clickCanvasNormalized(0.1, 0.2);
    guideAlignLeftController.setSelectedToolId("rectangle_tool");
    guideAlignLeftController.clickCanvasNormalized(0.3, 0.3);
    guideAlignLeftController.clickCanvasNormalized(0.5, 0.5);
    QVariantMap guideAlignRect = guideAlignLeftController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(guideAlignRect.value("align_to_guide_controls").toBool());
    assert(guideAlignLeftController.alignSelectionToNearestGuide(QStringLiteral("left")));
    guideAlignRect = guideAlignLeftController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideAlignRect.value("x").toDouble(), 0.1));

    DrawingDocumentController guideAlignCenterXController;
    guideAlignCenterXController.setSelectedToolId("vertical_guide_tool");
    guideAlignCenterXController.clickCanvasNormalized(0.6, 0.2);
    guideAlignCenterXController.setSelectedToolId("rectangle_tool");
    guideAlignCenterXController.clickCanvasNormalized(0.3, 0.3);
    guideAlignCenterXController.clickCanvasNormalized(0.5, 0.5);
    assert(guideAlignCenterXController.alignSelectionToNearestGuide(QStringLiteral("center_x")));
    guideAlignRect = guideAlignCenterXController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideAlignRect.value("x").toDouble(), 0.5));

    DrawingDocumentController guideAlignRightController;
    guideAlignRightController.setSelectedToolId("vertical_guide_tool");
    guideAlignRightController.clickCanvasNormalized(0.8, 0.2);
    guideAlignRightController.setSelectedToolId("rectangle_tool");
    guideAlignRightController.clickCanvasNormalized(0.3, 0.3);
    guideAlignRightController.clickCanvasNormalized(0.5, 0.5);
    assert(guideAlignRightController.alignSelectionToNearestGuide(QStringLiteral("right")));
    guideAlignRect = guideAlignRightController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideAlignRect.value("x").toDouble(), 0.6));

    DrawingDocumentController guideAlignTopController;
    guideAlignTopController.setSelectedToolId("horizontal_guide_tool");
    guideAlignTopController.clickCanvasNormalized(0.2, 0.1);
    guideAlignTopController.setSelectedToolId("rectangle_tool");
    guideAlignTopController.clickCanvasNormalized(0.3, 0.3);
    guideAlignTopController.clickCanvasNormalized(0.5, 0.5);
    assert(guideAlignTopController.alignSelectionToNearestGuide(QStringLiteral("top")));
    guideAlignRect = guideAlignTopController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideAlignRect.value("y").toDouble(), 0.1));

    DrawingDocumentController guideAlignCenterYController;
    guideAlignCenterYController.setSelectedToolId("horizontal_guide_tool");
    guideAlignCenterYController.clickCanvasNormalized(0.2, 0.6);
    guideAlignCenterYController.setSelectedToolId("rectangle_tool");
    guideAlignCenterYController.clickCanvasNormalized(0.3, 0.3);
    guideAlignCenterYController.clickCanvasNormalized(0.5, 0.5);
    assert(guideAlignCenterYController.alignSelectionToNearestGuide(QStringLiteral("center_y")));
    guideAlignRect = guideAlignCenterYController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideAlignRect.value("y").toDouble(), 0.5));

    DrawingDocumentController guideAlignBottomController;
    guideAlignBottomController.setSelectedToolId("horizontal_guide_tool");
    guideAlignBottomController.clickCanvasNormalized(0.2, 0.8);
    guideAlignBottomController.setSelectedToolId("rectangle_tool");
    guideAlignBottomController.clickCanvasNormalized(0.3, 0.3);
    guideAlignBottomController.clickCanvasNormalized(0.5, 0.5);
    assert(guideAlignBottomController.alignSelectionToNearestGuide(QStringLiteral("bottom")));
    guideAlignRect = guideAlignBottomController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideAlignRect.value("y").toDouble(), 0.6));

    DrawingDocumentController guideAlignNoGuideController;
    guideAlignNoGuideController.setSelectedToolId("rectangle_tool");
    guideAlignNoGuideController.clickCanvasNormalized(0.3, 0.3);
    guideAlignNoGuideController.clickCanvasNormalized(0.5, 0.5);
    const int guideAlignNoGuideRevision = guideAlignNoGuideController.modelDocument().value("revision").toInt();
    assert(!guideAlignNoGuideController.alignSelectionToNearestGuide(QStringLiteral("left")));
    assert(guideAlignNoGuideController.modelDocument().value("revision").toInt() == guideAlignNoGuideRevision);

    DrawingDocumentController guideAlignHiddenGuideController;
    guideAlignHiddenGuideController.setSelectedToolId("vertical_guide_tool");
    guideAlignHiddenGuideController.clickCanvasNormalized(0.1, 0.2);
    assert(guideAlignHiddenGuideController.setAllGuidesVisible(false));
    guideAlignHiddenGuideController.setSelectedToolId("rectangle_tool");
    guideAlignHiddenGuideController.clickCanvasNormalized(0.3, 0.3);
    guideAlignHiddenGuideController.clickCanvasNormalized(0.5, 0.5);
    const int guideAlignHiddenGuideRevision = guideAlignHiddenGuideController.modelDocument().value("revision").toInt();
    assert(!guideAlignHiddenGuideController.alignSelectionToNearestGuide(QStringLiteral("left")));
    assert(guideAlignHiddenGuideController.modelDocument().value("revision").toInt() == guideAlignHiddenGuideRevision);

    DrawingDocumentController lockedGuideAlignController;
    lockedGuideAlignController.setSelectedToolId("vertical_guide_tool");
    lockedGuideAlignController.clickCanvasNormalized(0.1, 0.2);
    lockedGuideAlignController.setSelectedToolId("rectangle_tool");
    lockedGuideAlignController.clickCanvasNormalized(0.3, 0.3);
    lockedGuideAlignController.clickCanvasNormalized(0.5, 0.5);
    assert(lockedGuideAlignController.setSelectedObjectLocked(true));
    const int lockedGuideAlignRevision = lockedGuideAlignController.modelDocument().value("revision").toInt();
    assert(!lockedGuideAlignController.alignSelectionToNearestGuide(QStringLiteral("left")));
    assert(lockedGuideAlignController.modelDocument().value("revision").toInt() == lockedGuideAlignRevision);

    DrawingDocumentController unsupportedGuideAlignController;
    unsupportedGuideAlignController.setSelectedToolId("vertical_guide_tool");
    unsupportedGuideAlignController.clickCanvasNormalized(0.1, 0.2);
    const int unsupportedGuideAlignRevision = unsupportedGuideAlignController.modelDocument().value("revision").toInt();
    assert(!unsupportedGuideAlignController.alignSelectionToNearestGuide(QStringLiteral("left")));
    assert(unsupportedGuideAlignController.modelDocument().value("revision").toInt() == unsupportedGuideAlignRevision);

    DrawingDocumentController deleteSelectedGuideController;
    deleteSelectedGuideController.setSelectedToolId("point_tool");
    deleteSelectedGuideController.clickCanvasNormalized(0.1, 0.1);
    deleteSelectedGuideController.setSelectedToolId("horizontal_guide_tool");
    deleteSelectedGuideController.clickCanvasNormalized(0.2, 0.3);
    assert(deleteSelectedGuideController.deleteSelectedGuide());
    QVariantList deleteSelectedGuideObjects = deleteSelectedGuideController.modelDocument().value("drawing_objects").toList();
    assert(deleteSelectedGuideObjects.size() == 1);
    assert(deleteSelectedGuideObjects.front().toMap().value("kind").toString() == "point");

    DrawingDocumentController lockedDeleteSelectedGuideController;
    lockedDeleteSelectedGuideController.setSelectedToolId("vertical_guide_tool");
    lockedDeleteSelectedGuideController.clickCanvasNormalized(0.6, 0.7);
    assert(lockedDeleteSelectedGuideController.setSelectedObjectLocked(true));
    const int lockedDeleteSelectedGuideRevision = lockedDeleteSelectedGuideController.modelDocument().value("revision").toInt();
    assert(!lockedDeleteSelectedGuideController.deleteSelectedGuide());
    assert(lockedDeleteSelectedGuideController.modelDocument().value("revision").toInt() == lockedDeleteSelectedGuideRevision);
    assert(lockedDeleteSelectedGuideController.modelDocument().value("drawing_objects").toList().size() == 1);

    DrawingDocumentController deleteAllGuidesController;
    deleteAllGuidesController.setSelectedToolId("point_tool");
    deleteAllGuidesController.clickCanvasNormalized(0.1, 0.1);
    deleteAllGuidesController.setSelectedToolId("horizontal_guide_tool");
    deleteAllGuidesController.clickCanvasNormalized(0.2, 0.3);
    deleteAllGuidesController.setSelectedToolId("vertical_guide_tool");
    deleteAllGuidesController.clickCanvasNormalized(0.6, 0.7);
    assert(deleteAllGuidesController.setAllGuidesLocked(true));
    assert(deleteAllGuidesController.deleteAllGuides());
    QVariantList deleteAllGuideObjects = deleteAllGuidesController.modelDocument().value("drawing_objects").toList();
    assert(deleteAllGuideObjects.size() == 1);
    assert(deleteAllGuideObjects.front().toMap().value("kind").toString() == "point");

    DrawingDocumentController guideLifecycleController;
    guideLifecycleController.setSelectedToolId("horizontal_guide_tool");
    guideLifecycleController.clickCanvasNormalized(0.2, 0.75);
    guideLifecycleController.setSelectedToolId("vertical_guide_tool");
    guideLifecycleController.clickCanvasNormalized(0.33, 0.2);
    guideLifecycleController.setObjectSnapEnabled(true);
    guideLifecycleController.updatePointerNormalized(0.34, 0.74);
    QVariantMap guideLifecyclePointer = guideLifecycleController.modelDocument().value("pointer").toMap();
    assert(guideLifecyclePointer.value("source").toString() == "guide");
    assert(guideLifecycleController.setAllGuidesVisible(false));
    QVariantList hiddenGuideObjects = guideLifecycleController.modelDocument().value("drawing_objects").toList();
    assert(!hiddenGuideObjects[0].toMap().value("visible").toBool());
    assert(!hiddenGuideObjects[1].toMap().value("visible").toBool());
    guideLifecycleController.updatePointerNormalized(0.34, 0.74);
    guideLifecyclePointer = guideLifecycleController.modelDocument().value("pointer").toMap();
    assert(guideLifecyclePointer.value("kind").toString() == "none");
    assert(guideLifecycleController.setAllGuidesVisible(true));
    assert(guideLifecycleController.setAllGuidesLocked(true));
    QVariantList lockedGuideObjects = guideLifecycleController.modelDocument().value("drawing_objects").toList();
    assert(lockedGuideObjects[0].toMap().value("locked").toBool());
    assert(lockedGuideObjects[1].toMap().value("locked").toBool());
    const int lockedGuideMoveRevision = guideLifecycleController.modelDocument().value("revision").toInt();
    assert(!guideLifecycleController.moveSelectedGuideToDrawableOrigin());
    assert(guideLifecycleController.modelDocument().value("revision").toInt() == lockedGuideMoveRevision);
    guideLifecycleController.updatePointerNormalized(0.34, 0.74);
    guideLifecyclePointer = guideLifecycleController.modelDocument().value("pointer").toMap();
    assert(guideLifecyclePointer.value("source").toString() == "guide");
    assert(guideLifecycleController.setAllGuidesLocked(false));
    assert(guideLifecycleController.moveSelectedGuideToDrawableOrigin());

    DrawingDocumentController constructionController;
    constructionController.setSelectedToolId("horizontal_construction_line_tool");
    constructionController.clickCanvasNormalized(0.2, 0.3);
    constructionController.setSelectedToolId("vertical_construction_line_tool");
    constructionController.clickCanvasNormalized(0.6, 0.7);
    QVariantList constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    assert(constructionObjects.size() == 2);
    QVariantMap horizontalConstruction = constructionObjects[0].toMap();
    QVariantMap verticalConstruction = constructionObjects[1].toMap();
    assert(horizontalConstruction.value("kind").toString() == "construction_line");
    assert(nearlyEqual(horizontalConstruction.value("x1").toDouble(), 0.0));
    assert(nearlyEqual(horizontalConstruction.value("y1").toDouble(), 0.3));
    assert(nearlyEqual(horizontalConstruction.value("x2").toDouble(), 1.0));
    assert(nearlyEqual(horizontalConstruction.value("y2").toDouble(), 0.3));
    assert(!horizontalConstruction.value("plot_ready").toBool());
    assert(nearlyEqual(verticalConstruction.value("x1").toDouble(), 0.6));
    assert(nearlyEqual(verticalConstruction.value("y1").toDouble(), 0.0));
    assert(nearlyEqual(verticalConstruction.value("x2").toDouble(), 0.6));
    assert(nearlyEqual(verticalConstruction.value("y2").toDouble(), 1.0));
    assert(verticalConstruction.value("construction_drawable_controls").toBool());
    assert(constructionController.selectedObjectId() == verticalConstruction.value("id").toString());
    assert(constructionController.offsetSelectedObject("left"));
    constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    assert(constructionObjects.size() == 3);
    QVariantMap offsetConstruction = constructionObjects.back().toMap();
    assert(offsetConstruction.value("kind").toString() == "construction_line");
    assert(nearlyEqual(offsetConstruction.value("x1").toDouble(), 0.55));
    assert(nearlyEqual(offsetConstruction.value("y1").toDouble(), 0.0));
    assert(nearlyEqual(offsetConstruction.value("x2").toDouble(), 0.55));
    assert(nearlyEqual(offsetConstruction.value("y2").toDouble(), 1.0));
    assert(constructionController.mirrorSelectedObject("horizontal"));
    constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    assert(constructionObjects.size() == 4);
    QVariantMap mirroredConstruction = constructionObjects.back().toMap();
    assert(mirroredConstruction.value("kind").toString() == "construction_line");
    assert(nearlyEqual(mirroredConstruction.value("x1").toDouble(), 0.55));
    assert(nearlyEqual(mirroredConstruction.value("y1").toDouble(), 1.0));
    assert(nearlyEqual(mirroredConstruction.value("x2").toDouble(), 0.55));
    assert(nearlyEqual(mirroredConstruction.value("y2").toDouble(), 0.0));

    assert(constructionController.repeatSelectedObject("y"));
    constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    assert(constructionObjects.size() == 7);
    QVariantMap repeatedConstruction = constructionObjects.back().toMap();
    assert(repeatedConstruction.value("kind").toString() == "construction_line");
    assert(nearlyEqual(repeatedConstruction.value("x1").toDouble(), 0.55));
    assert(nearlyEqual(repeatedConstruction.value("y1").toDouble(), 1.3));
    assert(nearlyEqual(repeatedConstruction.value("x2").toDouble(), 0.55));
    assert(nearlyEqual(repeatedConstruction.value("y2").toDouble(), 0.3));

    DrawingDocumentController verticalConstructionPlacementController;
    verticalConstructionPlacementController.setSelectedToolId("vertical_construction_line_tool");
    verticalConstructionPlacementController.clickCanvasNormalized(0.6, 0.7);
    assert(verticalConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    QVariantMap verticalFittedConstruction = verticalConstructionPlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(verticalFittedConstruction.value("x1").toDouble(), 0.6));
    assert(nearlyEqual(verticalFittedConstruction.value("y1").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(verticalFittedConstruction.value("x2").toDouble(), 0.6));
    assert(nearlyEqual(verticalFittedConstruction.value("y2").toDouble(), 1.0 - squareQuarterInchStep));

    DrawingDocumentController horizontalConstructionPlacementController;
    horizontalConstructionPlacementController.setSelectedToolId("horizontal_construction_line_tool");
    horizontalConstructionPlacementController.clickCanvasNormalized(0.2, 0.3);
    assert(horizontalConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    QVariantMap horizontalFittedConstruction = horizontalConstructionPlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(horizontalFittedConstruction.value("x1").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(horizontalFittedConstruction.value("y1").toDouble(), 0.3));
    assert(nearlyEqual(horizontalFittedConstruction.value("x2").toDouble(), 1.0 - squareQuarterInchStep));
    assert(nearlyEqual(horizontalFittedConstruction.value("y2").toDouble(), 0.3));

    DrawingDocumentController lockedConstructionPlacementController;
    lockedConstructionPlacementController.setSelectedToolId("vertical_construction_line_tool");
    lockedConstructionPlacementController.clickCanvasNormalized(0.6, 0.7);
    assert(lockedConstructionPlacementController.setSelectedObjectLocked(true));
    const int lockedConstructionRevision = lockedConstructionPlacementController.modelDocument().value("revision").toInt();
    assert(!lockedConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    assert(lockedConstructionPlacementController.modelDocument().value("revision").toInt() == lockedConstructionRevision);

    DrawingDocumentController layerLockedConstructionPlacementController;
    layerLockedConstructionPlacementController.setSelectedToolId("horizontal_construction_line_tool");
    layerLockedConstructionPlacementController.clickCanvasNormalized(0.2, 0.3);
    assert(layerLockedConstructionPlacementController.setActiveLayerLocked(true));
    const int layerLockedConstructionRevision = layerLockedConstructionPlacementController.modelDocument().value("revision").toInt();
    assert(!layerLockedConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    assert(layerLockedConstructionPlacementController.modelDocument().value("revision").toInt() == layerLockedConstructionRevision);

    DrawingDocumentController angledConstructionController;
    angledConstructionController.setSelectedToolId("angled_construction_line_tool");
    angledConstructionController.clickCanvasNormalized(0.1, 0.2);
    angledConstructionController.updateCreationPreviewNormalized(0.7, 0.4);
    QVariantMap angledPreview = angledConstructionController.modelDocument().value("preview_object").toMap();
    assert(!angledPreview.isEmpty());
    assert(angledPreview.value("kind").toString() == "construction_line");
    assert(!angledPreview.value("plot_ready").toBool());
    assert(nearlyEqual(angledPreview.value("x1").toDouble(), 0.1));
    assert(nearlyEqual(angledPreview.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(angledPreview.value("x2").toDouble(), 0.7));
    assert(nearlyEqual(angledPreview.value("y2").toDouble(), 0.4));
    assert(angledConstructionController.modelDocument().value("drawing_objects").toList().isEmpty());
    angledConstructionController.clickCanvasNormalized(0.7, 0.4);
    QVariantMap angledModel = angledConstructionController.modelDocument();
    assert(!angledModel.contains("preview_object"));
    QVariantList angledObjects = angledModel.value("drawing_objects").toList();
    assert(angledObjects.size() == 1);
    QVariantMap angledConstruction = angledObjects.front().toMap();
    assert(angledConstruction.value("kind").toString() == "construction_line");
    assert(nearlyEqual(angledConstruction.value("x1").toDouble(), 0.1));
    assert(nearlyEqual(angledConstruction.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(angledConstruction.value("x2").toDouble(), 0.7));
    assert(nearlyEqual(angledConstruction.value("y2").toDouble(), 0.4));
    assert(!angledConstruction.value("construction_drawable_controls").toBool());
    const int angledConstructionFitRevision = angledConstructionController.modelDocument().value("revision").toInt();
    assert(!angledConstructionController.fitSelectedConstructionLineToDrawable());
    assert(angledConstructionController.modelDocument().value("revision").toInt() == angledConstructionFitRevision);
    assert(angledConstructionController.updateSelectedObjectGeometryField("x2", 0.8));
    assert(angledConstructionController.updateSelectedObjectGeometryField("y2", 0.5));
    angledConstruction = angledConstructionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(angledConstruction.value("x2").toDouble(), 0.8));
    assert(nearlyEqual(angledConstruction.value("y2").toDouble(), 0.5));
    assert(angledConstructionController.updateSelectedObjectGeometryField("x2", 0.1));
    const int constructionRevisionBeforeInvalid = angledConstructionController.modelDocument().value("revision").toInt();
    assert(!angledConstructionController.updateSelectedObjectGeometryField("y2", 0.2));
    assert(angledConstructionController.modelDocument().value("revision").toInt() == constructionRevisionBeforeInvalid);

    DrawingDocumentController dimensionController;
    dimensionController.setSelectedToolId("distance_dimension_tool");
    dimensionController.clickCanvasNormalized(0.1, 0.2);
    dimensionController.updateCreationPreviewNormalized(0.4, 0.6);
    QVariantMap dimensionPreview = dimensionController.modelDocument().value("preview_object").toMap();
    assert(!dimensionPreview.isEmpty());
    assert(dimensionPreview.value("kind").toString() == "dimension");
    assert(!dimensionPreview.value("plot_ready").toBool());
    assert(dimensionPreview.value("dimension_kind").toString() == "distance");
    assert(dimensionPreview.value("dimension_show_label").toBool());
    assert(nearlyEqual(dimensionPreview.value("x1").toDouble(), 0.1));
    assert(nearlyEqual(dimensionPreview.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(dimensionPreview.value("x2").toDouble(), 0.4));
    assert(nearlyEqual(dimensionPreview.value("y2").toDouble(), 0.6));
    assert(dimensionPreview.value("label").toString() == "0.5 canvas_unit");
    assert(dimensionController.modelDocument().value("drawing_objects").toList().isEmpty());
    dimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap dimensionModel = dimensionController.modelDocument();
    assert(!dimensionModel.contains("preview_object"));
    QVariantList dimensionObjects = dimensionModel.value("drawing_objects").toList();
    assert(dimensionObjects.size() == 1);
    QVariantMap dimension = dimensionObjects.front().toMap();
    assert(dimension.value("kind").toString() == "dimension");
    assert(!dimension.value("plot_ready").toBool());
    assert(dimension.value("dimension_kind").toString() == "distance");
    assert(dimension.value("dimension_visual_controls").toBool());
    assert(dimension.value("dimension_show_label").toBool());
    QStringList dimensionFieldIds = numericFieldIds(dimension);
    assert(dimensionFieldIds.contains("dimension_length"));
    assert(dimensionFieldIds.contains("dimension_angle_deg"));
    QVariantMap dimensionLengthField = numericField(dimension, "dimension_length");
    assert(dimensionLengthField.value("physical_editable").toBool());
    assert(dimensionLengthField.value("physical_unit_kind").toString() == "length");
    assert(dimensionLengthField.value("physical_unit_label").toString() == "in");
    assert(nearlyEqual(dimensionLengthField.value("physical_minimum").toDouble(), 0.0));
    QVariantMap dimensionAngleField = numericField(dimension, "dimension_angle_deg");
    assert(dimensionAngleField.value("physical_editable").toBool());
    assert(dimensionAngleField.value("physical_unit_kind").toString() == "angle");
    assert(dimensionAngleField.value("physical_unit_label").toString() == "deg");
    QStringList dimensionHandleIds = editHandleIds(dimension);
    assert(dimension.value("editable_handle_count").toInt() == 3);
    assert(dimensionHandleIds.contains("dimension_start"));
    assert(dimensionHandleIds.contains("dimension_end"));
    assert(dimensionHandleIds.contains("dimension_offset"));
    QVariantList dimensionHandles = dimension.value("edit_handles").toList();
    QVariantMap dimensionOffsetHandle;
    for (const QVariant &handle : dimensionHandles) {
        QVariantMap candidate = handle.toMap();
        if (candidate.value("id").toString() == "dimension_offset") {
            dimensionOffsetHandle = candidate;
        }
    }
    assert(!dimensionOffsetHandle.isEmpty());
    assert(dimensionOffsetHandle.value("role").toString() == "offset");
    assert(dimensionOffsetHandle.value("editable").toBool());
    assert(dimensionOffsetHandle.value("cursor").toString() == "move");
    assert(dimensionOffsetHandle.value("shape").toString() == "diamond");
    assert(nearlyEqual(dimensionOffsetHandle.value("size_px").toDouble(), 8.0));
    assert(nearlyEqual(dimensionOffsetHandle.value("hit_tolerance_px").toDouble(), 14.0));
    assert(nearlyEqual(dimensionOffsetHandle.value("x").toDouble(), 0.218));
    assert(nearlyEqual(dimensionOffsetHandle.value("y").toDouble(), 0.424));
    assert(dimensionOffsetHandle.value("has_anchor").toBool());
    assert(nearlyEqual(dimensionOffsetHandle.value("anchor_x").toDouble(), 0.25));
    assert(nearlyEqual(dimensionOffsetHandle.value("anchor_y").toDouble(), 0.4));
    assert(dimension.value("label").toString() == "0.5 canvas_unit");

    auto projectedPointBuild = edi::drafting::buildDraftingObject(
        "projected_point",
        edi::drafting::DraftingShapeKind::Point,
        edi::drafting::PointGeometry{{0.2, 0.3}});
    assert(projectedPointBuild.ok);
    QVariantMap projectedPoint = drawing_core::draftingObjectToCanvasProjection(projectedPointBuild.object);
    QVariantMap projectedPointXField = numericField(projectedPoint, "x");
    assert(!projectedPointXField.value("physical_editable").toBool());
    assert(!projectedPoint.contains("physical_geometry"));

    auto projectedPolygonBuild = edi::drafting::buildDraftingObject(
        "projected_polygon",
        edi::drafting::DraftingShapeKind::Polygon,
        edi::drafting::PolygonGeometry{{{0.1, 0.1}, {0.4, 0.1}, {0.4, 0.4}}});
    assert(projectedPolygonBuild.ok);
    QVariantMap projectedPolygon = drawing_core::draftingObjectToCanvasProjection(projectedPolygonBuild.object);
    QVariantList projectedPolygonHandles = projectedPolygon.value("edit_handles").toList();
    assert(projectedPolygon.value("handle_count").toInt() == 3);
    assert(projectedPolygon.value("editable_handle_count").toInt() == 0);
    assert(projectedPolygonHandles.size() == 3);
    QVariantMap projectedVertex = projectedPolygonHandles.front().toMap();
    assert(projectedVertex.value("id").toString() == "vertex_0");
    assert(projectedVertex.value("role").toString() == "vertex");
    assert(!projectedVertex.value("editable").toBool());
    assert(projectedVertex.value("read_only").toBool());
    assert(projectedVertex.value("cursor").toString() == "default");
    assert(projectedVertex.value("shape").toString() == "square");
    assert(nearlyEqual(projectedVertex.value("size_px").toDouble(), 6.0));
    assert(nearlyEqual(projectedVertex.value("hit_tolerance_px").toDouble(), 0.0));

    QVariantMap dimensionPhysical = dimension.value("physical_geometry").toMap();
    assert(dimensionPhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(dimensionPhysical.value("dimension_distance").toDouble(), 6.0));
    assert(nearlyEqual(dimensionPhysical.value("dimension_length").toDouble(), 6.0));
    assert(nearlyEqual(dimensionPhysical.value("dimension_angle_deg").toDouble(), 53.1301023542));
    assert(nearlyEqual(dimensionPhysical.value("offset").toDouble(), 0.48));
    assert(dimensionPhysical.value("dimension_label").toString() == "6 in");
    dimensionController.updatePointerNormalized(0.218, 0.424);
    QVariantMap dimensionQuickMeasure = dimensionController.modelDocument().value("quick_measurement").toMap();
    assert(dimensionQuickMeasure.value("ok").toBool());
    assert(dimensionQuickMeasure.value("kind").toString() == "dimension");
    assert(dimensionQuickMeasure.value("object_kind").toString() == "dimension");
    assert(dimensionQuickMeasure.value("dimension_kind").toString() == "distance");
    assert(nearlyEqual(dimensionQuickMeasure.value("length").toDouble(), dimension.value("dimension_length").toDouble()));
    assert(nearlyEqual(dimensionQuickMeasure.value("displayed_length").toDouble(), dimension.value("dimension_length").toDouble()));
    assert(nearlyEqual(dimensionQuickMeasure.value("physical_displayed_length").toDouble(), dimensionPhysical.value("dimension_length").toDouble()));
    assert(nearlyEqual(dimensionQuickMeasure.value("physical_angle_deg").toDouble(), dimensionPhysical.value("dimension_angle_deg").toDouble()));
    assert(nearlyEqual(dimensionQuickMeasure.value("physical_offset").toDouble(), dimensionPhysical.value("offset").toDouble()));
    assert(nearlyEqual(dimension.value("dimension_x1").toDouble(), 0.068));
    assert(nearlyEqual(dimension.value("dimension_y1").toDouble(), 0.224));
    assert(nearlyEqual(dimension.value("dimension_x2").toDouble(), 0.368));
    assert(nearlyEqual(dimension.value("dimension_y2").toDouble(), 0.624));
    assert(nearlyEqual(dimension.value("extension_x1").toDouble(), 0.1));
    assert(nearlyEqual(dimension.value("extension_y1").toDouble(), 0.2));
    assert(nearlyEqual(dimension.value("extension_x2").toDouble(), 0.068));
    assert(nearlyEqual(dimension.value("extension_y2").toDouble(), 0.224));
    assert(nearlyEqual(dimension.value("offset").toDouble(), 0.04));
    assert(dimensionController.setSelectedDimensionLabelVisible(false));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(!dimension.value("dimension_show_label").toBool());
    assert(dimensionController.updateSelectedObjectPhysicalGeometryField("offset", 1.2));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(dimension.value("offset").toDouble(), 0.1));
    dimensionPhysical = dimension.value("physical_geometry").toMap();
    assert(nearlyEqual(dimensionPhysical.value("offset").toDouble(), 1.2));
    assert(dimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 3.0));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(dimension.value("x2").toDouble(), 0.25));
    assert(nearlyEqual(dimension.value("y2").toDouble(), 0.4));
    dimensionPhysical = dimension.value("physical_geometry").toMap();
    assert(nearlyEqual(dimensionPhysical.value("dimension_length").toDouble(), 3.0));
    assert(dimensionController.updateSelectedObjectPhysicalGeometryField("dimension_angle_deg", 0.0));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(dimension.value("x2").toDouble(), 0.35));
    assert(nearlyEqual(dimension.value("y2").toDouble(), 0.2));
    const int dimensionPhysicalRevisionBeforeInvalid = dimensionController.modelDocument().value("revision").toInt();
    assert(!dimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", -1.0));
    assert(dimensionController.modelDocument().value("revision").toInt() == dimensionPhysicalRevisionBeforeInvalid);
    DrawingDocumentController nonDimensionLabelController;
    nonDimensionLabelController.setSelectedToolId("point_tool");
    nonDimensionLabelController.clickCanvasNormalized(0.2, 0.2);
    assert(!nonDimensionLabelController.setSelectedDimensionLabelVisible(false));
    assert(dimensionController.updateSelectedObjectGeometryField("offset", 0.08));
    assert(dimensionController.updateSelectedObjectGeometryField("y2", 0.4));
    assert(dimensionController.updateSelectedObjectGeometryField("x2", 0.5));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(dimension.value("offset").toDouble(), 0.08));
    assert(nearlyEqual(dimension.value("x2").toDouble(), 0.5));
    assert(dimensionController.updateSelectedObjectGeometryField("x2", 0.1));
    const int dimensionRevisionBeforeInvalid = dimensionController.modelDocument().value("revision").toInt();
    assert(!dimensionController.updateSelectedObjectGeometryField("y2", 0.2));
    assert(dimensionController.modelDocument().value("revision").toInt() == dimensionRevisionBeforeInvalid);

    DrawingDocumentController widthDimensionController;
    widthDimensionController.setSelectedToolId("width_dimension_tool");
    widthDimensionController.clickCanvasNormalized(0.1, 0.2);
    widthDimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap widthDimension = widthDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(widthDimension.value("dimension_kind").toString() == "width");
    QStringList widthDimensionFieldIds = numericFieldIds(widthDimension);
    assert(widthDimensionFieldIds.contains("dimension_length"));
    assert(!widthDimensionFieldIds.contains("dimension_angle_deg"));
    assert(nearlyEqual(widthDimension.value("x1").toDouble(), 0.1));
    assert(nearlyEqual(widthDimension.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(widthDimension.value("x2").toDouble(), 0.4));
    assert(nearlyEqual(widthDimension.value("y2").toDouble(), 0.2));
    assert(widthDimension.value("label").toString() == "0.3 canvas_unit");
    assert(widthDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 6.0));
    widthDimension = widthDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(widthDimension.value("x2").toDouble(), 0.6));
    assert(nearlyEqual(widthDimension.value("y2").toDouble(), 0.2));
    assert(!widthDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_angle_deg", 45.0));

    DrawingDocumentController heightDimensionController;
    heightDimensionController.setSelectedToolId("height_dimension_tool");
    heightDimensionController.clickCanvasNormalized(0.1, 0.2);
    heightDimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap heightDimension = heightDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(heightDimension.value("dimension_kind").toString() == "height");
    QStringList heightDimensionFieldIds = numericFieldIds(heightDimension);
    assert(heightDimensionFieldIds.contains("dimension_length"));
    assert(!heightDimensionFieldIds.contains("dimension_angle_deg"));
    assert(nearlyEqual(heightDimension.value("x1").toDouble(), 0.1));
    assert(nearlyEqual(heightDimension.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(heightDimension.value("x2").toDouble(), 0.1));
    assert(nearlyEqual(heightDimension.value("y2").toDouble(), 0.6));
    assert(heightDimension.value("label").toString() == "0.4 canvas_unit");
    assert(heightDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 6.0));
    heightDimension = heightDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(heightDimension.value("x2").toDouble(), 0.1));
    assert(nearlyEqual(heightDimension.value("y2").toDouble(), 0.7));

    DrawingDocumentController diameterDimensionController;
    diameterDimensionController.setSelectedToolId("diameter_dimension_tool");
    diameterDimensionController.clickCanvasNormalized(0.1, 0.2);
    diameterDimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap diameterDimension = diameterDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(diameterDimension.value("dimension_kind").toString() == "diameter");
    QStringList diameterDimensionFieldIds = numericFieldIds(diameterDimension);
    assert(diameterDimensionFieldIds.contains("dimension_length"));
    assert(diameterDimensionFieldIds.contains("dimension_angle_deg"));
    assert(diameterDimension.value("label").toString() == "1 canvas_unit");
    assert(nearlyEqual(diameterDimension.value("dimension_distance").toDouble(), 1.0));
    assert(diameterDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 6.0));
    diameterDimension = diameterDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(diameterDimension.value("x2").toDouble(), 0.25));
    assert(nearlyEqual(diameterDimension.value("y2").toDouble(), 0.4));
    assert(nearlyEqual(diameterDimension.value("dimension_distance").toDouble(), 0.5));

    DrawingDocumentController dimensionOffsetScaleController;
    dimensionOffsetScaleController.setGridSize(12.0, 6.0);
    dimensionOffsetScaleController.setSelectedToolId("distance_dimension_tool");
    dimensionOffsetScaleController.clickCanvasNormalized(0.1, 0.2);
    dimensionOffsetScaleController.clickCanvasNormalized(0.4, 0.2);
    QVariantMap scaledOffsetDimension = dimensionOffsetScaleController.modelDocument().value("drawing_objects").toList().front().toMap();
    QVariantMap scaledOffsetPhysical = scaledOffsetDimension.value("physical_geometry").toMap();
    assert(nearlyEqual(scaledOffsetPhysical.value("offset").toDouble(), 0.24));
    assert(dimensionOffsetScaleController.updateSelectedObjectPhysicalGeometryField("offset", 1.2));
    scaledOffsetDimension = dimensionOffsetScaleController.modelDocument().value("drawing_objects").toList().front().toMap();
    scaledOffsetPhysical = scaledOffsetDimension.value("physical_geometry").toMap();
    assert(nearlyEqual(scaledOffsetDimension.value("offset").toDouble(), 0.2));
    assert(nearlyEqual(scaledOffsetPhysical.value("offset").toDouble(), 1.2));

    DrawingDocumentController dimensionKindController;
    dimensionKindController.setSelectedToolId("distance_dimension_tool");
    dimensionKindController.clickCanvasNormalized(0.1, 0.2);
    dimensionKindController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap dimensionKindObject = dimensionKindController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(dimensionKindObject.value("dimension_kind").toString() == "distance");
    assert(dimensionKindController.setSelectedDimensionKind("width"));
    dimensionKindObject = dimensionKindController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(dimensionKindObject.value("dimension_kind").toString() == "width");
    assert(nearlyEqual(dimensionKindObject.value("x2").toDouble(), 0.6));
    assert(nearlyEqual(dimensionKindObject.value("y2").toDouble(), 0.2));
    QStringList switchedWidthFields = numericFieldIds(dimensionKindObject);
    assert(switchedWidthFields.contains("dimension_length"));
    assert(!switchedWidthFields.contains("dimension_angle_deg"));
    assert(dimensionKindController.setSelectedDimensionKind("height"));
    dimensionKindObject = dimensionKindController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(dimensionKindObject.value("dimension_kind").toString() == "height");
    assert(nearlyEqual(dimensionKindObject.value("x2").toDouble(), 0.1));
    assert(nearlyEqual(dimensionKindObject.value("y2").toDouble(), 0.7));
    assert(!dimensionKindController.setSelectedDimensionKind("ordinal"));
    const int dimensionKindRevisionBeforeInvalidLength = dimensionKindController.modelDocument().value("revision").toInt();
    assert(!dimensionKindController.updateSelectedObjectGeometryField("dimension_length", 0.0));
    assert(dimensionKindController.modelDocument().value("revision").toInt() == dimensionKindRevisionBeforeInvalidLength);
    DrawingDocumentController nonDimensionKindController;
    nonDimensionKindController.setSelectedToolId("point_tool");
    nonDimensionKindController.clickCanvasNormalized(0.2, 0.2);
    assert(!nonDimensionKindController.setSelectedDimensionKind("width"));

    DrawingDocumentController dimensionHandleController;
    dimensionHandleController.setSelectedToolId("distance_dimension_tool");
    dimensionHandleController.clickCanvasNormalized(0.1, 0.2);
    dimensionHandleController.clickCanvasNormalized(0.5, 0.2);
    assert(dimensionHandleController.editSelectedHandleNormalized("dimension_end", 0.8, 0.4));
    QVariantMap handleDimension = dimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(handleDimension.value("x2").toDouble(), 0.8));
    assert(nearlyEqual(handleDimension.value("y2").toDouble(), 0.4));
    assert(dimensionHandleController.editSelectedHandleNormalized("dimension_offset", 0.45, 0.45));
    handleDimension = dimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(handleDimension.value("offset").toDouble() > 0.0);
    assert(!dimensionHandleController.editSelectedHandleNormalized("dimension_missing", 0.2, 0.2));

    DrawingDocumentController widthDimensionHandleController;
    widthDimensionHandleController.setSelectedToolId("width_dimension_tool");
    widthDimensionHandleController.clickCanvasNormalized(0.1, 0.2);
    widthDimensionHandleController.clickCanvasNormalized(0.5, 0.8);
    assert(widthDimensionHandleController.editSelectedHandleNormalized("dimension_end", 0.9, 0.9));
    QVariantMap widthHandleDimension = widthDimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(widthHandleDimension.value("dimension_kind").toString() == "width");
    assert(nearlyEqual(widthHandleDimension.value("x2").toDouble(), 0.9));
    assert(nearlyEqual(widthHandleDimension.value("y2").toDouble(), 0.2));

    DrawingDocumentController heightDimensionHandleController;
    heightDimensionHandleController.setSelectedToolId("height_dimension_tool");
    heightDimensionHandleController.clickCanvasNormalized(0.1, 0.2);
    heightDimensionHandleController.clickCanvasNormalized(0.5, 0.8);
    assert(heightDimensionHandleController.editSelectedHandleNormalized("dimension_end", 0.9, 0.9));
    QVariantMap heightHandleDimension = heightDimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(heightHandleDimension.value("dimension_kind").toString() == "height");
    assert(nearlyEqual(heightHandleDimension.value("x2").toDouble(), 0.1));
    assert(nearlyEqual(heightHandleDimension.value("y2").toDouble(), 0.9));

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

    QVariantList beforePointerObjects = objectSnapController.modelDocument().value("drawing_objects").toList();
    objectSnapController.updatePointerNormalized(0.26, 0.24);
    QVariantMap pointerModel = objectSnapController.modelDocument();
    QVariantMap pointer = pointerModel.value("pointer").toMap();
    assert(!pointer.isEmpty());
    assert(nearlyEqual(pointer.value("raw").toMap().value("x").toDouble(), 0.26));
    assert(nearlyEqual(pointer.value("raw").toMap().value("y").toDouble(), 0.24));
    assert(pointer.value("kind").toString() == "object");
    assert(pointer.value("source").toString() == "endpoint");
    assert(nearlyEqual(pointer.value("snapped").toMap().value("x").toDouble(), 0.25));
    assert(nearlyEqual(pointer.value("snapped").toMap().value("y").toDouble(), 0.25));
    assert(nearlyEqual(pointer.value("snapped_unit_x").toDouble(), 3.0));
    assert(nearlyEqual(pointer.value("snapped_unit_y").toDouble(), 3.0));
    assert(pointer.value("unit_label").toString() == "in");
    assert(pointer.value("inside_drawable").toBool());
    assert(pointerModel.value("drawing_objects").toList().size() == beforePointerObjects.size());

    DrawingDocumentController guidePointerController;
    guidePointerController.setSelectedToolId("horizontal_guide_tool");
    guidePointerController.clickCanvasNormalized(0.2, 0.75);
    guidePointerController.setSelectedToolId("vertical_guide_tool");
    guidePointerController.clickCanvasNormalized(0.33, 0.2);
    guidePointerController.setObjectSnapEnabled(true);
    QVariantList beforeGuidePointerObjects = guidePointerController.modelDocument().value("drawing_objects").toList();
    guidePointerController.updatePointerNormalized(0.34, 0.74);
    QVariantMap guidePointerModel = guidePointerController.modelDocument();
    QVariantMap guidePointer = guidePointerModel.value("pointer").toMap();
    assert(guidePointer.value("kind").toString() == "object");
    assert(guidePointer.value("source").toString() == "guide");
    assert(guidePointer.value("label").toString() == "guide");
    assert(!guidePointer.value("source_object_id").toString().isEmpty());
    assert(nearlyEqual(guidePointer.value("snapped").toMap().value("x").toDouble(), 0.33));
    assert(nearlyEqual(guidePointer.value("snapped").toMap().value("y").toDouble(), 0.75));
    assert(guidePointerModel.value("drawing_objects").toList().size() == beforeGuidePointerObjects.size());
    guidePointerController.setGuideSnapEnabled(false);
    guidePointerController.updatePointerNormalized(0.34, 0.74);
    guidePointer = guidePointerController.modelDocument().value("pointer").toMap();
    assert(guidePointer.value("kind").toString() == "none");

    DrawingDocumentController emptyMeasureController;
    emptyMeasureController.updatePointerNormalized(0.5, 0.5);
    QVariantMap emptyMeasureModel = emptyMeasureController.modelDocument();
    QVariantMap emptyMeasure = emptyMeasureModel.value("quick_measurement").toMap();
    assert(!emptyMeasure.value("ok").toBool());
    assert(emptyMeasure.value("kind").toString() == "none");
    assert(emptyMeasure.value("message").toString() == "no measurable target");

    DrawingDocumentController lineMeasureController;
    lineMeasureController.setSelectedToolId("line_tool");
    lineMeasureController.clickCanvasNormalized(0.1, 0.2);
    lineMeasureController.clickCanvasNormalized(0.4, 0.6);
    const int lineMeasureRevision = lineMeasureController.modelDocument().value("revision").toInt();
    const int lineMeasureObjectCount = lineMeasureController.modelDocument().value("drawing_objects").toList().size();
    lineMeasureController.updatePointerNormalized(0.25, 0.4);
    QVariantMap lineMeasureModel = lineMeasureController.modelDocument();
    QVariantMap lineMeasure = lineMeasureModel.value("quick_measurement").toMap();
    assert(lineMeasure.value("ok").toBool());
    assert(lineMeasure.value("kind").toString() == "line");
    assert(lineMeasure.value("object_kind").toString() == "line");
    assert(nearlyEqual(lineMeasure.value("length").toDouble(), 0.5));
    assert(nearlyEqual(lineMeasure.value("physical_length").toDouble(), 6.0));
    assert(nearlyEqual(lineMeasure.value("physical_angle_deg").toDouble(), 53.1301023542));
    assert(lineMeasure.value("unit_label").toString() == "in");
    QVariantMap projectedLinePhysical = lineMeasureModel.value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    assert(nearlyEqual(lineMeasure.value("physical_length").toDouble(), projectedLinePhysical.value("line_length").toDouble()));
    assert(nearlyEqual(lineMeasure.value("physical_angle_deg").toDouble(), projectedLinePhysical.value("line_angle_deg").toDouble()));
    assert(lineMeasureModel.value("revision").toInt() == lineMeasureRevision);
    assert(lineMeasureModel.value("drawing_objects").toList().size() == lineMeasureObjectCount);

    DrawingDocumentController circleMeasureController;
    circleMeasureController.setSelectedToolId("circle_tool");
    circleMeasureController.clickCanvasNormalized(0.5, 0.5);
    circleMeasureController.clickCanvasNormalized(0.7, 0.5);
    circleMeasureController.updatePointerNormalized(0.7, 0.5);
    QVariantMap circleMeasure = circleMeasureController.modelDocument().value("quick_measurement").toMap();
    assert(circleMeasure.value("ok").toBool());
    assert(circleMeasure.value("kind").toString() == "circle");
    assert(nearlyEqual(circleMeasure.value("radius").toDouble(), 0.2));
    assert(nearlyEqual(circleMeasure.value("diameter").toDouble(), 0.4));
    assert(nearlyEqual(circleMeasure.value("physical_radius").toDouble(), 2.4));
    assert(nearlyEqual(circleMeasure.value("physical_diameter").toDouble(), 4.8));
    QVariantMap projectedCirclePhysical = circleMeasureController.modelDocument().value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    assert(nearlyEqual(circleMeasure.value("physical_radius").toDouble(), projectedCirclePhysical.value("radius").toDouble()));
    assert(nearlyEqual(circleMeasure.value("physical_diameter").toDouble(), projectedCirclePhysical.value("diameter").toDouble()));

    DrawingDocumentController rectMeasureController;
    rectMeasureController.setSelectedToolId("rectangle_tool");
    rectMeasureController.clickCanvasNormalized(0.1, 0.2);
    rectMeasureController.clickCanvasNormalized(0.4, 0.6);
    rectMeasureController.updatePointerNormalized(0.2, 0.3);
    QVariantMap rectMeasure = rectMeasureController.modelDocument().value("quick_measurement").toMap();
    assert(rectMeasure.value("ok").toBool());
    assert(rectMeasure.value("kind").toString() == "rectangle");
    assert(nearlyEqual(rectMeasure.value("width").toDouble(), 0.3));
    assert(nearlyEqual(rectMeasure.value("height").toDouble(), 0.4));
    assert(nearlyEqual(rectMeasure.value("physical_width").toDouble(), 3.6));
    assert(nearlyEqual(rectMeasure.value("physical_height").toDouble(), 4.8));
    assert(nearlyEqual(rectMeasure.value("physical_area").toDouble(), 17.28));
    QVariantMap projectedRectPhysical = rectMeasureController.modelDocument().value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    assert(nearlyEqual(rectMeasure.value("physical_width").toDouble(), projectedRectPhysical.value("width").toDouble()));
    assert(nearlyEqual(rectMeasure.value("physical_height").toDouble(), projectedRectPhysical.value("height").toDouble()));

    DrawingDocumentController pointMeasureController;
    pointMeasureController.setSelectedToolId("point_tool");
    pointMeasureController.clickCanvasNormalized(0.25, 0.5);
    pointMeasureController.updatePointerNormalized(0.25, 0.5);
    QVariantMap pointMeasure = pointMeasureController.modelDocument().value("quick_measurement").toMap();
    assert(pointMeasure.value("ok").toBool());
    assert(pointMeasure.value("kind").toString() == "point");
    assert(nearlyEqual(pointMeasure.value("physical_x").toDouble(), 3.0));
    assert(nearlyEqual(pointMeasure.value("physical_y").toDouble(), 6.0));
    QVariantMap projectedPointPhysical = pointMeasureController.modelDocument().value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    assert(nearlyEqual(pointMeasure.value("physical_x").toDouble(), projectedPointPhysical.value("x").toDouble()));
    assert(nearlyEqual(pointMeasure.value("physical_y").toDouble(), projectedPointPhysical.value("y").toDouble()));

    DrawingDocumentController guideCreationSnapController;
    guideCreationSnapController.setSelectedToolId("horizontal_guide_tool");
    guideCreationSnapController.clickCanvasNormalized(0.2, 0.75);
    guideCreationSnapController.setSelectedToolId("vertical_guide_tool");
    guideCreationSnapController.clickCanvasNormalized(0.33, 0.2);
    guideCreationSnapController.setObjectSnapEnabled(true);
    guideCreationSnapController.setSelectedToolId("point_tool");
    guideCreationSnapController.clickCanvasNormalized(0.34, 0.74);
    QVariantList guideSnappedObjects = guideCreationSnapController.modelDocument().value("drawing_objects").toList();
    assert(guideSnappedObjects.size() == 3);
    QVariantMap guideSnappedPoint = guideSnappedObjects.back().toMap();
    assert(guideSnappedPoint.value("kind").toString() == "point");
    assert(nearlyEqual(guideSnappedPoint.value("x").toDouble(), 0.33));
    assert(nearlyEqual(guideSnappedPoint.value("y").toDouble(), 0.75));

    DrawingDocumentController guideMoveDisabledCreationController;
    guideMoveDisabledCreationController.setSelectedToolId("horizontal_guide_tool");
    guideMoveDisabledCreationController.clickCanvasNormalized(0.2, 0.75);
    guideMoveDisabledCreationController.setSelectedToolId("vertical_guide_tool");
    guideMoveDisabledCreationController.clickCanvasNormalized(0.33, 0.2);
    guideMoveDisabledCreationController.setObjectSnapEnabled(true);
    guideMoveDisabledCreationController.setGuideMoveSnapEnabled(false);
    guideMoveDisabledCreationController.setSelectedToolId("point_tool");
    guideMoveDisabledCreationController.clickCanvasNormalized(0.34, 0.74);
    QVariantMap guideMoveDisabledCreatedPoint = guideMoveDisabledCreationController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(guideMoveDisabledCreatedPoint.value("kind").toString() == "point");
    assert(nearlyEqual(guideMoveDisabledCreatedPoint.value("x").toDouble(), 0.33));
    assert(nearlyEqual(guideMoveDisabledCreatedPoint.value("y").toDouble(), 0.75));

    DrawingDocumentController guideCreationSnapDisabledController;
    guideCreationSnapDisabledController.setSelectedToolId("horizontal_guide_tool");
    guideCreationSnapDisabledController.clickCanvasNormalized(0.2, 0.75);
    guideCreationSnapDisabledController.setSelectedToolId("vertical_guide_tool");
    guideCreationSnapDisabledController.clickCanvasNormalized(0.33, 0.2);
    guideCreationSnapDisabledController.setObjectSnapEnabled(true);
    guideCreationSnapDisabledController.setGuideSnapEnabled(false);
    guideCreationSnapDisabledController.setSelectedToolId("point_tool");
    guideCreationSnapDisabledController.clickCanvasNormalized(0.34, 0.74);
    QVariantMap guideUnsnappedPoint = guideCreationSnapDisabledController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideUnsnappedPoint.value("x").toDouble(), 0.34));
    assert(nearlyEqual(guideUnsnappedPoint.value("y").toDouble(), 0.74));

    DrawingDocumentController hiddenGuideCreationController;
    hiddenGuideCreationController.setSelectedToolId("horizontal_guide_tool");
    hiddenGuideCreationController.clickCanvasNormalized(0.2, 0.75);
    hiddenGuideCreationController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideCreationController.clickCanvasNormalized(0.33, 0.2);
    hiddenGuideCreationController.setObjectSnapEnabled(true);
    assert(hiddenGuideCreationController.setAllGuidesVisible(false));
    hiddenGuideCreationController.setSelectedToolId("point_tool");
    hiddenGuideCreationController.clickCanvasNormalized(0.34, 0.74);
    QVariantMap hiddenGuideUnsnappedPoint = hiddenGuideCreationController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(hiddenGuideUnsnappedPoint.value("x").toDouble(), 0.34));
    assert(nearlyEqual(hiddenGuideUnsnappedPoint.value("y").toDouble(), 0.74));

    DrawingDocumentController hiddenGuideLayerCreationController;
    assert(hiddenGuideLayerCreationController.createLayer());
    hiddenGuideLayerCreationController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideLayerCreationController.clickCanvasNormalized(0.33, 0.2);
    assert(hiddenGuideLayerCreationController.setActiveLayerVisible(false));
    assert(hiddenGuideLayerCreationController.setActiveLayerId(QStringLiteral("default")));
    hiddenGuideLayerCreationController.setObjectSnapEnabled(true);
    hiddenGuideLayerCreationController.setSelectedToolId("point_tool");
    hiddenGuideLayerCreationController.clickCanvasNormalized(0.34, 0.2);
    QVariantMap hiddenGuideLayerPoint;
    for (const QVariant &objectValue : hiddenGuideLayerCreationController.modelDocument().value("drawing_objects").toList()) {
        const QVariantMap objectMap = objectValue.toMap();
        if (objectMap.value("kind").toString() == "point") {
            hiddenGuideLayerPoint = objectMap;
        }
    }
    assert(!hiddenGuideLayerPoint.isEmpty());
    assert(nearlyEqual(hiddenGuideLayerPoint.value("x").toDouble(), 0.34));
    assert(nearlyEqual(hiddenGuideLayerPoint.value("y").toDouble(), 0.2));

    DrawingDocumentController lockedGuideCreationController;
    lockedGuideCreationController.setSelectedToolId("vertical_guide_tool");
    lockedGuideCreationController.clickCanvasNormalized(0.33, 0.2);
    assert(lockedGuideCreationController.setSelectedObjectLocked(true));
    lockedGuideCreationController.setObjectSnapEnabled(true);
    lockedGuideCreationController.setSelectedToolId("point_tool");
    lockedGuideCreationController.clickCanvasNormalized(0.34, 0.2);
    QVariantMap lockedGuideSnappedPoint = lockedGuideCreationController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(lockedGuideSnappedPoint.value("x").toDouble(), 0.33));
    assert(nearlyEqual(lockedGuideSnappedPoint.value("y").toDouble(), 0.2));

    DrawingDocumentController guideMoveSnapController;
    guideMoveSnapController.setSelectedToolId("horizontal_guide_tool");
    guideMoveSnapController.clickCanvasNormalized(0.2, 0.75);
    guideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    guideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    guideMoveSnapController.setSelectedToolId("point_tool");
    guideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    guideMoveSnapController.setObjectSnapEnabled(true);
    assert(guideMoveSnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap guideMoveModel = guideMoveSnapController.modelDocument();
    QVariantMap guideMovedPoint = guideMoveModel.value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideMovedPoint.value("x").toDouble(), 0.33));
    assert(nearlyEqual(guideMovedPoint.value("y").toDouble(), 0.75));
    QVariantMap guideDragSnap = guideMoveModel.value("guide_drag_snap").toMap();
    assert(!guideDragSnap.isEmpty());
    assert(guideDragSnap.value("kind").toString() == "guide");
    assert(guideDragSnap.value("mode").toString() == "move_selection");
    assert(guideDragSnap.value("anchor_label").toString() == "point");
    assert(guideDragSnap.value("intersection").toBool());
    assert(!guideDragSnap.value("source_object_id").toString().isEmpty());
    assert(nearlyEqual(guideDragSnap.value("raw_anchor").toMap().value("x").toDouble(), 0.34));
    assert(nearlyEqual(guideDragSnap.value("raw_anchor").toMap().value("y").toDouble(), 0.74));
    assert(nearlyEqual(guideDragSnap.value("snapped_anchor").toMap().value("x").toDouble(), 0.33));
    assert(nearlyEqual(guideDragSnap.value("snapped_anchor").toMap().value("y").toDouble(), 0.75));

    DrawingDocumentController disabledGuideMoveSnapController;
    disabledGuideMoveSnapController.setSelectedToolId("horizontal_guide_tool");
    disabledGuideMoveSnapController.clickCanvasNormalized(0.2, 0.75);
    disabledGuideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    disabledGuideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideMoveSnapController.setSelectedToolId("point_tool");
    disabledGuideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideMoveSnapController.setObjectSnapEnabled(true);
    disabledGuideMoveSnapController.setGuideSnapEnabled(false);
    assert(disabledGuideMoveSnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap disabledGuideMoveModel = disabledGuideMoveSnapController.modelDocument();
    QVariantMap disabledGuideMovedPoint = disabledGuideMoveModel.value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(disabledGuideMovedPoint.value("x").toDouble(), 0.34));
    assert(nearlyEqual(disabledGuideMovedPoint.value("y").toDouble(), 0.74));
    assert(!disabledGuideMoveModel.contains("guide_drag_snap"));

    DrawingDocumentController disabledGuideMoveOnlySnapController;
    disabledGuideMoveOnlySnapController.setSelectedToolId("horizontal_guide_tool");
    disabledGuideMoveOnlySnapController.clickCanvasNormalized(0.2, 0.75);
    disabledGuideMoveOnlySnapController.setSelectedToolId("vertical_guide_tool");
    disabledGuideMoveOnlySnapController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideMoveOnlySnapController.setSelectedToolId("point_tool");
    disabledGuideMoveOnlySnapController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideMoveOnlySnapController.setObjectSnapEnabled(true);
    disabledGuideMoveOnlySnapController.setGuideMoveSnapEnabled(false);
    assert(disabledGuideMoveOnlySnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap disabledGuideMoveOnlyModel = disabledGuideMoveOnlySnapController.modelDocument();
    QVariantMap disabledGuideMoveOnlyPoint = disabledGuideMoveOnlyModel.value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(disabledGuideMoveOnlyPoint.value("x").toDouble(), 0.34));
    assert(nearlyEqual(disabledGuideMoveOnlyPoint.value("y").toDouble(), 0.74));
    assert(!disabledGuideMoveOnlyModel.contains("guide_drag_snap"));

    DrawingDocumentController hiddenGuideMoveSnapController;
    hiddenGuideMoveSnapController.setSelectedToolId("horizontal_guide_tool");
    hiddenGuideMoveSnapController.clickCanvasNormalized(0.2, 0.75);
    hiddenGuideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    assert(hiddenGuideMoveSnapController.setAllGuidesVisible(false));
    hiddenGuideMoveSnapController.setSelectedToolId("point_tool");
    hiddenGuideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    hiddenGuideMoveSnapController.setObjectSnapEnabled(true);
    assert(hiddenGuideMoveSnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap hiddenGuideMovedPoint = hiddenGuideMoveSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(hiddenGuideMovedPoint.value("x").toDouble(), 0.34));
    assert(nearlyEqual(hiddenGuideMovedPoint.value("y").toDouble(), 0.74));

    DrawingDocumentController lockedGuideMoveSnapController;
    lockedGuideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    lockedGuideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    assert(lockedGuideMoveSnapController.setSelectedObjectLocked(true));
    lockedGuideMoveSnapController.setSelectedToolId("point_tool");
    lockedGuideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    lockedGuideMoveSnapController.setObjectSnapEnabled(true);
    assert(lockedGuideMoveSnapController.moveSelectionNormalized(0.14, 0.0));
    QVariantMap lockedGuideMovedPoint = lockedGuideMoveSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(lockedGuideMovedPoint.value("x").toDouble(), 0.33));
    assert(nearlyEqual(lockedGuideMovedPoint.value("y").toDouble(), 0.2));

    DrawingDocumentController guideRectangleLeftEdgeMoveController;
    guideRectangleLeftEdgeMoveController.setSelectedToolId("vertical_guide_tool");
    guideRectangleLeftEdgeMoveController.clickCanvasNormalized(0.33, 0.2);
    guideRectangleLeftEdgeMoveController.setSelectedToolId("rectangle_tool");
    guideRectangleLeftEdgeMoveController.clickCanvasNormalized(0.2, 0.2);
    guideRectangleLeftEdgeMoveController.clickCanvasNormalized(0.4, 0.4);
    guideRectangleLeftEdgeMoveController.setObjectSnapEnabled(true);
    assert(guideRectangleLeftEdgeMoveController.moveSelectionNormalized(0.12, 0.0));
    QVariantMap guideLeftEdgeRect = lastObjectOfKind(guideRectangleLeftEdgeMoveController.modelDocument(), QStringLiteral("rectangle"));
    assert(nearlyEqual(guideLeftEdgeRect.value("x").toDouble(), 0.33));
    assert(nearlyEqual(guideLeftEdgeRect.value("y").toDouble(), 0.2));
    assert(nearlyEqual(guideLeftEdgeRect.value("width").toDouble(), 0.2));

    DrawingDocumentController guideRectangleTopEdgeMoveController;
    guideRectangleTopEdgeMoveController.setSelectedToolId("horizontal_guide_tool");
    guideRectangleTopEdgeMoveController.clickCanvasNormalized(0.2, 0.75);
    guideRectangleTopEdgeMoveController.setSelectedToolId("rectangle_tool");
    guideRectangleTopEdgeMoveController.clickCanvasNormalized(0.2, 0.2);
    guideRectangleTopEdgeMoveController.clickCanvasNormalized(0.4, 0.4);
    guideRectangleTopEdgeMoveController.setObjectSnapEnabled(true);
    assert(guideRectangleTopEdgeMoveController.moveSelectionNormalized(0.0, 0.54));
    QVariantMap guideTopEdgeRect = lastObjectOfKind(guideRectangleTopEdgeMoveController.modelDocument(), QStringLiteral("rectangle"));
    assert(nearlyEqual(guideTopEdgeRect.value("x").toDouble(), 0.2));
    assert(nearlyEqual(guideTopEdgeRect.value("y").toDouble(), 0.75));
    assert(nearlyEqual(guideTopEdgeRect.value("height").toDouble(), 0.2));

    DrawingDocumentController guideLineEndpointMoveController;
    guideLineEndpointMoveController.setSelectedToolId("horizontal_guide_tool");
    guideLineEndpointMoveController.clickCanvasNormalized(0.2, 0.75);
    guideLineEndpointMoveController.setSelectedToolId("vertical_guide_tool");
    guideLineEndpointMoveController.clickCanvasNormalized(0.33, 0.2);
    guideLineEndpointMoveController.setSelectedToolId("line_tool");
    guideLineEndpointMoveController.clickCanvasNormalized(0.1, 0.1);
    guideLineEndpointMoveController.clickCanvasNormalized(0.2, 0.2);
    guideLineEndpointMoveController.setObjectSnapEnabled(true);
    assert(guideLineEndpointMoveController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap guideEndpointLine = lastObjectOfKind(guideLineEndpointMoveController.modelDocument(), QStringLiteral("line"));
    assert(nearlyEqual(guideEndpointLine.value("x1").toDouble(), 0.23));
    assert(nearlyEqual(guideEndpointLine.value("y1").toDouble(), 0.65));
    assert(nearlyEqual(guideEndpointLine.value("x2").toDouble(), 0.33));
    assert(nearlyEqual(guideEndpointLine.value("y2").toDouble(), 0.75));

    DrawingDocumentController guideRectangleCornerMoveController;
    guideRectangleCornerMoveController.setSelectedToolId("horizontal_guide_tool");
    guideRectangleCornerMoveController.clickCanvasNormalized(0.2, 0.75);
    guideRectangleCornerMoveController.setSelectedToolId("vertical_guide_tool");
    guideRectangleCornerMoveController.clickCanvasNormalized(0.33, 0.2);
    guideRectangleCornerMoveController.setSelectedToolId("rectangle_tool");
    guideRectangleCornerMoveController.clickCanvasNormalized(0.2, 0.2);
    guideRectangleCornerMoveController.clickCanvasNormalized(0.4, 0.4);
    guideRectangleCornerMoveController.setObjectSnapEnabled(true);
    assert(guideRectangleCornerMoveController.moveSelectionNormalized(-0.06, 0.34));
    QVariantMap guideCornerRect = lastObjectOfKind(guideRectangleCornerMoveController.modelDocument(), QStringLiteral("rectangle"));
    assert(nearlyEqual(guideCornerRect.value("x").toDouble(), 0.13));
    assert(nearlyEqual(guideCornerRect.value("y").toDouble(), 0.55));
    assert(nearlyEqual(guideCornerRect.value("width").toDouble(), 0.2));
    assert(nearlyEqual(guideCornerRect.value("height").toDouble(), 0.2));

    DrawingDocumentController disabledGuideRectangleEdgeMoveController;
    disabledGuideRectangleEdgeMoveController.setSelectedToolId("vertical_guide_tool");
    disabledGuideRectangleEdgeMoveController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideRectangleEdgeMoveController.setSelectedToolId("rectangle_tool");
    disabledGuideRectangleEdgeMoveController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideRectangleEdgeMoveController.clickCanvasNormalized(0.4, 0.4);
    disabledGuideRectangleEdgeMoveController.setObjectSnapEnabled(true);
    disabledGuideRectangleEdgeMoveController.setGuideSnapEnabled(false);
    assert(disabledGuideRectangleEdgeMoveController.moveSelectionNormalized(0.12, 0.0));
    QVariantMap disabledGuideEdgeRect = lastObjectOfKind(disabledGuideRectangleEdgeMoveController.modelDocument(), QStringLiteral("rectangle"));
    assert(nearlyEqual(disabledGuideEdgeRect.value("x").toDouble(), 0.32));
    assert(nearlyEqual(disabledGuideEdgeRect.value("y").toDouble(), 0.2));

    DrawingDocumentController guideHandleSnapController;
    guideHandleSnapController.setSelectedToolId("horizontal_guide_tool");
    guideHandleSnapController.clickCanvasNormalized(0.2, 0.75);
    guideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    guideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    guideHandleSnapController.setSelectedToolId("line_tool");
    guideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    guideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    guideHandleSnapController.setObjectSnapEnabled(true);
    assert(guideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.74));
    QVariantMap guideHandleLine = guideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(guideHandleLine.value("x2").toDouble(), 0.33));
    assert(nearlyEqual(guideHandleLine.value("y2").toDouble(), 0.75));

    DrawingDocumentController disabledGuideHandleSnapController;
    disabledGuideHandleSnapController.setSelectedToolId("horizontal_guide_tool");
    disabledGuideHandleSnapController.clickCanvasNormalized(0.2, 0.75);
    disabledGuideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    disabledGuideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideHandleSnapController.setSelectedToolId("line_tool");
    disabledGuideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    disabledGuideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideHandleSnapController.setObjectSnapEnabled(true);
    disabledGuideHandleSnapController.setGuideSnapEnabled(false);
    assert(disabledGuideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.74));
    QVariantMap disabledGuideHandleLine = disabledGuideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(disabledGuideHandleLine.value("x2").toDouble(), 0.34));
    assert(nearlyEqual(disabledGuideHandleLine.value("y2").toDouble(), 0.74));

    DrawingDocumentController hiddenGuideHandleSnapController;
    hiddenGuideHandleSnapController.setSelectedToolId("horizontal_guide_tool");
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.2, 0.75);
    hiddenGuideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    assert(hiddenGuideHandleSnapController.setAllGuidesVisible(false));
    hiddenGuideHandleSnapController.setSelectedToolId("line_tool");
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    hiddenGuideHandleSnapController.setObjectSnapEnabled(true);
    assert(hiddenGuideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.74));
    QVariantMap hiddenGuideHandleLine = hiddenGuideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(hiddenGuideHandleLine.value("x2").toDouble(), 0.34));
    assert(nearlyEqual(hiddenGuideHandleLine.value("y2").toDouble(), 0.74));

    DrawingDocumentController lockedGuideHandleSnapController;
    lockedGuideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    lockedGuideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    assert(lockedGuideHandleSnapController.setSelectedObjectLocked(true));
    lockedGuideHandleSnapController.setSelectedToolId("line_tool");
    lockedGuideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    lockedGuideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    lockedGuideHandleSnapController.setObjectSnapEnabled(true);
    assert(lockedGuideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.2));
    QVariantMap lockedGuideHandleLine = lockedGuideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(nearlyEqual(lockedGuideHandleLine.value("x2").toDouble(), 0.33));
    assert(nearlyEqual(lockedGuideHandleLine.value("y2").toDouble(), 0.2));

    DrawingDocumentController invisibleSnapController;
    invisibleSnapController.setSelectedToolId("point_tool");
    invisibleSnapController.clickCanvasNormalized(0.25, 0.25);
    assert(invisibleSnapController.setSelectedObjectVisible(false));
    QVariantMap invisiblePoint = invisibleSnapController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(!invisiblePoint.value("visible").toBool());
    invisibleSnapController.setObjectSnapEnabled(true);
    invisibleSnapController.clickCanvasNormalized(0.26, 0.24);
    QVariantList invisibleSnapObjects = invisibleSnapController.modelDocument().value("drawing_objects").toList();
    assert(invisibleSnapObjects.size() == 2);
    QVariantMap unsnappedPoint = invisibleSnapObjects.back().toMap();
    assert(nearlyEqual(unsnappedPoint.value("x").toDouble(), 0.26));
    assert(nearlyEqual(unsnappedPoint.value("y").toDouble(), 0.24));

    DrawingDocumentController invisibleHitController;
    invisibleHitController.setSelectedToolId("point_tool");
    invisibleHitController.clickCanvasNormalized(0.25, 0.25);
    assert(invisibleHitController.setSelectedObjectVisible(false));
    invisibleHitController.setSelectedToolId("select_move");
    invisibleHitController.clickCanvasNormalized(0.25, 0.25);
    assert(invisibleHitController.selectedObjectId().isEmpty());

    DrawingDocumentController layerController;
    layerController.setSelectedToolId("point_tool");
    layerController.clickCanvasNormalized(0.25, 0.25);
    QVariantMap layerModel = layerController.modelDocument();
    QVariantList layers = layerModel.value("layers").toList();
    assert(layers.size() == 1);
    assert(layers.front().toMap().value("id").toString() == "default");
    assert(layers.front().toMap().value("visible").toBool());
    assert(!layers.front().toMap().value("locked").toBool());

    assert(layerController.setDefaultLayerVisible(false));
    layerModel = layerController.modelDocument();
    layers = layerModel.value("layers").toList();
    assert(!layers.front().toMap().value("visible").toBool());
    QVariantMap hiddenLayerPoint = layerModel.value("drawing_objects").toList().front().toMap();
    assert(hiddenLayerPoint.value("visible").toBool());
    assert(!hiddenLayerPoint.value("effective_visible").toBool());
    layerController.setObjectSnapEnabled(true);
    layerController.clickCanvasNormalized(0.26, 0.24);
    QVariantList hiddenLayerObjects = layerController.modelDocument().value("drawing_objects").toList();
    assert(hiddenLayerObjects.size() == 2);
    QVariantMap hiddenLayerUnsnappedPoint = hiddenLayerObjects.back().toMap();
    assert(nearlyEqual(hiddenLayerUnsnappedPoint.value("x").toDouble(), 0.26));
    assert(nearlyEqual(hiddenLayerUnsnappedPoint.value("y").toDouble(), 0.24));
    layerController.setSelectedToolId("select_move");
    layerController.clickCanvasNormalized(0.25, 0.25);
    assert(layerController.selectedObjectId().isEmpty());

    assert(layerController.setDefaultLayerVisible(true));
    assert(layerController.setDefaultLayerLocked(true));
    layerModel = layerController.modelDocument();
    layers = layerModel.value("layers").toList();
    assert(layers.front().toMap().value("locked").toBool());
    layerController.setSelectedToolId("point_tool");
    const int lockedLayerObjectCount = layerController.modelDocument().value("drawing_objects").toList().size();
    layerController.clickCanvasNormalized(0.5, 0.5);
    assert(layerController.modelDocument().value("drawing_objects").toList().size() == lockedLayerObjectCount);
    assert(!layerController.setSelectedObjectLocked(true));
    assert(!layerController.updateSelectedObjectGeometryField("x", 0.3));
    assert(!layerController.moveSelectionNormalized(0.1, 0.0));
    assert(layerController.setDefaultLayerLocked(false));
    layerController.clickCanvasNormalized(0.5, 0.5);
    assert(layerController.modelDocument().value("drawing_objects").toList().size() == lockedLayerObjectCount + 1);

    DrawingDocumentController layerManagementController;
    assert(layerManagementController.createLayer());
    QVariantMap layerManagementModel = layerManagementController.modelDocument();
    assert(layerManagementModel.value("active_layer_id").toString() == "layer_2");
    QVariantList managedLayers = layerManagementModel.value("layers").toList();
    assert(managedLayers.size() == 2);
    assert(layerManagementController.renameActiveLayer("Ink"));
    managedLayers = layerManagementController.modelDocument().value("layers").toList();
    assert(managedLayers[1].toMap().value("name").toString() == "Ink");
    layerManagementController.setSelectedToolId("point_tool");
    layerManagementController.clickCanvasNormalized(0.2, 0.2);
    QVariantList managedObjects = layerManagementController.modelDocument().value("drawing_objects").toList();
    assert(managedObjects.size() == 1);
    QVariantMap managedPoint = managedObjects.front().toMap();
    assert(managedPoint.value("layer_id").toString() == "layer_2");
    assert(layerManagementController.moveSelectedObjectToLayer("default"));
    managedPoint = layerManagementController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(managedPoint.value("layer_id").toString() == "default");
    assert(layerManagementController.setActiveLayerId("layer_2"));
    assert(layerManagementController.setActiveLayerLocked(true));
    const int managedObjectCountBeforeLockedCreate = layerManagementController.modelDocument().value("drawing_objects").toList().size();
    layerManagementController.clickCanvasNormalized(0.4, 0.4);
    assert(layerManagementController.modelDocument().value("drawing_objects").toList().size() == managedObjectCountBeforeLockedCreate);
    assert(layerManagementController.setActiveLayerLocked(false));
    assert(layerManagementController.setActiveLayerId("default"));
    assert(layerManagementController.setActiveLayerLocked(true));
    assert(!layerManagementController.moveSelectedObjectToLayer("layer_2"));
    assert(layerManagementController.setActiveLayerLocked(false));

    DrawingDocumentController layerOrderController;
    layerOrderController.setSelectedToolId("point_tool");
    layerOrderController.clickCanvasNormalized(0.1, 0.1);
    assert(layerOrderController.createLayer());
    assert(layerOrderController.activeLayerId() == "layer_2");
    layerOrderController.clickCanvasNormalized(0.2, 0.2);
    QVariantMap layerOrderModel = layerOrderController.modelDocument();
    QVariantList orderedLayers = layerOrderModel.value("layers").toList();
    assert(orderedLayers[0].toMap().value("id").toString() == "default");
    assert(orderedLayers[1].toMap().value("id").toString() == "layer_2");
    QVariantList orderedObjects = layerOrderModel.value("drawing_objects").toList();
    assert(orderedObjects[0].toMap().value("layer_id").toString() == "default");
    assert(orderedObjects[1].toMap().value("layer_id").toString() == "layer_2");
    assert(layerOrderController.moveActiveLayer("down"));
    layerOrderModel = layerOrderController.modelDocument();
    orderedLayers = layerOrderModel.value("layers").toList();
    assert(orderedLayers[0].toMap().value("id").toString() == "layer_2");
    assert(orderedLayers[1].toMap().value("id").toString() == "default");
    orderedObjects = layerOrderModel.value("drawing_objects").toList();
    assert(orderedObjects[0].toMap().value("layer_id").toString() == "layer_2");
    assert(orderedObjects[1].toMap().value("layer_id").toString() == "default");
    assert(!layerOrderController.moveActiveLayer("sideways"));

    DrawingDocumentController layerPlotController;
    assert(layerPlotController.createLayer());
    assert(layerPlotController.setActiveLayerPenPreset("pen_blue"));
    assert(layerPlotController.setActiveLayerStrokeWidthPreset("fine"));
    layerPlotController.setSelectedToolId("point_tool");
    layerPlotController.clickCanvasNormalized(0.2, 0.2);
    QVariantMap layerPlotModel = layerPlotController.modelDocument();
    QVariantMap layerPlotPoint = layerPlotModel.value("drawing_objects").toList().front().toMap();
    assert(layerPlotPoint.value("layer_id").toString() == "layer_2");
    assert(layerPlotPoint.value("effective_plot_enabled").toBool());
    assert(layerPlotPoint.value("effective_plot_ready").toBool());
    assert(layerPlotPoint.value("plot_ready").toBool());
    assert(!layerPlotPoint.value("plot_blocked").toBool());
    assert(layerPlotPoint.value("plot_safety_state").toString() == "ready");
    assert(layerPlotPoint.value("plot_warning_count").toInt() == 0);
    assert(layerPlotPoint.value("effective_pen_id").toString() == "pen_blue");
    assert(layerPlotPoint.value("effective_stroke_color").toString() == "#75c7ff");
    assert(nearlyEqual(layerPlotPoint.value("effective_stroke_width").toDouble(), 1.0));
    QVariantMap layerPlotSummary = layerPlotModel.value("plot_summary").toMap();
    assert(layerPlotSummary.value("order_mode").toString() == "layer_order");
    assert(layerPlotSummary.value("direction_mode").toString() == "preserve_direction");
    assert(layerPlotSummary.value("plot_object_count").toInt() == 1);
    assert(layerPlotSummary.value("segment_count").toInt() == 2);
    assert(layerPlotSummary.value("travel_segment_count").toInt() == 1);
    assert(layerPlotSummary.value("travel_distance").toDouble() > 0.0);
    QVariantList layerStats = layerPlotSummary.value("layer_stats").toList();
    assert(layerStats.size() == 1);
    QVariantMap layerStatsEntry = layerStats.front().toMap();
    assert(layerStatsEntry.value("layer_id").toString() == "layer_2");
    assert(layerStatsEntry.value("object_count").toInt() == 1);
    assert(layerStatsEntry.value("segment_count").toInt() == 2);
    assert(layerStatsEntry.value("stroke_distance").toDouble() > 0.0);
    assert(layerStatsEntry.value("travel_distance").toDouble() > 0.0);
    assert(layerStatsEntry.value("ready").toBool());
    assert(layerStatsEntry.value("blocked_reason").toString() == "ready");
    QVariantList penStats = layerPlotSummary.value("pen_stats").toList();
    assert(penStats.size() == 1);
    QVariantMap penStatsEntry = penStats.front().toMap();
    assert(penStatsEntry.value("pen_id").toString() == "pen_blue");
    assert(penStatsEntry.value("object_count").toInt() == 1);
    assert(penStatsEntry.value("segment_count").toInt() == 2);
    assert(penStatsEntry.value("stroke_distance").toDouble() > 0.0);
    assert(penStatsEntry.value("travel_distance").toDouble() > 0.0);
    assert(penStatsEntry.value("ready").toBool());
    assert(penStatsEntry.value("blocked_reason").toString() == "ready");
    QVariantMap layerPlotPreview = layerPlotSummary.value("preview").toMap();
    assert(layerPlotPreview.value("order_mode").toString() == "layer_order");
    assert(layerPlotPreview.value("direction_mode").toString() == "preserve_direction");
    assert(layerPlotPreview.value("segment_count").toInt() == 2);
    assert(layerPlotPreview.value("travel_segment_count").toInt() == 1);
    assert(layerPlotPreview.value("travel_distance").toDouble() > 0.0);
    assert(layerPlotPreview.value("segments").toList().size() == 2);
    const QVariantList layerPlotTravelSegments = layerPlotPreview.value("travel_segments").toList();
    assert(layerPlotTravelSegments.size() == 1);
    const QVariantMap layerPlotTravelSegment = layerPlotTravelSegments.front().toMap();
    assert(layerPlotTravelSegment.value("from_object_id").toString() == layerPlotPoint.value("id").toString());
    assert(layerPlotTravelSegment.value("to_object_id").toString() == layerPlotPoint.value("id").toString());
    assert(layerPlotTravelSegment.value("to_layer_id").toString() == "layer_2");
    assert(layerPlotTravelSegment.value("to_pen_id").toString() == "pen_blue");
    assert(layerPlotTravelSegment.value("distance").toDouble() > 0.0);
    assert(layerPlotSummary.value("warning_count").toInt() == 0);
    assert(layerPlotSummary.value("ready").toBool());
    assert(layerPlotSummary.value("status").toString() == "ready");
    assert(!layerPlotSummary.value("blocked").toBool());
    assert(layerPlotSummary.value("blocked_reason_count").toInt() == 0);
    assert(layerPlotController.plotOrderModeId() == "layer_order");
    layerPlotController.setPlotOrderModeId("nearest_next");
    assert(layerPlotController.plotOrderModeId() == "nearest_next");
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    assert(layerPlotSummary.value("order_mode").toString() == "nearest_next");
    assert(layerPlotSummary.value("preview").toMap().value("order_mode").toString() == "nearest_next");
    layerPlotController.setPlotOrderModeId("unknown");
    assert(layerPlotController.plotOrderModeId() == "layer_order");
    assert(layerPlotController.plotDirectionModeId() == "preserve_direction");
    layerPlotController.setPlotDirectionModeId("allow_reverse_segments");
    assert(layerPlotController.plotDirectionModeId() == "allow_reverse_segments");
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    assert(layerPlotSummary.value("direction_mode").toString() == "allow_reverse_segments");
    assert(layerPlotSummary.value("preview").toMap().value("direction_mode").toString() == "allow_reverse_segments");
    layerPlotController.setPlotDirectionModeId("unknown");
    assert(layerPlotController.plotDirectionModeId() == "preserve_direction");
    assert(layerPlotController.setActiveLayerPlotEnabled(false));
    layerPlotPoint = layerPlotController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(!layerPlotPoint.value("effective_plot_enabled").toBool());
    assert(!layerPlotPoint.value("effective_plot_ready").toBool());
    assert(!layerPlotPoint.value("plot_ready").toBool());
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    assert(layerPlotSummary.value("plot_object_count").toInt() == 0);
    assert(layerPlotSummary.value("segment_count").toInt() == 0);
    layerStats = layerPlotSummary.value("layer_stats").toList();
    assert(layerStats.size() == 1);
    layerStatsEntry = layerStats.front().toMap();
    assert(layerStatsEntry.value("layer_id").toString() == "layer_2");
    assert(!layerStatsEntry.value("ready").toBool());
    assert(layerStatsEntry.value("blocked_reason").toString() == "plot_disabled");
    penStats = layerPlotSummary.value("pen_stats").toList();
    assert(penStats.size() == 1);
    penStatsEntry = penStats.front().toMap();
    assert(penStatsEntry.value("pen_id").toString() == "pen_blue");
    assert(!penStatsEntry.value("ready").toBool());
    assert(penStatsEntry.value("blocked_reason").toString() == "no_assigned_segments");
    assert(layerPlotSummary.value("travel_segment_count").toInt() == 0);
    assert(nearlyEqual(layerPlotSummary.value("travel_distance").toDouble(), 0.0));
    assert(layerPlotController.setActiveLayerPlotEnabled(true));
    layerPlotController.setSelectedToolId("horizontal_guide_tool");
    layerPlotController.clickCanvasNormalized(0.4, 0.4);
    QVariantMap layerPlotGuide = layerPlotController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(layerPlotGuide.value("kind").toString() == "guide");
    assert(layerPlotGuide.value("effective_plot_enabled").toBool());
    assert(!layerPlotGuide.value("effective_plot_ready").toBool());
    assert(!layerPlotGuide.value("plot_ready").toBool());
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    assert(layerPlotSummary.value("plot_object_count").toInt() == 1);
    assert(layerPlotSummary.value("segment_count").toInt() == 2);
    assert(layerPlotSummary.value("travel_segment_count").toInt() == 1);
    layerPlotController.setSelectedToolId("point_tool");
    layerPlotController.clickCanvasNormalized(0.0, 0.0);
    QVariantMap blockedPlotModel = layerPlotController.modelDocument();
    layerPlotSummary = blockedPlotModel.value("plot_summary").toMap();
    assert(layerPlotSummary.value("plot_object_count").toInt() == 2);
    assert(layerPlotSummary.value("segment_count").toInt() == 4);
    assert(layerPlotSummary.value("travel_segment_count").toInt() == 3);
    assert(layerPlotSummary.value("travel_distance").toDouble() > 0.0);
    assert(layerPlotSummary.value("warning_count").toInt() == 1);
    assert(!layerPlotSummary.value("ready").toBool());
    assert(layerPlotSummary.value("status").toString() == "blocked");
    assert(layerPlotSummary.value("blocked").toBool());
    assert(layerPlotSummary.value("blocked_reason_count").toInt() == 1);
    assert(layerPlotSummary.value("blocked_reasons").toList().front().toString() == "raw_out_of_drawable_bounds");
    assert(layerPlotSummary.value("first_warning_kind").toString() == "raw_out_of_drawable_bounds");
    const QString blockedObjectId = layerPlotSummary.value("first_warning_object_id").toString();
    assert(!blockedObjectId.isEmpty());
    QVariantMap blockedObject;
    for (const QVariant &objectValue : blockedPlotModel.value("drawing_objects").toList()) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == blockedObjectId) {
            blockedObject = object;
            break;
        }
    }
    assert(!blockedObject.isEmpty());
    assert(blockedObject.value("plot_blocked").toBool());
    assert(blockedObject.value("plot_safety_state").toString() == "blocked");
    assert(blockedObject.value("plot_warning_count").toInt() == 1);
    assert(blockedObject.value("plot_warning_kind").toString() == "raw_out_of_drawable_bounds");
    assert(blockedObject.value("outside_drawable").toBool());
    assert(!blockedObject.value("calibrated_outside_drawable").toBool());
    assert(blockedPlotModel.value("warnings").toList().size() == 1);

    DrawingDocumentController fitNoSelectionController;
    assert(!fitNoSelectionController.fitSelectionToDrawableBounds());

    DrawingDocumentController fitInsideController;
    fitInsideController.setSelectedToolId("point_tool");
    fitInsideController.clickCanvasNormalized(0.5, 0.5);
    QVariantMap fitInsideModel = fitInsideController.modelDocument();
    const int fitInsideRevision = fitInsideModel.value("revision").toInt();
    assert(fitInsideModel.value("selection_drawable_relation").toString() == "inside");
    assert(fitInsideController.fitSelectionToDrawableBounds());
    fitInsideModel = fitInsideController.modelDocument();
    QVariantMap fitInsidePoint = fitInsideModel.value("drawing_objects").toList().front().toMap();
    assert(fitInsideModel.value("revision").toInt() == fitInsideRevision);
    assert(nearlyEqual(fitInsidePoint.value("x").toDouble(), 0.5));
    assert(nearlyEqual(fitInsidePoint.value("y").toDouble(), 0.5));

    DrawingDocumentController fitOutsideController;
    fitOutsideController.setSelectedToolId("line_tool");
    fitOutsideController.clickCanvasNormalized(0.0, 0.5);
    fitOutsideController.clickCanvasNormalized(0.1, 0.5);
    QVariantMap fitOutsideModel = fitOutsideController.modelDocument();
    assert(fitOutsideModel.value("plot_summary").toMap().value("blocked").toBool());
    assert(fitOutsideModel.value("selection_drawable_relation").toString() == "partially_outside");
    assert(fitOutsideController.fitSelectionToDrawableBounds());
    fitOutsideModel = fitOutsideController.modelDocument();
    QVariantMap fitOutsideLine = fitOutsideModel.value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(fitOutsideLine.value("x1").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(fitOutsideLine.value("x2").toDouble(), squareQuarterInchStep + 0.1));
    assert(nearlyEqual(fitOutsideLine.value("y1").toDouble(), 0.5));
    assert(nearlyEqual(fitOutsideLine.value("y2").toDouble(), 0.5));
    assert(!fitOutsideModel.value("plot_summary").toMap().value("blocked").toBool());

    DrawingDocumentController fitPointMarkController;
    fitPointMarkController.setSelectedToolId("point_tool");
    fitPointMarkController.clickCanvasNormalized(0.0, 0.0);
    QVariantMap fitPointMarkModel = fitPointMarkController.modelDocument();
    assert(fitPointMarkModel.value("plot_summary").toMap().value("blocked").toBool());
    assert(fitPointMarkController.fitSelectionToDrawableBounds());
    fitPointMarkModel = fitPointMarkController.modelDocument();
    QVariantMap fitPointMark = fitPointMarkModel.value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(fitPointMark.value("x").toDouble(), squareQuarterInchStep + 0.005));
    assert(nearlyEqual(fitPointMark.value("y").toDouble(), squareQuarterInchStep + 0.005));
    assert(!fitPointMarkModel.value("plot_summary").toMap().value("blocked").toBool());
    assert(fitPointMarkModel.value("has_selection_plot_bounds").toBool());
    QVariantMap fitPointMarkSelectionBounds = fitPointMarkModel.value("selection_plot_bounds").toMap();
    assert(nearlyEqual(fitPointMarkSelectionBounds.value("x").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(fitPointMarkSelectionBounds.value("y").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(fitPointMarkSelectionBounds.value("width").toDouble(), 0.01));
    assert(nearlyEqual(fitPointMarkSelectionBounds.value("height").toDouble(), 0.01));
    assert(nearlyEqual(fitPointMarkModel.value("selection_plot_bounds_width").toDouble(), 0.01));
    assert(nearlyEqual(fitPointMarkModel.value("selection_plot_bounds_height").toDouble(), 0.01));
    assert(fitPointMarkModel.value("selection_plot_bounds_status").toString() == "inside");
    assert(fitPointMarkModel.value("selection_drawable_relation").toString() == "inside");

    DrawingDocumentController fitTooLargeController;
    fitTooLargeController.setSelectedToolId("line_tool");
    fitTooLargeController.clickCanvasNormalized(0.0, 0.5);
    fitTooLargeController.clickCanvasNormalized(1.0, 0.5);
    const int fitTooLargeRevision = fitTooLargeController.modelDocument().value("revision").toInt();
    assert(fitTooLargeController.modelDocument().value("selection_drawable_relation").toString() == "too_large");
    assert(!fitTooLargeController.fitSelectionToDrawableBounds());
    assert(fitTooLargeController.modelDocument().value("revision").toInt() == fitTooLargeRevision);

    DrawingDocumentController fullyOutsideController;
    fullyOutsideController.setSelectedToolId("point_tool");
    fullyOutsideController.clickCanvasNormalized(0.5, 0.5);
    assert(fullyOutsideController.updateSelectedObjectGeometryField("x", 2.0));
    assert(fullyOutsideController.updateSelectedObjectGeometryField("y", 2.0));
    assert(fullyOutsideController.modelDocument().value("selection_drawable_relation").toString() == "fully_outside");

    DrawingDocumentController centerDrawableController;
    centerDrawableController.setSelectedToolId("line_tool");
    centerDrawableController.clickCanvasNormalized(0.0, 0.5);
    centerDrawableController.clickCanvasNormalized(0.1, 0.5);
    assert(centerDrawableController.centerSelectionInDrawable());
    QVariantMap centeredLine = centerDrawableController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(centeredLine.value("x1").toDouble(), 0.45));
    assert(nearlyEqual(centeredLine.value("x2").toDouble(), 0.55));
    assert(nearlyEqual(centeredLine.value("y1").toDouble(), 0.5));
    assert(nearlyEqual(centeredLine.value("y2").toDouble(), 0.5));
    assert(centerDrawableController.modelDocument().value("selection_drawable_relation").toString() == "inside");

    DrawingDocumentController originDrawableController;
    originDrawableController.setSelectedToolId("line_tool");
    originDrawableController.clickCanvasNormalized(0.5, 0.5);
    originDrawableController.clickCanvasNormalized(0.6, 0.7);
    assert(originDrawableController.moveSelectionToDrawableOrigin());
    QVariantMap originLine = originDrawableController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(originLine.value("x1").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(originLine.value("y1").toDouble(), squareQuarterInchStep));
    assert(nearlyEqual(originLine.value("x2").toDouble(), squareQuarterInchStep + 0.1));
    assert(nearlyEqual(originLine.value("y2").toDouble(), squareQuarterInchStep + 0.2));

    DrawingDocumentController fitLockedController;
    fitLockedController.setSelectedToolId("point_tool");
    fitLockedController.clickCanvasNormalized(0.0, 0.0);
    assert(fitLockedController.setSelectedObjectLocked(true));
    const int fitLockedRevision = fitLockedController.modelDocument().value("revision").toInt();
    assert(!fitLockedController.fitSelectionToDrawableBounds());
    assert(fitLockedController.modelDocument().value("revision").toInt() == fitLockedRevision);

    DrawingDocumentController fitNonPlottingController;
    fitNonPlottingController.setSelectedToolId("horizontal_guide_tool");
    fitNonPlottingController.clickCanvasNormalized(0.0, 0.0);
    const int fitNonPlottingRevision = fitNonPlottingController.modelDocument().value("revision").toInt();
    assert(!fitNonPlottingController.fitSelectionToDrawableBounds());
    assert(fitNonPlottingController.modelDocument().value("revision").toInt() == fitNonPlottingRevision);

    DrawingDocumentController safeNudgeNoSelectionController;
    assert(!safeNudgeNoSelectionController.nudgeSelectionInsideDrawable("right", "grid"));
    assert(!safeNudgeNoSelectionController.nudgeSelectionInsideDrawable("diagonal", "grid"));

    DrawingDocumentController safeNudgeController;
    safeNudgeController.setSelectedToolId("point_tool");
    safeNudgeController.clickCanvasNormalized(0.5, 0.5);
    assert(safeNudgeController.nudgeSelectionInsideDrawable("right", "grid"));
    QVariantMap safeNudgedPoint = safeNudgeController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(safeNudgedPoint.value("x").toDouble(), 0.5 + squareQuarterInchStep));
    assert(nearlyEqual(safeNudgedPoint.value("y").toDouble(), 0.5));

    DrawingDocumentController safeNudgeBlockedController;
    safeNudgeBlockedController.setSelectedToolId("point_tool");
    safeNudgeBlockedController.clickCanvasNormalized(0.0, 0.0);
    assert(safeNudgeBlockedController.fitSelectionToDrawableBounds());
    const int safeNudgeBlockedRevision = safeNudgeBlockedController.modelDocument().value("revision").toInt();
    assert(!safeNudgeBlockedController.nudgeSelectionInsideDrawable("left", "grid"));
    QVariantMap safeNudgeBlockedPoint = safeNudgeBlockedController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(safeNudgeBlockedController.modelDocument().value("revision").toInt() == safeNudgeBlockedRevision);
    assert(nearlyEqual(safeNudgeBlockedPoint.value("x").toDouble(), squareQuarterInchStep + 0.005));

    DrawingDocumentController safeNudgeLockedController;
    safeNudgeLockedController.setSelectedToolId("point_tool");
    safeNudgeLockedController.clickCanvasNormalized(0.5, 0.5);
    assert(safeNudgeLockedController.setSelectedObjectLocked(true));
    const int safeNudgeLockedRevision = safeNudgeLockedController.modelDocument().value("revision").toInt();
    assert(!safeNudgeLockedController.nudgeSelectionInsideDrawable("right", "grid"));
    assert(safeNudgeLockedController.modelDocument().value("revision").toInt() == safeNudgeLockedRevision);

    DrawingDocumentController safeNudgeNonPlottingController;
    safeNudgeNonPlottingController.setSelectedToolId("horizontal_guide_tool");
    safeNudgeNonPlottingController.clickCanvasNormalized(0.5, 0.5);
    const int safeNudgeNonPlottingRevision = safeNudgeNonPlottingController.modelDocument().value("revision").toInt();
    assert(!safeNudgeNonPlottingController.nudgeSelectionInsideDrawable("right", "grid"));
    assert(safeNudgeNonPlottingController.modelDocument().value("revision").toInt() == safeNudgeNonPlottingRevision);

    DrawingDocumentController calibrationController;
    assert(calibrationController.createCalibrationPattern("test_square"));
    QVariantMap calibrationModel = calibrationController.modelDocument();
    QVariantList calibrationObjects = calibrationModel.value("drawing_objects").toList();
    assert(calibrationObjects.size() == 1);
    QVariantMap calibrationSquare = calibrationObjects.front().toMap();
    assert(calibrationSquare.value("kind").toString() == "rectangle");
    assert(calibrationSquare.value("layer_id").toString() == "default");
    assert(nearlyEqual(calibrationSquare.value("x").toDouble(), 0.15));
    assert(nearlyEqual(calibrationSquare.value("y").toDouble(), 0.15));
    assert(nearlyEqual(calibrationSquare.value("width").toDouble(), 0.24));
    assert(nearlyEqual(calibrationSquare.value("height").toDouble(), 0.24));
    assert(calibrationModel.value("selected_object_ids").toList().size() == 1);
    assert(calibrationController.recordCalibrationMeasurement(0.238));
    calibrationModel = calibrationController.modelDocument();
    QVariantMap calibrationMeasurement = calibrationModel.value("calibration_measurement").toMap();
    assert(calibrationMeasurement.value("pattern_id").toString() == "calibration_square");
    assert(nearlyEqual(calibrationMeasurement.value("expected_value").toDouble(), 0.24));
    assert(nearlyEqual(calibrationMeasurement.value("measured_value").toDouble(), 0.238));
    assert(nearlyEqual(calibrationMeasurement.value("error_value").toDouble(), -0.002));
    assert(calibrationMeasurement.value("percent_error").toDouble() < 0.0);
    QVariantMap calibrationCorrection = calibrationModel.value("calibration_correction").toMap();
    assert(calibrationCorrection.value("ok").toBool());
    assert(nearlyEqual(calibrationCorrection.value("scale_factor").toDouble(), 0.24 / 0.238));
    assert(calibrationCorrection.value("correction_percent").toDouble() > 0.0);
    assert(nearlyEqual(calibrationModel.value("plot_summary").toMap().value("calibration_scale").toDouble(), 1.0));
    calibrationSquare = calibrationModel.value("drawing_objects").toList().front().toMap();
    assert(calibrationSquare.value("measurement_note").toString().contains("calibration_square"));
    const double calibrationSquareXBeforeApply = calibrationSquare.value("x").toDouble();
    assert(calibrationController.applyCalibrationCorrection());
    calibrationModel = calibrationController.modelDocument();
    const double appliedCalibrationScale = 0.24 / 0.238;
    const QVariantMap appliedCalibrationPlot = calibrationModel.value("plot_summary").toMap();
    assert(nearlyEqual(appliedCalibrationPlot.value("calibration_scale").toDouble(), appliedCalibrationScale));
    assert(appliedCalibrationPlot.value("has_plot_bounds").toBool());
    const QVariantMap appliedCalibrationPlotBounds = appliedCalibrationPlot.value("plot_bounds").toMap();
    assert(nearlyEqual(appliedCalibrationPlotBounds.value("x").toDouble(), 0.15 * appliedCalibrationScale));
    assert(nearlyEqual(appliedCalibrationPlotBounds.value("width").toDouble(), 0.24 * appliedCalibrationScale));
    const QVariantList appliedCalibrationSegments = appliedCalibrationPlot.value("preview").toMap().value("segments").toList();
    assert(!appliedCalibrationSegments.isEmpty());
    const QVariantMap appliedCalibrationSegment = appliedCalibrationSegments.front().toMap();
    assert(nearlyEqual(appliedCalibrationSegment.value("raw_x1").toDouble(), 0.15));
    assert(nearlyEqual(appliedCalibrationSegment.value("raw_x2").toDouble(), 0.39));
    assert(nearlyEqual(appliedCalibrationSegment.value("x1").toDouble(), 0.15 * appliedCalibrationScale));
    assert(nearlyEqual(appliedCalibrationSegment.value("calibrated_x2").toDouble(), 0.39 * appliedCalibrationScale));
    calibrationSquare = calibrationModel.value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(calibrationSquare.value("x").toDouble(), calibrationSquareXBeforeApply));

    assert(calibrationController.createLayer());
    assert(calibrationController.activeLayerId() == "layer_2");
    assert(calibrationController.createCalibrationPattern("test_circle"));
    calibrationModel = calibrationController.modelDocument();
    calibrationObjects = calibrationModel.value("drawing_objects").toList();
    assert(calibrationObjects.size() == 2);
    QVariantMap calibrationCircle = calibrationObjects.back().toMap();
    assert(calibrationCircle.value("kind").toString() == "circle");
    assert(calibrationCircle.value("layer_id").toString() == "layer_2");
    assert(nearlyEqual(calibrationCircle.value("cx").toDouble(), 0.27));
    assert(nearlyEqual(calibrationCircle.value("cy").toDouble(), 0.27));
    assert(nearlyEqual(calibrationCircle.value("radius").toDouble(), 0.12));

    assert(calibrationController.createCalibrationPattern("line_spacing"));
    calibrationModel = calibrationController.modelDocument();
    calibrationObjects = calibrationModel.value("drawing_objects").toList();
    assert(calibrationObjects.size() == 7);
    QVariantList calibrationSelection = calibrationModel.value("selected_object_ids").toList();
    assert(calibrationSelection.size() == 5);
    for (int index = 2; index < calibrationObjects.size(); ++index) {
        const QVariantMap line = calibrationObjects[index].toMap();
        assert(line.value("kind").toString() == "line");
        assert(line.value("layer_id").toString() == "layer_2");
        assert(nearlyEqual(line.value("x1").toDouble(), 0.15));
        assert(nearlyEqual(line.value("x2").toDouble(), 0.39));
    }
    assert(calibrationController.recordCalibrationMeasurement(0.041));
    calibrationModel = calibrationController.modelDocument();
    calibrationMeasurement = calibrationModel.value("calibration_measurement").toMap();
    assert(calibrationMeasurement.value("pattern_id").toString() == "calibration_line_spacing");
    assert(nearlyEqual(calibrationMeasurement.value("expected_value").toDouble(), 0.04));
    assert(nearlyEqual(calibrationMeasurement.value("measured_value").toDouble(), 0.041));
    assert(calibrationMeasurement.value("percent_error").toDouble() > 0.0);
    calibrationCorrection = calibrationModel.value("calibration_correction").toMap();
    assert(nearlyEqual(calibrationCorrection.value("scale_factor").toDouble(), 0.04 / 0.041));
    assert(nearlyEqual(calibrationModel.value("plot_summary").toMap().value("calibration_scale").toDouble(), 0.24 / 0.238));
    calibrationObjects = calibrationModel.value("drawing_objects").toList();
    for (int index = 2; index < calibrationObjects.size(); ++index) {
        const QVariantMap line = calibrationObjects[index].toMap();
        assert(line.value("measurement_note").toString().contains("calibration_line_spacing"));
    }
    QVariantMap calibrationPlotSummary = calibrationModel.value("plot_summary").toMap();
    assert(calibrationPlotSummary.value("plot_object_count").toInt() == 7);
    assert(calibrationPlotSummary.value("segment_count").toInt() > 7);
    assert(!calibrationPlotSummary.value("blocked").toBool());
    assert(calibrationController.setActiveLayerLocked(true));
    assert(!calibrationController.createCalibrationPattern("test_square"));

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

    DrawingDocumentController arrangeController;
    arrangeController.setSelectedToolId("point_tool");
    arrangeController.clickCanvasNormalized(0.2, 0.2);
    arrangeController.clickCanvasNormalized(0.5, 0.7);
    arrangeController.clickCanvasNormalized(0.8, 0.4);
    assert(arrangeController.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0));
    assert(arrangeController.alignSelection("left"));
    QVariantList arrangedObjects = arrangeController.modelDocument().value("drawing_objects").toList();
    assert(arrangedObjects.size() == 3);
    assert(nearlyEqual(arrangedObjects[0].toMap().value("x").toDouble(), 0.2));
    assert(nearlyEqual(arrangedObjects[1].toMap().value("x").toDouble(), 0.2));
    assert(nearlyEqual(arrangedObjects[2].toMap().value("x").toDouble(), 0.2));
    assert(!arrangeController.alignSelection("diagonal"));
    assert(arrangeController.distributeSelection("y"));
    arrangedObjects = arrangeController.modelDocument().value("drawing_objects").toList();
    assert(nearlyEqual(arrangedObjects[0].toMap().value("y").toDouble(), 0.2));
    assert(nearlyEqual(arrangedObjects[1].toMap().value("y").toDouble(), 0.7));
    assert(nearlyEqual(arrangedObjects[2].toMap().value("y").toDouble(), 0.45));
    assert(!arrangeController.distributeSelection("diagonal"));

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

    DrawingDocumentController guideLinePreviewController;
    guideLinePreviewController.setSelectedToolId("horizontal_guide_tool");
    guideLinePreviewController.clickCanvasNormalized(0.2, 0.75);
    guideLinePreviewController.setSelectedToolId("vertical_guide_tool");
    guideLinePreviewController.clickCanvasNormalized(0.33, 0.2);
    guideLinePreviewController.setObjectSnapEnabled(true);
    guideLinePreviewController.setSelectedToolId("line_tool");
    guideLinePreviewController.clickCanvasNormalized(0.34, 0.74);
    guideLinePreviewController.updateCreationPreviewNormalized(0.52, 0.74);
    QVariantMap guideLinePreview = guideLinePreviewController.modelDocument().value("preview_object").toMap();
    assert(guideLinePreview.value("kind").toString() == "line");
    assert(nearlyEqual(guideLinePreview.value("x1").toDouble(), 0.33));
    assert(nearlyEqual(guideLinePreview.value("y1").toDouble(), 0.75));
    assert(nearlyEqual(guideLinePreview.value("x2").toDouble(), 0.52));
    assert(nearlyEqual(guideLinePreview.value("y2").toDouble(), 0.75));
    guideLinePreviewController.clickCanvasNormalized(0.52, 0.74);
    QVariantMap guideLineCommitted = guideLinePreviewController.modelDocument().value("drawing_objects").toList().back().toMap();
    assert(guideLineCommitted.value("kind").toString() == "line");
    assert(nearlyEqual(guideLineCommitted.value("x1").toDouble(), 0.33));
    assert(nearlyEqual(guideLineCommitted.value("y1").toDouble(), 0.75));
    assert(nearlyEqual(guideLineCommitted.value("x2").toDouble(), 0.52));
    assert(nearlyEqual(guideLineCommitted.value("y2").toDouble(), 0.75));

    DrawingDocumentController guideRectanglePreviewController;
    guideRectanglePreviewController.setSelectedToolId("horizontal_guide_tool");
    guideRectanglePreviewController.clickCanvasNormalized(0.2, 0.75);
    guideRectanglePreviewController.setSelectedToolId("vertical_guide_tool");
    guideRectanglePreviewController.clickCanvasNormalized(0.33, 0.2);
    guideRectanglePreviewController.setObjectSnapEnabled(true);
    guideRectanglePreviewController.setSelectedToolId("rectangle_tool");
    guideRectanglePreviewController.clickCanvasNormalized(0.34, 0.70);
    guideRectanglePreviewController.updateCreationPreviewNormalized(0.52, 0.74);
    QVariantMap guideRectanglePreview = guideRectanglePreviewController.modelDocument().value("preview_object").toMap();
    assert(guideRectanglePreview.value("kind").toString() == "rectangle");
    assert(nearlyEqual(guideRectanglePreview.value("x").toDouble(), 0.33));
    assert(nearlyEqual(guideRectanglePreview.value("y").toDouble(), 0.70));
    assert(nearlyEqual(guideRectanglePreview.value("width").toDouble(), 0.19));
    assert(nearlyEqual(guideRectanglePreview.value("height").toDouble(), 0.05));

    DrawingDocumentController guideCirclePreviewController;
    guideCirclePreviewController.setSelectedToolId("horizontal_guide_tool");
    guideCirclePreviewController.clickCanvasNormalized(0.2, 0.75);
    guideCirclePreviewController.setSelectedToolId("vertical_guide_tool");
    guideCirclePreviewController.clickCanvasNormalized(0.33, 0.2);
    guideCirclePreviewController.setObjectSnapEnabled(true);
    guideCirclePreviewController.setSelectedToolId("circle_tool");
    guideCirclePreviewController.clickCanvasNormalized(0.34, 0.74);
    guideCirclePreviewController.updateCreationPreviewNormalized(0.43, 0.74);
    QVariantMap guideCirclePreview = guideCirclePreviewController.modelDocument().value("preview_object").toMap();
    assert(guideCirclePreview.value("kind").toString() == "circle");
    assert(nearlyEqual(guideCirclePreview.value("cx").toDouble(), 0.33));
    assert(nearlyEqual(guideCirclePreview.value("cy").toDouble(), 0.75));
    assert(nearlyEqual(guideCirclePreview.value("radius").toDouble(), 0.10));

    DrawingDocumentController disabledGuideLinePreviewController;
    disabledGuideLinePreviewController.setSelectedToolId("horizontal_guide_tool");
    disabledGuideLinePreviewController.clickCanvasNormalized(0.2, 0.75);
    disabledGuideLinePreviewController.setSelectedToolId("vertical_guide_tool");
    disabledGuideLinePreviewController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideLinePreviewController.setObjectSnapEnabled(true);
    disabledGuideLinePreviewController.setGuideSnapEnabled(false);
    disabledGuideLinePreviewController.setSelectedToolId("line_tool");
    disabledGuideLinePreviewController.clickCanvasNormalized(0.34, 0.74);
    disabledGuideLinePreviewController.updateCreationPreviewNormalized(0.52, 0.74);
    QVariantMap disabledGuideLinePreview = disabledGuideLinePreviewController.modelDocument().value("preview_object").toMap();
    assert(disabledGuideLinePreview.value("kind").toString() == "line");
    assert(nearlyEqual(disabledGuideLinePreview.value("x1").toDouble(), 0.34));
    assert(nearlyEqual(disabledGuideLinePreview.value("y1").toDouble(), 0.74));
    assert(nearlyEqual(disabledGuideLinePreview.value("x2").toDouble(), 0.52));
    assert(nearlyEqual(disabledGuideLinePreview.value("y2").toDouble(), 0.74));

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

    DrawingDocumentController numericRectController;
    numericRectController.setSelectedToolId("rectangle_tool");
    numericRectController.clickCanvasNormalized(0.1, 0.1);
    numericRectController.clickCanvasNormalized(0.4, 0.4);
    assert(numericRectController.updateSelectedObjectGeometryField("width", 0.5));
    assert(numericRectController.updateSelectedObjectGeometryField("height", 0.25));
    assert(numericRectController.updateSelectedObjectGeometryField("rotation_deg", 45.0));
    QVariantMap numericRect = numericRectController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(numericFieldIds(numericRect) == QStringList({
        QStringLiteral("x"),
        QStringLiteral("y"),
        QStringLiteral("width"),
        QStringLiteral("height"),
        QStringLiteral("rotation_deg"),
    }));
    QVariantMap numericRectPhysical = numericRect.value("physical_geometry").toMap();
    assert(numericRectPhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(numericRectPhysical.value("width").toDouble(), 6.0));
    assert(nearlyEqual(numericRectPhysical.value("height").toDouble(), 3.0));
    assert(nearlyEqual(numericRectPhysical.value("rotation_deg").toDouble(), 45.0));
    assert(nearlyEqual(numericRect.value("width").toDouble(), 0.5));
    assert(nearlyEqual(numericRect.value("height").toDouble(), 0.25));
    assert(nearlyEqual(numericRect.value("rotation_deg").toDouble(), 45.0));
    QVariantList numericRectMeasurement = numericRect.value("measurement_lines").toList();
    assert(numericRectMeasurement.size() == 3);
    assert(numericRectMeasurement[0].toString().startsWith("area: "));
    assert(!numericRectController.updateSelectedObjectGeometryField("width", -0.1));
    QVariantMap rectEditStatus = editStatus(numericRectController);
    assert(!rectEditStatus.value("ok").toBool());
    assert(rectEditStatus.value("mode").toString() == "normalized");
    assert(rectEditStatus.value("field_id").toString() == "width");
    assert(rectEditStatus.value("code").toString() == "invalid_geometry");
    assert(rectEditStatus.value("message").toString() == "rectangle dimensions must be non-negative");
    QVariantMap numericRectAfterInvalid = numericRectController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(numericRectAfterInvalid.value("width").toDouble(), 0.5));
    assert(numericRectController.updateSelectedObjectPhysicalGeometryField("width", 3.0));
    rectEditStatus = editStatus(numericRectController);
    assert(rectEditStatus.value("ok").toBool());
    assert(rectEditStatus.value("mode").toString() == "physical");
    assert(rectEditStatus.value("field_id").toString() == "width");
    assert(rectEditStatus.value("message").toString().isEmpty());
    assert(numericRectController.updateSelectedObjectPhysicalGeometryField("height", 6.0));
    QVariantMap physicalRect = numericRectController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(physicalRect.value("width").toDouble(), 0.25));
    assert(nearlyEqual(physicalRect.value("height").toDouble(), 0.5));

    DrawingDocumentController numericCircleController;
    numericCircleController.setSelectedToolId("circle_tool");
    numericCircleController.clickCanvasNormalized(0.5, 0.5);
    numericCircleController.clickCanvasNormalized(0.7, 0.5);
    assert(numericCircleController.updateSelectedObjectGeometryField("cx", 0.4));
    assert(numericCircleController.updateSelectedObjectGeometryField("cy", 0.45));
    assert(numericCircleController.updateSelectedObjectGeometryField("radius", 0.125));
    QVariantMap numericCircle = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(numericFieldIds(numericCircle) == QStringList({
        QStringLiteral("cx"),
        QStringLiteral("cy"),
        QStringLiteral("radius"),
        QStringLiteral("diameter"),
    }));
    QVariantMap numericCirclePhysical = numericCircle.value("physical_geometry").toMap();
    assert(numericCirclePhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(numericCirclePhysical.value("radius").toDouble(), 1.5));
    assert(nearlyEqual(numericCirclePhysical.value("diameter").toDouble(), 3.0));
    assert(nearlyEqual(numericCircle.value("cx").toDouble(), 0.4));
    assert(nearlyEqual(numericCircle.value("cy").toDouble(), 0.45));
    assert(nearlyEqual(numericCircle.value("radius").toDouble(), 0.125));
    assert(nearlyEqual(numericCircle.value("diameter").toDouble(), 0.25));
    QVariantList numericCircleMeasurement = numericCircle.value("measurement_lines").toList();
    assert(numericCircleMeasurement.size() == 3);
    assert(numericCircleMeasurement[0].toString().startsWith("area: "));
    assert(numericCircleController.updateSelectedObjectGeometryField("diameter", 0.5));
    numericCircle = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(numericCircle.value("radius").toDouble(), 0.25));
    assert(nearlyEqual(numericCircle.value("diameter").toDouble(), 0.5));
    assert(!numericCircleController.updateSelectedObjectGeometryField("radius", -0.01));
    assert(!numericCircleController.updateSelectedObjectGeometryField("diameter", -0.01));
    QVariantMap numericCircleAfterInvalid = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(numericCircleAfterInvalid.value("radius").toDouble(), 0.25));
    assert(nearlyEqual(numericCircleAfterInvalid.value("diameter").toDouble(), 0.5));
    assert(numericCircleController.updateSelectedObjectPhysicalGeometryField("radius", 3.0));
    QVariantMap physicalCircle = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(physicalCircle.value("radius").toDouble(), 0.25));
    assert(nearlyEqual(physicalCircle.value("diameter").toDouble(), 0.5));
    const int circleRevisionBeforePhysicalInvalid = numericCircleController.modelDocument().value("revision").toInt();
    assert(!numericCircleController.updateSelectedObjectPhysicalGeometryField("diameter", -1.0));
    QVariantMap circleEditStatus = editStatus(numericCircleController);
    assert(!circleEditStatus.value("ok").toBool());
    assert(circleEditStatus.value("mode").toString() == "physical");
    assert(circleEditStatus.value("field_id").toString() == "diameter");
    assert(circleEditStatus.value("code").toString() == "invalid_geometry");
    assert(circleEditStatus.value("message").toString() == "circle diameter must be non-negative");
    assert(numericCircleController.modelDocument().value("revision").toInt() == circleRevisionBeforePhysicalInvalid);
    numericCircleController.setSelectedToolId("select_move");
    assert(editStatus(numericCircleController).isEmpty());

    DrawingDocumentController numericLineController;
    numericLineController.setSelectedToolId("line_tool");
    numericLineController.clickCanvasNormalized(0.1, 0.2);
    numericLineController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap numericLine = numericLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(numericFieldIds(numericLine) == QStringList({
        QStringLiteral("x1"),
        QStringLiteral("y1"),
        QStringLiteral("x2"),
        QStringLiteral("y2"),
        QStringLiteral("line_length"),
        QStringLiteral("line_angle_deg"),
    }));
    QVariantMap numericLinePhysical = numericLine.value("physical_geometry").toMap();
    assert(numericLinePhysical.value("unit_label").toString() == "in");
    assert(nearlyEqual(numericLinePhysical.value("x1").toDouble(), 1.2));
    assert(nearlyEqual(numericLinePhysical.value("y1").toDouble(), 2.4));
    assert(nearlyEqual(numericLinePhysical.value("x2").toDouble(), 4.8));
    assert(nearlyEqual(numericLinePhysical.value("y2").toDouble(), 7.2));
    assert(nearlyEqual(numericLinePhysical.value("line_length").toDouble(), 6.0));
    assert(nearlyEqual(numericLinePhysical.value("line_angle_deg").toDouble(), 53.1301023542));
    assert(nearlyEqual(numericLine.value("line_length").toDouble(), 0.5));
    assert(nearlyEqual(numericLine.value("line_angle_deg").toDouble(), 53.1301023542));
    assert(numericLineController.updateSelectedObjectPhysicalGeometryField("line_length", 12.0));
    numericLine = numericLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(numericLine.value("x1").toDouble(), 0.1));
    assert(nearlyEqual(numericLine.value("y1").toDouble(), 0.2));
    assert(nearlyEqual(numericLine.value("x2").toDouble(), 0.7));
    assert(nearlyEqual(numericLine.value("y2").toDouble(), 1.0));
    assert(nearlyEqual(numericLine.value("line_length").toDouble(), 1.0));
    assert(numericLineController.updateSelectedObjectPhysicalGeometryField("line_angle_deg", 0.0));
    numericLine = numericLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(numericLine.value("x2").toDouble(), 1.1));
    assert(nearlyEqual(numericLine.value("y2").toDouble(), 0.2));
    assert(nearlyEqual(numericLine.value("line_angle_deg").toDouble(), 0.0));
    const int lineRevisionBeforeInvalid = numericLineController.modelDocument().value("revision").toInt();
    assert(!numericLineController.updateSelectedObjectGeometryField("line_length", -0.1));
    assert(!numericLineController.updateSelectedObjectPhysicalGeometryField("line_length", -1.0));
    assert(!numericLineController.updateSelectedObjectGeometryField("line_angle_deg", std::numeric_limits<double>::infinity()));
    assert(numericLineController.modelDocument().value("revision").toInt() == lineRevisionBeforeInvalid);

    DrawingDocumentController lockedLineController;
    lockedLineController.setSelectedToolId("line_tool");
    lockedLineController.clickCanvasNormalized(0.1, 0.1);
    lockedLineController.clickCanvasNormalized(0.4, 0.4);
    QVariantMap lockedLine = lockedLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(!lockedLine.value("locked").toBool());
    assert(lockedLine.value("visible").toBool());
    assert(lockedLineController.setSelectedObjectLocked(true));
    lockedLine = lockedLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(lockedLine.value("locked").toBool());
    const int lockedRevision = lockedLineController.modelDocument().value("revision").toInt();
    assert(!lockedLineController.updateSelectedObjectGeometryField("x2", 0.8));
    assert(!lockedLineController.editSelectedHandleNormalized("line_end", 0.8, 0.8));
    assert(!lockedLineController.moveSelectionNormalized(0.1, 0.0));
    assert(!lockedLineController.offsetSelectedObject("left"));
    assert(!lockedLineController.mirrorSelectedObject("vertical"));
    assert(!lockedLineController.repeatSelectedObject("x"));
    assert(lockedLineController.modelDocument().value("revision").toInt() == lockedRevision);
    assert(lockedLineController.setSelectedObjectLocked(false));
    assert(lockedLineController.updateSelectedObjectGeometryField("x2", 0.8));
    lockedLine = lockedLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(lockedLine.value("x2").toDouble(), 0.8));

    DrawingDocumentController noSelectionEditController;
    assert(!noSelectionEditController.updateSelectedObjectGeometryField("x", 0.2));
    assert(!noSelectionEditController.setSelectedObjectLocked(true));
    assert(!noSelectionEditController.setSelectedObjectVisible(false));
    assert(!noSelectionEditController.nudgeSelection("right", "grid"));

    DrawingDocumentController nudgeController;
    nudgeController.setSelectedToolId("point_tool");
    nudgeController.clickCanvasNormalized(0.5, 0.5);
    assert(nudgeController.nudgeSelection("right", "grid"));
    assert(nudgeController.nudgeSelection("up", "fine"));
    QVariantMap nudgedPoint = nudgeController.modelDocument().value("drawing_objects").toList().front().toMap();
    assert(nearlyEqual(nudgedPoint.value("x").toDouble(), 0.5 + squareQuarterInchStep));
    assert(nearlyEqual(nudgedPoint.value("y").toDouble(), 0.5 - squareQuarterInchStep * 0.25));
    assert(!nudgeController.nudgeSelection("diagonal", "grid"));

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

    // Save/open round-trip: a document written to disk and reopened projects
    // to the same model, and the reopened controller keeps minting fresh ids.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("roundtrip.edidraw")));

        DrawingDocumentController saveController;
        saveController.setSelectedToolId("line_tool");
        saveController.clickCanvasNormalized(0.1, 0.2);
        saveController.clickCanvasNormalized(0.8, 0.9);
        saveController.setSelectedToolId("circle_tool");
        saveController.clickCanvasNormalized(0.4, 0.4);
        saveController.clickCanvasNormalized(0.6, 0.4);
        const QVariantList savedObjects = saveController.modelDocument().value("drawing_objects").toList();
        assert(savedObjects.size() == 2);
        const QString savedSelected = saveController.selectedObjectId();

        assert(saveController.saveDocument(url));

        // A second controller, mutated differently, then opens the saved file.
        DrawingDocumentController openController;
        openController.setSelectedToolId("point_tool");
        openController.clickCanvasNormalized(0.5, 0.5);
        assert(openController.modelDocument().value("drawing_objects").toList().size() == 1);

        assert(openController.openDocument(url));
        const QVariantList openedObjects = openController.modelDocument().value("drawing_objects").toList();
        assert(openedObjects.size() == 2);
        assert(openController.selectedObjectId() == savedSelected);
        // Preview/pending state is cleared by open.
        assert(!openController.modelDocument().contains("preview_object"));

        // Object ids match the saved document positionally.
        for (int i = 0; i < savedObjects.size(); ++i) {
            assert(openedObjects[i].toMap().value("id").toString()
                   == savedObjects[i].toMap().value("id").toString());
        }

        // Newly created objects after open keep minting above the highest
        // trailing serial already present (the loaded line/circle are _0001/
        // _0002, so the next id must carry a serial of at least 3).
        int highestLoadedSerial = 0;
        for (const QVariant &existing : openedObjects) {
            highestLoadedSerial = std::max(highestLoadedSerial, trailingSerial(existing.toMap().value("id").toString()));
        }
        assert(highestLoadedSerial >= 2);
        openController.setSelectedToolId("point_tool");
        openController.clickCanvasNormalized(0.3, 0.3);
        const QVariantList grownObjects = openController.modelDocument().value("drawing_objects").toList();
        assert(grownObjects.size() == 3);
        const QString newId = grownObjects.back().toMap().value("id").toString();
        for (const QVariant &existing : openedObjects) {
            assert(existing.toMap().value("id").toString() != newId);
        }
        assert(trailingSerial(newId) > highestLoadedSerial);

        // Opening a missing file fails without disturbing the document.
        const QUrl missing = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("nope.edidraw")));
        assert(!openController.openDocument(missing));
        assert(openController.modelDocument().value("drawing_objects").toList().size() == 3);
    }

    // SVG / HPGL export write files whose contents start with the right markers.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        DrawingDocumentController exportController;
        exportController.setSelectedToolId("line_tool");
        exportController.clickCanvasNormalized(0.1, 0.1);
        exportController.clickCanvasNormalized(0.9, 0.9);

        const QString svgPath = tempDir.filePath(QStringLiteral("out.svg"));
        assert(exportController.exportSvgDocument(QUrl::fromLocalFile(svgPath)));
        QFile svgFile(svgPath);
        assert(svgFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString svg = QString::fromUtf8(svgFile.readAll());
        assert(svg.startsWith(QStringLiteral("<svg")));
        assert(svg.contains(QStringLiteral("<path")));

        const QString hpglPath = tempDir.filePath(QStringLiteral("out.hpgl"));
        assert(exportController.exportHpglDocument(QUrl::fromLocalFile(hpglPath)));
        QFile hpglFile(hpglPath);
        assert(hpglFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString hpgl = QString::fromUtf8(hpglFile.readAll());
        assert(hpgl.startsWith(QStringLiteral("IN;")));
        assert(hpgl.contains(QStringLiteral("PD")));
    }

    // Dirty tracking is content-based: selecting an object after a save does NOT
    // mark the document dirty (selection is excluded, like undo).
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("dirty.edidraw")));

        DrawingDocumentController dirtyController;
        assert(!dirtyController.isDocumentDirty()); // fresh document is clean
        dirtyController.setSelectedToolId("point_tool");
        dirtyController.clickCanvasNormalized(0.2, 0.2);
        dirtyController.clickCanvasNormalized(0.8, 0.8);
        assert(dirtyController.isDocumentDirty()); // created objects -> dirty
        assert(dirtyController.saveDocument(url));
        assert(!dirtyController.isDocumentDirty()); // saved -> clean

        // Select / deselect after save: selection-only, must stay clean.
        dirtyController.setSelectedToolId("select_move");
        dirtyController.clickCanvasNormalized(0.2, 0.2);
        assert(!dirtyController.isDocumentDirty());
        dirtyController.clickCanvasNormalized(0.45, 0.05); // empty space: clears selection
        assert(!dirtyController.isDocumentDirty());

        // A real geometry change marks dirty again.
        dirtyController.clickCanvasNormalized(0.8, 0.8); // select an object
        assert(dirtyController.nudgeSelection("right", "grid"));
        assert(dirtyController.isDocumentDirty());
    }

    // Dirty tracking is epoch-based, immune to revision aliasing across undo.
    // Regression: save at revision N, undo to N-1, then a DIFFERENT edit whose
    // revision collides back to N must STILL read dirty. The old revision-equality
    // shortcut reported false-clean here and silently lost changes on close.
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("alias.edidraw")));

        DrawingDocumentController c;
        c.setSelectedToolId("point_tool");
        c.clickCanvasNormalized(0.2, 0.2); // object A (revision 1)
        c.clickCanvasNormalized(0.8, 0.8); // object B (revision 2)
        assert(c.saveDocument(url));
        assert(!c.isDocumentDirty());      // saved at revision 2 -> clean
        assert(c.undo());                  // back to {A} (revision 1)
        assert(c.isDocumentDirty());       // diverged from the saved state -> dirty
        c.clickCanvasNormalized(0.5, 0.5); // object C: revision aliases back to 2
        assert(c.isDocumentDirty());       // content differs from saved {A,B} -> dirty
    }

    // Undo/redo.
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value("drawing_objects").toList().size();
        };

        // create -> undo -> empty -> redo -> restored.
        DrawingDocumentController undoController;
        assert(!undoController.canUndo());
        assert(!undoController.canRedo());
        assert(!undoController.undo()); // nothing to undo
        undoController.setSelectedToolId("point_tool");
        undoController.clickCanvasNormalized(0.3, 0.4);
        assert(objectCount(undoController) == 1);
        assert(undoController.canUndo());
        const QString createdId = undoController.selectedObjectId();
        assert(undoController.undo());
        assert(objectCount(undoController) == 0);
        assert(!undoController.canUndo());
        assert(undoController.canRedo());
        assert(undoController.redo());
        assert(objectCount(undoController) == 1);
        assert(undoController.modelDocument().value("drawing_objects").toList().front().toMap().value("id").toString() == createdId);
        assert(!undoController.canRedo());

        // nudge twice -> undo once -> one nudge remains (each nudge is one step).
        DrawingDocumentController nudgeUndoController;
        nudgeUndoController.setSelectedToolId("point_tool");
        nudgeUndoController.clickCanvasNormalized(0.5, 0.5);
        const double startY = nudgeUndoController.modelDocument().value("drawing_objects").toList().front().toMap().value("y").toDouble();
        assert(nudgeUndoController.nudgeSelection("up", "grid"));
        assert(nudgeUndoController.nudgeSelection("up", "grid"));
        const double twiceY = nudgeUndoController.modelDocument().value("drawing_objects").toList().front().toMap().value("y").toDouble();
        assert(!nearlyEqual(twiceY, startY));
        assert(nudgeUndoController.undo());
        const double onceY = nudgeUndoController.modelDocument().value("drawing_objects").toList().front().toMap().value("y").toDouble();
        // After one undo, exactly one nudge remains: halfway between start and twice.
        assert(nearlyEqual(onceY, (startY + twiceY) / 2.0));
        assert(nudgeUndoController.canUndo()); // create + one nudge still undoable

        // A guide preset that creates several guides is a single undo step.
        DrawingDocumentController guideUndoController;
        assert(guideUndoController.applyGuidePreset("drawable_bounds"));
        const int guideObjects = guideUndoController.modelDocument().value("drawing_objects").toList().size();
        assert(guideObjects >= 2); // preset adds multiple guides
        assert(guideUndoController.canUndo());
        assert(guideUndoController.undo());
        assert(guideUndoController.modelDocument().value("drawing_objects").toList().isEmpty());
        assert(!guideUndoController.canUndo()); // exactly one step for the whole preset

        // Pure selection changes are not undoable and do not clear redo.
        DrawingDocumentController selectionUndoController;
        selectionUndoController.setSelectedToolId("point_tool");
        selectionUndoController.clickCanvasNormalized(0.2, 0.2);
        selectionUndoController.clickCanvasNormalized(0.8, 0.8);
        assert(objectCount(selectionUndoController) == 2);
        selectionUndoController.undo(); // remove second point
        assert(objectCount(selectionUndoController) == 1);
        assert(selectionUndoController.canRedo());
        // Select the remaining point: selection-only, must not clear redo or add a step.
        selectionUndoController.setSelectedToolId("select_move");
        selectionUndoController.clickCanvasNormalized(0.2, 0.2);
        assert(selectionUndoController.canRedo());
        assert(selectionUndoController.redo());
        assert(objectCount(selectionUndoController) == 2);

        // redo is cleared by a new edit.
        DrawingDocumentController redoClearController;
        redoClearController.setSelectedToolId("point_tool");
        redoClearController.clickCanvasNormalized(0.3, 0.3);
        redoClearController.clickCanvasNormalized(0.6, 0.6);
        redoClearController.undo();
        assert(redoClearController.canRedo());
        redoClearController.clickCanvasNormalized(0.9, 0.9); // new edit
        assert(!redoClearController.canRedo());

        // 100-step cap: the oldest edits drop out of the undo history.
        DrawingDocumentController capController;
        capController.setSelectedToolId("point_tool");
        for (int i = 0; i < 102; ++i) {
            capController.clickCanvasNormalized(0.1 + 0.001 * i, 0.1);
        }
        assert(capController.modelDocument().value("drawing_objects").toList().size() == 102);
        int undone = 0;
        while (capController.undo()) {
            ++undone;
        }
        assert(undone == 100); // capped at 100, the first two creations are unrecoverable
        assert(capController.modelDocument().value("drawing_objects").toList().size() == 2);

        // A drag bracket interrupted by undo must not poison later undo history.
        // Open a bracket (as a drag would), then undo mid-gesture: the bracket
        // is abandoned, and a subsequent edit must still push its own undo step.
        DrawingDocumentController leakController;
        leakController.setSelectedToolId("point_tool");
        leakController.clickCanvasNormalized(0.5, 0.5); // step 1: create
        leakController.beginInteractiveEdit();          // a drag starts...
        assert(leakController.undo());                  // ...but undo fires first
        assert(objectCount(leakController) == 0);
        // The leaked bracket is gone: a fresh edit is independently undoable.
        leakController.setSelectedToolId("point_tool");
        leakController.clickCanvasNormalized(0.4, 0.4);
        assert(objectCount(leakController) == 1);
        assert(leakController.canUndo());
        assert(leakController.undo());                  // the new edit undoes cleanly
        assert(objectCount(leakController) == 0);
    }

    // Keyboard-action controller seams: cancel, delete, duplicate, coarse nudge.
    {
        // cancelPendingCreation clears an in-flight two-click creation + preview.
        DrawingDocumentController cancelController;
        cancelController.setSelectedToolId("line_tool");
        cancelController.clickCanvasNormalized(0.2, 0.2); // first click: pending
        cancelController.updateCreationPreviewNormalized(0.6, 0.6);
        assert(cancelController.modelDocument().contains("preview_object"));
        cancelController.cancelPendingCreation();
        assert(!cancelController.modelDocument().contains("preview_object"));
        assert(cancelController.modelDocument().value("drawing_objects").toList().isEmpty());
        // No-op when nothing pending.
        cancelController.cancelPendingCreation();

        // deleteSelectedObject removes the active object of any kind and is undoable.
        DrawingDocumentController deleteController;
        deleteController.setSelectedToolId("point_tool");
        deleteController.clickCanvasNormalized(0.4, 0.4);
        assert(deleteController.modelDocument().value("drawing_objects").toList().size() == 1);
        assert(deleteController.deleteSelectedObject());
        assert(deleteController.modelDocument().value("drawing_objects").toList().isEmpty());
        assert(deleteController.canUndo());
        deleteController.undo();
        assert(deleteController.modelDocument().value("drawing_objects").toList().size() == 1);
        // Delete with nothing selected fails.
        DrawingDocumentController emptyDeleteController;
        assert(!emptyDeleteController.deleteSelectedObject());

        // duplicateSelectedObject clones the active object at a small offset.
        DrawingDocumentController dupController;
        dupController.setSelectedToolId("point_tool");
        dupController.clickCanvasNormalized(0.3, 0.3);
        const QVariantMap original = dupController.modelDocument().value("drawing_objects").toList().front().toMap();
        assert(dupController.duplicateSelectedObject());
        const QVariantList afterDup = dupController.modelDocument().value("drawing_objects").toList();
        assert(afterDup.size() == 2);
        const QVariantMap copy = afterDup.back().toMap();
        assert(copy.value("id").toString() != original.value("id").toString());
        assert(nearlyEqual(copy.value("x").toDouble(), original.value("x").toDouble() + 0.02));
        assert(nearlyEqual(copy.value("y").toDouble(), original.value("y").toDouble() + 0.02));
        // The duplicate is selected and the action is a single undo step.
        assert(dupController.selectedObjectId() == copy.value("id").toString());
        dupController.undo();
        assert(dupController.modelDocument().value("drawing_objects").toList().size() == 1);
        DrawingDocumentController emptyDupController;
        assert(!emptyDupController.duplicateSelectedObject());

        // Shift-nudge maps to the "coarse" step (4x the grid step).
        DrawingDocumentController coarseController;
        coarseController.setSelectedToolId("point_tool");
        coarseController.clickCanvasNormalized(0.5, 0.5);
        const double baseX = coarseController.modelDocument().value("drawing_objects").toList().front().toMap().value("x").toDouble();
        assert(coarseController.nudgeSelection("right", "grid"));
        const double gridX = coarseController.modelDocument().value("drawing_objects").toList().front().toMap().value("x").toDouble();
        coarseController.undo();
        assert(coarseController.nudgeSelection("right", "coarse"));
        const double coarseX = coarseController.modelDocument().value("drawing_objects").toList().front().toMap().value("x").toDouble();
        // coarse step is 4x the grid step.
        assert(nearlyEqual(coarseX - baseX, (gridX - baseX) * 4.0));
    }

    // Polyline: the first multi-click tool. Clicks anchor vertices into the
    // pending request; nothing reaches the document until the finish verb.
    {
        DrawingDocumentController polyController;
        polyController.setSelectedToolId("polyline_tool");
        polyController.clickCanvasNormalized(0.2, 0.2);
        polyController.clickCanvasNormalized(0.5, 0.3);
        assert(polyController.modelDocument().value("drawing_objects").toList().isEmpty());

        // The pointer previews as a provisional last vertex.
        polyController.updateCreationPreviewNormalized(0.6, 0.6);
        assert(polyController.modelDocument().contains("preview_object"));

        polyController.clickCanvasNormalized(0.6, 0.6);
        assert(polyController.finishPendingMultiClick());
        const QVariantList objects = polyController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 1);
        const QVariantMap polyline = objects.front().toMap();
        assert(polyline.value("kind").toString() == QStringLiteral("polyline"));
        assert(polyController.selectedObjectId() == polyline.value("id").toString());

        // The whole gesture is ONE undo step, not one per click.
        assert(polyController.canUndo());
        polyController.undo();
        assert(polyController.modelDocument().value("drawing_objects").toList().isEmpty());
        polyController.redo();
        assert(polyController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Finishing with nothing pending refuses; a one-vertex trail
        // dissolves silently (same as Escape).
        assert(!polyController.finishPendingMultiClick());
        polyController.clickCanvasNormalized(0.8, 0.8);
        assert(!polyController.finishPendingMultiClick());
        assert(polyController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Escape drops an in-flight trail without touching the document.
        polyController.clickCanvasNormalized(0.1, 0.8);
        polyController.clickCanvasNormalized(0.3, 0.9);
        polyController.cancelPendingCreation();
        assert(!polyController.finishPendingMultiClick());
        assert(polyController.modelDocument().value("drawing_objects").toList().size() == 1);

        // A finished polyline is selectable by clicking near a segment.
        polyController.setSelectedToolId("select_move");
        polyController.clickCanvasNormalized(0.35, 0.25);
        assert(polyController.selectedObjectId() == polyline.value("id").toString());
    }

    // Spline: the SECOND multi-click tool. It rides the exact same gesture as
    // the polyline (clicks accumulate, finish commits, one undo step) — proof
    // the multi-click branch and finishPendingMultiClick are shared, not cloned.
    {
        DrawingDocumentController splineController;
        splineController.setSelectedToolId("spline_tool");
        splineController.clickCanvasNormalized(0.2, 0.2);
        splineController.clickCanvasNormalized(0.4, 0.5);
        splineController.clickCanvasNormalized(0.7, 0.3);
        // Nothing reaches the document until the finish verb.
        assert(splineController.modelDocument().value("drawing_objects").toList().isEmpty());

        assert(splineController.finishPendingMultiClick());
        const QVariantList objects = splineController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 1);
        const QVariantMap spline = objects.front().toMap();
        assert(spline.value("kind").toString() == QStringLiteral("spline"));
        // The projection flattened the sampled curve into drawable points.
        assert(!spline.value("points").toList().isEmpty());

        // The whole gesture is ONE undo step; redo restores it whole.
        assert(splineController.canUndo());
        splineController.undo();
        assert(splineController.modelDocument().value("drawing_objects").toList().isEmpty());
        splineController.redo();
        assert(splineController.modelDocument().value("drawing_objects").toList().size() == 1);

        // A finished spline is selectable by clicking near the curve.
        splineController.setSelectedToolId("select_move");
        splineController.clickCanvasNormalized(0.4, 0.5); // a control point lies on the curve
        assert(splineController.selectedObjectId() == spline.value("id").toString());
    }

    // N1 copy/cut/paste.
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value("drawing_objects").toList().size();
        };

        DrawingDocumentController clip;
        clip.setSelectedToolId("point_tool");
        clip.clickCanvasNormalized(0.3, 0.3);
        clip.clickCanvasNormalized(0.6, 0.6);
        assert(objectCount(clip) == 2);

        // Copy with nothing selected is a no-op (clear the click selection
        // with an empty marquee first).
        clip.selectObjectsInBoundsNormalized(0.9, 0.9, 0.95, 0.95);
        assert(!clip.copySelection());
        assert(!clip.canPaste());

        // Marquee-select both points, copy, paste: two fresh objects appear,
        // the originals stay, and the pasted pair is what's now selected.
        clip.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(clip.copySelection());
        assert(clip.canPaste());
        const int beforePaste = objectCount(clip);
        assert(clip.paste());
        assert(objectCount(clip) == beforePaste + 2);
        assert(clip.modelDocument().value("selected_object_ids").toList().size() == 2);

        // Every object id in the document is unique after the paste.
        {
            const QVariantList objects = clip.modelDocument().value("drawing_objects").toList();
            QSet<QString> ids;
            for (const QVariant &value : objects) {
                ids.insert(value.toMap().value("id").toString());
            }
            assert(ids.size() == objects.size());
        }

        // Paste is exactly one undo step (not one per pasted object).
        clip.undo();
        assert(objectCount(clip) == beforePaste);
        clip.redo();
        assert(objectCount(clip) == beforePaste + 2);

        // The clipboard survives selection changes: clear the selection, paste
        // again — still pastes BOTH copied points.
        clip.selectObjectsInBoundsNormalized(0.9, 0.9, 0.95, 0.95);
        const int beforeSecond = objectCount(clip);
        assert(clip.paste());
        assert(objectCount(clip) == beforeSecond + 2);

        // Cut: copies then removes the selection as one undo step.
        DrawingDocumentController cutter;
        cutter.setSelectedToolId("point_tool");
        cutter.clickCanvasNormalized(0.4, 0.4);
        cutter.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(cutter.cutSelection());
        assert(objectCount(cutter) == 0); // cut removed it
        assert(cutter.canPaste());
        cutter.undo();                    // the cut's delete is one undo step
        assert(objectCount(cutter) == 1);
        cutter.redo();
        assert(objectCount(cutter) == 0);
        assert(cutter.paste());           // the cut clipboard still pastes
        assert(objectCount(cutter) == 1);

        // Empty clipboard pastes nothing.
        DrawingDocumentController fresh;
        assert(!fresh.canPaste());
        assert(!fresh.paste());

        // Paste is ATOMIC (user decision 2026-06-11). The DISCRIMINATING
        // setup is a clipboard spanning two layers with only one locked:
        // the old per-object loop pasted the unlocked subset (returned
        // true, count +1, selection = the partial paste) — every assertion
        // below fails under it.
        DrawingDocumentController atomicPaste;
        atomicPaste.setSelectedToolId("point_tool");
        atomicPaste.clickCanvasNormalized(0.3, 0.3); // point on the default layer (serial 1)
        assert(atomicPaste.createLayer());           // second layer becomes active
        atomicPaste.clickCanvasNormalized(0.6, 0.6); // point on the second layer (serial 2)
        atomicPaste.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(atomicPaste.copySelection());            // clipboard spans both layers
        assert(atomicPaste.setActiveLayerLocked(true)); // lock ONLY the second layer
        const int beforeLockedPaste = objectCount(atomicPaste);
        assert(!atomicPaste.paste());
        assert(objectCount(atomicPaste) == beforeLockedPaste);
        // The failure preserves the selection (the old loop cleared it via
        // SelectObjectsCommand{empty}) and reports through edit_status.
        assert(atomicPaste.modelDocument().value("selected_object_ids").toList().size() == 2);
        QVariantMap pasteStatus = atomicPaste.modelDocument().value("edit_status").toMap();
        assert(pasteStatus.value("ok").toBool() == false);
        assert(pasteStatus.value("mode").toString() == "paste");
        // Unlock: the same clipboard pastes whole.
        assert(atomicPaste.setActiveLayerLocked(false));
        assert(atomicPaste.paste());
        assert(objectCount(atomicPaste) == beforeLockedPaste + 2);
        // Pins the serial reclaim: points minted serials 1-2, the FAILED
        // paste minted 3-4 then restored, so this paste re-mints 3-4 —
        // without the restore in paste() these would be 5-6.
        QVariantList pastedSelection = atomicPaste.modelDocument().value("selected_object_ids").toList();
        assert(pastedSelection.size() == 2);
        assert(trailingSerial(pastedSelection.first().toString()) == 3);
        assert(trailingSerial(pastedSelection.last().toString()) == 4);
        // A clean paste leaves no stale rejection behind.
        assert(atomicPaste.modelDocument().value("edit_status").toMap().isEmpty());
    }

    // N3 object metadata: role / material / export_group / tags, editable
    // through the controller and surfaced in the projection.
    {
        DrawingDocumentController meta;
        meta.setSelectedToolId("point_tool");
        meta.clickCanvasNormalized(0.5, 0.5); // creates + selects a point

        auto activeObj = [&]() {
            const QVariantMap model = meta.modelDocument();
            const QString id = model.value("active_object_id").toString();
            for (const QVariant &v : model.value("drawing_objects").toList()) {
                if (v.toMap().value("id").toString() == id) {
                    return v.toMap();
                }
            }
            return QVariantMap{};
        };

        assert(meta.setSelectedObjectRole("cutout"));
        assert(meta.setSelectedObjectMaterial("oak"));
        assert(meta.setSelectedObjectExportGroup("frame"));
        assert(meta.setSelectedObjectTags({QStringLiteral("load-bearing"), QStringLiteral(" visible "),
                                            QStringLiteral("")})); // blanks dropped, others trimmed
        const QVariantMap projected = activeObj();
        assert(projected.value("role").toString() == "cutout");
        assert(projected.value("material").toString() == "oak");
        assert(projected.value("export_group").toString() == "frame");
        assert(projected.value("tags").toString() == "load-bearing, visible");

        // Each edit is undoable; undoing the tags edit restores the prior tags.
        meta.undo();
        assert(activeObj().value("tags").toString().isEmpty());

        // An unknown role name falls back to none rather than erroring.
        assert(meta.setSelectedObjectRole("not_a_role"));
        assert(activeObj().value("role").toString() == "none");

        // No selection: setters refuse.
        DrawingDocumentController noSel;
        assert(!noSel.setSelectedObjectRole("wall"));
        assert(!noSel.setSelectedObjectMaterial("steel"));
    }

    // N4 rectangle variants + aspect-lock through the controller.
    {
        auto activeRect = [](DrawingDocumentController &c) {
            const QVariantMap model = c.modelDocument();
            const QString id = model.value("active_object_id").toString();
            for (const QVariant &v : model.value("drawing_objects").toList()) {
                if (v.toMap().value("id").toString() == id) {
                    return v.toMap();
                }
            }
            return QVariantMap{};
        };

        // Rounded + frame options ride into the created rectangle.
        DrawingDocumentController rectCtl;
        rectCtl.setRectCornerRadius(0.05);
        rectCtl.setRectInset(0.02);
        rectCtl.setSelectedToolId("rectangle_tool");
        rectCtl.clickCanvasNormalized(0.2, 0.2);
        rectCtl.clickCanvasNormalized(0.6, 0.5);
        const QVariantMap madeRect = activeRect(rectCtl);
        assert(madeRect.value("kind").toString() == "rectangle");
        assert(nearlyEqual(madeRect.value("corner_radius").toDouble(), 0.05));
        assert(nearlyEqual(madeRect.value("inset").toDouble(), 0.02));

        // Negative/non-finite options normalize to a plain box.
        rectCtl.setRectCornerRadius(-1.0);
        assert(rectCtl.rectCornerRadius() == 0.0);

        // Aspect-lock: a fresh square, then a corner drag with the lock on
        // preserves the 1:1 ratio even though the cursor demands a rectangle.
        DrawingDocumentController lockCtl;
        lockCtl.setSelectedToolId("rectangle_tool");
        lockCtl.clickCanvasNormalized(0.2, 0.2);
        lockCtl.clickCanvasNormalized(0.4, 0.4); // a square (equal w/h)
        const QVariantMap square = activeRect(lockCtl);
        const double w0 = square.value("bounds").toMap().value("width").toDouble();
        const double h0 = square.value("bounds").toMap().value("height").toDouble();
        assert(nearlyEqual(w0, h0));

        lockCtl.setAspectLockEnabled(true);
        assert(lockCtl.aspectLockEnabled());
        // Drag the SE corner far off-square; the lock keeps width==height.
        lockCtl.editSelectedHandleNormalized("rect_se", 0.9, 0.5);
        const QVariantMap locked = activeRect(lockCtl);
        const double wL = locked.value("bounds").toMap().value("width").toDouble();
        const double hL = locked.value("bounds").toMap().value("height").toDouble();
        assert(nearlyEqual(wL, hL));

        // With the lock off, the same kind of drag frees the ratio.
        lockCtl.setAspectLockEnabled(false);
        lockCtl.editSelectedHandleNormalized("rect_se", 0.95, 0.45);
        const QVariantMap freed = activeRect(lockCtl);
        assert(!nearlyEqual(freed.value("bounds").toMap().value("width").toDouble(),
                            freed.value("bounds").toMap().value("height").toDouble()));
    }

    // Per-object styling: the object's own stroke wins over the layer's,
    // inherit sentinels (empty color / zero width) hand control back, and
    // the whole edit is one undo step.
    {
        DrawingDocumentController styleController;
        styleController.setSelectedToolId(QStringLiteral("line_tool"));
        styleController.clickCanvasNormalized(0.2, 0.2);
        styleController.clickCanvasNormalized(0.8, 0.8); // line, auto-selected

        const auto activeProjection = [&styleController]() {
            const QVariantMap model = styleController.modelDocument();
            const QString activeId = model.value(QStringLiteral("active_object_id")).toString();
            for (const QVariant &value : model.value(QStringLiteral("drawing_objects")).toList()) {
                const QVariantMap object = value.toMap();
                if (object.value(QStringLiteral("id")).toString() == activeId) {
                    return object;
                }
            }
            return QVariantMap{};
        };

        const QString layerColor = activeProjection().value(QStringLiteral("effective_stroke_color")).toString();
        assert(!layerColor.isEmpty());

        assert(styleController.setSelectedObjectStrokeColor(QStringLiteral("#ff6600")));
        assert(styleController.setSelectedObjectStrokeWidth(4.5));
        assert(styleController.setSelectedObjectLineStyle(QStringLiteral("dash")));
        QVariantMap styled = activeProjection();
        assert(styled.value(QStringLiteral("effective_stroke_color")).toString() == QStringLiteral("#ff6600"));
        assert(styled.value(QStringLiteral("effective_stroke_width")).toDouble() == 4.5);
        assert(styled.value(QStringLiteral("effective_line_style")).toString() == QStringLiteral("dash"));
        // An art color keeps the layer's physical pen (no preset match).
        assert(styled.value(QStringLiteral("effective_pen_id")).toString() == QStringLiteral("pen_black"));
        // A preset color SELECTS its pen.
        assert(styleController.setSelectedObjectStrokeColor(QStringLiteral("#75c7ff")));
        assert(activeProjection().value(QStringLiteral("effective_pen_id")).toString() == QStringLiteral("pen_blue"));

        // Inherit sentinels hand control back to the layer.
        assert(styleController.setSelectedObjectStrokeColor(QString()));
        assert(styleController.setSelectedObjectStrokeWidth(0.0));
        styled = activeProjection();
        assert(styled.value(QStringLiteral("effective_stroke_color")).toString() == layerColor);

        // Undo walks the style edits back one command at a time: undoing
        // the width-0 (inherit) command restores the explicit 4.5 — an
        // assertion that FAILS if undo restores nothing (the first draft
        // checked a value that held either way; the review caught it).
        assert(styleController.undo());
        styled = activeProjection();
        assert(styled.value(QStringLiteral("effective_stroke_width")).toDouble() == 4.5);

        // Opacity: per-object only (no layer fallback), clamped to [0, 1],
        // surfaced through BOTH projection keys, one undo step per edit.
        assert(styleController.setSelectedObjectStrokeOpacity(0.4));
        styled = activeProjection();
        assert(styled.value(QStringLiteral("effective_stroke_opacity")).toDouble() == 0.4);
        assert(styled.value(QStringLiteral("own_stroke_opacity")).toDouble() == 0.4);
        assert(styleController.setSelectedObjectStrokeOpacity(5.0)); // clamps high
        assert(activeProjection().value(QStringLiteral("effective_stroke_opacity")).toDouble() == 1.0);
        assert(styleController.setSelectedObjectStrokeOpacity(-1.0)); // clamps to transparent
        assert(activeProjection().value(QStringLiteral("effective_stroke_opacity")).toDouble() == 0.0);
        assert(!styleController.setSelectedObjectStrokeOpacity(std::numeric_limits<double>::quiet_NaN()));
        assert(styleController.undo()); // undoing the 0.0 edit restores the clamp-high 1.0
        assert(activeProjection().value(QStringLiteral("effective_stroke_opacity")).toDouble() == 1.0);

        // Fill: gated by kind — only Rectangle/Circle/Ellipse/Polygon carry a fill
        // that the painter/SVG actually renders.  Open kinds (Line, Arc, …) are
        // rejected at the setter so no invisible "write-only" fill state can build up.
        // Add a Circle to this controller (away from the existing line) and select it.
        styleController.setSelectedToolId(QStringLiteral("circle_tool"));
        styleController.clickCanvasNormalized(0.5, 0.1); // center
        styleController.clickCanvasNormalized(0.7, 0.1); // edge → radius 0.2
        // circle_tool auto-selects the new object
        assert(activeProjection().value(QStringLiteral("own_fill_opacity")).toDouble() == 0.0);
        assert(styleController.setSelectedObjectFillColor(QStringLiteral("#2244aa")));
        assert(styleController.setSelectedObjectFillOpacity(0.6));
        styled = activeProjection();
        assert(styled.value(QStringLiteral("own_fill_color")).toString() == QStringLiteral("#2244aa"));
        assert(styled.value(QStringLiteral("own_fill_opacity")).toDouble() == 0.6);
        assert(styleController.setSelectedObjectFillOpacity(5.0)); // clamps high
        assert(activeProjection().value(QStringLiteral("own_fill_opacity")).toDouble() == 1.0);
        assert(!styleController.setSelectedObjectFillColor(QStringLiteral("not-a-color"))); // junk rejected
        assert(!styleController.setSelectedObjectFillOpacity(std::numeric_limits<double>::quiet_NaN())); // non-finite rejected
        // No-op guard: re-setting the value already in place must NOT push an undo step —
        // so the undo below restores 0.6, not 1.0.
        assert(styleController.setSelectedObjectFillOpacity(1.0)); // already 1.0 after the clamp
        assert(styleController.undo());
        assert(activeProjection().value(QStringLiteral("own_fill_opacity")).toDouble() == 0.6);

        // DR-15: fill setters on an open kind (Line) return false; object.fill unchanged.
        // Re-select the diagonal line created at the top of this block.
        styleController.setSelectedToolId(QStringLiteral("select_move"));
        styleController.clickCanvasNormalized(0.3, 0.3); // midpoint of the (0.2,0.2)→(0.8,0.8) line
        const QVariantMap lineBeforeFill = activeProjection();
        assert(!styleController.setSelectedObjectFillColor(QStringLiteral("#aabbcc")));
        assert(!styleController.setSelectedObjectFillOpacity(0.7));
        const QVariantMap lineAfterFill = activeProjection();
        // fill state must be byte-identical after the rejected calls
        assert(lineAfterFill.value(QStringLiteral("own_fill_color"))
            == lineBeforeFill.value(QStringLiteral("own_fill_color")));
        assert(lineAfterFill.value(QStringLiteral("own_fill_opacity"))
            == lineBeforeFill.value(QStringLiteral("own_fill_opacity")));
    }

    // #30 parametric arrays: option state (count + spacings) drives repeat
    // instead of the retired hardcoded 3 x 0.1.
    {
        DrawingDocumentController arrayController;
        // Option setters normalize garbage instead of failing later actions.
        arrayController.setArrayCount(0);
        assert(arrayController.arrayCount() == 1);
        arrayController.setArrayCount(500);
        assert(arrayController.arrayCount() == 99);
        arrayController.setArraySpacingX(std::numeric_limits<double>::infinity());
        assert(arrayController.arraySpacingX() == 0.0);
        arrayController.setArraySpacingY(-0.25); // negative = march up/left, legal
        assert(arrayController.arraySpacingY() == -0.25);
        arrayController.setFixedRadius(-2.0);
        assert(arrayController.fixedRadius() == 0.0);
        // Magnitudes clamp to the unit document space — stored state always
        // matches what the spins can show and what the build stamps.
        arrayController.setArraySpacingX(5.0);
        assert(arrayController.arraySpacingX() == 1.0);
        arrayController.setArraySpacingY(-5.0);
        assert(arrayController.arraySpacingY() == -1.0);
        arrayController.setFixedRadius(5.0);
        assert(arrayController.fixedRadius() == 1.0);
        arrayController.setFixedRadius(0.0);

        arrayController.setSelectedToolId("line_tool");
        arrayController.clickCanvasNormalized(0.2, 0.3);
        arrayController.clickCanvasNormalized(0.3, 0.3);
        assert(arrayController.modelDocument().value("drawing_objects").toList().size() == 1);

        arrayController.setArrayCount(2);
        arrayController.setArraySpacingX(0.2);
        assert(arrayController.repeatSelectedObject("x"));
        QVariantList arrayObjects = arrayController.modelDocument().value("drawing_objects").toList();
        assert(arrayObjects.size() == 3);
        QVariantMap secondCopy = arrayObjects[2].toMap();
        assert(nearlyEqual(secondCopy.value("x1").toDouble(), 0.6));
        assert(nearlyEqual(secondCopy.value("y1").toDouble(), 0.3));
        assert(arrayController.modelDocument().value("selected_object_ids").toList().size() == 2);
    }

    // Grid array: count x count cells from one count spin, both spacings.
    {
        DrawingDocumentController gridArrayController;
        gridArrayController.setSelectedToolId("line_tool");
        gridArrayController.clickCanvasNormalized(0.1, 0.1);
        gridArrayController.clickCanvasNormalized(0.15, 0.1);

        // A 1x1 grid has nothing to create.
        gridArrayController.setArrayCount(1);
        assert(!gridArrayController.gridArraySelectedObject());

        gridArrayController.setArrayCount(2);
        gridArrayController.setArraySpacingX(0.2);
        gridArrayController.setArraySpacingY(0.3);
        assert(gridArrayController.gridArraySelectedObject());
        QVariantList gridObjects = gridArrayController.modelDocument().value("drawing_objects").toList();
        assert(gridObjects.size() == 4); // source + 3 copies
        QVariantMap rightCopy = gridObjects[1].toMap();    // cell (1,0)
        QVariantMap downCopy = gridObjects[2].toMap();     // cell (0,1)
        QVariantMap diagonalCopy = gridObjects[3].toMap(); // cell (1,1)
        assert(nearlyEqual(rightCopy.value("x1").toDouble(), 0.3));
        assert(nearlyEqual(rightCopy.value("y1").toDouble(), 0.1));
        assert(nearlyEqual(downCopy.value("x1").toDouble(), 0.1));
        assert(nearlyEqual(downCopy.value("y1").toDouble(), 0.4));
        assert(nearlyEqual(diagonalCopy.value("x1").toDouble(), 0.3));
        assert(nearlyEqual(diagonalCopy.value("y1").toDouble(), 0.4));
        assert(gridArrayController.modelDocument().value("selected_object_ids").toList().size() == 3);

        // One undo step removes the whole grid.
        assert(gridArrayController.undo());
        assert(gridArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
    }

    // Radial array now PICKS its centre: arm the capture, then a canvas click
    // sets the ring centre. Copies ring that picked point at the source's
    // distance — proof the pick-a-point capture feeds the array end-to-end.
    {
        DrawingDocumentController radialController;
        // The picked centre is arbitrary now, not the drawable centre.
        const double centerX = 0.5;
        const double centerY = 0.45;

        radialController.setSelectedToolId("circle_tool");
        radialController.setFixedRadius(0.02);
        radialController.clickCanvasNormalized(centerX - 0.2, centerY);
        radialController.clickCanvasNormalized(centerX - 0.2, centerY); // fixed radius: a same-point click still sizes
        QVariantList seeded = radialController.modelDocument().value("drawing_objects").toList();
        assert(seeded.size() == 1);
        assert(nearlyEqual(seeded[0].toMap().value("radius").toDouble(), 0.02));

        radialController.setArrayCount(3);
        // Arming requires a usable source and exposes the prompt; the click
        // that follows is consumed as the centre, NOT as a new selection.
        assert(radialController.beginRadialArrayCenterPick());
        assert(radialController.isAwaitingPointCapture());
        assert(radialController.modelDocument().value("awaiting_point_capture").toBool());
        assert(!radialController.modelDocument().value("point_capture_prompt").toString().isEmpty());

        radialController.clickCanvasNormalized(centerX, centerY); // sets the ring centre
        assert(!radialController.isAwaitingPointCapture());       // capture consumed
        assert(!radialController.modelDocument().contains("awaiting_point_capture"));
        QVariantList ringObjects = radialController.modelDocument().value("drawing_objects").toList();
        assert(ringObjects.size() == 4);
        // Slots = 4 -> copies at 90/180/270 degrees, all at ring radius 0.2.
        for (int i = 1; i < ringObjects.size(); ++i) {
            QVariantMap copy = ringObjects[i].toMap();
            const double dx = copy.value("cx").toDouble() - centerX;
            const double dy = copy.value("cy").toDouble() - centerY;
            assert(nearlyEqual(std::hypot(dx, dy), 0.2));
            assert(nearlyEqual(copy.value("radius").toDouble(), 0.02));
        }

        // The capture click did not change selection — the source is still
        // active (so a second array would work), and the array is ONE undo step.
        assert(radialController.canUndo());
        radialController.undo();
        assert(radialController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Arming with NOTHING selected refuses and arms nothing.
        DrawingDocumentController emptyController;
        assert(!emptyController.beginRadialArrayCenterPick());
        assert(!emptyController.isAwaitingPointCapture());

        // Escape (cancelPendingCreation) drops an armed capture without arraying.
        radialController.setSelectedToolId("select_move");
        radialController.clickCanvasNormalized(centerX - 0.2, centerY); // reselect the source circle
        assert(radialController.beginRadialArrayCenterPick());
        radialController.cancelPendingCreation();
        assert(!radialController.isAwaitingPointCapture());
        assert(radialController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Switching tools also cancels an armed capture.
        assert(radialController.beginRadialArrayCenterPick());
        radialController.setSelectedToolId("line_tool");
        assert(!radialController.isAwaitingPointCapture());

        // A source sitting ON the picked centre has a zero arm: the planner
        // rejects, the document is untouched, and the capture still clears.
        DrawingDocumentController degenerateController;
        degenerateController.setSelectedToolId("circle_tool");
        degenerateController.setFixedRadius(0.05);
        degenerateController.clickCanvasNormalized(0.5, 0.5);
        degenerateController.clickCanvasNormalized(0.5, 0.5);
        assert(degenerateController.beginRadialArrayCenterPick());
        degenerateController.clickCanvasNormalized(0.5, 0.5); // centre == source centre -> zero arm
        assert(!degenerateController.isAwaitingPointCapture());
        assert(degenerateController.modelDocument().value("drawing_objects").toList().size() == 1);
    }

    // Rotate-copies rosette: the rotating sibling intent. Arming + the captured
    // centre makes m_arrayCount copies in ONE undo step (the op test covers the
    // per-spoke rotation; here we verify the controller wiring runs and orbits).
    {
        const double centerX = 0.5;
        const double centerY = 0.5;
        DrawingDocumentController rosette;
        rosette.setSelectedToolId("circle_tool");
        rosette.setFixedRadius(0.02);
        rosette.clickCanvasNormalized(centerX - 0.2, centerY);
        rosette.clickCanvasNormalized(centerX - 0.2, centerY); // fixed radius: sizes on a same-point click
        assert(rosette.modelDocument().value("drawing_objects").toList().size() == 1);

        rosette.setSelectedToolId("select_move");
        rosette.clickCanvasNormalized(centerX - 0.2, centerY); // reselect the source
        rosette.setArrayCount(3);
        rosette.setRotateCopiesTotalAngle(360.0);
        assert(nearlyEqual(rosette.rotateCopiesTotalAngle(), 360.0));
        assert(rosette.beginRotateCopiesCenterPick());
        assert(rosette.isAwaitingPointCapture());
        rosette.clickCanvasNormalized(centerX, centerY); // sets the rosette centre
        assert(!rosette.isAwaitingPointCapture());

        QVariantList objs = rosette.modelDocument().value("drawing_objects").toList();
        assert(objs.size() == 4); // source + 3 rotated copies
        for (int i = 1; i < objs.size(); ++i) {
            QVariantMap copy = objs[i].toMap();
            const double dx = copy.value("cx").toDouble() - centerX;
            const double dy = copy.value("cy").toDouble() - centerY;
            assert(nearlyEqual(std::hypot(dx, dy), 0.2)); // each copy orbits the ring
        }
        // ONE undo step removes all copies.
        assert(rosette.canUndo());
        rosette.undo();
        assert(rosette.modelDocument().value("drawing_objects").toList().size() == 1);

        // Refuses to arm with nothing selected.
        DrawingDocumentController emptyRosette;
        assert(!emptyRosette.beginRotateCopiesCenterPick());
        assert(!emptyRosette.isAwaitingPointCapture());

        // Setter must store faithfully — no silent swallow of small values.
        // Before the fix, |angle| < 1.0 was ignored and the spin diverged from
        // the stored value.
        DrawingDocumentController setterCheck;
        setterCheck.setRotateCopiesTotalAngle(0.0);
        assert(nearlyEqual(setterCheck.rotateCopiesTotalAngle(), 0.0));
        setterCheck.setRotateCopiesTotalAngle(0.5);
        assert(nearlyEqual(setterCheck.rotateCopiesTotalAngle(), 0.5));
        // Non-finite must still be ignored (prior value preserved).
        setterCheck.setRotateCopiesTotalAngle(std::numeric_limits<double>::quiet_NaN());
        assert(nearlyEqual(setterCheck.rotateCopiesTotalAngle(), 0.5));
    }

    // Kaleidoscope: arming + the captured centre reflects the source across
    // arrayCount() axes (one copy per axis) in ONE undo step. The single-axis
    // mirror verb is unchanged; the op test covers the per-kind orientation flip.
    {
        DrawingDocumentController kaleido;
        kaleido.setSelectedToolId("line_tool");
        kaleido.clickCanvasNormalized(0.6, 0.55);
        kaleido.clickCanvasNormalized(0.7, 0.6); // a line (a mirrorable kind)
        assert(kaleido.modelDocument().value("drawing_objects").toList().size() == 1);

        kaleido.setSelectedToolId("select_move");
        kaleido.clickCanvasNormalized(0.65, 0.575); // select the line
        assert(!kaleido.selectedObjectId().isEmpty());
        kaleido.setArrayCount(3); // 3 axes -> 3 reflected copies

        assert(kaleido.beginKaleidoscopeCenterPick());
        assert(kaleido.isAwaitingPointCapture());
        kaleido.clickCanvasNormalized(0.5, 0.5); // sets the kaleidoscope centre
        assert(!kaleido.isAwaitingPointCapture());
        assert(kaleido.modelDocument().value("drawing_objects").toList().size() == 4); // source + 3

        // ONE undo step removes all reflected copies.
        assert(kaleido.canUndo());
        kaleido.undo();
        assert(kaleido.modelDocument().value("drawing_objects").toList().size() == 1);

        // Refuses to arm with nothing selected.
        DrawingDocumentController emptyKaleido;
        assert(!emptyKaleido.beginKaleidoscopeCenterPick());
        assert(!emptyKaleido.isAwaitingPointCapture());
    }

    // Trim verb: a line trimmed back to where another line crosses it, the
    // doomed side chosen by the captured click — pick-a-point's SECOND consumer
    // end-to-end (a different intent down the same capture path).
    {
        DrawingDocumentController trimController;
        // Target: a horizontal line. Boundary: a vertical line crossing at 0.5.
        trimController.setSelectedToolId("line_tool");
        trimController.clickCanvasNormalized(0.2, 0.5);
        trimController.clickCanvasNormalized(0.8, 0.5);
        trimController.clickCanvasNormalized(0.5, 0.2);
        trimController.clickCanvasNormalized(0.5, 0.8);
        assert(trimController.modelDocument().value("drawing_objects").toList().size() == 2);

        // Select the horizontal target (click on it, away from the crossing).
        trimController.setSelectedToolId("select_move");
        trimController.clickCanvasNormalized(0.3, 0.5);
        const QString targetId = trimController.selectedObjectId();
        assert(!targetId.isEmpty());

        // Arm trim, then click the RIGHT stub: the b end trims to the crossing.
        assert(trimController.beginTrimSelectedLine());
        assert(trimController.isAwaitingPointCapture());
        trimController.clickCanvasNormalized(0.72, 0.5);
        assert(!trimController.isAwaitingPointCapture());

        // The target now runs (0.2,0.5)..(0.5,0.5); trim mutates, never adds.
        QVariantList objects = trimController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 2);
        auto findById = [](const QVariantList &list, const QString &id) {
            QVariantMap found;
            for (const QVariant &v : list) {
                if (v.toMap().value("id").toString() == id) { found = v.toMap(); }
            }
            return found;
        };
        const QVariantMap trimmed = findById(objects, targetId);
        assert(!trimmed.isEmpty());
        assert(nearlyEqual(trimmed.value("x1").toDouble(), 0.2));
        assert(nearlyEqual(trimmed.value("x2").toDouble(), 0.5)); // b pulled to the cut
        assert(nearlyEqual(trimmed.value("y2").toDouble(), 0.5));

        // One undo step restores the full line.
        assert(trimController.canUndo());
        trimController.undo();
        const QVariantMap restored = findById(
            trimController.modelDocument().value("drawing_objects").toList(), targetId);
        assert(nearlyEqual(restored.value("x2").toDouble(), 0.8));

        // Trim with no crossing boundary surfaces a status and changes nothing.
        DrawingDocumentController lonelyController;
        lonelyController.setSelectedToolId("line_tool");
        lonelyController.clickCanvasNormalized(0.2, 0.3);
        lonelyController.clickCanvasNormalized(0.8, 0.3);
        lonelyController.setSelectedToolId("select_move");
        lonelyController.clickCanvasNormalized(0.5, 0.3);
        assert(lonelyController.beginTrimSelectedLine());
        lonelyController.clickCanvasNormalized(0.7, 0.3);
        assert(!lonelyController.isAwaitingPointCapture());
        assert(lonelyController.modelDocument().contains("edit_status"));
        const QVariantList lonelyObjects = lonelyController.modelDocument().value("drawing_objects").toList();
        assert(lonelyObjects.size() == 1);
        assert(nearlyEqual(lonelyObjects.front().toMap().value("x2").toDouble(), 0.8)); // unchanged

        // Trim refuses to arm when the selection is not a line.
        DrawingDocumentController nonLineController;
        nonLineController.setSelectedToolId("circle_tool");
        nonLineController.clickCanvasNormalized(0.5, 0.5);
        nonLineController.clickCanvasNormalized(0.6, 0.5);
        assert(!nonLineController.beginTrimSelectedLine());
        assert(!nonLineController.isAwaitingPointCapture());
    }

    // Fillet verb: select a line, pick the other line + corner; BOTH lines trim
    // to the tangent points and a rounding arc is created — three objects from
    // two, all in ONE undo step. Pick-a-point's THIRD consumer (FilletSecondLine).
    {
        DrawingDocumentController filletController;
        // Two lines forming an L at (0.3,0.3).
        filletController.setSelectedToolId("line_tool");
        filletController.clickCanvasNormalized(0.3, 0.3);
        filletController.clickCanvasNormalized(0.8, 0.3); // horizontal arm
        filletController.clickCanvasNormalized(0.3, 0.3);
        filletController.clickCanvasNormalized(0.3, 0.8); // vertical arm
        assert(filletController.modelDocument().value("drawing_objects").toList().size() == 2);

        // Select the horizontal line as the fillet target.
        filletController.setSelectedToolId("select_move");
        filletController.clickCanvasNormalized(0.55, 0.3);
        const QString targetId = filletController.selectedObjectId();
        assert(!targetId.isEmpty());

        filletController.setFilletRadius(0.1);
        assert(nearlyEqual(filletController.filletRadius(), 0.1));
        assert(filletController.beginFilletSelectedLine());
        assert(filletController.isAwaitingPointCapture());
        // Pick near the vertical line, inside the corner.
        filletController.clickCanvasNormalized(0.33, 0.5);
        assert(!filletController.isAwaitingPointCapture());

        // Two trimmed lines + one new arc = 3 objects.
        const QVariantList objects = filletController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 3);
        int arcCount = 0;
        for (const QVariant &v : objects) {
            if (v.toMap().value("kind").toString() == QStringLiteral("arc")) {
                ++arcCount;
            }
        }
        assert(arcCount == 1);

        // The whole fillet (two trims + the arc) is ONE undo step.
        assert(filletController.canUndo());
        filletController.undo();
        assert(filletController.modelDocument().value("drawing_objects").toList().size() == 2);

        // Fillet refuses to arm when the selection is not a line.
        DrawingDocumentController nonLine;
        nonLine.setSelectedToolId("circle_tool");
        nonLine.clickCanvasNormalized(0.5, 0.5);
        nonLine.clickCanvasNormalized(0.6, 0.5);
        assert(!nonLine.beginFilletSelectedLine());
        assert(!nonLine.isAwaitingPointCapture());
    }

    // Chamfer verb: the angular sibling of fillet — select a line, pick the other
    // line + corner; BOTH lines set back and a straight bevel is created (three
    // objects from two) in ONE undo step. Pick-a-point's ChamferSecondLine consumer.
    {
        DrawingDocumentController chamferController;
        chamferController.setSelectedToolId("line_tool");
        chamferController.clickCanvasNormalized(0.3, 0.3);
        chamferController.clickCanvasNormalized(0.8, 0.3); // horizontal arm
        chamferController.clickCanvasNormalized(0.3, 0.3);
        chamferController.clickCanvasNormalized(0.3, 0.8); // vertical arm
        assert(chamferController.modelDocument().value("drawing_objects").toList().size() == 2);

        chamferController.setSelectedToolId("select_move");
        chamferController.clickCanvasNormalized(0.55, 0.3);
        const QString targetId = chamferController.selectedObjectId();
        assert(!targetId.isEmpty());

        chamferController.setChamferSetback(0.1);
        assert(nearlyEqual(chamferController.chamferSetback(), 0.1));
        assert(chamferController.beginChamferSelectedLine());
        assert(chamferController.isAwaitingPointCapture());
        chamferController.clickCanvasNormalized(0.33, 0.5); // near the vertical line, in the corner
        assert(!chamferController.isAwaitingPointCapture());

        // Two set-back lines + one new bevel = 3 objects, and the bevel is a LINE.
        const QVariantList objects = chamferController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 3);
        int lineCount = 0;
        for (const QVariant &v : objects) {
            if (v.toMap().value("kind").toString() == QStringLiteral("line")) {
                ++lineCount;
            }
        }
        assert(lineCount == 3); // both arms + the bevel are all lines

        // The whole chamfer (two trims + the bevel) is ONE undo step.
        assert(chamferController.canUndo());
        chamferController.undo();
        assert(chamferController.modelDocument().value("drawing_objects").toList().size() == 2);

        // A rejection (no second line) surfaces via edit_status, not a silent no-op.
        DrawingDocumentController loneLine;
        loneLine.setSelectedToolId("line_tool");
        loneLine.clickCanvasNormalized(0.2, 0.2);
        loneLine.clickCanvasNormalized(0.8, 0.2);
        loneLine.setSelectedToolId("select_move");
        loneLine.clickCanvasNormalized(0.5, 0.2);
        assert(loneLine.beginChamferSelectedLine());
        loneLine.clickCanvasNormalized(0.5, 0.5);
        assert(!loneLine.isAwaitingPointCapture());
        const QVariantMap status = loneLine.modelDocument().value("edit_status").toMap();
        assert(status.value("ok").toBool() == false);
        assert(status.value("mode").toString() == "chamfer");
        assert(!status.value("message").toString().isEmpty());

        // Chamfer refuses to arm when the selection is not a line.
        DrawingDocumentController nonLineChamfer;
        nonLineChamfer.setSelectedToolId("circle_tool");
        nonLineChamfer.clickCanvasNormalized(0.5, 0.5);
        nonLineChamfer.clickCanvasNormalized(0.6, 0.5);
        assert(!nonLineChamfer.beginChamferSelectedLine());
        assert(!nonLineChamfer.isAwaitingPointCapture());
    }

    // Extend verb: the mirror of trim — select a line, pick the end to lengthen;
    // the line stretches to the nearest boundary line its extension reaches, in ONE
    // undo step (no new object). Pick-a-point's ExtendPoint consumer.
    {
        DrawingDocumentController extendController;
        extendController.setSelectedToolId("line_tool");
        extendController.clickCanvasNormalized(0.3, 0.5);
        extendController.clickCanvasNormalized(0.5, 0.5); // short horizontal target
        extendController.clickCanvasNormalized(0.8, 0.2);
        extendController.clickCanvasNormalized(0.8, 0.8); // vertical boundary at x=0.8
        assert(extendController.modelDocument().value("drawing_objects").toList().size() == 2);

        extendController.setSelectedToolId("select_move");
        extendController.clickCanvasNormalized(0.4, 0.5); // select the horizontal line
        const QString targetId = extendController.selectedObjectId();
        assert(!targetId.isEmpty());

        assert(extendController.beginExtendSelectedLine());
        assert(extendController.isAwaitingPointCapture());
        extendController.clickCanvasNormalized(0.55, 0.5); // pick near the b-end → extend it
        assert(!extendController.isAwaitingPointCapture());

        // Still 2 objects (extend updates, never creates); the b-end reached x=0.8.
        QVariantList objects = extendController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 2);
        QVariantMap extended;
        for (const QVariant &v : objects) {
            if (v.toMap().value("id").toString() == targetId) {
                extended = v.toMap();
            }
        }
        assert(!extended.isEmpty());
        assert(nearlyEqual(extended.value("x2").toDouble(), 0.8));
        assert(nearlyEqual(extended.value("y2").toDouble(), 0.5));
        assert(nearlyEqual(extended.value("x1").toDouble(), 0.3)); // anchor end unchanged

        // ONE undo step restores the original short line.
        assert(extendController.canUndo());
        extendController.undo();
        for (const QVariant &v : extendController.modelDocument().value("drawing_objects").toList()) {
            if (v.toMap().value("id").toString() == targetId) {
                assert(nearlyEqual(v.toMap().value("x2").toDouble(), 0.5));
            }
        }

        // A dead click (no reachable boundary) surfaces via edit_status, not silent.
        DrawingDocumentController loneExtend;
        loneExtend.setSelectedToolId("line_tool");
        loneExtend.clickCanvasNormalized(0.3, 0.5);
        loneExtend.clickCanvasNormalized(0.5, 0.5);
        loneExtend.setSelectedToolId("select_move");
        loneExtend.clickCanvasNormalized(0.4, 0.5);
        assert(loneExtend.beginExtendSelectedLine());
        loneExtend.clickCanvasNormalized(0.55, 0.5);
        assert(!loneExtend.isAwaitingPointCapture());
        const QVariantMap status = loneExtend.modelDocument().value("edit_status").toMap();
        assert(status.value("ok").toBool() == false);
        assert(status.value("mode").toString() == "extend");
        assert(!status.value("message").toString().isEmpty());

        // Extend refuses to arm when the selection is not a line.
        DrawingDocumentController nonLineExtend;
        nonLineExtend.setSelectedToolId("circle_tool");
        nonLineExtend.clickCanvasNormalized(0.5, 0.5);
        nonLineExtend.clickCanvasNormalized(0.6, 0.5);
        assert(!nonLineExtend.beginExtendSelectedLine());
        assert(!nonLineExtend.isAwaitingPointCapture());
    }

    // Break verb: split a line at the picked point into TWO independent objects
    // (original shortened + new piece) in ONE undo step. Pick-a-point's BreakPoint
    // consumer; reuses the chamfer atomic multi-object pattern.
    {
        DrawingDocumentController breakController;
        breakController.setSelectedToolId("line_tool");
        breakController.clickCanvasNormalized(0.2, 0.5);
        breakController.clickCanvasNormalized(0.8, 0.5); // one horizontal line
        assert(breakController.modelDocument().value("drawing_objects").toList().size() == 1);

        breakController.setSelectedToolId("select_move");
        breakController.clickCanvasNormalized(0.5, 0.5); // select the line
        const QString targetId = breakController.selectedObjectId();
        assert(!targetId.isEmpty());

        assert(breakController.beginBreakSelectedObject());
        assert(breakController.isAwaitingPointCapture());
        breakController.clickCanvasNormalized(0.5, 0.5); // break at the midpoint
        assert(!breakController.isAwaitingPointCapture());

        // Two lines now: the original shortened + the new piece.
        QVariantList objects = breakController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 2);
        QVariantMap original;
        QVariantMap piece;
        for (const QVariant &v : objects) {
            const QVariantMap m = v.toMap();
            assert(m.value("kind").toString() == QStringLiteral("line"));
            if (m.value("id").toString() == targetId) {
                original = m;
            } else {
                piece = m;
            }
        }
        assert(!original.isEmpty() && !piece.isEmpty());
        assert(nearlyEqual(original.value("x1").toDouble(), 0.2) && nearlyEqual(original.value("x2").toDouble(), 0.5));
        assert(nearlyEqual(piece.value("x1").toDouble(), 0.5) && nearlyEqual(piece.value("x2").toDouble(), 0.8));
        // The original (first piece) stays selected.
        assert(breakController.selectedObjectId() == targetId);

        // ONE undo step restores the single original line.
        assert(breakController.canUndo());
        breakController.undo();
        assert(breakController.modelDocument().value("drawing_objects").toList().size() == 1);

        // A dead break (exactly at an endpoint) surfaces via edit_status, not silent.
        DrawingDocumentController deadBreak;
        deadBreak.setSelectedToolId("line_tool");
        deadBreak.clickCanvasNormalized(0.2, 0.5);
        deadBreak.clickCanvasNormalized(0.8, 0.5);
        deadBreak.setSelectedToolId("select_move");
        deadBreak.clickCanvasNormalized(0.5, 0.5);
        assert(deadBreak.beginBreakSelectedObject());
        deadBreak.clickCanvasNormalized(0.2, 0.5); // exactly the a endpoint
        assert(!deadBreak.isAwaitingPointCapture());
        const QVariantMap status = deadBreak.modelDocument().value("edit_status").toMap();
        assert(status.value("ok").toBool() == false);
        assert(status.value("mode").toString() == "break");
        assert(!status.value("message").toString().isEmpty());
        assert(deadBreak.modelDocument().value("drawing_objects").toList().size() == 1); // unchanged

        // Break refuses to arm when the selection is neither line nor polyline.
        DrawingDocumentController nonBreakable;
        nonBreakable.setSelectedToolId("circle_tool");
        nonBreakable.clickCanvasNormalized(0.5, 0.5);
        nonBreakable.clickCanvasNormalized(0.6, 0.5);
        assert(!nonBreakable.beginBreakSelectedObject());
        assert(!nonBreakable.isAwaitingPointCapture());
    }

    // Array failure paths: a user-reachable rejection (zero spacing) must
    // surface through edit_status, reclaim its minted serials, and a later
    // success must clear the stale status. Negative spacing must march the
    // copies backwards (not abs() or clamp-to-zero anywhere en route).
    {
        DrawingDocumentController failureController;
        failureController.setSelectedToolId("line_tool");
        failureController.clickCanvasNormalized(0.5, 0.5); // line_1: serial 1
        failureController.clickCanvasNormalized(0.6, 0.5);

        failureController.setArrayCount(2);
        failureController.setArraySpacingX(0.0);
        assert(!failureController.repeatSelectedObject("x"));
        QVariantMap arrayStatus = failureController.modelDocument().value("edit_status").toMap();
        assert(arrayStatus.value("ok").toBool() == false);
        assert(arrayStatus.value("mode").toString() == "array");
        assert(!arrayStatus.value("message").toString().isEmpty());
        assert(failureController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Guides reject grid arrays at the planner; the controller surfaces
        // it the same way and creates nothing.
        // (Covered here at the planner seam; the guide tool path is separate.)

        failureController.setArraySpacingX(-0.1);
        assert(failureController.repeatSelectedObject("x"));
        QVariantList marched = failureController.modelDocument().value("drawing_objects").toList();
        assert(marched.size() == 3);
        // The failed attempt reclaimed serials 2-3, so the first successful
        // copy is repeat_2, not repeat_4.
        assert(trailingSerial(marched[1].toMap().value("id").toString()) == 2);
        // Negative spacing marches left: 0.5 + 2 * -0.1 = 0.3.
        assert(nearlyEqual(marched[2].toMap().value("x1").toDouble(), 0.3));
        // Success cleared the stale rejection.
        assert(failureController.modelDocument().value("edit_status").toMap().isEmpty());
    }

    // Guides cannot grid/radial-array (single-axis translation would stack
    // coincident copies): the controller reports failure and creates nothing.
    {
        DrawingDocumentController guideArrayController;
        guideArrayController.setSelectedToolId("horizontal_guide_tool");
        guideArrayController.clickCanvasNormalized(0.5, 0.62);
        assert(guideArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
        guideArrayController.setArrayCount(2);
        guideArrayController.setArraySpacingX(0.1);
        guideArrayController.setArraySpacingY(0.1);
        assert(!guideArrayController.gridArraySelectedObject());
        assert(guideArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
        // Radial arming succeeds (a guide IS an editable object), but the array
        // planner rejects a guide source when the centre click runs it — so the
        // rejection now lands at the pick click, and the document is untouched.
        assert(guideArrayController.beginRadialArrayCenterPick());
        guideArrayController.clickCanvasNormalized(0.3, 0.3);
        assert(!guideArrayController.isAwaitingPointCapture());
        assert(guideArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
    }

    // fixedRadius rides into creation for the radius-from-gesture tools;
    // resetting it to 0 restores gesture sizing.
    {
        DrawingDocumentController fixedRadiusController;
        fixedRadiusController.setSelectedToolId("circle_tool");
        fixedRadiusController.setFixedRadius(0.1);
        fixedRadiusController.clickCanvasNormalized(0.5, 0.5);
        fixedRadiusController.clickCanvasNormalized(0.9, 0.5); // gesture says 0.4
        QVariantList fixedObjects = fixedRadiusController.modelDocument().value("drawing_objects").toList();
        assert(nearlyEqual(fixedObjects[0].toMap().value("radius").toDouble(), 0.1));

        fixedRadiusController.setFixedRadius(0.0);
        fixedRadiusController.clickCanvasNormalized(0.5, 0.5);
        fixedRadiusController.clickCanvasNormalized(0.8, 0.5);
        fixedObjects = fixedRadiusController.modelDocument().value("drawing_objects").toList();
        assert(nearlyEqual(fixedObjects[1].toMap().value("radius").toDouble(), 0.3));
    }

    // The Seam-A authoring path: a whole room from one neutral spec. The
    // guard-antechamber layout (corridor + secret + door openings, one solid
    // wall) lands its perimeter as wall objects in a single undoable step.
    {
        DrawingDocumentController roomController;
        edi::drafting::RoomSpec spec;
        spec.origin = {0.1, 0.1};
        spec.width = 0.6;
        spec.height = 0.4;
        spec.wallThickness = 0.02;
        spec.openings = {
            {edi::drafting::RoomEdge::North, 0.30, 0.10, "corridor"},
            {edi::drafting::RoomEdge::East, 0.30, 0.04, "secret"},
            {edi::drafting::RoomEdge::South, 0.30, 0.05, "door"},
        };
        assert(roomController.createRoomFromSpec(spec));
        const QVariantList objects = roomController.modelDocument().value("drawing_objects").toList();
        assert(objects.size() == 7); // 2 + 2 + 2 (opened edges) + 1 (solid W)
        for (const QVariant &value : objects) {
            assert(value.toMap().value("kind").toString() == "wall");
        }
        // One undo removes the whole room (atomic batch, not 7 separate creates).
        assert(roomController.undo());
        assert(roomController.modelDocument().value("drawing_objects").toList().empty());

        // A degenerate spec is refused without touching the document.
        edi::drafting::RoomSpec bad;
        bad.width = 0.0;
        bad.height = 0.3;
        assert(!roomController.createRoomFromSpec(bad));
        assert(roomController.modelDocument().value("drawing_objects").toList().empty());
    }

    // S5: authored plugs land end to end — each plug's Point marker becomes an
    // object AND a DraftingPlug anchored to it is declared, all in ONE undo step.
    {
        DrawingDocumentController plugController;
        edi::drafting::RoomSpec spec;
        spec.origin = {0.2, 0.3};
        spec.width = 0.4;
        spec.height = 0.2;
        spec.wallThickness = 0.02;
        spec.plugs = {
            {edi::drafting::RoomEdge::North, 0.2, "north_door", "door"},
            {edi::drafting::RoomEdge::East, 0.1, "east_portal", "portal"},
        };
        assert(plugController.createRoomFromSpec(spec));

        const edi::drafting::DraftingDocument &doc = plugController.draftingDocument();
        assert(doc.plugs.size() == 2);
        assert(doc.objects.size() == 6); // 4 solid walls + 2 plug markers

        const edi::drafting::DraftingPlug &north = doc.plugs.front();
        assert(north.name == "north_door" && north.type == "door");
        // Each plug anchors to a real Point marker in the SAME document.
        const edi::drafting::DraftingObject *marker = edi::drafting::findObject(doc, north.anchorObjectId);
        assert(marker != nullptr);
        assert(marker->kind == edi::drafting::DraftingShapeKind::Point);
        assert(marker->metadata.toolProvenance == "plug");

        // The whole room — walls, markers, AND plugs — collapses in one undo.
        assert(plugController.undo());
        assert(plugController.draftingDocument().plugs.empty());
        assert(plugController.draftingDocument().objects.empty());
    }

    // S6: an authored connection between two plugs lands as a DraftingDeclaredConnection
    // referencing the minted plug ids — the whole graph from one file, one undo.
    {
        DrawingDocumentController connController;
        edi::drafting::RoomSpec spec;
        spec.origin = {0.2, 0.3};
        spec.width = 0.4;
        spec.height = 0.2;
        spec.wallThickness = 0.02;
        spec.plugs = {
            {edi::drafting::RoomEdge::North, 0.2, "north_door", "door"},
            {edi::drafting::RoomEdge::South, 0.2, "south_door", "door"},
        };
        spec.connections = {{"north_door", "south_door", "corridor"}};
        assert(connController.createRoomFromSpec(spec));

        const edi::drafting::DraftingDocument &doc = connController.draftingDocument();
        assert(doc.plugs.size() == 2);
        assert(doc.connections.size() == 1);
        const edi::drafting::DraftingDeclaredConnection &edge = doc.connections.front();
        assert(edge.type == "corridor");
        // The edge references the two plugs by the ids the controller minted.
        assert(edge.plugA == doc.plugs[0].id && edge.plugB == doc.plugs[1].id);

        // One undo removes walls, markers, plugs, AND the connection together.
        assert(connController.undo());
        assert(connController.draftingDocument().connections.empty());
        assert(connController.draftingDocument().plugs.empty());
    }

    // Multi-room: createMapFromSpec builds many rooms + cross-room connections,
    // with plug names namespaced by room, the whole map in ONE undo step.
    {
        edi::drafting::MapSpec map;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {0.0, 0.0};
        a.spec.width = 0.4;
        a.spec.height = 0.4;
        a.spec.wallThickness = 0.02;
        a.spec.plugs = {{edi::drafting::RoomEdge::East, 0.2, "door", "door"}};
        edi::drafting::NamedRoomSpec b;
        b.name = "b";
        b.spec.origin = {0.6, 0.0};
        b.spec.width = 0.4;
        b.spec.height = 0.4;
        b.spec.wallThickness = 0.02;
        // Same bare plug name "door" as room a — legal, names are room-scoped.
        b.spec.plugs = {{edi::drafting::RoomEdge::West, 0.2, "door", "door"}};
        map.rooms = {a, b};
        edi::drafting::MapConnectionSpec corridor;
        corridor.from = {"a", "door"};
        corridor.to = {"b", "door"};
        corridor.type = "corridor";
        map.connections = {corridor};

        DrawingDocumentController mapController;
        assert(mapController.createMapFromSpec(map));
        const edi::drafting::DraftingDocument &doc = mapController.draftingDocument();
        assert(doc.plugs.size() == 2);
        assert(doc.connections.size() == 1);
        // Each plug's exported name is namespaced room.plug, so the two "door"
        // plugs are distinguishable in the graph.
        bool foundA = false;
        bool foundB = false;
        for (const edi::drafting::DraftingPlug &p : doc.plugs) {
            foundA = foundA || p.name == "a.door";
            foundB = foundB || p.name == "b.door";
        }
        assert(foundA && foundB);
        // The connection joins the two distinct plug ids.
        const edi::drafting::DraftingDeclaredConnection &edge = doc.connections.front();
        assert(edge.plugA != edge.plugB);
        assert(edge.type == "corridor");

        // The connection also drew a corridor: wall objects tagged provenance
        // "corridor" exist (the two aligned E/W doors give a straight corridor).
        int corridorWalls = 0;
        int doorLeaves = 0;
        for (const edi::drafting::DraftingObject &o : doc.objects) {
            if (o.metadata.toolProvenance == "corridor") {
                ++corridorWalls;
            }
            if (o.metadata.toolProvenance == "door") {
                ++doorLeaves;
            }
        }
        assert(corridorWalls >= 2);
        // A door leaf per connected plug (both ends of the one connection), rendered
        // as a Door-type wall band.
        assert(doorLeaves == 2);
        for (const edi::drafting::DraftingObject &o : doc.objects) {
            if (o.metadata.toolProvenance == "door") {
                assert(o.metadata.wallVisual.type == edi::drafting::WallType::Door);
            }
        }

        // One undo collapses the entire map — every room's walls, every plug,
        // every connection, AND the corridors — together.
        assert(mapController.undo());
        assert(mapController.draftingDocument().objects.empty());
        assert(mapController.draftingDocument().plugs.empty());
        assert(mapController.draftingDocument().connections.empty());
    }

    // DM-03: interior features realize as ordinary tagged Point objects. The
    // room-local AUTHORED-FEET offset is scaled by canvasPerAuthoredUnit and added
    // to the room's CANVAS origin (the origin is not re-scaled).
    {
        edi::drafting::MapSpec map;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {1.0, 2.0}; // canvas units
        a.spec.width = 0.4;
        a.spec.height = 0.4;
        a.spec.wallThickness = 0.02;
        a.spec.features = {
            {3.0, 4.0, "rubble", "cave_in"}, // 3,4 ft offset -> canvas +{0.06,0.08}
            {5.0, 0.0, "statue", ""},         // 5,0 ft offset, no name
        };
        map.rooms = {a};

        DrawingDocumentController featureController;
        const double scale = 0.02; // canvas per authored foot
        assert(featureController.createMapFromSpec(map, scale));
        const edi::drafting::DraftingDocument &doc = featureController.draftingDocument();

        int featureCount = 0;
        bool sawRubble = false;
        bool sawStatue = false;
        for (const edi::drafting::DraftingObject &o : doc.objects) {
            if (o.metadata.toolProvenance != "feature") {
                continue;
            }
            ++featureCount;
            assert(o.kind == edi::drafting::DraftingShapeKind::Point);
            const auto tagHas = [&o](const std::string &t) {
                return std::find(o.metadata.tags.begin(), o.metadata.tags.end(), t) != o.metadata.tags.end();
            };
            const auto point = std::get<edi::drafting::PointGeometry>(o.geometry).point;
            if (tagHas("feature:rubble")) {
                sawRubble = true;
                assert(tagHas("name:cave_in"));     // named -> name:<name> tag
                assert(nearlyEqual(point.x, 1.0 + 3.0 * scale)); // 1.06
                assert(nearlyEqual(point.y, 2.0 + 4.0 * scale)); // 2.08
            } else if (tagHas("feature:statue")) {
                sawStatue = true;
                // No name -> no name: tag, only the feature: tag.
                assert(o.metadata.tags.size() == 1);
                assert(nearlyEqual(point.x, 1.0 + 5.0 * scale)); // 1.10
                assert(nearlyEqual(point.y, 2.0));               // y offset 0
            }
            // Neutral law: a feature carries NO ObjectRole.
            assert(o.metadata.role == edi::drafting::ObjectRole::None);
        }
        assert(featureCount == 2 && sawRubble && sawStatue);

        // Undo collapses the features with the rest of the map.
        assert(featureController.undo());
        assert(featureController.draftingDocument().objects.empty());
    }

    // DM-03: a room with NO features adds no extra objects (behavior unchanged).
    {
        edi::drafting::MapSpec map;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {0.0, 0.0};
        a.spec.width = 0.4;
        a.spec.height = 0.4;
        a.spec.wallThickness = 0.02;
        map.rooms = {a};

        DrawingDocumentController plainController;
        assert(plainController.createMapFromSpec(map));
        int featureCount = 0;
        for (const edi::drafting::DraftingObject &o : plainController.draftingDocument().objects) {
            if (o.metadata.toolProvenance == "feature") {
                ++featureCount;
            }
        }
        assert(featureCount == 0); // no features authored -> no feature markers
    }

    // The wall tool's thickness option rides into the freshly drawn wall; an
    // invalid value falls back to the 0.1 default (a wall is never invisible).
    {
        DrawingDocumentController wallThicknessController;
        wallThicknessController.setSelectedToolId("wall_tool");
        wallThicknessController.setWallThickness(0.25);
        wallThicknessController.clickCanvasNormalized(0.2, 0.5);
        wallThicknessController.clickCanvasNormalized(0.8, 0.5);
        QVariantList walls = wallThicknessController.modelDocument().value("drawing_objects").toList();
        assert(nearlyEqual(walls[0].toMap().value("thickness").toDouble(), 0.25));

        wallThicknessController.setWallThickness(0.0); // invalid -> 0.1 default
        assert(nearlyEqual(wallThicknessController.wallThickness(), 0.1));
        wallThicknessController.clickCanvasNormalized(0.2, 0.6);
        wallThicknessController.clickCanvasNormalized(0.8, 0.6);
        walls = wallThicknessController.modelDocument().value("drawing_objects").toList();
        assert(nearlyEqual(walls[1].toMap().value("thickness").toDouble(), 0.1));
    }

    // Phase C block library — define a block from a selection, then stamp a
    // FLATTEN instance: independent transformed copies, one undo step, the source
    // definition byte-unchanged (the independence the FLATTEN fork buys).
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value("drawing_objects").toList().size();
        };

        DrawingDocumentController blockCtl;
        blockCtl.setSelectedToolId("point_tool");
        blockCtl.clickCanvasNormalized(0.3, 0.3);
        blockCtl.clickCanvasNormalized(0.6, 0.6);
        assert(objectCount(blockCtl) == 2);

        // Define with nothing selected is refused (a dead button must say so).
        blockCtl.selectObjectsInBoundsNormalized(0.9, 0.9, 0.95, 0.95);
        assert(!blockCtl.defineBlockFromSelection("table"));
        assert(blockCtl.draftingDocument().blocks.empty());

        // Marquee-select both points and save them as a named block. The
        // definition lands in one undo step; its members are normalized to the
        // origin (lower-left at 0,0), extent = the selection's union span.
        blockCtl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(blockCtl.defineBlockFromSelection("table", "recipe.tavern_table"));
        assert(blockCtl.draftingDocument().blocks.size() == 1);
        const edi::drafting::DraftingBlock &def = blockCtl.draftingDocument().blocks.front();
        const QString blockId = QString::fromStdString(def.id);
        assert(blockId.startsWith("block_"));
        assert(def.name == "table");
        assert(def.assetRef == "recipe.tavern_table"); // Seam B: linked at define time
        assert(def.objects.size() == 2);
        assert(nearlyEqual(def.bounds.x, 0.0) && nearlyEqual(def.bounds.y, 0.0));
        assert(nearlyEqual(def.bounds.width, 0.3) && nearlyEqual(def.bounds.height, 0.3));
        // Capture the normalized definition to prove independence after placement.
        const auto defPointA = std::get<edi::drafting::PointGeometry>(def.objects[0].geometry).point;
        assert(nearlyEqual(defPointA.x, 0.0) && nearlyEqual(defPointA.y, 0.0));

        // Defining is one undo step (the document objects are untouched by it).
        blockCtl.undo();
        assert(blockCtl.draftingDocument().blocks.empty());
        assert(objectCount(blockCtl) == 2);
        blockCtl.redo();
        assert(blockCtl.draftingDocument().blocks.size() == 1);

        // Stamping an unknown block is a no-op.
        assert(!blockCtl.placeBlockInstance("block_9999", 0.5, 0.5));
        assert(objectCount(blockCtl) == 2);

        // Stamp the block centred on (0.5,0.5): two fresh "instance_" objects
        // appear (centre 0.15,0.15 -> offset 0.35,0.35), selected as one unit.
        const int beforeStamp = objectCount(blockCtl);
        assert(blockCtl.placeBlockInstance(blockId, 0.5, 0.5));
        assert(objectCount(blockCtl) == beforeStamp + 2);
        assert(blockCtl.modelDocument().value("selected_object_ids").toList().size() == 2);

        // The placed objects are independent shapes carrying the centred geometry,
        // and every id in the document stays unique.
        {
            const QVariantList objects = blockCtl.modelDocument().value("drawing_objects").toList();
            QSet<QString> ids;
            int instanceCount = 0;
            for (const QVariant &value : objects) {
                const QVariantMap obj = value.toMap();
                ids.insert(obj.value("id").toString());
                if (obj.value("id").toString().startsWith("instance_")) {
                    ++instanceCount;
                    const double px = obj.value("x").toDouble();
                    const double py = obj.value("y").toDouble();
                    // A maps to (0.35,0.35), B to (0.65,0.65).
                    assert((nearlyEqual(px, 0.35) && nearlyEqual(py, 0.35))
                           || (nearlyEqual(px, 0.65) && nearlyEqual(py, 0.65)));
                }
            }
            assert(instanceCount == 2);
            assert(ids.size() == objects.size());
        }

        // Seam B provenance: every placed object is traceable to its block + asset,
        // and the two share ONE instance id (so Seam C can re-form the placement).
        {
            QString sharedInstanceId;
            int stamped = 0;
            for (const edi::drafting::DraftingObject &obj : blockCtl.draftingDocument().objects) {
                const auto &bp = obj.metadata.blockPlacement;
                if (bp.instanceId.empty()) {
                    continue; // an original (hand-drawn) object, not a placement
                }
                ++stamped;
                // `blockId` is a stable VALUE captured before the undo/redo above
                // (which repopulated draftingDocument().blocks, so the `def`
                // reference is stale here — read the value, not the reference).
                assert(bp.blockId == blockId.toStdString());
                assert(bp.assetRef == "recipe.tavern_table");
                if (sharedInstanceId.isEmpty()) {
                    sharedInstanceId = QString::fromStdString(bp.instanceId);
                } else {
                    assert(QString::fromStdString(bp.instanceId) == sharedInstanceId); // one placement
                }
            }
            assert(stamped == 2);
            assert(sharedInstanceId.startsWith("blockinst_"));
        }

        // Independence (FLATTEN): the definition is byte-unchanged by placement,
        // and stamping is exactly one undo step.
        const auto defPointAfter = std::get<edi::drafting::PointGeometry>(
            blockCtl.draftingDocument().blocks.front().objects[0].geometry).point;
        assert(nearlyEqual(defPointAfter.x, 0.0) && nearlyEqual(defPointAfter.y, 0.0));
        blockCtl.undo();
        assert(objectCount(blockCtl) == beforeStamp);
        assert(blockCtl.draftingDocument().blocks.size() == 1); // definition survives
        blockCtl.redo();
        assert(objectCount(blockCtl) == beforeStamp + 2);
    }

    // DM-14: place a rotated/scaled block. A block of a rectangle (a faithfully-
    // transforming, center-anchored shape) placed at identity is byte-identical to the
    // pre-DM-14 stamp; placed at 90deg/x2 its geometry transforms about the click and
    // the placement metadata records the transform.
    {
        const auto placedRect = [](DrawingDocumentController &c) -> edi::drafting::RectangleGeometry {
            for (const edi::drafting::DraftingObject &o : c.draftingDocument().objects) {
                if (o.kind == edi::drafting::DraftingShapeKind::Rectangle
                    && !o.metadata.blockPlacement.instanceId.empty()) {
                    return std::get<edi::drafting::RectangleGeometry>(o.geometry);
                }
            }
            assert(false && "no placed rectangle");
            return {};
        };

        DrawingDocumentController ctl;
        ctl.setSelectedToolId("rectangle_tool");
        ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.clickCanvasNormalized(0.5, 0.5); // 0.2 x 0.2 rectangle
        ctl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(ctl.defineBlockFromSelection("box"));
        const QString blockId = QString::fromStdString(ctl.draftingDocument().blocks.front().id);

        // Setter guards: a non-finite/non-positive value is rejected (state kept).
        ctl.setBlockPlacementScale(2.0);
        ctl.setBlockPlacementScale(0.0);   // invalid -> stays 2.0
        ctl.setBlockPlacementScale(-1.0);  // invalid -> stays 2.0
        assert(nearlyEqual(ctl.blockPlacementScale(), 2.0));
        ctl.setBlockPlacementRotation(90.0);
        assert(nearlyEqual(ctl.blockPlacementRotation(), 90.0));

        // IDENTITY placement (0deg / 1.0) is byte-identical to the pre-DM-14 stamp: the
        // rectangle keeps its 0.2 footprint and identity placement metadata.
        ctl.setBlockPlacementRotation(0.0);
        ctl.setBlockPlacementScale(1.0);
        assert(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        {
            const edi::drafting::RectangleGeometry r = placedRect(ctl);
            assert(nearlyEqual(r.width, 0.2) && nearlyEqual(r.height, 0.2)); // NOT scaled
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (!o.metadata.blockPlacement.instanceId.empty()) {
                    assert(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 0.0));
                    assert(nearlyEqual(o.metadata.blockPlacement.scale, 1.0));
                }
            }
        }
        ctl.undo(); // drop the identity stamp

        // 90deg / x2 placement: the rectangle scales (0.2 -> 0.4) and its metadata
        // records the transform.
        ctl.setBlockPlacementRotation(90.0);
        ctl.setBlockPlacementScale(2.0);
        assert(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        {
            const edi::drafting::RectangleGeometry r = placedRect(ctl);
            assert(nearlyEqual(r.width, 0.4) && nearlyEqual(r.height, 0.4)); // scaled x2
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (!o.metadata.blockPlacement.instanceId.empty()) {
                    assert(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 90.0));
                    assert(nearlyEqual(o.metadata.blockPlacement.scale, 2.0));
                }
            }
        }
    }

    // DM-15: transform a PLACED instance about its group centre, the metadata
    // accumulating, in one undo step.
    {
        const auto placedRect = [](DrawingDocumentController &c) -> edi::drafting::RectangleGeometry {
            for (const edi::drafting::DraftingObject &o : c.draftingDocument().objects) {
                if (o.kind == edi::drafting::DraftingShapeKind::Rectangle
                    && !o.metadata.blockPlacement.instanceId.empty()) {
                    return std::get<edi::drafting::RectangleGeometry>(o.geometry);
                }
            }
            assert(false && "no placed rectangle");
            return {};
        };

        DrawingDocumentController ctl;
        ctl.setSelectedToolId("rectangle_tool");
        ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.clickCanvasNormalized(0.5, 0.5);
        ctl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(ctl.defineBlockFromSelection("box"));
        const QString blockId = QString::fromStdString(ctl.draftingDocument().blocks.front().id);

        // Place a 90deg / x2 instance (rectangle now 0.4) and capture its instance id.
        ctl.setBlockPlacementRotation(90.0);
        ctl.setBlockPlacementScale(2.0);
        assert(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        std::string instanceId;
        for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                instanceId = o.metadata.blockPlacement.instanceId;
            }
        }
        assert(!instanceId.empty());

        // A bad instance id refuses with no change.
        const std::uint64_t revBefore = ctl.draftingDocument().revision;
        assert(!ctl.transformBlockInstance(QStringLiteral("blockinst_nope"), 45.0, 1.5));
        assert(ctl.draftingDocument().revision == revBefore);
        // A non-positive scale factor refuses too (NaN/range guard).
        assert(!ctl.transformBlockInstance(QString::fromStdString(instanceId), 45.0, 0.0));

        // Transform by (+45deg, x1.5): geometry scales again (0.4 -> 0.6) and the
        // metadata ACCUMULATES (90+45=135, 2*1.5=3.0) in one undo step.
        assert(ctl.transformBlockInstance(QString::fromStdString(instanceId), 45.0, 1.5));
        {
            const edi::drafting::RectangleGeometry r = placedRect(ctl);
            assert(nearlyEqual(r.width, 0.6) && nearlyEqual(r.height, 0.6)); // 0.4 x 1.5
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (o.metadata.blockPlacement.instanceId == instanceId) {
                    assert(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 135.0));
                    assert(nearlyEqual(o.metadata.blockPlacement.scale, 3.0));
                }
            }
        }
        // One undo reverts the whole group transform (back to 0.4).
        assert(ctl.undo());
        assert(nearlyEqual(placedRect(ctl).width, 0.4));
    }

    // DM-15 hardening: a pathological-but-finite scale sequence must not compose to a
    // non-finite value. A placement scale of 1e200 then a transform factor of 1e200
    // overflows to inf; the member is REFUSED (skipped), so its metadata scale stays
    // finite (1e200, not inf) — the codec never persists a non-finite value.
    {
        const auto placedRect = [](DrawingDocumentController &c) -> edi::drafting::RectangleGeometry {
            for (const edi::drafting::DraftingObject &o : c.draftingDocument().objects) {
                if (o.kind == edi::drafting::DraftingShapeKind::Rectangle
                    && !o.metadata.blockPlacement.instanceId.empty()) {
                    return std::get<edi::drafting::RectangleGeometry>(o.geometry);
                }
            }
            assert(false && "no placed rectangle");
            return {};
        };

        DrawingDocumentController ctl;
        ctl.setSelectedToolId("rectangle_tool");
        ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.clickCanvasNormalized(0.5, 0.5);
        ctl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        assert(ctl.defineBlockFromSelection("box"));
        const QString blockId = QString::fromStdString(ctl.draftingDocument().blocks.front().id);

        // A huge-but-finite placement scale is accepted (finite, > 0).
        ctl.setBlockPlacementScale(1e200);
        assert(nearlyEqual(ctl.blockPlacementScale(), 1e200));
        assert(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        std::string instanceId;
        for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                instanceId = o.metadata.blockPlacement.instanceId;
            }
        }
        assert(!instanceId.empty());
        assert(std::isfinite(ctl.draftingDocument().objects.back().metadata.blockPlacement.scale));

        // 1e200 * 1e200 -> inf: the member is refused, leaving scale finite (1e200).
        ctl.transformBlockInstance(QString::fromStdString(instanceId), 0.0, 1e200);
        for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
            if (o.metadata.blockPlacement.instanceId == instanceId) {
                assert(std::isfinite(o.metadata.blockPlacement.scale)); // never inf
                assert(nearlyEqual(o.metadata.blockPlacement.scale, 1e200)); // unchanged
            }
        }
        // The geometry stayed finite too (no inf coords written).
        assert(std::isfinite(placedRect(ctl).width));
    }

    // Seam C: createMapFromSpec records its rooms on the document (name + authored
    // footprint + material), atomically with the walls — one undo clears them all.
    {
        edi::drafting::MapSpec spec;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {0.1, 0.1};
        a.spec.width = 0.2;
        a.spec.height = 0.15;
        a.spec.wallMaterial = "stone";
        spec.rooms.push_back(a);
        edi::drafting::NamedRoomSpec b;
        b.name = "b";
        b.spec.origin = {0.5, 0.1};
        b.spec.width = 0.2;
        b.spec.height = 0.15;
        b.spec.wallMaterial = "wood";
        spec.rooms.push_back(b);

        DrawingDocumentController mapCtl;
        assert(mapCtl.createMapFromSpec(spec, 0.02)); // authored at 0.02 canvas/ft
        assert(nearlyEqual(mapCtl.draftingDocument().canvasPerAuthoredUnit, 0.02)); // scale recorded
        assert(mapCtl.draftingDocument().rooms.size() == 2);
        const edi::drafting::DraftingMapRoom &ra = mapCtl.draftingDocument().rooms[0];
        assert(ra.name == "a" && ra.material == "stone");
        assert(nearlyEqual(ra.origin.x, 0.1) && nearlyEqual(ra.origin.y, 0.1));
        assert(nearlyEqual(ra.width, 0.2) && nearlyEqual(ra.height, 0.15));
        assert(mapCtl.draftingDocument().rooms[1].name == "b");

        // Atomic with the map create: one undo clears the rooms (and the walls).
        mapCtl.undo();
        assert(mapCtl.draftingDocument().rooms.empty());
        mapCtl.redo();
        assert(mapCtl.draftingDocument().rooms.size() == 2);

        // DM-07/08 PERSISTENCE LEG: an EDITED document survives .edidraw save/reload
        // with its rooms intact — the proof that Seam C reads rooms FROM the document
        // (not the transient MapSpec). Encode to bytes, decode, and assert every
        // room's name + footprint + material is byte-faithful. This is what would
        // FAIL if mapRoomValue/readMapRoom ever dropped a field.
        const edi::formats::ByteBuffer bytes =
            edi::drafting::encodeDraftingDocument(mapCtl.draftingDocument());
        const auto reloaded = edi::drafting::decodeDraftingDocument(bytes, "roundtrip");
        assert(reloaded.ok && reloaded.value);
        const auto &saved = mapCtl.draftingDocument().rooms;
        const auto &back = reloaded.value->rooms;
        assert(back.size() == saved.size() && back.size() == 2);
        for (std::size_t i = 0; i < saved.size(); ++i) {
            assert(back[i].name == saved[i].name);
            assert(back[i].material == saved[i].material);
            assert(nearlyEqual(back[i].origin.x, saved[i].origin.x));
            assert(nearlyEqual(back[i].origin.y, saved[i].origin.y));
            assert(nearlyEqual(back[i].width, saved[i].width));
            assert(nearlyEqual(back[i].height, saved[i].height));
        }
    }

    // DM-10: region fill — arm a pick-a-point capture, then a click inside a room
    // mints a filled Polygon of its footprint, auto-selected in one undo step.
    {
        edi::drafting::MapSpec spec;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {0.1, 0.1}; // canvas; footprint spans [0.1,0.4] in both axes
        a.spec.width = 0.3;
        a.spec.height = 0.3;
        a.spec.wallThickness = 0.02;
        spec.rooms.push_back(a);

        DrawingDocumentController fillCtl;
        assert(fillCtl.createMapFromSpec(spec, 0.02));
        const int objectsAfterMap = static_cast<int>(fillCtl.draftingDocument().objects.size());
        const std::uint64_t revAfterMap = fillCtl.draftingDocument().revision;

        // Arming on a doc WITH rooms returns true, exposes the prompt, and does NOT
        // touch the document (revision unchanged).
        assert(fillCtl.beginRegionFillPick());
        assert(fillCtl.isAwaitingPointCapture());
        assert(fillCtl.pointCapturePrompt() == QStringLiteral("Click inside a room to fill"));
        assert(fillCtl.draftingDocument().revision == revAfterMap);

        // A click INSIDE the room footprint mints exactly one Polygon, filled
        // (opacity > 0), auto-selected as the active object, revision bumped once,
        // capture cleared.
        fillCtl.clickCanvasNormalized(0.25, 0.25);
        assert(!fillCtl.isAwaitingPointCapture()); // capture consumed
        const auto &objs = fillCtl.draftingDocument().objects;
        assert(static_cast<int>(objs.size()) == objectsAfterMap + 1);
        const edi::drafting::DraftingObject &poly = objs.back();
        assert(poly.kind == edi::drafting::DraftingShapeKind::Polygon);
        assert(poly.fill.opacity > 0.0);
        assert(fillCtl.draftingDocument().activeObjectId.has_value()
               && *fillCtl.draftingDocument().activeObjectId == poly.id);
        // The fill bumped the revision (it is one ATOMIC edit — createObjectsAndSelect
        // brackets the create+select in a single beginEdit/commitEdit, so it collapses
        // to ONE undo step, asserted below; the raw counter bumps per command).
        assert(fillCtl.draftingDocument().revision > revAfterMap);
        // Neutral: the fill carries NO ObjectRole (presentation only).
        assert(poly.metadata.role == edi::drafting::ObjectRole::None);

        // One undo removes the whole fill (the atomicity proof).
        assert(fillCtl.undo());
        assert(static_cast<int>(fillCtl.draftingDocument().objects.size()) == objectsAfterMap);

        // Armed → click in OPEN SPACE → no object created, capture cleared, refusal
        // surfaced (the document gains nothing).
        assert(fillCtl.beginRegionFillPick());
        const int before = static_cast<int>(fillCtl.draftingDocument().objects.size());
        fillCtl.clickCanvasNormalized(0.9, 0.9); // outside the only room
        assert(!fillCtl.isAwaitingPointCapture());
        assert(static_cast<int>(fillCtl.draftingDocument().objects.size()) == before);

        // Arming on an EMPTY-rooms document refuses (no dead prompt).
        DrawingDocumentController emptyCtl;
        assert(!emptyCtl.beginRegionFillPick());
        assert(!emptyCtl.isAwaitingPointCapture());
    }

    // B2-1: interactive plug-placement tool.
    // beginPlugPick() arms a PlugPlacement capture; a canvas click mints exactly one
    // Point marker + one DraftingPlug in one undo step; one undo removes both.
    {
        DrawingDocumentController plugCtl;

        // Arming: prompt set, document untouched — same up-front test as other
        // pick tools (region fill, block instance).
        assert(plugCtl.beginPlugPick());
        assert(plugCtl.isAwaitingPointCapture());
        assert(plugCtl.pointCapturePrompt() == QStringLiteral("Click a wall to place a plug"));
        const std::uint64_t revBefore = plugCtl.draftingDocument().revision;
        assert(plugCtl.draftingDocument().plugs.empty());
        assert(plugCtl.draftingDocument().objects.empty());

        // A click at (0.5, 0.5): capture consumed, one marker + one plug created,
        // revision bumped. objectSnapEnabled is false on a fresh controller, so the
        // click lands at the raw canvas coordinate with no snap interference.
        plugCtl.clickCanvasNormalized(0.5, 0.5);
        assert(!plugCtl.isAwaitingPointCapture()); // capture consumed
        {
            const edi::drafting::DraftingDocument &doc = plugCtl.draftingDocument();
            assert(doc.plugs.size() == 1);
            assert(doc.objects.size() == 1); // the anchor Point marker
            assert(doc.revision > revBefore);

            const edi::drafting::DraftingPlug &plug = doc.plugs.front();
            assert(plug.type == "door"); // default neutral type — no game rule
            // anchor cache matches the click position (no snap offset).
            assert(nearlyEqual(plug.anchor.x, 0.5) && nearlyEqual(plug.anchor.y, 0.5));

            // The plug anchors to the minted Point marker.
            const edi::drafting::DraftingObject *marker =
                edi::drafting::findObject(doc, plug.anchorObjectId);
            assert(marker != nullptr);
            assert(marker->kind == edi::drafting::DraftingShapeKind::Point);
            assert(marker->metadata.toolProvenance == "plug");
        }

        // One undo removes BOTH marker and plug — they are in one undo step.
        assert(plugCtl.undo());
        assert(plugCtl.draftingDocument().plugs.empty());
        assert(plugCtl.draftingDocument().objects.empty());
    }

    // B2-2: interactive connection tool — two-click capture + on-demand corridor.
    // beginConnectionPick() arms a PlugConnect capture; first click stores plug A and
    // advances the prompt; second click (on a DIFFERENT plug) declares one connection
    // and emits corridor walls all tagged "connection:<connId>"; one undo removes the
    // connection AND all corridor objects; a same-plug second click refuses with no change.
    {
        DrawingDocumentController connCtl;

        // Place two plugs at known canvas positions (objectSnap disabled on a fresh
        // controller, so no snapping — markers land at exactly these coordinates).
        connCtl.beginPlugPick();
        connCtl.clickCanvasNormalized(0.3, 0.3); // plug A anchor at (0.3, 0.3)
        connCtl.beginPlugPick();
        connCtl.clickCanvasNormalized(0.7, 0.7); // plug B anchor at (0.7, 0.7)

        {
            const edi::drafting::DraftingDocument &doc = connCtl.draftingDocument();
            assert(doc.plugs.size() == 2);
        }
        const std::uint64_t revAfterPlugs   = connCtl.draftingDocument().revision;
        const int           markersAfterPlug = static_cast<int>(connCtl.draftingDocument().objects.size()); // 2

        // Arm the connection tool.
        assert(connCtl.beginConnectionPick());
        assert(connCtl.isAwaitingPointCapture());
        assert(connCtl.pointCapturePrompt() == QStringLiteral("Click the first plug"));
        // Arming must NOT touch the document.
        assert(connCtl.draftingDocument().revision == revAfterPlugs);

        // First click on plug A's marker — prompt advances, no connection yet.
        connCtl.clickCanvasNormalized(0.3, 0.3);
        assert(connCtl.isAwaitingPointCapture()); // still armed for second click
        assert(connCtl.pointCapturePrompt() == QStringLiteral("Click the second plug"));
        assert(connCtl.draftingDocument().connections.empty()); // not yet connected

        // Second click on SAME plug A — refuse, tool disarms, no change.
        connCtl.clickCanvasNormalized(0.3, 0.3);
        assert(!connCtl.isAwaitingPointCapture()); // tool disarmed on refusal
        assert(connCtl.draftingDocument().connections.empty()); // still no connection
        // Document content must be unchanged (refusal = edit-status only, no object change).
        assert(static_cast<int>(connCtl.draftingDocument().objects.size()) == markersAfterPlug);

        // Re-arm and complete the full two-click flow.
        assert(connCtl.beginConnectionPick());
        connCtl.clickCanvasNormalized(0.3, 0.3); // first click → plug A stored
        connCtl.clickCanvasNormalized(0.7, 0.7); // second click → plug B → connect!

        assert(!connCtl.isAwaitingPointCapture()); // capture consumed
        {
            const edi::drafting::DraftingDocument &doc = connCtl.draftingDocument();
            assert(doc.connections.size() == 1); // one connection declared

            const edi::drafting::DraftingDeclaredConnection &conn = doc.connections.front();
            // The connection references the two minted plug ids.
            assert(conn.plugA == doc.plugs[0].id && conn.plugB == doc.plugs[1].id);

            // Every emitted corridor wall carries tag "connection:<connId>". The tag is
            // the neutral open-vocabulary breadcrumb (like "feature:<type>") — no new
            // metadata field, no codec change — so delete/re-route can filter by it.
            const std::string expectedTag = "connection:" + conn.id;
            int taggedWallCount = 0;
            for (const edi::drafting::DraftingObject &obj : doc.objects) {
                for (const std::string &tag : obj.metadata.tags) {
                    if (tag == expectedTag) {
                        ++taggedWallCount;
                        break;
                    }
                }
            }
            // corridorWalls() produces 2*segments walls; for two distinct door points
            // the minimum centerline has >=2 segments → >=4 tagged walls.
            assert(taggedWallCount > 0);
        }

        // One undo removes the connection AND all corridor objects in one step,
        // leaving only the two plug markers (the atomicity proof).
        assert(connCtl.undo());
        assert(connCtl.draftingDocument().connections.empty());
        assert(static_cast<int>(connCtl.draftingDocument().objects.size()) == markersAfterPlug);
    }

    // DM-15 block-instance projection keys (brief 021).
    // has_block_instance_selection and instance_id are PURE projection derives:
    // they read the active object's blockPlacement.instanceId and surface it
    // to the inspector so it can (a) show the "Block instance" section only when
    // relevant and (b) pass the right id to transformBlockInstance().
    //
    // Three sub-cases mirror the acceptance criteria in brief 021:
    //   A) block-instance object active  → true / non-empty id
    //   B) ordinary object active        → false / ""
    //   C) nothing selected              → false / ""
    {
        // Set up: one ordinary point, one block instance (a single point block
        // placed at 0.7,0.7). After placeBlockInstance the placed objects are
        // selected and the last one is the active object.
        DrawingDocumentController projCtl;
        projCtl.setSelectedToolId("point_tool");
        projCtl.clickCanvasNormalized(0.1, 0.1); // ordinary point
        const std::string ordinaryId = projCtl.draftingDocument().objects.back().id;

        projCtl.selectObjectsInBoundsNormalized(0.05, 0.05, 0.15, 0.15);
        assert(projCtl.defineBlockFromSelection("dot"));
        const QString blkId = QString::fromStdString(projCtl.draftingDocument().blocks.front().id);

        assert(projCtl.placeBlockInstance(blkId, 0.7, 0.7));

        // Capture the shared instance id from the document.
        std::string placedInstanceId;
        for (const edi::drafting::DraftingObject &o : projCtl.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                placedInstanceId = o.metadata.blockPlacement.instanceId;
                break;
            }
        }
        assert(!placedInstanceId.empty());

        // Case A: the active object IS a block-instance placement → keys are
        // true and the instance id.  placeBlockInstance auto-selects the stamped
        // objects, so one of them is already active.
        {
            const QVariantMap model = projCtl.modelDocument();
            assert(model.value(QStringLiteral("has_block_instance_selection")).toBool() == true);
            assert(model.value(QStringLiteral("instance_id")).toString()
                   == QString::fromStdString(placedInstanceId));
        }

        // Case B: select the ordinary point (non-placement) → keys revert to
        // false / "".
        assert(projCtl.selectObjectById(QString::fromStdString(ordinaryId)));
        {
            const QVariantMap model = projCtl.modelDocument();
            assert(model.value(QStringLiteral("has_block_instance_selection")).toBool() == false);
            assert(model.value(QStringLiteral("instance_id")).toString().isEmpty());
        }

        // Case C: deselect everything (empty selection, no active object) →
        // keys are false / "".
        projCtl.selectObjectsInBoundsNormalized(0.99, 0.99, 1.0, 1.0); // empty region
        assert(projCtl.draftingDocument().selectedObjectIds.empty());
        {
            const QVariantMap model = projCtl.modelDocument();
            assert(model.value(QStringLiteral("has_block_instance_selection")).toBool() == false);
            assert(model.value(QStringLiteral("instance_id")).toString().isEmpty());
        }
    }

    return 0;
}
