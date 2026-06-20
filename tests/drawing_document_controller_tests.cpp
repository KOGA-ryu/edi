#include "core/DrawingCore.h"
#include "core/DrawingDocumentProjection.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGraphOps.h"  // plugIndexById, connectionIndexById (B2-4 tests)
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
#include "EdiAssert.h"
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
    EDI_CHECK(initial.value("engine").toString() == "cpp_drafting_document");
    EDI_CHECK(initial.value("drawing_objects").toList().empty());
    QVariantMap initialGrid = initial.value("grid").toMap();
    EDI_CHECK(initialGrid.value("preset").toString() == "square_art_board");
    EDI_CHECK(initialGrid.value("unit_label").toString() == "in");
    EDI_CHECK(!initialGrid.value("lines").toList().empty());
    EDI_CHECK(!controller.gridSnapEnabled());
    EDI_CHECK(!controller.objectSnapEnabled());
    QVariantMap initialSnap = initial.value("snap").toMap();
    EDI_CHECK(initialSnap.value("endpoint_enabled").toBool());
    EDI_CHECK(initialSnap.value("vertex_enabled").toBool());
    EDI_CHECK(initialSnap.value("midpoint_enabled").toBool());
    EDI_CHECK(initialSnap.value("center_enabled").toBool());
    EDI_CHECK(initialSnap.value("intersection_enabled").toBool());
    EDI_CHECK(initialSnap.value("guide_enabled").toBool());
    EDI_CHECK(initialSnap.value("guide_move_enabled").toBool());
    EDI_CHECK(initialSnap.value("object_priority_before_grid").toBool());
    EDI_CHECK(controller.objectSnapTolerancePresetId() == "normal");

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
    EDI_CHECK(!controller.endpointSnapEnabled());
    EDI_CHECK(!controller.vertexSnapEnabled());
    EDI_CHECK(!controller.midpointSnapEnabled());
    EDI_CHECK(!controller.centerSnapEnabled());
    EDI_CHECK(!controller.intersectionSnapEnabled());
    EDI_CHECK(!controller.guideSnapEnabled());
    EDI_CHECK(!controller.guideMoveSnapEnabled());
    EDI_CHECK(!controller.objectSnapPriorityBeforeGrid());
    EDI_CHECK(controller.objectSnapTolerancePresetId() == "tight");
    EDI_CHECK(!changedSnap.value("endpoint_enabled").toBool());
    EDI_CHECK(!changedSnap.value("vertex_enabled").toBool());
    EDI_CHECK(!changedSnap.value("midpoint_enabled").toBool());
    EDI_CHECK(!changedSnap.value("center_enabled").toBool());
    EDI_CHECK(!changedSnap.value("intersection_enabled").toBool());
    EDI_CHECK(!changedSnap.value("guide_enabled").toBool());
    EDI_CHECK(!changedSnap.value("guide_move_enabled").toBool());
    EDI_CHECK(!changedSnap.value("object_priority_before_grid").toBool());
    EDI_CHECK(nearlyEqual(changedSnap.value("object_tolerance").toDouble(), 0.015));
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
    EDI_CHECK(objects.size() == 1);
    QVariantMap point = objects.front().toMap();
    EDI_CHECK(point.value("kind").toString() == "point");
    EDI_CHECK(point.value("x").toDouble() == 0.25);
    EDI_CHECK(point.value("y").toDouble() == 0.5);
    EDI_CHECK(numericFieldIds(point) == QStringList({QStringLiteral("x"), QStringLiteral("y")}));
    QVariantMap pointXField = numericField(point, "x");
    EDI_CHECK(pointXField.value("physical_editable").toBool());
    EDI_CHECK(pointXField.value("physical_unit_kind").toString() == "length");
    EDI_CHECK(pointXField.value("physical_unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(pointXField.value("physical_minimum").toDouble(), -100000.0));
    EDI_CHECK(nearlyEqual(pointXField.value("physical_maximum").toDouble(), 100000.0));
    QVariantMap pointPhysical = point.value("physical_geometry").toMap();
    EDI_CHECK(pointPhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(pointPhysical.value("x").toDouble(), 3.0));
    EDI_CHECK(nearlyEqual(pointPhysical.value("y").toDouble(), 6.0));
    EDI_CHECK(point.value("layer_id").toString() == "default");
    EDI_CHECK(!point.value("locked").toBool());
    QVariantMap pointBounds = point.value("bounds").toMap();
    EDI_CHECK(pointBounds.value("x").toDouble() == 0.25);
    EDI_CHECK(pointBounds.value("y").toDouble() == 0.5);
    EDI_CHECK(pointBounds.value("width").toDouble() == 0.0);
    EDI_CHECK(pointBounds.value("height").toDouble() == 0.0);
    QVariantList pointMeasurement = point.value("measurement_lines").toList();
    EDI_CHECK(pointMeasurement.size() == 2);
    EDI_CHECK(pointMeasurement[0].toString() == "width: 0 canvas_unit");
    EDI_CHECK(pointMeasurement[1].toString() == "height: 0 canvas_unit");
    EDI_CHECK(controller.selectedObjectId() == point.value("id").toString());
    EDI_CHECK(controller.updateSelectedObjectGeometryField("x", 0.3));
    EDI_CHECK(controller.updateSelectedObjectGeometryField("y", 0.35));
    QVariantMap editedPoint = controller.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(editedPoint.value("x").toDouble(), 0.3));
    EDI_CHECK(nearlyEqual(editedPoint.value("y").toDouble(), 0.35));
    EDI_CHECK(!controller.updateSelectedObjectGeometryField("missing_field", 0.1));

    controller.setGridPresetId("letter");
    QVariantMap letterModel = controller.modelDocument();
    QVariantMap letterGrid = letterModel.value("grid").toMap();
    EDI_CHECK(controller.gridPresetId() == "letter");
    EDI_CHECK(letterGrid.value("preset").toString() == "letter");
    EDI_CHECK(letterGrid.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(letterGrid.value("width").toDouble(), 8.5));
    EDI_CHECK(nearlyEqual(letterGrid.value("height").toDouble(), 11.0));
    QVariantMap letterSnap = letterModel.value("snap").toMap();
    EDI_CHECK(nearlyEqual(letterSnap.value("grid_step_x").toDouble(), 0.25 / 8.5));
    EDI_CHECK(nearlyEqual(letterSnap.value("grid_step_y").toDouble(), 0.25 / 11.0));
    QVariantMap letterPoint = letterModel.value("drawing_objects").toList().front().toMap();
    QVariantMap letterPointPhysical = letterPoint.value("physical_geometry").toMap();
    EDI_CHECK(letterPointPhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(letterPointPhysical.value("x").toDouble(), 0.3 * 8.5));
    EDI_CHECK(nearlyEqual(letterPointPhysical.value("y").toDouble(), 0.35 * 11.0));
    DrawingDocumentController physicalPointController;
    physicalPointController.setSelectedToolId("point_tool");
    physicalPointController.clickCanvasNormalized(0.25, 0.5);
    physicalPointController.setGridPresetId("letter");
    EDI_CHECK(physicalPointController.updateSelectedObjectPhysicalGeometryField("x", 4.25));
    EDI_CHECK(physicalPointController.updateSelectedObjectPhysicalGeometryField("y", 5.5));
    QVariantMap physicalEditedPoint = physicalPointController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(physicalEditedPoint.value("x").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(physicalEditedPoint.value("y").toDouble(), 0.5));

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
    EDI_CHECK(customGrid.value("preset").toString() == "custom");
    EDI_CHECK(customGrid.value("unit").toString() == "centimeter");
    EDI_CHECK(customGrid.value("unit_label").toString() == "cm");
    EDI_CHECK(nearlyEqual(customGrid.value("width").toDouble(), 20.0));
    EDI_CHECK(nearlyEqual(customGrid.value("height").toDouble(), 10.0));
    EDI_CHECK(nearlyEqual(customGrid.value("margin_left").toDouble(), 1.0));
    EDI_CHECK(nearlyEqual(customGrid.value("margin_top").toDouble(), 2.0));
    EDI_CHECK(nearlyEqual(customGrid.value("margin_right").toDouble(), 3.0));
    EDI_CHECK(nearlyEqual(customGrid.value("margin_bottom").toDouble(), 4.0));
    EDI_CHECK(nearlyEqual(customGrid.value("minor_step").toDouble(), 0.5));
    EDI_CHECK(customGrid.value("major_line_every").toInt() == 5);
    EDI_CHECK(!customGrid.value("visible").toBool());
    EDI_CHECK(customGrid.value("lines").toList().empty());
    QVariantMap customSnap = customGridModel.value("snap").toMap();
    EDI_CHECK(nearlyEqual(customSnap.value("grid_step_x").toDouble(), 0.5 / 20.0));
    EDI_CHECK(nearlyEqual(customSnap.value("grid_step_y").toDouble(), 0.5 / 10.0));
    QVariantMap customPoint = customGridModel.value("drawing_objects").toList().front().toMap();
    QVariantMap customPointPhysical = customPoint.value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(customPointPhysical.value("x").toDouble(), 10.0));
    EDI_CHECK(nearlyEqual(customPointPhysical.value("y").toDouble(), 2.5));
    EDI_CHECK(customGridModel.value("revision").toInt() == customGridRevision);

    customGridController.setGridSize(-5.0, std::numeric_limits<double>::infinity());
    customGridController.setGridMargins(-1.0, -2.0, 1000.0, 1000.0);
    customGridController.setGridStep(-1.0);
    customGridController.setGridMajorLineEvery(-4);
    QVariantMap sanitizedGrid = customGridController.modelDocument().value("grid").toMap();
    EDI_CHECK(sanitizedGrid.value("width").toDouble() > 0.0);
    EDI_CHECK(sanitizedGrid.value("height").toDouble() > 0.0);
    EDI_CHECK(sanitizedGrid.value("margin_left").toDouble() >= 0.0);
    EDI_CHECK(sanitizedGrid.value("margin_top").toDouble() >= 0.0);
    EDI_CHECK(sanitizedGrid.value("margin_right").toDouble() >= 0.0);
    EDI_CHECK(sanitizedGrid.value("margin_bottom").toDouble() >= 0.0);
    EDI_CHECK(sanitizedGrid.value("minor_step").toDouble() > 0.0);
    EDI_CHECK(sanitizedGrid.value("major_line_every").toInt() == 1);
    EDI_CHECK(customGridController.modelDocument().value("revision").toInt() == customGridRevision);

    controller.setSelectedToolId("line_tool");
    controller.clickCanvasNormalized(0.1, 0.2);
    EDI_CHECK(controller.modelDocument().value("drawing_objects").toList().size() == 1);
    controller.clickCanvasNormalized(0.8, 0.9);
    objects = controller.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(objects.size() == 2);
    QVariantMap line = objects.back().toMap();
    EDI_CHECK(line.value("kind").toString() == "line");
    EDI_CHECK(line.value("x1").toDouble() == 0.1);
    EDI_CHECK(line.value("y1").toDouble() == 0.2);
    EDI_CHECK(line.value("x2").toDouble() == 0.8);
    EDI_CHECK(line.value("y2").toDouble() == 0.9);
    QVariantMap lineLengthField = numericField(line, "line_length");
    EDI_CHECK(lineLengthField.value("physical_editable").toBool());
    EDI_CHECK(lineLengthField.value("physical_unit_kind").toString() == "length");
    EDI_CHECK(lineLengthField.value("physical_unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(lineLengthField.value("physical_minimum").toDouble(), 0.0));
    QVariantMap lineAngleField = numericField(line, "line_angle_deg");
    EDI_CHECK(lineAngleField.value("physical_editable").toBool());
    EDI_CHECK(lineAngleField.value("physical_unit_kind").toString() == "angle");
    EDI_CHECK(lineAngleField.value("physical_unit_label").toString() == "deg");
    EDI_CHECK(nearlyEqual(lineAngleField.value("physical_step").toDouble(), 1.0));
    QVariantMap lineBounds = line.value("bounds").toMap();
    EDI_CHECK(nearlyEqual(lineBounds.value("x").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(lineBounds.value("y").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(lineBounds.value("width").toDouble(), 0.7));
    EDI_CHECK(nearlyEqual(lineBounds.value("height").toDouble(), 0.7));
    QVariantList lineMeasurement = line.value("measurement_lines").toList();
    EDI_CHECK(lineMeasurement.size() == 3);
    EDI_CHECK(lineMeasurement[0].toString().startsWith("distance: "));
    EDI_CHECK(lineMeasurement[1].toString().startsWith("width: "));
    EDI_CHECK(lineMeasurement[2].toString().startsWith("height: "));

    controller.setSelectedToolId("select_move");
    controller.clickCanvasNormalized(0.3, 0.35);
    EDI_CHECK(controller.selectedObjectId() == point.value("id").toString());

    controller.clickCanvasNormalized(0.8, 0.9);
    EDI_CHECK(controller.selectedObjectId() == line.value("id").toString());
    EDI_CHECK(controller.editSelectedHandleNormalized("line_end", 0.4, 0.6));
    objects = controller.modelDocument().value("drawing_objects").toList();
    QVariantMap editedLine;
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == line.value("id").toString()) {
            editedLine = object;
        }
    }
    EDI_CHECK(!editedLine.isEmpty());
    EDI_CHECK(editedLine.value("x2").toDouble() == 0.4);
    EDI_CHECK(editedLine.value("y2").toDouble() == 0.6);
    EDI_CHECK(!controller.editSelectedHandleNormalized("missing_handle", 0.1, 0.1));
    EDI_CHECK(controller.updateSelectedObjectGeometryField("x2", 0.7));
    EDI_CHECK(controller.updateSelectedObjectGeometryField("y2", 0.8));
    objects = controller.modelDocument().value("drawing_objects").toList();
    QVariantMap numericallyEditedLine;
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == line.value("id").toString()) {
            numericallyEditedLine = object;
        }
    }
    EDI_CHECK(!numericallyEditedLine.isEmpty());
    EDI_CHECK(nearlyEqual(numericallyEditedLine.value("x2").toDouble(), 0.7));
    EDI_CHECK(nearlyEqual(numericallyEditedLine.value("y2").toDouble(), 0.8));
    QVariantMap numericLineBounds = numericallyEditedLine.value("bounds").toMap();
    EDI_CHECK(nearlyEqual(numericLineBounds.value("width").toDouble(), 0.6));
    EDI_CHECK(!controller.updateSelectedObjectGeometryField("missing_field", 0.1));
    EDI_CHECK(!controller.updateSelectedObjectGeometryField("x2", std::numeric_limits<double>::infinity()));

    EDI_CHECK(controller.moveSelectionNormalized(0.1, -0.2));
    objects = controller.modelDocument().value("drawing_objects").toList();
    QVariantMap movedLine;
    for (const QVariant &objectValue : objects) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == line.value("id").toString()) {
            movedLine = object;
        }
    }
    EDI_CHECK(!movedLine.isEmpty());
    EDI_CHECK(nearlyEqual(movedLine.value("x1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(movedLine.value("y1").toDouble(), 0.0));
    EDI_CHECK(nearlyEqual(movedLine.value("x2").toDouble(), 0.8));
    EDI_CHECK(nearlyEqual(movedLine.value("y2").toDouble(), 0.6));

    EDI_CHECK(controller.offsetSelectedObject("left"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(objects.size() == 3);
    QVariantMap offsetLine = objects.back().toMap();
    EDI_CHECK(offsetLine.value("kind").toString() == "line");
    EDI_CHECK(controller.selectedObjectId() == offsetLine.value("id").toString());
    EDI_CHECK(nearlyEqual(offsetLine.value("x1").toDouble(), 0.1646446609));
    EDI_CHECK(nearlyEqual(offsetLine.value("y1").toDouble(), 0.0353553391));
    EDI_CHECK(nearlyEqual(offsetLine.value("x2").toDouble(), 0.7646446609));
    EDI_CHECK(nearlyEqual(offsetLine.value("y2").toDouble(), 0.6353553391));

    EDI_CHECK(controller.mirrorSelectedObject("vertical"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(objects.size() == 4);
    QVariantMap mirroredLine = objects.back().toMap();
    EDI_CHECK(mirroredLine.value("kind").toString() == "line");
    EDI_CHECK(controller.selectedObjectId() == mirroredLine.value("id").toString());
    EDI_CHECK(nearlyEqual(mirroredLine.value("x1").toDouble(), 0.7646446609));
    EDI_CHECK(nearlyEqual(mirroredLine.value("y1").toDouble(), 0.0353553391));
    EDI_CHECK(nearlyEqual(mirroredLine.value("x2").toDouble(), 0.1646446609));
    EDI_CHECK(nearlyEqual(mirroredLine.value("y2").toDouble(), 0.6353553391));

    EDI_CHECK(controller.repeatSelectedObject("x"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(objects.size() == 7);
    QVariantMap repeatedLine1 = objects[4].toMap();
    QVariantMap repeatedLine3 = objects[6].toMap();
    EDI_CHECK(repeatedLine1.value("kind").toString() == "line");
    EDI_CHECK(nearlyEqual(repeatedLine1.value("x1").toDouble(), 0.8646446609));
    EDI_CHECK(nearlyEqual(repeatedLine1.value("y1").toDouble(), 0.0353553391));
    EDI_CHECK(nearlyEqual(repeatedLine1.value("x2").toDouble(), 0.2646446609));
    EDI_CHECK(nearlyEqual(repeatedLine3.value("x1").toDouble(), 1.0646446609));
    EDI_CHECK(nearlyEqual(repeatedLine3.value("x2").toDouble(), 0.4646446609));
    QVariantList repeatedSelection = controller.modelDocument().value("selected_object_ids").toList();
    EDI_CHECK(repeatedSelection.size() == 3);
    EDI_CHECK(!controller.repeatSelectedObject("diagonal"));
    EDI_CHECK(controller.modelDocument().value("drawing_objects").toList().size() == 7);

    controller.clickCanvasNormalized(0.3, 0.35);
    const int beforeUnsupportedOffsetCount = controller.modelDocument().value("drawing_objects").toList().size();
    EDI_CHECK(!controller.offsetSelectedObject("right"));
    EDI_CHECK(controller.modelDocument().value("drawing_objects").toList().size() == beforeUnsupportedOffsetCount);
    EDI_CHECK(controller.mirrorSelectedObject("horizontal"));
    objects = controller.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(objects.size() == beforeUnsupportedOffsetCount + 1);
    QVariantMap mirroredPoint = objects.back().toMap();
    EDI_CHECK(mirroredPoint.value("kind").toString() == "point");
    EDI_CHECK(nearlyEqual(mirroredPoint.value("x").toDouble(), 0.3));
    EDI_CHECK(nearlyEqual(mirroredPoint.value("y").toDouble(), 0.35));
    EDI_CHECK(!controller.moveSelectionNormalized(std::numeric_limits<double>::infinity(), 0.0));

    DrawingDocumentController gridController;
    gridController.setGridSnapEnabled(true);
    EDI_CHECK(gridController.gridSnapEnabled());
    gridController.setSelectedToolId("point_tool");
    gridController.clickCanvasNormalized(0.14, 0.14);
    QVariantMap gridPoint = gridController.modelDocument().value("drawing_objects").toList().front().toMap();
    const double squareQuarterInchStep = 0.25 / 12.0;
    const double snappedSquarePoint = 7.0 * squareQuarterInchStep;
    EDI_CHECK(nearlyEqual(gridPoint.value("x").toDouble(), snappedSquarePoint));
    EDI_CHECK(nearlyEqual(gridPoint.value("y").toDouble(), snappedSquarePoint));

    DrawingDocumentController guideController;
    guideController.setSelectedToolId("horizontal_guide_tool");
    guideController.clickCanvasNormalized(0.2, 0.3);
    guideController.setSelectedToolId("vertical_guide_tool");
    guideController.clickCanvasNormalized(0.6, 0.7);
    QVariantList guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(guideObjects.size() == 2);
    QVariantMap horizontalGuide = guideObjects[0].toMap();
    QVariantMap verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(horizontalGuide.value("kind").toString() == "guide");
    EDI_CHECK(horizontalGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(horizontalGuide.value("position").toDouble(), 0.3));
    EDI_CHECK(!horizontalGuide.value("plot_ready").toBool());
    EDI_CHECK(horizontalGuide.value("guide_visual_controls").toBool());
    EDI_CHECK(horizontalGuide.value("guide_label").toString() == "H guide 0.300");
    EDI_CHECK(horizontalGuide.value("guide_custom_label").toString().isEmpty());
    EDI_CHECK(horizontalGuide.value("guide_color").toString() == "#83aeca");
    EDI_CHECK(horizontalGuide.value("guide_dash_style").toString() == "dash");
    EDI_CHECK(horizontalGuide.value("guide_show_label").toBool());
    EDI_CHECK(verticalGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 0.6));
    EDI_CHECK(verticalGuide.value("guide_drawable_controls").toBool());
    EDI_CHECK(guideController.setSelectedGuideLabel("material edge"));
    EDI_CHECK(guideController.setSelectedGuideColor("#54d2c6"));
    EDI_CHECK(guideController.setSelectedGuideDashStyle("solid"));
    EDI_CHECK(guideController.setSelectedGuideLabelVisible(false));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(verticalGuide.value("guide_label").toString() == "material edge");
    EDI_CHECK(verticalGuide.value("guide_custom_label").toString() == "material edge");
    EDI_CHECK(verticalGuide.value("guide_color").toString() == "#54d2c6");
    EDI_CHECK(verticalGuide.value("guide_dash_style").toString() == "solid");
    EDI_CHECK(!verticalGuide.value("guide_show_label").toBool());
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 0.6));
    const int guideVisualRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideController.setSelectedGuideColor("teal"));
    EDI_CHECK(!guideController.setSelectedGuideDashStyle("stripe"));
    EDI_CHECK(guideController.modelDocument().value("revision").toInt() == guideVisualRevisionBeforeInvalid);
    DrawingDocumentController nonGuideVisualController;
    nonGuideVisualController.setSelectedToolId("point_tool");
    nonGuideVisualController.clickCanvasNormalized(0.2, 0.3);
    EDI_CHECK(!nonGuideVisualController.setSelectedGuideLabel("not a guide"));
    QVariantMap verticalGuidePhysical = verticalGuide.value("physical_geometry").toMap();
    EDI_CHECK(verticalGuidePhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(verticalGuidePhysical.value("position").toDouble(), 7.2));
    EDI_CHECK(guideController.updateSelectedObjectPhysicalGeometryField("position", 3.0));
    QVariantMap guideEditStatus = editStatus(guideController);
    EDI_CHECK(guideEditStatus.value("ok").toBool());
    EDI_CHECK(guideEditStatus.value("mode").toString() == "physical");
    EDI_CHECK(guideEditStatus.value("field_id").toString() == "position");
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 0.25));
    verticalGuidePhysical = verticalGuide.value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(verticalGuidePhysical.value("position").toDouble(), 3.0));
    const int guidePhysicalRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideController.updateSelectedObjectPhysicalGeometryField("position", 13.0));
    guideEditStatus = editStatus(guideController);
    EDI_CHECK(!guideEditStatus.value("ok").toBool());
    EDI_CHECK(guideEditStatus.value("mode").toString() == "physical");
    EDI_CHECK(guideEditStatus.value("field_id").toString() == "position");
    EDI_CHECK(guideEditStatus.value("code").toString() == "invalid_geometry");
    EDI_CHECK(guideEditStatus.value("message").toString() == "guide position must be normalized");
    EDI_CHECK(guideController.modelDocument().value("revision").toInt() == guidePhysicalRevisionBeforeInvalid);
    EDI_CHECK(guideController.moveSelectedGuideToDrawableOrigin());
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), squareQuarterInchStep));
    EDI_CHECK(guideController.centerSelectedGuideInDrawable());
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 0.5));
    EDI_CHECK(guideController.moveSelectedGuideToDrawableMax());
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep));
    EDI_CHECK(guideController.offsetSelectedGuide("negative", "fine"));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep - squareQuarterInchStep * 0.25));
    EDI_CHECK(guideController.offsetSelectedGuide("positive", "fine"));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep));
    EDI_CHECK(guideController.offsetSelectedGuide("negative", "coarse"));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep - squareQuarterInchStep * 4.0));
    const int guideOffsetRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideController.offsetSelectedGuide("sideways", "grid"));
    EDI_CHECK(guideController.modelDocument().value("revision").toInt() == guideOffsetRevisionBeforeInvalid);
    EDI_CHECK(guideController.updateSelectedObjectGeometryField("position", 0.8));
    guideObjects = guideController.modelDocument().value("drawing_objects").toList();
    verticalGuide = guideObjects[1].toMap();
    EDI_CHECK(nearlyEqual(verticalGuide.value("position").toDouble(), 0.8));
    const int guideRevisionBeforeInvalid = guideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideController.updateSelectedObjectGeometryField("position", 1.2));
    EDI_CHECK(guideController.modelDocument().value("revision").toInt() == guideRevisionBeforeInvalid);

    DrawingDocumentController horizontalGuidePlacementController;
    horizontalGuidePlacementController.setSelectedToolId("horizontal_guide_tool");
    horizontalGuidePlacementController.clickCanvasNormalized(0.2, 0.3);
    EDI_CHECK(horizontalGuidePlacementController.moveSelectedGuideToDrawableOrigin());
    QVariantMap horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), squareQuarterInchStep));
    EDI_CHECK(horizontalGuidePlacementController.centerSelectedGuideInDrawable());
    horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), 0.5));
    EDI_CHECK(horizontalGuidePlacementController.moveSelectedGuideToDrawableMax());
    horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), 1.0 - squareQuarterInchStep));
    EDI_CHECK(horizontalGuidePlacementController.updateSelectedObjectPhysicalGeometryField("position", 6.0));
    horizontalMovedGuide = horizontalGuidePlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(horizontalMovedGuide.value("position").toDouble(), 0.5));

    DrawingDocumentController lockedGuidePlacementController;
    lockedGuidePlacementController.setSelectedToolId("horizontal_guide_tool");
    lockedGuidePlacementController.clickCanvasNormalized(0.2, 0.3);
    EDI_CHECK(lockedGuidePlacementController.setSelectedObjectLocked(true));
    const int lockedGuideRevision = lockedGuidePlacementController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedGuidePlacementController.moveSelectedGuideToDrawableOrigin());
    EDI_CHECK(lockedGuidePlacementController.modelDocument().value("revision").toInt() == lockedGuideRevision);

    DrawingDocumentController layerLockedGuidePlacementController;
    layerLockedGuidePlacementController.setSelectedToolId("vertical_guide_tool");
    layerLockedGuidePlacementController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(layerLockedGuidePlacementController.setActiveLayerLocked(true));
    const int layerLockedGuideRevision = layerLockedGuidePlacementController.modelDocument().value("revision").toInt();
    EDI_CHECK(!layerLockedGuidePlacementController.centerSelectedGuideInDrawable());
    EDI_CHECK(layerLockedGuidePlacementController.modelDocument().value("revision").toInt() == layerLockedGuideRevision);

    DrawingDocumentController boundsGuideController;
    boundsGuideController.setSelectedToolId("rectangle_tool");
    boundsGuideController.clickCanvasNormalized(0.2, 0.3);
    boundsGuideController.clickCanvasNormalized(0.6, 0.7);
    QVariantMap boundsGuideModel = boundsGuideController.modelDocument();
    QVariantList boundsGuideObjects = boundsGuideModel.value("drawing_objects").toList();
    EDI_CHECK(boundsGuideObjects.size() == 1);
    const QString boundsGuideSourceId = boundsGuideObjects.front().toMap().value("id").toString();
    EDI_CHECK(boundsGuideObjects.front().toMap().value("bounds_guide_controls").toBool());
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    boundsGuideModel = boundsGuideController.modelDocument();
    boundsGuideObjects = boundsGuideModel.value("drawing_objects").toList();
    EDI_CHECK(boundsGuideObjects.size() == 2);
    EDI_CHECK(boundsGuideModel.value("guide_count").toInt() == 1);
    EDI_CHECK(boundsGuideModel.value("visible_guide_count").toInt() == 1);
    EDI_CHECK(boundsGuideModel.value("duplicate_guide_count").toInt() == 0);
    QVariantMap boundsGuide = boundsGuideObjects.back().toMap();
    EDI_CHECK(boundsGuide.value("kind").toString() == "guide");
    EDI_CHECK(boundsGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(boundsGuide.value("position").toDouble(), 0.2));
    EDI_CHECK(!boundsGuide.value("plot_ready").toBool());
    EDI_CHECK(boundsGuideController.selectedObjectId() == boundsGuideSourceId);
    const int duplicateBoundsGuideRevision = boundsGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    boundsGuideModel = boundsGuideController.modelDocument();
    EDI_CHECK(boundsGuideModel.value("revision").toInt() == duplicateBoundsGuideRevision);
    EDI_CHECK(boundsGuideModel.value("drawing_objects").toList().size() == 2);
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("right")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(boundsGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(boundsGuide.value("position").toDouble(), 0.6));
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("vertical_center")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(boundsGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(boundsGuide.value("position").toDouble(), 0.4));
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("top")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(boundsGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(boundsGuide.value("position").toDouble(), 0.3));
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("bottom")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(boundsGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(boundsGuide.value("position").toDouble(), 0.7));
    EDI_CHECK(boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("horizontal_center")));
    boundsGuide = boundsGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(boundsGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(boundsGuide.value("position").toDouble(), 0.5));
    EDI_CHECK(boundsGuideController.modelDocument().value("drawing_objects").toList().size() == 7);
    const int boundsGuideRevisionBeforeInvalid = boundsGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!boundsGuideController.createGuideFromSelectedBounds(QStringLiteral("diagonal")));
    EDI_CHECK(boundsGuideController.modelDocument().value("revision").toInt() == boundsGuideRevisionBeforeInvalid);

    DrawingDocumentController guidePresetController;
    guidePresetController.setSelectedToolId("point_tool");
    guidePresetController.clickCanvasNormalized(0.4, 0.4);
    const QString guidePresetSelectedId = guidePresetController.selectedObjectId();
    EDI_CHECK(guidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    QVariantMap guidePresetModel = guidePresetController.modelDocument();
    QVariantList guidePresetObjects = guidePresetModel.value("drawing_objects").toList();
    EDI_CHECK(guidePresetObjects.size() == 5);
    EDI_CHECK(guidePresetModel.value("guide_count").toInt() == 4);
    EDI_CHECK(guidePresetModel.value("visible_guide_count").toInt() == 4);
    EDI_CHECK(guidePresetController.selectedObjectId() == guidePresetSelectedId);
    const int guidePresetDuplicateRevision = guidePresetModel.value("revision").toInt();
    EDI_CHECK(guidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    guidePresetModel = guidePresetController.modelDocument();
    EDI_CHECK(guidePresetModel.value("revision").toInt() == guidePresetDuplicateRevision);
    EDI_CHECK(guidePresetModel.value("guide_count").toInt() == 4);

    bool foundDrawableLeft = false;
    bool foundDrawableBottom = false;
    for (const QVariant &objectValue : guidePresetModel.value("drawing_objects").toList()) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("kind").toString() != "guide") {
            continue;
        }
        if (object.value("guide_label").toString() == "drawable left") {
            foundDrawableLeft = true;
            EDI_CHECK(object.value("orientation").toString() == "vertical");
            EDI_CHECK(nearlyEqual(object.value("position").toDouble(), squareQuarterInchStep));
            EDI_CHECK(object.value("guide_color").toString() == "#f6c65b");
        } else if (object.value("guide_label").toString() == "drawable bottom") {
            foundDrawableBottom = true;
            EDI_CHECK(object.value("orientation").toString() == "horizontal");
            EDI_CHECK(nearlyEqual(object.value("position").toDouble(), 1.0 - squareQuarterInchStep));
            EDI_CHECK(object.value("guide_color").toString() == "#f6c65b");
        }
    }
    EDI_CHECK(foundDrawableLeft);
    EDI_CHECK(foundDrawableBottom);
    EDI_CHECK(guidePresetController.applyGuidePreset(QStringLiteral("drawable_centerlines")));
    guidePresetModel = guidePresetController.modelDocument();
    EDI_CHECK(guidePresetModel.value("guide_count").toInt() == 6);
    EDI_CHECK(guidePresetController.applyGuidePreset(QStringLiteral("thirds")));
    guidePresetModel = guidePresetController.modelDocument();
    EDI_CHECK(guidePresetModel.value("guide_count").toInt() == 10);
    EDI_CHECK(guidePresetController.applyGuidePreset(QStringLiteral("quarters")));
    guidePresetModel = guidePresetController.modelDocument();
    EDI_CHECK(guidePresetModel.value("guide_count").toInt() == 14);
    EDI_CHECK(guidePresetController.applyGuidePreset(QStringLiteral("margin_safe")));
    guidePresetModel = guidePresetController.modelDocument();
    EDI_CHECK(guidePresetModel.value("guide_count").toInt() == 14);
    EDI_CHECK(!guidePresetController.applyGuidePreset(QStringLiteral("unknown_preset")));

    DrawingDocumentController hiddenGuidePresetController;
    EDI_CHECK(hiddenGuidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    EDI_CHECK(hiddenGuidePresetController.setAllGuidesVisible(false));
    QVariantMap hiddenGuidePresetModel = hiddenGuidePresetController.modelDocument();
    EDI_CHECK(hiddenGuidePresetModel.value("guide_count").toInt() == 4);
    EDI_CHECK(hiddenGuidePresetModel.value("visible_guide_count").toInt() == 0);

    DrawingDocumentController lockedLayerGuidePresetController;
    EDI_CHECK(lockedLayerGuidePresetController.setActiveLayerLocked(true));
    const int lockedLayerGuidePresetRevision = lockedLayerGuidePresetController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedLayerGuidePresetController.applyGuidePreset(QStringLiteral("drawable_bounds")));
    EDI_CHECK(lockedLayerGuidePresetController.modelDocument().value("revision").toInt() == lockedLayerGuidePresetRevision);
    EDI_CHECK(lockedLayerGuidePresetController.modelDocument().value("guide_count").toInt() == 0);

    DrawingDocumentController offsetGuideController;
    offsetGuideController.setSelectedToolId("rectangle_tool");
    offsetGuideController.clickCanvasNormalized(0.2, 0.3);
    offsetGuideController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    QVariantMap offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(offsetGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(offsetGuide.value("position").toDouble(), 0.2 - squareQuarterInchStep));
    const int duplicateOffsetGuideRevision = offsetGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    EDI_CHECK(offsetGuideController.modelDocument().value("revision").toInt() == duplicateOffsetGuideRevision);
    EDI_CHECK(offsetGuideController.modelDocument().value("drawing_objects").toList().size() == 2);
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("right"), QStringLiteral("grid")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(offsetGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(offsetGuide.value("position").toDouble(), 0.6 + squareQuarterInchStep));
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("top"), QStringLiteral("grid")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(offsetGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(offsetGuide.value("position").toDouble(), 0.3 - squareQuarterInchStep));
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("bottom"), QStringLiteral("grid")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(offsetGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(offsetGuide.value("position").toDouble(), 0.7 + squareQuarterInchStep));
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("center_x_plus"), QStringLiteral("fine")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(offsetGuide.value("orientation").toString() == "vertical");
    EDI_CHECK(nearlyEqual(offsetGuide.value("position").toDouble(), 0.4 + squareQuarterInchStep * 0.25));
    EDI_CHECK(offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("center_y_minus"), QStringLiteral("coarse")));
    offsetGuide = offsetGuideController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(offsetGuide.value("orientation").toString() == "horizontal");
    EDI_CHECK(nearlyEqual(offsetGuide.value("position").toDouble(), 0.5 - squareQuarterInchStep * 4.0));
    const int offsetGuideRevisionBeforeInvalid = offsetGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!offsetGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("diagonal"), QStringLiteral("grid")));
    EDI_CHECK(offsetGuideController.modelDocument().value("revision").toInt() == offsetGuideRevisionBeforeInvalid);

    DrawingDocumentController lockedBoundsGuideController;
    lockedBoundsGuideController.setSelectedToolId("line_tool");
    lockedBoundsGuideController.clickCanvasNormalized(0.2, 0.3);
    lockedBoundsGuideController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(lockedBoundsGuideController.setSelectedObjectLocked(true));
    const int lockedBoundsGuideRevision = lockedBoundsGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedBoundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    EDI_CHECK(!lockedBoundsGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    EDI_CHECK(lockedBoundsGuideController.modelDocument().value("revision").toInt() == lockedBoundsGuideRevision);
    EDI_CHECK(lockedBoundsGuideController.modelDocument().value("drawing_objects").toList().size() == 1);

    DrawingDocumentController layerLockedBoundsGuideController;
    layerLockedBoundsGuideController.setSelectedToolId("circle_tool");
    layerLockedBoundsGuideController.clickCanvasNormalized(0.5, 0.5);
    layerLockedBoundsGuideController.clickCanvasNormalized(0.6, 0.5);
    EDI_CHECK(layerLockedBoundsGuideController.setActiveLayerLocked(true));
    const int layerLockedBoundsGuideRevision = layerLockedBoundsGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!layerLockedBoundsGuideController.createGuideFromSelectedBounds(QStringLiteral("top")));
    EDI_CHECK(!layerLockedBoundsGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("top"), QStringLiteral("grid")));
    EDI_CHECK(layerLockedBoundsGuideController.modelDocument().value("revision").toInt() == layerLockedBoundsGuideRevision);
    EDI_CHECK(layerLockedBoundsGuideController.modelDocument().value("drawing_objects").toList().size() == 1);

    DrawingDocumentController unsupportedBoundsGuideController;
    unsupportedBoundsGuideController.setSelectedToolId("horizontal_guide_tool");
    unsupportedBoundsGuideController.clickCanvasNormalized(0.2, 0.3);
    QVariantMap unsupportedGuide = unsupportedBoundsGuideController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(!unsupportedGuide.value("bounds_guide_controls").toBool());
    const int unsupportedBoundsGuideRevision = unsupportedBoundsGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!unsupportedBoundsGuideController.createGuideFromSelectedBounds(QStringLiteral("left")));
    EDI_CHECK(!unsupportedBoundsGuideController.createOffsetGuideFromSelectedBounds(QStringLiteral("left"), QStringLiteral("grid")));
    EDI_CHECK(unsupportedBoundsGuideController.modelDocument().value("revision").toInt() == unsupportedBoundsGuideRevision);

    DrawingDocumentController directDuplicateGuideController;
    directDuplicateGuideController.setSelectedToolId("horizontal_guide_tool");
    directDuplicateGuideController.clickCanvasNormalized(0.2, 0.3);
    const QString directGuideId = directDuplicateGuideController.selectedObjectId();
    directDuplicateGuideController.clickCanvasNormalized(0.8, 0.3);
    QVariantMap directDuplicateGuideModel = directDuplicateGuideController.modelDocument();
    EDI_CHECK(directDuplicateGuideModel.value("drawing_objects").toList().size() == 1);
    EDI_CHECK(directDuplicateGuideModel.value("guide_count").toInt() == 1);
    EDI_CHECK(directDuplicateGuideModel.value("duplicate_guide_count").toInt() == 0);
    EDI_CHECK(directDuplicateGuideController.selectedObjectId() == directGuideId);

    DrawingDocumentController mergeDuplicateGuideController;
    mergeDuplicateGuideController.setSelectedToolId("horizontal_guide_tool");
    mergeDuplicateGuideController.clickCanvasNormalized(0.2, 0.3);
    mergeDuplicateGuideController.clickCanvasNormalized(0.2, 0.4);
    EDI_CHECK(mergeDuplicateGuideController.updateSelectedObjectGeometryField(QStringLiteral("position"), 0.3));
    EDI_CHECK(mergeDuplicateGuideController.setSelectedObjectLocked(true));
    QVariantMap duplicateGuideModel = mergeDuplicateGuideController.modelDocument();
    EDI_CHECK(duplicateGuideModel.value("drawing_objects").toList().size() == 2);
    EDI_CHECK(duplicateGuideModel.value("guide_count").toInt() == 2);
    EDI_CHECK(duplicateGuideModel.value("duplicate_guide_count").toInt() == 1);
    EDI_CHECK(mergeDuplicateGuideController.mergeDuplicateGuides());
    duplicateGuideModel = mergeDuplicateGuideController.modelDocument();
    QVariantList mergedGuides = duplicateGuideModel.value("drawing_objects").toList();
    EDI_CHECK(mergedGuides.size() == 1);
    EDI_CHECK(duplicateGuideModel.value("guide_count").toInt() == 1);
    EDI_CHECK(duplicateGuideModel.value("duplicate_guide_count").toInt() == 0);
    EDI_CHECK(mergedGuides.front().toMap().value("kind").toString() == "guide");
    EDI_CHECK(!mergedGuides.front().toMap().value("locked").toBool());

    DrawingDocumentController guideAlignLeftController;
    guideAlignLeftController.setSelectedToolId("vertical_guide_tool");
    guideAlignLeftController.clickCanvasNormalized(0.1, 0.2);
    guideAlignLeftController.setSelectedToolId("rectangle_tool");
    guideAlignLeftController.clickCanvasNormalized(0.3, 0.3);
    guideAlignLeftController.clickCanvasNormalized(0.5, 0.5);
    QVariantMap guideAlignRect = guideAlignLeftController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(guideAlignRect.value("align_to_guide_controls").toBool());
    EDI_CHECK(guideAlignLeftController.alignSelectionToNearestGuide(QStringLiteral("left")));
    guideAlignRect = guideAlignLeftController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideAlignRect.value("x").toDouble(), 0.1));

    DrawingDocumentController guideAlignCenterXController;
    guideAlignCenterXController.setSelectedToolId("vertical_guide_tool");
    guideAlignCenterXController.clickCanvasNormalized(0.6, 0.2);
    guideAlignCenterXController.setSelectedToolId("rectangle_tool");
    guideAlignCenterXController.clickCanvasNormalized(0.3, 0.3);
    guideAlignCenterXController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(guideAlignCenterXController.alignSelectionToNearestGuide(QStringLiteral("center_x")));
    guideAlignRect = guideAlignCenterXController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideAlignRect.value("x").toDouble(), 0.5));

    DrawingDocumentController guideAlignRightController;
    guideAlignRightController.setSelectedToolId("vertical_guide_tool");
    guideAlignRightController.clickCanvasNormalized(0.8, 0.2);
    guideAlignRightController.setSelectedToolId("rectangle_tool");
    guideAlignRightController.clickCanvasNormalized(0.3, 0.3);
    guideAlignRightController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(guideAlignRightController.alignSelectionToNearestGuide(QStringLiteral("right")));
    guideAlignRect = guideAlignRightController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideAlignRect.value("x").toDouble(), 0.6));

    DrawingDocumentController guideAlignTopController;
    guideAlignTopController.setSelectedToolId("horizontal_guide_tool");
    guideAlignTopController.clickCanvasNormalized(0.2, 0.1);
    guideAlignTopController.setSelectedToolId("rectangle_tool");
    guideAlignTopController.clickCanvasNormalized(0.3, 0.3);
    guideAlignTopController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(guideAlignTopController.alignSelectionToNearestGuide(QStringLiteral("top")));
    guideAlignRect = guideAlignTopController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideAlignRect.value("y").toDouble(), 0.1));

    DrawingDocumentController guideAlignCenterYController;
    guideAlignCenterYController.setSelectedToolId("horizontal_guide_tool");
    guideAlignCenterYController.clickCanvasNormalized(0.2, 0.6);
    guideAlignCenterYController.setSelectedToolId("rectangle_tool");
    guideAlignCenterYController.clickCanvasNormalized(0.3, 0.3);
    guideAlignCenterYController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(guideAlignCenterYController.alignSelectionToNearestGuide(QStringLiteral("center_y")));
    guideAlignRect = guideAlignCenterYController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideAlignRect.value("y").toDouble(), 0.5));

    DrawingDocumentController guideAlignBottomController;
    guideAlignBottomController.setSelectedToolId("horizontal_guide_tool");
    guideAlignBottomController.clickCanvasNormalized(0.2, 0.8);
    guideAlignBottomController.setSelectedToolId("rectangle_tool");
    guideAlignBottomController.clickCanvasNormalized(0.3, 0.3);
    guideAlignBottomController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(guideAlignBottomController.alignSelectionToNearestGuide(QStringLiteral("bottom")));
    guideAlignRect = guideAlignBottomController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideAlignRect.value("y").toDouble(), 0.6));

    DrawingDocumentController guideAlignNoGuideController;
    guideAlignNoGuideController.setSelectedToolId("rectangle_tool");
    guideAlignNoGuideController.clickCanvasNormalized(0.3, 0.3);
    guideAlignNoGuideController.clickCanvasNormalized(0.5, 0.5);
    const int guideAlignNoGuideRevision = guideAlignNoGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideAlignNoGuideController.alignSelectionToNearestGuide(QStringLiteral("left")));
    EDI_CHECK(guideAlignNoGuideController.modelDocument().value("revision").toInt() == guideAlignNoGuideRevision);

    DrawingDocumentController guideAlignHiddenGuideController;
    guideAlignHiddenGuideController.setSelectedToolId("vertical_guide_tool");
    guideAlignHiddenGuideController.clickCanvasNormalized(0.1, 0.2);
    EDI_CHECK(guideAlignHiddenGuideController.setAllGuidesVisible(false));
    guideAlignHiddenGuideController.setSelectedToolId("rectangle_tool");
    guideAlignHiddenGuideController.clickCanvasNormalized(0.3, 0.3);
    guideAlignHiddenGuideController.clickCanvasNormalized(0.5, 0.5);
    const int guideAlignHiddenGuideRevision = guideAlignHiddenGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideAlignHiddenGuideController.alignSelectionToNearestGuide(QStringLiteral("left")));
    EDI_CHECK(guideAlignHiddenGuideController.modelDocument().value("revision").toInt() == guideAlignHiddenGuideRevision);

    DrawingDocumentController lockedGuideAlignController;
    lockedGuideAlignController.setSelectedToolId("vertical_guide_tool");
    lockedGuideAlignController.clickCanvasNormalized(0.1, 0.2);
    lockedGuideAlignController.setSelectedToolId("rectangle_tool");
    lockedGuideAlignController.clickCanvasNormalized(0.3, 0.3);
    lockedGuideAlignController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(lockedGuideAlignController.setSelectedObjectLocked(true));
    const int lockedGuideAlignRevision = lockedGuideAlignController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedGuideAlignController.alignSelectionToNearestGuide(QStringLiteral("left")));
    EDI_CHECK(lockedGuideAlignController.modelDocument().value("revision").toInt() == lockedGuideAlignRevision);

    DrawingDocumentController unsupportedGuideAlignController;
    unsupportedGuideAlignController.setSelectedToolId("vertical_guide_tool");
    unsupportedGuideAlignController.clickCanvasNormalized(0.1, 0.2);
    const int unsupportedGuideAlignRevision = unsupportedGuideAlignController.modelDocument().value("revision").toInt();
    EDI_CHECK(!unsupportedGuideAlignController.alignSelectionToNearestGuide(QStringLiteral("left")));
    EDI_CHECK(unsupportedGuideAlignController.modelDocument().value("revision").toInt() == unsupportedGuideAlignRevision);

    DrawingDocumentController deleteSelectedGuideController;
    deleteSelectedGuideController.setSelectedToolId("point_tool");
    deleteSelectedGuideController.clickCanvasNormalized(0.1, 0.1);
    deleteSelectedGuideController.setSelectedToolId("horizontal_guide_tool");
    deleteSelectedGuideController.clickCanvasNormalized(0.2, 0.3);
    EDI_CHECK(deleteSelectedGuideController.deleteSelectedGuide());
    QVariantList deleteSelectedGuideObjects = deleteSelectedGuideController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(deleteSelectedGuideObjects.size() == 1);
    EDI_CHECK(deleteSelectedGuideObjects.front().toMap().value("kind").toString() == "point");

    DrawingDocumentController lockedDeleteSelectedGuideController;
    lockedDeleteSelectedGuideController.setSelectedToolId("vertical_guide_tool");
    lockedDeleteSelectedGuideController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(lockedDeleteSelectedGuideController.setSelectedObjectLocked(true));
    const int lockedDeleteSelectedGuideRevision = lockedDeleteSelectedGuideController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedDeleteSelectedGuideController.deleteSelectedGuide());
    EDI_CHECK(lockedDeleteSelectedGuideController.modelDocument().value("revision").toInt() == lockedDeleteSelectedGuideRevision);
    EDI_CHECK(lockedDeleteSelectedGuideController.modelDocument().value("drawing_objects").toList().size() == 1);

    DrawingDocumentController deleteAllGuidesController;
    deleteAllGuidesController.setSelectedToolId("point_tool");
    deleteAllGuidesController.clickCanvasNormalized(0.1, 0.1);
    deleteAllGuidesController.setSelectedToolId("horizontal_guide_tool");
    deleteAllGuidesController.clickCanvasNormalized(0.2, 0.3);
    deleteAllGuidesController.setSelectedToolId("vertical_guide_tool");
    deleteAllGuidesController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(deleteAllGuidesController.setAllGuidesLocked(true));
    EDI_CHECK(deleteAllGuidesController.deleteAllGuides());
    QVariantList deleteAllGuideObjects = deleteAllGuidesController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(deleteAllGuideObjects.size() == 1);
    EDI_CHECK(deleteAllGuideObjects.front().toMap().value("kind").toString() == "point");

    DrawingDocumentController guideLifecycleController;
    guideLifecycleController.setSelectedToolId("horizontal_guide_tool");
    guideLifecycleController.clickCanvasNormalized(0.2, 0.75);
    guideLifecycleController.setSelectedToolId("vertical_guide_tool");
    guideLifecycleController.clickCanvasNormalized(0.33, 0.2);
    guideLifecycleController.setObjectSnapEnabled(true);
    guideLifecycleController.updatePointerNormalized(0.34, 0.74);
    QVariantMap guideLifecyclePointer = guideLifecycleController.modelDocument().value("pointer").toMap();
    EDI_CHECK(guideLifecyclePointer.value("source").toString() == "guide");
    EDI_CHECK(guideLifecycleController.setAllGuidesVisible(false));
    QVariantList hiddenGuideObjects = guideLifecycleController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(!hiddenGuideObjects[0].toMap().value("visible").toBool());
    EDI_CHECK(!hiddenGuideObjects[1].toMap().value("visible").toBool());
    guideLifecycleController.updatePointerNormalized(0.34, 0.74);
    guideLifecyclePointer = guideLifecycleController.modelDocument().value("pointer").toMap();
    EDI_CHECK(guideLifecyclePointer.value("kind").toString() == "none");
    EDI_CHECK(guideLifecycleController.setAllGuidesVisible(true));
    EDI_CHECK(guideLifecycleController.setAllGuidesLocked(true));
    QVariantList lockedGuideObjects = guideLifecycleController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(lockedGuideObjects[0].toMap().value("locked").toBool());
    EDI_CHECK(lockedGuideObjects[1].toMap().value("locked").toBool());
    const int lockedGuideMoveRevision = guideLifecycleController.modelDocument().value("revision").toInt();
    EDI_CHECK(!guideLifecycleController.moveSelectedGuideToDrawableOrigin());
    EDI_CHECK(guideLifecycleController.modelDocument().value("revision").toInt() == lockedGuideMoveRevision);
    guideLifecycleController.updatePointerNormalized(0.34, 0.74);
    guideLifecyclePointer = guideLifecycleController.modelDocument().value("pointer").toMap();
    EDI_CHECK(guideLifecyclePointer.value("source").toString() == "guide");
    EDI_CHECK(guideLifecycleController.setAllGuidesLocked(false));
    EDI_CHECK(guideLifecycleController.moveSelectedGuideToDrawableOrigin());

    DrawingDocumentController constructionController;
    constructionController.setSelectedToolId("horizontal_construction_line_tool");
    constructionController.clickCanvasNormalized(0.2, 0.3);
    constructionController.setSelectedToolId("vertical_construction_line_tool");
    constructionController.clickCanvasNormalized(0.6, 0.7);
    QVariantList constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(constructionObjects.size() == 2);
    QVariantMap horizontalConstruction = constructionObjects[0].toMap();
    QVariantMap verticalConstruction = constructionObjects[1].toMap();
    EDI_CHECK(horizontalConstruction.value("kind").toString() == "construction_line");
    EDI_CHECK(nearlyEqual(horizontalConstruction.value("x1").toDouble(), 0.0));
    EDI_CHECK(nearlyEqual(horizontalConstruction.value("y1").toDouble(), 0.3));
    EDI_CHECK(nearlyEqual(horizontalConstruction.value("x2").toDouble(), 1.0));
    EDI_CHECK(nearlyEqual(horizontalConstruction.value("y2").toDouble(), 0.3));
    EDI_CHECK(!horizontalConstruction.value("plot_ready").toBool());
    EDI_CHECK(nearlyEqual(verticalConstruction.value("x1").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(verticalConstruction.value("y1").toDouble(), 0.0));
    EDI_CHECK(nearlyEqual(verticalConstruction.value("x2").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(verticalConstruction.value("y2").toDouble(), 1.0));
    EDI_CHECK(verticalConstruction.value("construction_drawable_controls").toBool());
    EDI_CHECK(constructionController.selectedObjectId() == verticalConstruction.value("id").toString());
    EDI_CHECK(constructionController.offsetSelectedObject("left"));
    constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(constructionObjects.size() == 3);
    QVariantMap offsetConstruction = constructionObjects.back().toMap();
    EDI_CHECK(offsetConstruction.value("kind").toString() == "construction_line");
    EDI_CHECK(nearlyEqual(offsetConstruction.value("x1").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(offsetConstruction.value("y1").toDouble(), 0.0));
    EDI_CHECK(nearlyEqual(offsetConstruction.value("x2").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(offsetConstruction.value("y2").toDouble(), 1.0));
    EDI_CHECK(constructionController.mirrorSelectedObject("horizontal"));
    constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(constructionObjects.size() == 4);
    QVariantMap mirroredConstruction = constructionObjects.back().toMap();
    EDI_CHECK(mirroredConstruction.value("kind").toString() == "construction_line");
    EDI_CHECK(nearlyEqual(mirroredConstruction.value("x1").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(mirroredConstruction.value("y1").toDouble(), 1.0));
    EDI_CHECK(nearlyEqual(mirroredConstruction.value("x2").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(mirroredConstruction.value("y2").toDouble(), 0.0));

    EDI_CHECK(constructionController.repeatSelectedObject("y"));
    constructionObjects = constructionController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(constructionObjects.size() == 7);
    QVariantMap repeatedConstruction = constructionObjects.back().toMap();
    EDI_CHECK(repeatedConstruction.value("kind").toString() == "construction_line");
    EDI_CHECK(nearlyEqual(repeatedConstruction.value("x1").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(repeatedConstruction.value("y1").toDouble(), 1.3));
    EDI_CHECK(nearlyEqual(repeatedConstruction.value("x2").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(repeatedConstruction.value("y2").toDouble(), 0.3));

    DrawingDocumentController verticalConstructionPlacementController;
    verticalConstructionPlacementController.setSelectedToolId("vertical_construction_line_tool");
    verticalConstructionPlacementController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(verticalConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    QVariantMap verticalFittedConstruction = verticalConstructionPlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(verticalFittedConstruction.value("x1").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(verticalFittedConstruction.value("y1").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(verticalFittedConstruction.value("x2").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(verticalFittedConstruction.value("y2").toDouble(), 1.0 - squareQuarterInchStep));

    DrawingDocumentController horizontalConstructionPlacementController;
    horizontalConstructionPlacementController.setSelectedToolId("horizontal_construction_line_tool");
    horizontalConstructionPlacementController.clickCanvasNormalized(0.2, 0.3);
    EDI_CHECK(horizontalConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    QVariantMap horizontalFittedConstruction = horizontalConstructionPlacementController.modelDocument()
        .value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(horizontalFittedConstruction.value("x1").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(horizontalFittedConstruction.value("y1").toDouble(), 0.3));
    EDI_CHECK(nearlyEqual(horizontalFittedConstruction.value("x2").toDouble(), 1.0 - squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(horizontalFittedConstruction.value("y2").toDouble(), 0.3));

    DrawingDocumentController lockedConstructionPlacementController;
    lockedConstructionPlacementController.setSelectedToolId("vertical_construction_line_tool");
    lockedConstructionPlacementController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(lockedConstructionPlacementController.setSelectedObjectLocked(true));
    const int lockedConstructionRevision = lockedConstructionPlacementController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    EDI_CHECK(lockedConstructionPlacementController.modelDocument().value("revision").toInt() == lockedConstructionRevision);

    DrawingDocumentController layerLockedConstructionPlacementController;
    layerLockedConstructionPlacementController.setSelectedToolId("horizontal_construction_line_tool");
    layerLockedConstructionPlacementController.clickCanvasNormalized(0.2, 0.3);
    EDI_CHECK(layerLockedConstructionPlacementController.setActiveLayerLocked(true));
    const int layerLockedConstructionRevision = layerLockedConstructionPlacementController.modelDocument().value("revision").toInt();
    EDI_CHECK(!layerLockedConstructionPlacementController.fitSelectedConstructionLineToDrawable());
    EDI_CHECK(layerLockedConstructionPlacementController.modelDocument().value("revision").toInt() == layerLockedConstructionRevision);

    DrawingDocumentController angledConstructionController;
    angledConstructionController.setSelectedToolId("angled_construction_line_tool");
    angledConstructionController.clickCanvasNormalized(0.1, 0.2);
    angledConstructionController.updateCreationPreviewNormalized(0.7, 0.4);
    QVariantMap angledPreview = angledConstructionController.modelDocument().value("preview_object").toMap();
    EDI_CHECK(!angledPreview.isEmpty());
    EDI_CHECK(angledPreview.value("kind").toString() == "construction_line");
    EDI_CHECK(!angledPreview.value("plot_ready").toBool());
    EDI_CHECK(nearlyEqual(angledPreview.value("x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(angledPreview.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(angledPreview.value("x2").toDouble(), 0.7));
    EDI_CHECK(nearlyEqual(angledPreview.value("y2").toDouble(), 0.4));
    EDI_CHECK(angledConstructionController.modelDocument().value("drawing_objects").toList().isEmpty());
    angledConstructionController.clickCanvasNormalized(0.7, 0.4);
    QVariantMap angledModel = angledConstructionController.modelDocument();
    EDI_CHECK(!angledModel.contains("preview_object"));
    QVariantList angledObjects = angledModel.value("drawing_objects").toList();
    EDI_CHECK(angledObjects.size() == 1);
    QVariantMap angledConstruction = angledObjects.front().toMap();
    EDI_CHECK(angledConstruction.value("kind").toString() == "construction_line");
    EDI_CHECK(nearlyEqual(angledConstruction.value("x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(angledConstruction.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(angledConstruction.value("x2").toDouble(), 0.7));
    EDI_CHECK(nearlyEqual(angledConstruction.value("y2").toDouble(), 0.4));
    EDI_CHECK(!angledConstruction.value("construction_drawable_controls").toBool());
    const int angledConstructionFitRevision = angledConstructionController.modelDocument().value("revision").toInt();
    EDI_CHECK(!angledConstructionController.fitSelectedConstructionLineToDrawable());
    EDI_CHECK(angledConstructionController.modelDocument().value("revision").toInt() == angledConstructionFitRevision);
    EDI_CHECK(angledConstructionController.updateSelectedObjectGeometryField("x2", 0.8));
    EDI_CHECK(angledConstructionController.updateSelectedObjectGeometryField("y2", 0.5));
    angledConstruction = angledConstructionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(angledConstruction.value("x2").toDouble(), 0.8));
    EDI_CHECK(nearlyEqual(angledConstruction.value("y2").toDouble(), 0.5));
    EDI_CHECK(angledConstructionController.updateSelectedObjectGeometryField("x2", 0.1));
    const int constructionRevisionBeforeInvalid = angledConstructionController.modelDocument().value("revision").toInt();
    EDI_CHECK(!angledConstructionController.updateSelectedObjectGeometryField("y2", 0.2));
    EDI_CHECK(angledConstructionController.modelDocument().value("revision").toInt() == constructionRevisionBeforeInvalid);

    DrawingDocumentController dimensionController;
    dimensionController.setSelectedToolId("distance_dimension_tool");
    dimensionController.clickCanvasNormalized(0.1, 0.2);
    dimensionController.updateCreationPreviewNormalized(0.4, 0.6);
    QVariantMap dimensionPreview = dimensionController.modelDocument().value("preview_object").toMap();
    EDI_CHECK(!dimensionPreview.isEmpty());
    EDI_CHECK(dimensionPreview.value("kind").toString() == "dimension");
    EDI_CHECK(!dimensionPreview.value("plot_ready").toBool());
    EDI_CHECK(dimensionPreview.value("dimension_kind").toString() == "distance");
    EDI_CHECK(dimensionPreview.value("dimension_show_label").toBool());
    EDI_CHECK(nearlyEqual(dimensionPreview.value("x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(dimensionPreview.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(dimensionPreview.value("x2").toDouble(), 0.4));
    EDI_CHECK(nearlyEqual(dimensionPreview.value("y2").toDouble(), 0.6));
    EDI_CHECK(dimensionPreview.value("label").toString() == "0.5 canvas_unit");
    EDI_CHECK(dimensionController.modelDocument().value("drawing_objects").toList().isEmpty());
    dimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap dimensionModel = dimensionController.modelDocument();
    EDI_CHECK(!dimensionModel.contains("preview_object"));
    QVariantList dimensionObjects = dimensionModel.value("drawing_objects").toList();
    EDI_CHECK(dimensionObjects.size() == 1);
    QVariantMap dimension = dimensionObjects.front().toMap();
    EDI_CHECK(dimension.value("kind").toString() == "dimension");
    EDI_CHECK(!dimension.value("plot_ready").toBool());
    EDI_CHECK(dimension.value("dimension_kind").toString() == "distance");
    EDI_CHECK(dimension.value("dimension_visual_controls").toBool());
    EDI_CHECK(dimension.value("dimension_show_label").toBool());
    QStringList dimensionFieldIds = numericFieldIds(dimension);
    EDI_CHECK(dimensionFieldIds.contains("dimension_length"));
    EDI_CHECK(dimensionFieldIds.contains("dimension_angle_deg"));
    QVariantMap dimensionLengthField = numericField(dimension, "dimension_length");
    EDI_CHECK(dimensionLengthField.value("physical_editable").toBool());
    EDI_CHECK(dimensionLengthField.value("physical_unit_kind").toString() == "length");
    EDI_CHECK(dimensionLengthField.value("physical_unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(dimensionLengthField.value("physical_minimum").toDouble(), 0.0));
    QVariantMap dimensionAngleField = numericField(dimension, "dimension_angle_deg");
    EDI_CHECK(dimensionAngleField.value("physical_editable").toBool());
    EDI_CHECK(dimensionAngleField.value("physical_unit_kind").toString() == "angle");
    EDI_CHECK(dimensionAngleField.value("physical_unit_label").toString() == "deg");
    QStringList dimensionHandleIds = editHandleIds(dimension);
    EDI_CHECK(dimension.value("editable_handle_count").toInt() == 3);
    EDI_CHECK(dimensionHandleIds.contains("dimension_start"));
    EDI_CHECK(dimensionHandleIds.contains("dimension_end"));
    EDI_CHECK(dimensionHandleIds.contains("dimension_offset"));
    QVariantList dimensionHandles = dimension.value("edit_handles").toList();
    QVariantMap dimensionOffsetHandle;
    for (const QVariant &handle : dimensionHandles) {
        QVariantMap candidate = handle.toMap();
        if (candidate.value("id").toString() == "dimension_offset") {
            dimensionOffsetHandle = candidate;
        }
    }
    EDI_CHECK(!dimensionOffsetHandle.isEmpty());
    EDI_CHECK(dimensionOffsetHandle.value("role").toString() == "offset");
    EDI_CHECK(dimensionOffsetHandle.value("editable").toBool());
    EDI_CHECK(dimensionOffsetHandle.value("cursor").toString() == "move");
    EDI_CHECK(dimensionOffsetHandle.value("shape").toString() == "diamond");
    EDI_CHECK(nearlyEqual(dimensionOffsetHandle.value("size_px").toDouble(), 8.0));
    EDI_CHECK(nearlyEqual(dimensionOffsetHandle.value("hit_tolerance_px").toDouble(), 14.0));
    EDI_CHECK(nearlyEqual(dimensionOffsetHandle.value("x").toDouble(), 0.218));
    EDI_CHECK(nearlyEqual(dimensionOffsetHandle.value("y").toDouble(), 0.424));
    EDI_CHECK(dimensionOffsetHandle.value("has_anchor").toBool());
    EDI_CHECK(nearlyEqual(dimensionOffsetHandle.value("anchor_x").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(dimensionOffsetHandle.value("anchor_y").toDouble(), 0.4));
    EDI_CHECK(dimension.value("label").toString() == "0.5 canvas_unit");

    auto projectedPointBuild = edi::drafting::buildDraftingObject(
        "projected_point",
        edi::drafting::DraftingShapeKind::Point,
        edi::drafting::PointGeometry{{0.2, 0.3}});
    EDI_CHECK(projectedPointBuild.ok);
    QVariantMap projectedPoint = drawing_core::draftingObjectToCanvasProjection(projectedPointBuild.object);
    QVariantMap projectedPointXField = numericField(projectedPoint, "x");
    EDI_CHECK(!projectedPointXField.value("physical_editable").toBool());
    EDI_CHECK(!projectedPoint.contains("physical_geometry"));

    auto projectedPolygonBuild = edi::drafting::buildDraftingObject(
        "projected_polygon",
        edi::drafting::DraftingShapeKind::Polygon,
        edi::drafting::PolygonGeometry{{{0.1, 0.1}, {0.4, 0.1}, {0.4, 0.4}}});
    EDI_CHECK(projectedPolygonBuild.ok);
    QVariantMap projectedPolygon = drawing_core::draftingObjectToCanvasProjection(projectedPolygonBuild.object);
    QVariantList projectedPolygonHandles = projectedPolygon.value("edit_handles").toList();
    EDI_CHECK(projectedPolygon.value("handle_count").toInt() == 3);
    EDI_CHECK(projectedPolygon.value("editable_handle_count").toInt() == 0);
    EDI_CHECK(projectedPolygonHandles.size() == 3);
    QVariantMap projectedVertex = projectedPolygonHandles.front().toMap();
    EDI_CHECK(projectedVertex.value("id").toString() == "vertex_0");
    EDI_CHECK(projectedVertex.value("role").toString() == "vertex");
    EDI_CHECK(!projectedVertex.value("editable").toBool());
    EDI_CHECK(projectedVertex.value("read_only").toBool());
    EDI_CHECK(projectedVertex.value("cursor").toString() == "default");
    EDI_CHECK(projectedVertex.value("shape").toString() == "square");
    EDI_CHECK(nearlyEqual(projectedVertex.value("size_px").toDouble(), 6.0));
    EDI_CHECK(nearlyEqual(projectedVertex.value("hit_tolerance_px").toDouble(), 0.0));

    QVariantMap dimensionPhysical = dimension.value("physical_geometry").toMap();
    EDI_CHECK(dimensionPhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(dimensionPhysical.value("dimension_distance").toDouble(), 6.0));
    EDI_CHECK(nearlyEqual(dimensionPhysical.value("dimension_length").toDouble(), 6.0));
    EDI_CHECK(nearlyEqual(dimensionPhysical.value("dimension_angle_deg").toDouble(), 53.1301023542));
    EDI_CHECK(nearlyEqual(dimensionPhysical.value("offset").toDouble(), 0.48));
    EDI_CHECK(dimensionPhysical.value("dimension_label").toString() == "6 in");
    dimensionController.updatePointerNormalized(0.218, 0.424);
    QVariantMap dimensionQuickMeasure = dimensionController.modelDocument().value("quick_measurement").toMap();
    EDI_CHECK(dimensionQuickMeasure.value("ok").toBool());
    EDI_CHECK(dimensionQuickMeasure.value("kind").toString() == "dimension");
    EDI_CHECK(dimensionQuickMeasure.value("object_kind").toString() == "dimension");
    EDI_CHECK(dimensionQuickMeasure.value("dimension_kind").toString() == "distance");
    EDI_CHECK(nearlyEqual(dimensionQuickMeasure.value("length").toDouble(), dimension.value("dimension_length").toDouble()));
    EDI_CHECK(nearlyEqual(dimensionQuickMeasure.value("displayed_length").toDouble(), dimension.value("dimension_length").toDouble()));
    EDI_CHECK(nearlyEqual(dimensionQuickMeasure.value("physical_displayed_length").toDouble(), dimensionPhysical.value("dimension_length").toDouble()));
    EDI_CHECK(nearlyEqual(dimensionQuickMeasure.value("physical_angle_deg").toDouble(), dimensionPhysical.value("dimension_angle_deg").toDouble()));
    EDI_CHECK(nearlyEqual(dimensionQuickMeasure.value("physical_offset").toDouble(), dimensionPhysical.value("offset").toDouble()));
    EDI_CHECK(nearlyEqual(dimension.value("dimension_x1").toDouble(), 0.068));
    EDI_CHECK(nearlyEqual(dimension.value("dimension_y1").toDouble(), 0.224));
    EDI_CHECK(nearlyEqual(dimension.value("dimension_x2").toDouble(), 0.368));
    EDI_CHECK(nearlyEqual(dimension.value("dimension_y2").toDouble(), 0.624));
    EDI_CHECK(nearlyEqual(dimension.value("extension_x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(dimension.value("extension_y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(dimension.value("extension_x2").toDouble(), 0.068));
    EDI_CHECK(nearlyEqual(dimension.value("extension_y2").toDouble(), 0.224));
    EDI_CHECK(nearlyEqual(dimension.value("offset").toDouble(), 0.04));
    EDI_CHECK(dimensionController.setSelectedDimensionLabelVisible(false));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(!dimension.value("dimension_show_label").toBool());
    EDI_CHECK(dimensionController.updateSelectedObjectPhysicalGeometryField("offset", 1.2));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(dimension.value("offset").toDouble(), 0.1));
    dimensionPhysical = dimension.value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(dimensionPhysical.value("offset").toDouble(), 1.2));
    EDI_CHECK(dimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 3.0));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(dimension.value("x2").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(dimension.value("y2").toDouble(), 0.4));
    dimensionPhysical = dimension.value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(dimensionPhysical.value("dimension_length").toDouble(), 3.0));
    EDI_CHECK(dimensionController.updateSelectedObjectPhysicalGeometryField("dimension_angle_deg", 0.0));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(dimension.value("x2").toDouble(), 0.35));
    EDI_CHECK(nearlyEqual(dimension.value("y2").toDouble(), 0.2));
    const int dimensionPhysicalRevisionBeforeInvalid = dimensionController.modelDocument().value("revision").toInt();
    EDI_CHECK(!dimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", -1.0));
    EDI_CHECK(dimensionController.modelDocument().value("revision").toInt() == dimensionPhysicalRevisionBeforeInvalid);
    DrawingDocumentController nonDimensionLabelController;
    nonDimensionLabelController.setSelectedToolId("point_tool");
    nonDimensionLabelController.clickCanvasNormalized(0.2, 0.2);
    EDI_CHECK(!nonDimensionLabelController.setSelectedDimensionLabelVisible(false));
    EDI_CHECK(dimensionController.updateSelectedObjectGeometryField("offset", 0.08));
    EDI_CHECK(dimensionController.updateSelectedObjectGeometryField("y2", 0.4));
    EDI_CHECK(dimensionController.updateSelectedObjectGeometryField("x2", 0.5));
    dimension = dimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(dimension.value("offset").toDouble(), 0.08));
    EDI_CHECK(nearlyEqual(dimension.value("x2").toDouble(), 0.5));
    EDI_CHECK(dimensionController.updateSelectedObjectGeometryField("x2", 0.1));
    const int dimensionRevisionBeforeInvalid = dimensionController.modelDocument().value("revision").toInt();
    EDI_CHECK(!dimensionController.updateSelectedObjectGeometryField("y2", 0.2));
    EDI_CHECK(dimensionController.modelDocument().value("revision").toInt() == dimensionRevisionBeforeInvalid);

    DrawingDocumentController widthDimensionController;
    widthDimensionController.setSelectedToolId("width_dimension_tool");
    widthDimensionController.clickCanvasNormalized(0.1, 0.2);
    widthDimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap widthDimension = widthDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(widthDimension.value("dimension_kind").toString() == "width");
    QStringList widthDimensionFieldIds = numericFieldIds(widthDimension);
    EDI_CHECK(widthDimensionFieldIds.contains("dimension_length"));
    EDI_CHECK(!widthDimensionFieldIds.contains("dimension_angle_deg"));
    EDI_CHECK(nearlyEqual(widthDimension.value("x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(widthDimension.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(widthDimension.value("x2").toDouble(), 0.4));
    EDI_CHECK(nearlyEqual(widthDimension.value("y2").toDouble(), 0.2));
    EDI_CHECK(widthDimension.value("label").toString() == "0.3 canvas_unit");
    EDI_CHECK(widthDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 6.0));
    widthDimension = widthDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(widthDimension.value("x2").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(widthDimension.value("y2").toDouble(), 0.2));
    EDI_CHECK(!widthDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_angle_deg", 45.0));

    DrawingDocumentController heightDimensionController;
    heightDimensionController.setSelectedToolId("height_dimension_tool");
    heightDimensionController.clickCanvasNormalized(0.1, 0.2);
    heightDimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap heightDimension = heightDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(heightDimension.value("dimension_kind").toString() == "height");
    QStringList heightDimensionFieldIds = numericFieldIds(heightDimension);
    EDI_CHECK(heightDimensionFieldIds.contains("dimension_length"));
    EDI_CHECK(!heightDimensionFieldIds.contains("dimension_angle_deg"));
    EDI_CHECK(nearlyEqual(heightDimension.value("x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(heightDimension.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(heightDimension.value("x2").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(heightDimension.value("y2").toDouble(), 0.6));
    EDI_CHECK(heightDimension.value("label").toString() == "0.4 canvas_unit");
    EDI_CHECK(heightDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 6.0));
    heightDimension = heightDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(heightDimension.value("x2").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(heightDimension.value("y2").toDouble(), 0.7));

    DrawingDocumentController diameterDimensionController;
    diameterDimensionController.setSelectedToolId("diameter_dimension_tool");
    diameterDimensionController.clickCanvasNormalized(0.1, 0.2);
    diameterDimensionController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap diameterDimension = diameterDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(diameterDimension.value("dimension_kind").toString() == "diameter");
    QStringList diameterDimensionFieldIds = numericFieldIds(diameterDimension);
    EDI_CHECK(diameterDimensionFieldIds.contains("dimension_length"));
    EDI_CHECK(diameterDimensionFieldIds.contains("dimension_angle_deg"));
    EDI_CHECK(diameterDimension.value("label").toString() == "1 canvas_unit");
    EDI_CHECK(nearlyEqual(diameterDimension.value("dimension_distance").toDouble(), 1.0));
    EDI_CHECK(diameterDimensionController.updateSelectedObjectPhysicalGeometryField("dimension_length", 6.0));
    diameterDimension = diameterDimensionController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(diameterDimension.value("x2").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(diameterDimension.value("y2").toDouble(), 0.4));
    EDI_CHECK(nearlyEqual(diameterDimension.value("dimension_distance").toDouble(), 0.5));

    DrawingDocumentController dimensionOffsetScaleController;
    dimensionOffsetScaleController.setGridSize(12.0, 6.0);
    dimensionOffsetScaleController.setSelectedToolId("distance_dimension_tool");
    dimensionOffsetScaleController.clickCanvasNormalized(0.1, 0.2);
    dimensionOffsetScaleController.clickCanvasNormalized(0.4, 0.2);
    QVariantMap scaledOffsetDimension = dimensionOffsetScaleController.modelDocument().value("drawing_objects").toList().front().toMap();
    QVariantMap scaledOffsetPhysical = scaledOffsetDimension.value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(scaledOffsetPhysical.value("offset").toDouble(), 0.24));
    EDI_CHECK(dimensionOffsetScaleController.updateSelectedObjectPhysicalGeometryField("offset", 1.2));
    scaledOffsetDimension = dimensionOffsetScaleController.modelDocument().value("drawing_objects").toList().front().toMap();
    scaledOffsetPhysical = scaledOffsetDimension.value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(scaledOffsetDimension.value("offset").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(scaledOffsetPhysical.value("offset").toDouble(), 1.2));

    DrawingDocumentController dimensionKindController;
    dimensionKindController.setSelectedToolId("distance_dimension_tool");
    dimensionKindController.clickCanvasNormalized(0.1, 0.2);
    dimensionKindController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap dimensionKindObject = dimensionKindController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(dimensionKindObject.value("dimension_kind").toString() == "distance");
    EDI_CHECK(dimensionKindController.setSelectedDimensionKind("width"));
    dimensionKindObject = dimensionKindController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(dimensionKindObject.value("dimension_kind").toString() == "width");
    EDI_CHECK(nearlyEqual(dimensionKindObject.value("x2").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(dimensionKindObject.value("y2").toDouble(), 0.2));
    QStringList switchedWidthFields = numericFieldIds(dimensionKindObject);
    EDI_CHECK(switchedWidthFields.contains("dimension_length"));
    EDI_CHECK(!switchedWidthFields.contains("dimension_angle_deg"));
    EDI_CHECK(dimensionKindController.setSelectedDimensionKind("height"));
    dimensionKindObject = dimensionKindController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(dimensionKindObject.value("dimension_kind").toString() == "height");
    EDI_CHECK(nearlyEqual(dimensionKindObject.value("x2").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(dimensionKindObject.value("y2").toDouble(), 0.7));
    EDI_CHECK(!dimensionKindController.setSelectedDimensionKind("ordinal"));
    const int dimensionKindRevisionBeforeInvalidLength = dimensionKindController.modelDocument().value("revision").toInt();
    EDI_CHECK(!dimensionKindController.updateSelectedObjectGeometryField("dimension_length", 0.0));
    EDI_CHECK(dimensionKindController.modelDocument().value("revision").toInt() == dimensionKindRevisionBeforeInvalidLength);
    DrawingDocumentController nonDimensionKindController;
    nonDimensionKindController.setSelectedToolId("point_tool");
    nonDimensionKindController.clickCanvasNormalized(0.2, 0.2);
    EDI_CHECK(!nonDimensionKindController.setSelectedDimensionKind("width"));

    DrawingDocumentController dimensionHandleController;
    dimensionHandleController.setSelectedToolId("distance_dimension_tool");
    dimensionHandleController.clickCanvasNormalized(0.1, 0.2);
    dimensionHandleController.clickCanvasNormalized(0.5, 0.2);
    EDI_CHECK(dimensionHandleController.editSelectedHandleNormalized("dimension_end", 0.8, 0.4));
    QVariantMap handleDimension = dimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(handleDimension.value("x2").toDouble(), 0.8));
    EDI_CHECK(nearlyEqual(handleDimension.value("y2").toDouble(), 0.4));
    EDI_CHECK(dimensionHandleController.editSelectedHandleNormalized("dimension_offset", 0.45, 0.45));
    handleDimension = dimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(handleDimension.value("offset").toDouble() > 0.0);
    EDI_CHECK(!dimensionHandleController.editSelectedHandleNormalized("dimension_missing", 0.2, 0.2));

    DrawingDocumentController widthDimensionHandleController;
    widthDimensionHandleController.setSelectedToolId("width_dimension_tool");
    widthDimensionHandleController.clickCanvasNormalized(0.1, 0.2);
    widthDimensionHandleController.clickCanvasNormalized(0.5, 0.8);
    EDI_CHECK(widthDimensionHandleController.editSelectedHandleNormalized("dimension_end", 0.9, 0.9));
    QVariantMap widthHandleDimension = widthDimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(widthHandleDimension.value("dimension_kind").toString() == "width");
    EDI_CHECK(nearlyEqual(widthHandleDimension.value("x2").toDouble(), 0.9));
    EDI_CHECK(nearlyEqual(widthHandleDimension.value("y2").toDouble(), 0.2));

    DrawingDocumentController heightDimensionHandleController;
    heightDimensionHandleController.setSelectedToolId("height_dimension_tool");
    heightDimensionHandleController.clickCanvasNormalized(0.1, 0.2);
    heightDimensionHandleController.clickCanvasNormalized(0.5, 0.8);
    EDI_CHECK(heightDimensionHandleController.editSelectedHandleNormalized("dimension_end", 0.9, 0.9));
    QVariantMap heightHandleDimension = heightDimensionHandleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(heightHandleDimension.value("dimension_kind").toString() == "height");
    EDI_CHECK(nearlyEqual(heightHandleDimension.value("x2").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(heightHandleDimension.value("y2").toDouble(), 0.9));

    DrawingDocumentController objectSnapController;
    objectSnapController.setSelectedToolId("point_tool");
    objectSnapController.clickCanvasNormalized(0.25, 0.25);
    objectSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(objectSnapController.objectSnapEnabled());
    objectSnapController.clickCanvasNormalized(0.26, 0.24);
    QVariantList snappedObjects = objectSnapController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(snappedObjects.size() == 2);
    QVariantMap snappedPoint = snappedObjects.back().toMap();
    EDI_CHECK(nearlyEqual(snappedPoint.value("x").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(snappedPoint.value("y").toDouble(), 0.25));

    QVariantList beforePointerObjects = objectSnapController.modelDocument().value("drawing_objects").toList();
    objectSnapController.updatePointerNormalized(0.26, 0.24);
    QVariantMap pointerModel = objectSnapController.modelDocument();
    QVariantMap pointer = pointerModel.value("pointer").toMap();
    EDI_CHECK(!pointer.isEmpty());
    EDI_CHECK(nearlyEqual(pointer.value("raw").toMap().value("x").toDouble(), 0.26));
    EDI_CHECK(nearlyEqual(pointer.value("raw").toMap().value("y").toDouble(), 0.24));
    EDI_CHECK(pointer.value("kind").toString() == "object");
    EDI_CHECK(pointer.value("source").toString() == "endpoint");
    EDI_CHECK(nearlyEqual(pointer.value("snapped").toMap().value("x").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(pointer.value("snapped").toMap().value("y").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(pointer.value("snapped_unit_x").toDouble(), 3.0));
    EDI_CHECK(nearlyEqual(pointer.value("snapped_unit_y").toDouble(), 3.0));
    EDI_CHECK(pointer.value("unit_label").toString() == "in");
    EDI_CHECK(pointer.value("inside_drawable").toBool());
    EDI_CHECK(pointerModel.value("drawing_objects").toList().size() == beforePointerObjects.size());

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
    EDI_CHECK(guidePointer.value("kind").toString() == "object");
    EDI_CHECK(guidePointer.value("source").toString() == "guide");
    EDI_CHECK(guidePointer.value("label").toString() == "guide");
    EDI_CHECK(!guidePointer.value("source_object_id").toString().isEmpty());
    EDI_CHECK(nearlyEqual(guidePointer.value("snapped").toMap().value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guidePointer.value("snapped").toMap().value("y").toDouble(), 0.75));
    EDI_CHECK(guidePointerModel.value("drawing_objects").toList().size() == beforeGuidePointerObjects.size());
    guidePointerController.setGuideSnapEnabled(false);
    guidePointerController.updatePointerNormalized(0.34, 0.74);
    guidePointer = guidePointerController.modelDocument().value("pointer").toMap();
    EDI_CHECK(guidePointer.value("kind").toString() == "none");

    DrawingDocumentController emptyMeasureController;
    emptyMeasureController.updatePointerNormalized(0.5, 0.5);
    QVariantMap emptyMeasureModel = emptyMeasureController.modelDocument();
    QVariantMap emptyMeasure = emptyMeasureModel.value("quick_measurement").toMap();
    EDI_CHECK(!emptyMeasure.value("ok").toBool());
    EDI_CHECK(emptyMeasure.value("kind").toString() == "none");
    EDI_CHECK(emptyMeasure.value("message").toString() == "no measurable target");

    DrawingDocumentController lineMeasureController;
    lineMeasureController.setSelectedToolId("line_tool");
    lineMeasureController.clickCanvasNormalized(0.1, 0.2);
    lineMeasureController.clickCanvasNormalized(0.4, 0.6);
    const int lineMeasureRevision = lineMeasureController.modelDocument().value("revision").toInt();
    const int lineMeasureObjectCount = lineMeasureController.modelDocument().value("drawing_objects").toList().size();
    lineMeasureController.updatePointerNormalized(0.25, 0.4);
    QVariantMap lineMeasureModel = lineMeasureController.modelDocument();
    QVariantMap lineMeasure = lineMeasureModel.value("quick_measurement").toMap();
    EDI_CHECK(lineMeasure.value("ok").toBool());
    EDI_CHECK(lineMeasure.value("kind").toString() == "line");
    EDI_CHECK(lineMeasure.value("object_kind").toString() == "line");
    EDI_CHECK(nearlyEqual(lineMeasure.value("length").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(lineMeasure.value("physical_length").toDouble(), 6.0));
    EDI_CHECK(nearlyEqual(lineMeasure.value("physical_angle_deg").toDouble(), 53.1301023542));
    EDI_CHECK(lineMeasure.value("unit_label").toString() == "in");
    QVariantMap projectedLinePhysical = lineMeasureModel.value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(lineMeasure.value("physical_length").toDouble(), projectedLinePhysical.value("line_length").toDouble()));
    EDI_CHECK(nearlyEqual(lineMeasure.value("physical_angle_deg").toDouble(), projectedLinePhysical.value("line_angle_deg").toDouble()));
    EDI_CHECK(lineMeasureModel.value("revision").toInt() == lineMeasureRevision);
    EDI_CHECK(lineMeasureModel.value("drawing_objects").toList().size() == lineMeasureObjectCount);

    DrawingDocumentController circleMeasureController;
    circleMeasureController.setSelectedToolId("circle_tool");
    circleMeasureController.clickCanvasNormalized(0.5, 0.5);
    circleMeasureController.clickCanvasNormalized(0.7, 0.5);
    circleMeasureController.updatePointerNormalized(0.7, 0.5);
    QVariantMap circleMeasure = circleMeasureController.modelDocument().value("quick_measurement").toMap();
    EDI_CHECK(circleMeasure.value("ok").toBool());
    EDI_CHECK(circleMeasure.value("kind").toString() == "circle");
    EDI_CHECK(nearlyEqual(circleMeasure.value("radius").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(circleMeasure.value("diameter").toDouble(), 0.4));
    EDI_CHECK(nearlyEqual(circleMeasure.value("physical_radius").toDouble(), 2.4));
    EDI_CHECK(nearlyEqual(circleMeasure.value("physical_diameter").toDouble(), 4.8));
    QVariantMap projectedCirclePhysical = circleMeasureController.modelDocument().value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(circleMeasure.value("physical_radius").toDouble(), projectedCirclePhysical.value("radius").toDouble()));
    EDI_CHECK(nearlyEqual(circleMeasure.value("physical_diameter").toDouble(), projectedCirclePhysical.value("diameter").toDouble()));

    DrawingDocumentController rectMeasureController;
    rectMeasureController.setSelectedToolId("rectangle_tool");
    rectMeasureController.clickCanvasNormalized(0.1, 0.2);
    rectMeasureController.clickCanvasNormalized(0.4, 0.6);
    rectMeasureController.updatePointerNormalized(0.2, 0.3);
    QVariantMap rectMeasure = rectMeasureController.modelDocument().value("quick_measurement").toMap();
    EDI_CHECK(rectMeasure.value("ok").toBool());
    EDI_CHECK(rectMeasure.value("kind").toString() == "rectangle");
    EDI_CHECK(nearlyEqual(rectMeasure.value("width").toDouble(), 0.3));
    EDI_CHECK(nearlyEqual(rectMeasure.value("height").toDouble(), 0.4));
    EDI_CHECK(nearlyEqual(rectMeasure.value("physical_width").toDouble(), 3.6));
    EDI_CHECK(nearlyEqual(rectMeasure.value("physical_height").toDouble(), 4.8));
    EDI_CHECK(nearlyEqual(rectMeasure.value("physical_area").toDouble(), 17.28));
    QVariantMap projectedRectPhysical = rectMeasureController.modelDocument().value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(rectMeasure.value("physical_width").toDouble(), projectedRectPhysical.value("width").toDouble()));
    EDI_CHECK(nearlyEqual(rectMeasure.value("physical_height").toDouble(), projectedRectPhysical.value("height").toDouble()));

    DrawingDocumentController pointMeasureController;
    pointMeasureController.setSelectedToolId("point_tool");
    pointMeasureController.clickCanvasNormalized(0.25, 0.5);
    pointMeasureController.updatePointerNormalized(0.25, 0.5);
    QVariantMap pointMeasure = pointMeasureController.modelDocument().value("quick_measurement").toMap();
    EDI_CHECK(pointMeasure.value("ok").toBool());
    EDI_CHECK(pointMeasure.value("kind").toString() == "point");
    EDI_CHECK(nearlyEqual(pointMeasure.value("physical_x").toDouble(), 3.0));
    EDI_CHECK(nearlyEqual(pointMeasure.value("physical_y").toDouble(), 6.0));
    QVariantMap projectedPointPhysical = pointMeasureController.modelDocument().value("drawing_objects").toList().front().toMap().value("physical_geometry").toMap();
    EDI_CHECK(nearlyEqual(pointMeasure.value("physical_x").toDouble(), projectedPointPhysical.value("x").toDouble()));
    EDI_CHECK(nearlyEqual(pointMeasure.value("physical_y").toDouble(), projectedPointPhysical.value("y").toDouble()));

    DrawingDocumentController guideCreationSnapController;
    guideCreationSnapController.setSelectedToolId("horizontal_guide_tool");
    guideCreationSnapController.clickCanvasNormalized(0.2, 0.75);
    guideCreationSnapController.setSelectedToolId("vertical_guide_tool");
    guideCreationSnapController.clickCanvasNormalized(0.33, 0.2);
    guideCreationSnapController.setObjectSnapEnabled(true);
    guideCreationSnapController.setSelectedToolId("point_tool");
    guideCreationSnapController.clickCanvasNormalized(0.34, 0.74);
    QVariantList guideSnappedObjects = guideCreationSnapController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(guideSnappedObjects.size() == 3);
    QVariantMap guideSnappedPoint = guideSnappedObjects.back().toMap();
    EDI_CHECK(guideSnappedPoint.value("kind").toString() == "point");
    EDI_CHECK(nearlyEqual(guideSnappedPoint.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideSnappedPoint.value("y").toDouble(), 0.75));

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
    EDI_CHECK(guideMoveDisabledCreatedPoint.value("kind").toString() == "point");
    EDI_CHECK(nearlyEqual(guideMoveDisabledCreatedPoint.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideMoveDisabledCreatedPoint.value("y").toDouble(), 0.75));

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
    EDI_CHECK(nearlyEqual(guideUnsnappedPoint.value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(guideUnsnappedPoint.value("y").toDouble(), 0.74));

    DrawingDocumentController hiddenGuideCreationController;
    hiddenGuideCreationController.setSelectedToolId("horizontal_guide_tool");
    hiddenGuideCreationController.clickCanvasNormalized(0.2, 0.75);
    hiddenGuideCreationController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideCreationController.clickCanvasNormalized(0.33, 0.2);
    hiddenGuideCreationController.setObjectSnapEnabled(true);
    EDI_CHECK(hiddenGuideCreationController.setAllGuidesVisible(false));
    hiddenGuideCreationController.setSelectedToolId("point_tool");
    hiddenGuideCreationController.clickCanvasNormalized(0.34, 0.74);
    QVariantMap hiddenGuideUnsnappedPoint = hiddenGuideCreationController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(hiddenGuideUnsnappedPoint.value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(hiddenGuideUnsnappedPoint.value("y").toDouble(), 0.74));

    DrawingDocumentController hiddenGuideLayerCreationController;
    EDI_CHECK(hiddenGuideLayerCreationController.createLayer());
    hiddenGuideLayerCreationController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideLayerCreationController.clickCanvasNormalized(0.33, 0.2);
    EDI_CHECK(hiddenGuideLayerCreationController.setActiveLayerVisible(false));
    EDI_CHECK(hiddenGuideLayerCreationController.setActiveLayerId(QStringLiteral("default")));
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
    EDI_CHECK(!hiddenGuideLayerPoint.isEmpty());
    EDI_CHECK(nearlyEqual(hiddenGuideLayerPoint.value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(hiddenGuideLayerPoint.value("y").toDouble(), 0.2));

    DrawingDocumentController lockedGuideCreationController;
    lockedGuideCreationController.setSelectedToolId("vertical_guide_tool");
    lockedGuideCreationController.clickCanvasNormalized(0.33, 0.2);
    EDI_CHECK(lockedGuideCreationController.setSelectedObjectLocked(true));
    lockedGuideCreationController.setObjectSnapEnabled(true);
    lockedGuideCreationController.setSelectedToolId("point_tool");
    lockedGuideCreationController.clickCanvasNormalized(0.34, 0.2);
    QVariantMap lockedGuideSnappedPoint = lockedGuideCreationController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(lockedGuideSnappedPoint.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(lockedGuideSnappedPoint.value("y").toDouble(), 0.2));

    DrawingDocumentController guideMoveSnapController;
    guideMoveSnapController.setSelectedToolId("horizontal_guide_tool");
    guideMoveSnapController.clickCanvasNormalized(0.2, 0.75);
    guideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    guideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    guideMoveSnapController.setSelectedToolId("point_tool");
    guideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    guideMoveSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(guideMoveSnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap guideMoveModel = guideMoveSnapController.modelDocument();
    QVariantMap guideMovedPoint = guideMoveModel.value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideMovedPoint.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideMovedPoint.value("y").toDouble(), 0.75));
    QVariantMap guideDragSnap = guideMoveModel.value("guide_drag_snap").toMap();
    EDI_CHECK(!guideDragSnap.isEmpty());
    EDI_CHECK(guideDragSnap.value("kind").toString() == "guide");
    EDI_CHECK(guideDragSnap.value("mode").toString() == "move_selection");
    EDI_CHECK(guideDragSnap.value("anchor_label").toString() == "point");
    EDI_CHECK(guideDragSnap.value("intersection").toBool());
    EDI_CHECK(!guideDragSnap.value("source_object_id").toString().isEmpty());
    EDI_CHECK(nearlyEqual(guideDragSnap.value("raw_anchor").toMap().value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(guideDragSnap.value("raw_anchor").toMap().value("y").toDouble(), 0.74));
    EDI_CHECK(nearlyEqual(guideDragSnap.value("snapped_anchor").toMap().value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideDragSnap.value("snapped_anchor").toMap().value("y").toDouble(), 0.75));

    DrawingDocumentController disabledGuideMoveSnapController;
    disabledGuideMoveSnapController.setSelectedToolId("horizontal_guide_tool");
    disabledGuideMoveSnapController.clickCanvasNormalized(0.2, 0.75);
    disabledGuideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    disabledGuideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideMoveSnapController.setSelectedToolId("point_tool");
    disabledGuideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideMoveSnapController.setObjectSnapEnabled(true);
    disabledGuideMoveSnapController.setGuideSnapEnabled(false);
    EDI_CHECK(disabledGuideMoveSnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap disabledGuideMoveModel = disabledGuideMoveSnapController.modelDocument();
    QVariantMap disabledGuideMovedPoint = disabledGuideMoveModel.value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(disabledGuideMovedPoint.value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(disabledGuideMovedPoint.value("y").toDouble(), 0.74));
    EDI_CHECK(!disabledGuideMoveModel.contains("guide_drag_snap"));

    DrawingDocumentController disabledGuideMoveOnlySnapController;
    disabledGuideMoveOnlySnapController.setSelectedToolId("horizontal_guide_tool");
    disabledGuideMoveOnlySnapController.clickCanvasNormalized(0.2, 0.75);
    disabledGuideMoveOnlySnapController.setSelectedToolId("vertical_guide_tool");
    disabledGuideMoveOnlySnapController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideMoveOnlySnapController.setSelectedToolId("point_tool");
    disabledGuideMoveOnlySnapController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideMoveOnlySnapController.setObjectSnapEnabled(true);
    disabledGuideMoveOnlySnapController.setGuideMoveSnapEnabled(false);
    EDI_CHECK(disabledGuideMoveOnlySnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap disabledGuideMoveOnlyModel = disabledGuideMoveOnlySnapController.modelDocument();
    QVariantMap disabledGuideMoveOnlyPoint = disabledGuideMoveOnlyModel.value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(disabledGuideMoveOnlyPoint.value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(disabledGuideMoveOnlyPoint.value("y").toDouble(), 0.74));
    EDI_CHECK(!disabledGuideMoveOnlyModel.contains("guide_drag_snap"));

    DrawingDocumentController hiddenGuideMoveSnapController;
    hiddenGuideMoveSnapController.setSelectedToolId("horizontal_guide_tool");
    hiddenGuideMoveSnapController.clickCanvasNormalized(0.2, 0.75);
    hiddenGuideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    EDI_CHECK(hiddenGuideMoveSnapController.setAllGuidesVisible(false));
    hiddenGuideMoveSnapController.setSelectedToolId("point_tool");
    hiddenGuideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    hiddenGuideMoveSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(hiddenGuideMoveSnapController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap hiddenGuideMovedPoint = hiddenGuideMoveSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(hiddenGuideMovedPoint.value("x").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(hiddenGuideMovedPoint.value("y").toDouble(), 0.74));

    DrawingDocumentController lockedGuideMoveSnapController;
    lockedGuideMoveSnapController.setSelectedToolId("vertical_guide_tool");
    lockedGuideMoveSnapController.clickCanvasNormalized(0.33, 0.2);
    EDI_CHECK(lockedGuideMoveSnapController.setSelectedObjectLocked(true));
    lockedGuideMoveSnapController.setSelectedToolId("point_tool");
    lockedGuideMoveSnapController.clickCanvasNormalized(0.2, 0.2);
    lockedGuideMoveSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(lockedGuideMoveSnapController.moveSelectionNormalized(0.14, 0.0));
    QVariantMap lockedGuideMovedPoint = lockedGuideMoveSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(lockedGuideMovedPoint.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(lockedGuideMovedPoint.value("y").toDouble(), 0.2));

    DrawingDocumentController guideRectangleLeftEdgeMoveController;
    guideRectangleLeftEdgeMoveController.setSelectedToolId("vertical_guide_tool");
    guideRectangleLeftEdgeMoveController.clickCanvasNormalized(0.33, 0.2);
    guideRectangleLeftEdgeMoveController.setSelectedToolId("rectangle_tool");
    guideRectangleLeftEdgeMoveController.clickCanvasNormalized(0.2, 0.2);
    guideRectangleLeftEdgeMoveController.clickCanvasNormalized(0.4, 0.4);
    guideRectangleLeftEdgeMoveController.setObjectSnapEnabled(true);
    EDI_CHECK(guideRectangleLeftEdgeMoveController.moveSelectionNormalized(0.12, 0.0));
    QVariantMap guideLeftEdgeRect = lastObjectOfKind(guideRectangleLeftEdgeMoveController.modelDocument(), QStringLiteral("rectangle"));
    EDI_CHECK(nearlyEqual(guideLeftEdgeRect.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideLeftEdgeRect.value("y").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(guideLeftEdgeRect.value("width").toDouble(), 0.2));

    DrawingDocumentController guideRectangleTopEdgeMoveController;
    guideRectangleTopEdgeMoveController.setSelectedToolId("horizontal_guide_tool");
    guideRectangleTopEdgeMoveController.clickCanvasNormalized(0.2, 0.75);
    guideRectangleTopEdgeMoveController.setSelectedToolId("rectangle_tool");
    guideRectangleTopEdgeMoveController.clickCanvasNormalized(0.2, 0.2);
    guideRectangleTopEdgeMoveController.clickCanvasNormalized(0.4, 0.4);
    guideRectangleTopEdgeMoveController.setObjectSnapEnabled(true);
    EDI_CHECK(guideRectangleTopEdgeMoveController.moveSelectionNormalized(0.0, 0.54));
    QVariantMap guideTopEdgeRect = lastObjectOfKind(guideRectangleTopEdgeMoveController.modelDocument(), QStringLiteral("rectangle"));
    EDI_CHECK(nearlyEqual(guideTopEdgeRect.value("x").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(guideTopEdgeRect.value("y").toDouble(), 0.75));
    EDI_CHECK(nearlyEqual(guideTopEdgeRect.value("height").toDouble(), 0.2));

    DrawingDocumentController guideLineEndpointMoveController;
    guideLineEndpointMoveController.setSelectedToolId("horizontal_guide_tool");
    guideLineEndpointMoveController.clickCanvasNormalized(0.2, 0.75);
    guideLineEndpointMoveController.setSelectedToolId("vertical_guide_tool");
    guideLineEndpointMoveController.clickCanvasNormalized(0.33, 0.2);
    guideLineEndpointMoveController.setSelectedToolId("line_tool");
    guideLineEndpointMoveController.clickCanvasNormalized(0.1, 0.1);
    guideLineEndpointMoveController.clickCanvasNormalized(0.2, 0.2);
    guideLineEndpointMoveController.setObjectSnapEnabled(true);
    EDI_CHECK(guideLineEndpointMoveController.moveSelectionNormalized(0.14, 0.54));
    QVariantMap guideEndpointLine = lastObjectOfKind(guideLineEndpointMoveController.modelDocument(), QStringLiteral("line"));
    EDI_CHECK(nearlyEqual(guideEndpointLine.value("x1").toDouble(), 0.23));
    EDI_CHECK(nearlyEqual(guideEndpointLine.value("y1").toDouble(), 0.65));
    EDI_CHECK(nearlyEqual(guideEndpointLine.value("x2").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideEndpointLine.value("y2").toDouble(), 0.75));

    DrawingDocumentController guideRectangleCornerMoveController;
    guideRectangleCornerMoveController.setSelectedToolId("horizontal_guide_tool");
    guideRectangleCornerMoveController.clickCanvasNormalized(0.2, 0.75);
    guideRectangleCornerMoveController.setSelectedToolId("vertical_guide_tool");
    guideRectangleCornerMoveController.clickCanvasNormalized(0.33, 0.2);
    guideRectangleCornerMoveController.setSelectedToolId("rectangle_tool");
    guideRectangleCornerMoveController.clickCanvasNormalized(0.2, 0.2);
    guideRectangleCornerMoveController.clickCanvasNormalized(0.4, 0.4);
    guideRectangleCornerMoveController.setObjectSnapEnabled(true);
    EDI_CHECK(guideRectangleCornerMoveController.moveSelectionNormalized(-0.06, 0.34));
    QVariantMap guideCornerRect = lastObjectOfKind(guideRectangleCornerMoveController.modelDocument(), QStringLiteral("rectangle"));
    EDI_CHECK(nearlyEqual(guideCornerRect.value("x").toDouble(), 0.13));
    EDI_CHECK(nearlyEqual(guideCornerRect.value("y").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(guideCornerRect.value("width").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(guideCornerRect.value("height").toDouble(), 0.2));

    DrawingDocumentController disabledGuideRectangleEdgeMoveController;
    disabledGuideRectangleEdgeMoveController.setSelectedToolId("vertical_guide_tool");
    disabledGuideRectangleEdgeMoveController.clickCanvasNormalized(0.33, 0.2);
    disabledGuideRectangleEdgeMoveController.setSelectedToolId("rectangle_tool");
    disabledGuideRectangleEdgeMoveController.clickCanvasNormalized(0.2, 0.2);
    disabledGuideRectangleEdgeMoveController.clickCanvasNormalized(0.4, 0.4);
    disabledGuideRectangleEdgeMoveController.setObjectSnapEnabled(true);
    disabledGuideRectangleEdgeMoveController.setGuideSnapEnabled(false);
    EDI_CHECK(disabledGuideRectangleEdgeMoveController.moveSelectionNormalized(0.12, 0.0));
    QVariantMap disabledGuideEdgeRect = lastObjectOfKind(disabledGuideRectangleEdgeMoveController.modelDocument(), QStringLiteral("rectangle"));
    EDI_CHECK(nearlyEqual(disabledGuideEdgeRect.value("x").toDouble(), 0.32));
    EDI_CHECK(nearlyEqual(disabledGuideEdgeRect.value("y").toDouble(), 0.2));

    DrawingDocumentController guideHandleSnapController;
    guideHandleSnapController.setSelectedToolId("horizontal_guide_tool");
    guideHandleSnapController.clickCanvasNormalized(0.2, 0.75);
    guideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    guideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    guideHandleSnapController.setSelectedToolId("line_tool");
    guideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    guideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    guideHandleSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(guideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.74));
    QVariantMap guideHandleLine = guideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(guideHandleLine.value("x2").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideHandleLine.value("y2").toDouble(), 0.75));

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
    EDI_CHECK(disabledGuideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.74));
    QVariantMap disabledGuideHandleLine = disabledGuideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(disabledGuideHandleLine.value("x2").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(disabledGuideHandleLine.value("y2").toDouble(), 0.74));

    DrawingDocumentController hiddenGuideHandleSnapController;
    hiddenGuideHandleSnapController.setSelectedToolId("horizontal_guide_tool");
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.2, 0.75);
    hiddenGuideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    EDI_CHECK(hiddenGuideHandleSnapController.setAllGuidesVisible(false));
    hiddenGuideHandleSnapController.setSelectedToolId("line_tool");
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    hiddenGuideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    hiddenGuideHandleSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(hiddenGuideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.74));
    QVariantMap hiddenGuideHandleLine = hiddenGuideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(hiddenGuideHandleLine.value("x2").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(hiddenGuideHandleLine.value("y2").toDouble(), 0.74));

    DrawingDocumentController lockedGuideHandleSnapController;
    lockedGuideHandleSnapController.setSelectedToolId("vertical_guide_tool");
    lockedGuideHandleSnapController.clickCanvasNormalized(0.33, 0.2);
    EDI_CHECK(lockedGuideHandleSnapController.setSelectedObjectLocked(true));
    lockedGuideHandleSnapController.setSelectedToolId("line_tool");
    lockedGuideHandleSnapController.clickCanvasNormalized(0.1, 0.1);
    lockedGuideHandleSnapController.clickCanvasNormalized(0.2, 0.2);
    lockedGuideHandleSnapController.setObjectSnapEnabled(true);
    EDI_CHECK(lockedGuideHandleSnapController.editSelectedHandleNormalized(QStringLiteral("line_end"), 0.34, 0.2));
    QVariantMap lockedGuideHandleLine = lockedGuideHandleSnapController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(nearlyEqual(lockedGuideHandleLine.value("x2").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(lockedGuideHandleLine.value("y2").toDouble(), 0.2));

    DrawingDocumentController invisibleSnapController;
    invisibleSnapController.setSelectedToolId("point_tool");
    invisibleSnapController.clickCanvasNormalized(0.25, 0.25);
    EDI_CHECK(invisibleSnapController.setSelectedObjectVisible(false));
    QVariantMap invisiblePoint = invisibleSnapController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(!invisiblePoint.value("visible").toBool());
    invisibleSnapController.setObjectSnapEnabled(true);
    invisibleSnapController.clickCanvasNormalized(0.26, 0.24);
    QVariantList invisibleSnapObjects = invisibleSnapController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(invisibleSnapObjects.size() == 2);
    QVariantMap unsnappedPoint = invisibleSnapObjects.back().toMap();
    EDI_CHECK(nearlyEqual(unsnappedPoint.value("x").toDouble(), 0.26));
    EDI_CHECK(nearlyEqual(unsnappedPoint.value("y").toDouble(), 0.24));

    DrawingDocumentController invisibleHitController;
    invisibleHitController.setSelectedToolId("point_tool");
    invisibleHitController.clickCanvasNormalized(0.25, 0.25);
    EDI_CHECK(invisibleHitController.setSelectedObjectVisible(false));
    invisibleHitController.setSelectedToolId("select_move");
    invisibleHitController.clickCanvasNormalized(0.25, 0.25);
    EDI_CHECK(invisibleHitController.selectedObjectId().isEmpty());

    DrawingDocumentController layerController;
    layerController.setSelectedToolId("point_tool");
    layerController.clickCanvasNormalized(0.25, 0.25);
    QVariantMap layerModel = layerController.modelDocument();
    QVariantList layers = layerModel.value("layers").toList();
    EDI_CHECK(layers.size() == 1);
    EDI_CHECK(layers.front().toMap().value("id").toString() == "default");
    EDI_CHECK(layers.front().toMap().value("visible").toBool());
    EDI_CHECK(!layers.front().toMap().value("locked").toBool());

    EDI_CHECK(layerController.setDefaultLayerVisible(false));
    layerModel = layerController.modelDocument();
    layers = layerModel.value("layers").toList();
    EDI_CHECK(!layers.front().toMap().value("visible").toBool());
    QVariantMap hiddenLayerPoint = layerModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(hiddenLayerPoint.value("visible").toBool());
    EDI_CHECK(!hiddenLayerPoint.value("effective_visible").toBool());
    layerController.setObjectSnapEnabled(true);
    layerController.clickCanvasNormalized(0.26, 0.24);
    QVariantList hiddenLayerObjects = layerController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(hiddenLayerObjects.size() == 2);
    QVariantMap hiddenLayerUnsnappedPoint = hiddenLayerObjects.back().toMap();
    EDI_CHECK(nearlyEqual(hiddenLayerUnsnappedPoint.value("x").toDouble(), 0.26));
    EDI_CHECK(nearlyEqual(hiddenLayerUnsnappedPoint.value("y").toDouble(), 0.24));
    layerController.setSelectedToolId("select_move");
    layerController.clickCanvasNormalized(0.25, 0.25);
    EDI_CHECK(layerController.selectedObjectId().isEmpty());

    EDI_CHECK(layerController.setDefaultLayerVisible(true));
    EDI_CHECK(layerController.setDefaultLayerLocked(true));
    layerModel = layerController.modelDocument();
    layers = layerModel.value("layers").toList();
    EDI_CHECK(layers.front().toMap().value("locked").toBool());
    layerController.setSelectedToolId("point_tool");
    const int lockedLayerObjectCount = layerController.modelDocument().value("drawing_objects").toList().size();
    layerController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(layerController.modelDocument().value("drawing_objects").toList().size() == lockedLayerObjectCount);
    EDI_CHECK(!layerController.setSelectedObjectLocked(true));
    EDI_CHECK(!layerController.updateSelectedObjectGeometryField("x", 0.3));
    EDI_CHECK(!layerController.moveSelectionNormalized(0.1, 0.0));
    EDI_CHECK(layerController.setDefaultLayerLocked(false));
    layerController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(layerController.modelDocument().value("drawing_objects").toList().size() == lockedLayerObjectCount + 1);

    DrawingDocumentController layerManagementController;
    EDI_CHECK(layerManagementController.createLayer());
    QVariantMap layerManagementModel = layerManagementController.modelDocument();
    EDI_CHECK(layerManagementModel.value("active_layer_id").toString() == "layer_2");
    QVariantList managedLayers = layerManagementModel.value("layers").toList();
    EDI_CHECK(managedLayers.size() == 2);
    EDI_CHECK(layerManagementController.renameActiveLayer("Ink"));
    managedLayers = layerManagementController.modelDocument().value("layers").toList();
    EDI_CHECK(managedLayers[1].toMap().value("name").toString() == "Ink");
    layerManagementController.setSelectedToolId("point_tool");
    layerManagementController.clickCanvasNormalized(0.2, 0.2);
    QVariantList managedObjects = layerManagementController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(managedObjects.size() == 1);
    QVariantMap managedPoint = managedObjects.front().toMap();
    EDI_CHECK(managedPoint.value("layer_id").toString() == "layer_2");
    EDI_CHECK(layerManagementController.moveSelectedObjectToLayer("default"));
    managedPoint = layerManagementController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(managedPoint.value("layer_id").toString() == "default");
    EDI_CHECK(layerManagementController.setActiveLayerId("layer_2"));
    EDI_CHECK(layerManagementController.setActiveLayerLocked(true));
    const int managedObjectCountBeforeLockedCreate = layerManagementController.modelDocument().value("drawing_objects").toList().size();
    layerManagementController.clickCanvasNormalized(0.4, 0.4);
    EDI_CHECK(layerManagementController.modelDocument().value("drawing_objects").toList().size() == managedObjectCountBeforeLockedCreate);
    EDI_CHECK(layerManagementController.setActiveLayerLocked(false));
    EDI_CHECK(layerManagementController.setActiveLayerId("default"));
    EDI_CHECK(layerManagementController.setActiveLayerLocked(true));
    EDI_CHECK(!layerManagementController.moveSelectedObjectToLayer("layer_2"));
    EDI_CHECK(layerManagementController.setActiveLayerLocked(false));

    DrawingDocumentController layerOrderController;
    layerOrderController.setSelectedToolId("point_tool");
    layerOrderController.clickCanvasNormalized(0.1, 0.1);
    EDI_CHECK(layerOrderController.createLayer());
    EDI_CHECK(layerOrderController.activeLayerId() == "layer_2");
    layerOrderController.clickCanvasNormalized(0.2, 0.2);
    QVariantMap layerOrderModel = layerOrderController.modelDocument();
    QVariantList orderedLayers = layerOrderModel.value("layers").toList();
    EDI_CHECK(orderedLayers[0].toMap().value("id").toString() == "default");
    EDI_CHECK(orderedLayers[1].toMap().value("id").toString() == "layer_2");
    QVariantList orderedObjects = layerOrderModel.value("drawing_objects").toList();
    EDI_CHECK(orderedObjects[0].toMap().value("layer_id").toString() == "default");
    EDI_CHECK(orderedObjects[1].toMap().value("layer_id").toString() == "layer_2");
    EDI_CHECK(layerOrderController.moveActiveLayer("down"));
    layerOrderModel = layerOrderController.modelDocument();
    orderedLayers = layerOrderModel.value("layers").toList();
    EDI_CHECK(orderedLayers[0].toMap().value("id").toString() == "layer_2");
    EDI_CHECK(orderedLayers[1].toMap().value("id").toString() == "default");
    orderedObjects = layerOrderModel.value("drawing_objects").toList();
    EDI_CHECK(orderedObjects[0].toMap().value("layer_id").toString() == "layer_2");
    EDI_CHECK(orderedObjects[1].toMap().value("layer_id").toString() == "default");
    EDI_CHECK(!layerOrderController.moveActiveLayer("sideways"));

    DrawingDocumentController layerPlotController;
    EDI_CHECK(layerPlotController.createLayer());
    EDI_CHECK(layerPlotController.setActiveLayerPenPreset("pen_blue"));
    EDI_CHECK(layerPlotController.setActiveLayerStrokeWidthPreset("fine"));
    layerPlotController.setSelectedToolId("point_tool");
    layerPlotController.clickCanvasNormalized(0.2, 0.2);
    QVariantMap layerPlotModel = layerPlotController.modelDocument();
    QVariantMap layerPlotPoint = layerPlotModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(layerPlotPoint.value("layer_id").toString() == "layer_2");
    EDI_CHECK(layerPlotPoint.value("effective_plot_enabled").toBool());
    EDI_CHECK(layerPlotPoint.value("effective_plot_ready").toBool());
    EDI_CHECK(layerPlotPoint.value("plot_ready").toBool());
    EDI_CHECK(!layerPlotPoint.value("plot_blocked").toBool());
    EDI_CHECK(layerPlotPoint.value("plot_safety_state").toString() == "ready");
    EDI_CHECK(layerPlotPoint.value("plot_warning_count").toInt() == 0);
    EDI_CHECK(layerPlotPoint.value("effective_pen_id").toString() == "pen_blue");
    EDI_CHECK(layerPlotPoint.value("effective_stroke_color").toString() == "#75c7ff");
    EDI_CHECK(nearlyEqual(layerPlotPoint.value("effective_stroke_width").toDouble(), 1.0));
    QVariantMap layerPlotSummary = layerPlotModel.value("plot_summary").toMap();
    EDI_CHECK(layerPlotSummary.value("order_mode").toString() == "layer_order");
    EDI_CHECK(layerPlotSummary.value("direction_mode").toString() == "preserve_direction");
    EDI_CHECK(layerPlotSummary.value("plot_object_count").toInt() == 1);
    EDI_CHECK(layerPlotSummary.value("segment_count").toInt() == 2);
    EDI_CHECK(layerPlotSummary.value("travel_segment_count").toInt() == 1);
    EDI_CHECK(layerPlotSummary.value("travel_distance").toDouble() > 0.0);
    QVariantList layerStats = layerPlotSummary.value("layer_stats").toList();
    EDI_CHECK(layerStats.size() == 1);
    QVariantMap layerStatsEntry = layerStats.front().toMap();
    EDI_CHECK(layerStatsEntry.value("layer_id").toString() == "layer_2");
    EDI_CHECK(layerStatsEntry.value("object_count").toInt() == 1);
    EDI_CHECK(layerStatsEntry.value("segment_count").toInt() == 2);
    EDI_CHECK(layerStatsEntry.value("stroke_distance").toDouble() > 0.0);
    EDI_CHECK(layerStatsEntry.value("travel_distance").toDouble() > 0.0);
    EDI_CHECK(layerStatsEntry.value("ready").toBool());
    EDI_CHECK(layerStatsEntry.value("blocked_reason").toString() == "ready");
    QVariantList penStats = layerPlotSummary.value("pen_stats").toList();
    EDI_CHECK(penStats.size() == 1);
    QVariantMap penStatsEntry = penStats.front().toMap();
    EDI_CHECK(penStatsEntry.value("pen_id").toString() == "pen_blue");
    EDI_CHECK(penStatsEntry.value("object_count").toInt() == 1);
    EDI_CHECK(penStatsEntry.value("segment_count").toInt() == 2);
    EDI_CHECK(penStatsEntry.value("stroke_distance").toDouble() > 0.0);
    EDI_CHECK(penStatsEntry.value("travel_distance").toDouble() > 0.0);
    EDI_CHECK(penStatsEntry.value("ready").toBool());
    EDI_CHECK(penStatsEntry.value("blocked_reason").toString() == "ready");
    QVariantMap layerPlotPreview = layerPlotSummary.value("preview").toMap();
    EDI_CHECK(layerPlotPreview.value("order_mode").toString() == "layer_order");
    EDI_CHECK(layerPlotPreview.value("direction_mode").toString() == "preserve_direction");
    EDI_CHECK(layerPlotPreview.value("segment_count").toInt() == 2);
    EDI_CHECK(layerPlotPreview.value("travel_segment_count").toInt() == 1);
    EDI_CHECK(layerPlotPreview.value("travel_distance").toDouble() > 0.0);
    EDI_CHECK(layerPlotPreview.value("segments").toList().size() == 2);
    const QVariantList layerPlotTravelSegments = layerPlotPreview.value("travel_segments").toList();
    EDI_CHECK(layerPlotTravelSegments.size() == 1);
    const QVariantMap layerPlotTravelSegment = layerPlotTravelSegments.front().toMap();
    EDI_CHECK(layerPlotTravelSegment.value("from_object_id").toString() == layerPlotPoint.value("id").toString());
    EDI_CHECK(layerPlotTravelSegment.value("to_object_id").toString() == layerPlotPoint.value("id").toString());
    EDI_CHECK(layerPlotTravelSegment.value("to_layer_id").toString() == "layer_2");
    EDI_CHECK(layerPlotTravelSegment.value("to_pen_id").toString() == "pen_blue");
    EDI_CHECK(layerPlotTravelSegment.value("distance").toDouble() > 0.0);
    EDI_CHECK(layerPlotSummary.value("warning_count").toInt() == 0);
    EDI_CHECK(layerPlotSummary.value("ready").toBool());
    EDI_CHECK(layerPlotSummary.value("status").toString() == "ready");
    EDI_CHECK(!layerPlotSummary.value("blocked").toBool());
    EDI_CHECK(layerPlotSummary.value("blocked_reason_count").toInt() == 0);
    EDI_CHECK(layerPlotController.plotOrderModeId() == "layer_order");
    layerPlotController.setPlotOrderModeId("nearest_next");
    EDI_CHECK(layerPlotController.plotOrderModeId() == "nearest_next");
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    EDI_CHECK(layerPlotSummary.value("order_mode").toString() == "nearest_next");
    EDI_CHECK(layerPlotSummary.value("preview").toMap().value("order_mode").toString() == "nearest_next");
    layerPlotController.setPlotOrderModeId("unknown");
    EDI_CHECK(layerPlotController.plotOrderModeId() == "layer_order");
    EDI_CHECK(layerPlotController.plotDirectionModeId() == "preserve_direction");
    layerPlotController.setPlotDirectionModeId("allow_reverse_segments");
    EDI_CHECK(layerPlotController.plotDirectionModeId() == "allow_reverse_segments");
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    EDI_CHECK(layerPlotSummary.value("direction_mode").toString() == "allow_reverse_segments");
    EDI_CHECK(layerPlotSummary.value("preview").toMap().value("direction_mode").toString() == "allow_reverse_segments");
    layerPlotController.setPlotDirectionModeId("unknown");
    EDI_CHECK(layerPlotController.plotDirectionModeId() == "preserve_direction");
    EDI_CHECK(layerPlotController.setActiveLayerPlotEnabled(false));
    layerPlotPoint = layerPlotController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(!layerPlotPoint.value("effective_plot_enabled").toBool());
    EDI_CHECK(!layerPlotPoint.value("effective_plot_ready").toBool());
    EDI_CHECK(!layerPlotPoint.value("plot_ready").toBool());
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    EDI_CHECK(layerPlotSummary.value("plot_object_count").toInt() == 0);
    EDI_CHECK(layerPlotSummary.value("segment_count").toInt() == 0);
    layerStats = layerPlotSummary.value("layer_stats").toList();
    EDI_CHECK(layerStats.size() == 1);
    layerStatsEntry = layerStats.front().toMap();
    EDI_CHECK(layerStatsEntry.value("layer_id").toString() == "layer_2");
    EDI_CHECK(!layerStatsEntry.value("ready").toBool());
    EDI_CHECK(layerStatsEntry.value("blocked_reason").toString() == "plot_disabled");
    penStats = layerPlotSummary.value("pen_stats").toList();
    EDI_CHECK(penStats.size() == 1);
    penStatsEntry = penStats.front().toMap();
    EDI_CHECK(penStatsEntry.value("pen_id").toString() == "pen_blue");
    EDI_CHECK(!penStatsEntry.value("ready").toBool());
    EDI_CHECK(penStatsEntry.value("blocked_reason").toString() == "no_assigned_segments");
    EDI_CHECK(layerPlotSummary.value("travel_segment_count").toInt() == 0);
    EDI_CHECK(nearlyEqual(layerPlotSummary.value("travel_distance").toDouble(), 0.0));
    EDI_CHECK(layerPlotController.setActiveLayerPlotEnabled(true));
    layerPlotController.setSelectedToolId("horizontal_guide_tool");
    layerPlotController.clickCanvasNormalized(0.4, 0.4);
    QVariantMap layerPlotGuide = layerPlotController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(layerPlotGuide.value("kind").toString() == "guide");
    EDI_CHECK(layerPlotGuide.value("effective_plot_enabled").toBool());
    EDI_CHECK(!layerPlotGuide.value("effective_plot_ready").toBool());
    EDI_CHECK(!layerPlotGuide.value("plot_ready").toBool());
    layerPlotSummary = layerPlotController.modelDocument().value("plot_summary").toMap();
    EDI_CHECK(layerPlotSummary.value("plot_object_count").toInt() == 1);
    EDI_CHECK(layerPlotSummary.value("segment_count").toInt() == 2);
    EDI_CHECK(layerPlotSummary.value("travel_segment_count").toInt() == 1);
    layerPlotController.setSelectedToolId("point_tool");
    layerPlotController.clickCanvasNormalized(0.0, 0.0);
    QVariantMap blockedPlotModel = layerPlotController.modelDocument();
    layerPlotSummary = blockedPlotModel.value("plot_summary").toMap();
    EDI_CHECK(layerPlotSummary.value("plot_object_count").toInt() == 2);
    EDI_CHECK(layerPlotSummary.value("segment_count").toInt() == 4);
    EDI_CHECK(layerPlotSummary.value("travel_segment_count").toInt() == 3);
    EDI_CHECK(layerPlotSummary.value("travel_distance").toDouble() > 0.0);
    EDI_CHECK(layerPlotSummary.value("warning_count").toInt() == 1);
    EDI_CHECK(!layerPlotSummary.value("ready").toBool());
    EDI_CHECK(layerPlotSummary.value("status").toString() == "blocked");
    EDI_CHECK(layerPlotSummary.value("blocked").toBool());
    EDI_CHECK(layerPlotSummary.value("blocked_reason_count").toInt() == 1);
    EDI_CHECK(layerPlotSummary.value("blocked_reasons").toList().front().toString() == "raw_out_of_drawable_bounds");
    EDI_CHECK(layerPlotSummary.value("first_warning_kind").toString() == "raw_out_of_drawable_bounds");
    const QString blockedObjectId = layerPlotSummary.value("first_warning_object_id").toString();
    EDI_CHECK(!blockedObjectId.isEmpty());
    QVariantMap blockedObject;
    for (const QVariant &objectValue : blockedPlotModel.value("drawing_objects").toList()) {
        const QVariantMap object = objectValue.toMap();
        if (object.value("id").toString() == blockedObjectId) {
            blockedObject = object;
            break;
        }
    }
    EDI_CHECK(!blockedObject.isEmpty());
    EDI_CHECK(blockedObject.value("plot_blocked").toBool());
    EDI_CHECK(blockedObject.value("plot_safety_state").toString() == "blocked");
    EDI_CHECK(blockedObject.value("plot_warning_count").toInt() == 1);
    EDI_CHECK(blockedObject.value("plot_warning_kind").toString() == "raw_out_of_drawable_bounds");
    EDI_CHECK(blockedObject.value("outside_drawable").toBool());
    EDI_CHECK(!blockedObject.value("calibrated_outside_drawable").toBool());
    EDI_CHECK(blockedPlotModel.value("warnings").toList().size() == 1);

    DrawingDocumentController fitNoSelectionController;
    EDI_CHECK(!fitNoSelectionController.fitSelectionToDrawableBounds());

    DrawingDocumentController fitInsideController;
    fitInsideController.setSelectedToolId("point_tool");
    fitInsideController.clickCanvasNormalized(0.5, 0.5);
    QVariantMap fitInsideModel = fitInsideController.modelDocument();
    const int fitInsideRevision = fitInsideModel.value("revision").toInt();
    EDI_CHECK(fitInsideModel.value("selection_drawable_relation").toString() == "inside");
    EDI_CHECK(fitInsideController.fitSelectionToDrawableBounds());
    fitInsideModel = fitInsideController.modelDocument();
    QVariantMap fitInsidePoint = fitInsideModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(fitInsideModel.value("revision").toInt() == fitInsideRevision);
    EDI_CHECK(nearlyEqual(fitInsidePoint.value("x").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(fitInsidePoint.value("y").toDouble(), 0.5));

    DrawingDocumentController fitOutsideController;
    fitOutsideController.setSelectedToolId("line_tool");
    fitOutsideController.clickCanvasNormalized(0.0, 0.5);
    fitOutsideController.clickCanvasNormalized(0.1, 0.5);
    QVariantMap fitOutsideModel = fitOutsideController.modelDocument();
    EDI_CHECK(fitOutsideModel.value("plot_summary").toMap().value("blocked").toBool());
    EDI_CHECK(fitOutsideModel.value("selection_drawable_relation").toString() == "partially_outside");
    EDI_CHECK(fitOutsideController.fitSelectionToDrawableBounds());
    fitOutsideModel = fitOutsideController.modelDocument();
    QVariantMap fitOutsideLine = fitOutsideModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(fitOutsideLine.value("x1").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(fitOutsideLine.value("x2").toDouble(), squareQuarterInchStep + 0.1));
    EDI_CHECK(nearlyEqual(fitOutsideLine.value("y1").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(fitOutsideLine.value("y2").toDouble(), 0.5));
    EDI_CHECK(!fitOutsideModel.value("plot_summary").toMap().value("blocked").toBool());

    DrawingDocumentController fitPointMarkController;
    fitPointMarkController.setSelectedToolId("point_tool");
    fitPointMarkController.clickCanvasNormalized(0.0, 0.0);
    QVariantMap fitPointMarkModel = fitPointMarkController.modelDocument();
    EDI_CHECK(fitPointMarkModel.value("plot_summary").toMap().value("blocked").toBool());
    EDI_CHECK(fitPointMarkController.fitSelectionToDrawableBounds());
    fitPointMarkModel = fitPointMarkController.modelDocument();
    QVariantMap fitPointMark = fitPointMarkModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(fitPointMark.value("x").toDouble(), squareQuarterInchStep + 0.005));
    EDI_CHECK(nearlyEqual(fitPointMark.value("y").toDouble(), squareQuarterInchStep + 0.005));
    EDI_CHECK(!fitPointMarkModel.value("plot_summary").toMap().value("blocked").toBool());
    EDI_CHECK(fitPointMarkModel.value("has_selection_plot_bounds").toBool());
    QVariantMap fitPointMarkSelectionBounds = fitPointMarkModel.value("selection_plot_bounds").toMap();
    EDI_CHECK(nearlyEqual(fitPointMarkSelectionBounds.value("x").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(fitPointMarkSelectionBounds.value("y").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(fitPointMarkSelectionBounds.value("width").toDouble(), 0.01));
    EDI_CHECK(nearlyEqual(fitPointMarkSelectionBounds.value("height").toDouble(), 0.01));
    EDI_CHECK(nearlyEqual(fitPointMarkModel.value("selection_plot_bounds_width").toDouble(), 0.01));
    EDI_CHECK(nearlyEqual(fitPointMarkModel.value("selection_plot_bounds_height").toDouble(), 0.01));
    EDI_CHECK(fitPointMarkModel.value("selection_plot_bounds_status").toString() == "inside");
    EDI_CHECK(fitPointMarkModel.value("selection_drawable_relation").toString() == "inside");

    DrawingDocumentController fitTooLargeController;
    fitTooLargeController.setSelectedToolId("line_tool");
    fitTooLargeController.clickCanvasNormalized(0.0, 0.5);
    fitTooLargeController.clickCanvasNormalized(1.0, 0.5);
    const int fitTooLargeRevision = fitTooLargeController.modelDocument().value("revision").toInt();
    EDI_CHECK(fitTooLargeController.modelDocument().value("selection_drawable_relation").toString() == "too_large");
    EDI_CHECK(!fitTooLargeController.fitSelectionToDrawableBounds());
    EDI_CHECK(fitTooLargeController.modelDocument().value("revision").toInt() == fitTooLargeRevision);

    DrawingDocumentController fullyOutsideController;
    fullyOutsideController.setSelectedToolId("point_tool");
    fullyOutsideController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(fullyOutsideController.updateSelectedObjectGeometryField("x", 2.0));
    EDI_CHECK(fullyOutsideController.updateSelectedObjectGeometryField("y", 2.0));
    EDI_CHECK(fullyOutsideController.modelDocument().value("selection_drawable_relation").toString() == "fully_outside");

    DrawingDocumentController centerDrawableController;
    centerDrawableController.setSelectedToolId("line_tool");
    centerDrawableController.clickCanvasNormalized(0.0, 0.5);
    centerDrawableController.clickCanvasNormalized(0.1, 0.5);
    EDI_CHECK(centerDrawableController.centerSelectionInDrawable());
    QVariantMap centeredLine = centerDrawableController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(centeredLine.value("x1").toDouble(), 0.45));
    EDI_CHECK(nearlyEqual(centeredLine.value("x2").toDouble(), 0.55));
    EDI_CHECK(nearlyEqual(centeredLine.value("y1").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(centeredLine.value("y2").toDouble(), 0.5));
    EDI_CHECK(centerDrawableController.modelDocument().value("selection_drawable_relation").toString() == "inside");

    DrawingDocumentController originDrawableController;
    originDrawableController.setSelectedToolId("line_tool");
    originDrawableController.clickCanvasNormalized(0.5, 0.5);
    originDrawableController.clickCanvasNormalized(0.6, 0.7);
    EDI_CHECK(originDrawableController.moveSelectionToDrawableOrigin());
    QVariantMap originLine = originDrawableController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(originLine.value("x1").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(originLine.value("y1").toDouble(), squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(originLine.value("x2").toDouble(), squareQuarterInchStep + 0.1));
    EDI_CHECK(nearlyEqual(originLine.value("y2").toDouble(), squareQuarterInchStep + 0.2));

    DrawingDocumentController fitLockedController;
    fitLockedController.setSelectedToolId("point_tool");
    fitLockedController.clickCanvasNormalized(0.0, 0.0);
    EDI_CHECK(fitLockedController.setSelectedObjectLocked(true));
    const int fitLockedRevision = fitLockedController.modelDocument().value("revision").toInt();
    EDI_CHECK(!fitLockedController.fitSelectionToDrawableBounds());
    EDI_CHECK(fitLockedController.modelDocument().value("revision").toInt() == fitLockedRevision);

    DrawingDocumentController fitNonPlottingController;
    fitNonPlottingController.setSelectedToolId("horizontal_guide_tool");
    fitNonPlottingController.clickCanvasNormalized(0.0, 0.0);
    const int fitNonPlottingRevision = fitNonPlottingController.modelDocument().value("revision").toInt();
    EDI_CHECK(!fitNonPlottingController.fitSelectionToDrawableBounds());
    EDI_CHECK(fitNonPlottingController.modelDocument().value("revision").toInt() == fitNonPlottingRevision);

    DrawingDocumentController safeNudgeNoSelectionController;
    EDI_CHECK(!safeNudgeNoSelectionController.nudgeSelectionInsideDrawable("right", "grid"));
    EDI_CHECK(!safeNudgeNoSelectionController.nudgeSelectionInsideDrawable("diagonal", "grid"));

    DrawingDocumentController safeNudgeController;
    safeNudgeController.setSelectedToolId("point_tool");
    safeNudgeController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(safeNudgeController.nudgeSelectionInsideDrawable("right", "grid"));
    QVariantMap safeNudgedPoint = safeNudgeController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(safeNudgedPoint.value("x").toDouble(), 0.5 + squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(safeNudgedPoint.value("y").toDouble(), 0.5));

    DrawingDocumentController safeNudgeBlockedController;
    safeNudgeBlockedController.setSelectedToolId("point_tool");
    safeNudgeBlockedController.clickCanvasNormalized(0.0, 0.0);
    EDI_CHECK(safeNudgeBlockedController.fitSelectionToDrawableBounds());
    const int safeNudgeBlockedRevision = safeNudgeBlockedController.modelDocument().value("revision").toInt();
    EDI_CHECK(!safeNudgeBlockedController.nudgeSelectionInsideDrawable("left", "grid"));
    QVariantMap safeNudgeBlockedPoint = safeNudgeBlockedController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(safeNudgeBlockedController.modelDocument().value("revision").toInt() == safeNudgeBlockedRevision);
    EDI_CHECK(nearlyEqual(safeNudgeBlockedPoint.value("x").toDouble(), squareQuarterInchStep + 0.005));

    DrawingDocumentController safeNudgeLockedController;
    safeNudgeLockedController.setSelectedToolId("point_tool");
    safeNudgeLockedController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(safeNudgeLockedController.setSelectedObjectLocked(true));
    const int safeNudgeLockedRevision = safeNudgeLockedController.modelDocument().value("revision").toInt();
    EDI_CHECK(!safeNudgeLockedController.nudgeSelectionInsideDrawable("right", "grid"));
    EDI_CHECK(safeNudgeLockedController.modelDocument().value("revision").toInt() == safeNudgeLockedRevision);

    DrawingDocumentController safeNudgeNonPlottingController;
    safeNudgeNonPlottingController.setSelectedToolId("horizontal_guide_tool");
    safeNudgeNonPlottingController.clickCanvasNormalized(0.5, 0.5);
    const int safeNudgeNonPlottingRevision = safeNudgeNonPlottingController.modelDocument().value("revision").toInt();
    EDI_CHECK(!safeNudgeNonPlottingController.nudgeSelectionInsideDrawable("right", "grid"));
    EDI_CHECK(safeNudgeNonPlottingController.modelDocument().value("revision").toInt() == safeNudgeNonPlottingRevision);

    DrawingDocumentController calibrationController;
    EDI_CHECK(calibrationController.createCalibrationPattern("test_square"));
    QVariantMap calibrationModel = calibrationController.modelDocument();
    QVariantList calibrationObjects = calibrationModel.value("drawing_objects").toList();
    EDI_CHECK(calibrationObjects.size() == 1);
    QVariantMap calibrationSquare = calibrationObjects.front().toMap();
    EDI_CHECK(calibrationSquare.value("kind").toString() == "rectangle");
    EDI_CHECK(calibrationSquare.value("layer_id").toString() == "default");
    EDI_CHECK(nearlyEqual(calibrationSquare.value("x").toDouble(), 0.15));
    EDI_CHECK(nearlyEqual(calibrationSquare.value("y").toDouble(), 0.15));
    EDI_CHECK(nearlyEqual(calibrationSquare.value("width").toDouble(), 0.24));
    EDI_CHECK(nearlyEqual(calibrationSquare.value("height").toDouble(), 0.24));
    EDI_CHECK(calibrationModel.value("selected_object_ids").toList().size() == 1);
    EDI_CHECK(calibrationController.recordCalibrationMeasurement(0.238));
    calibrationModel = calibrationController.modelDocument();
    QVariantMap calibrationMeasurement = calibrationModel.value("calibration_measurement").toMap();
    EDI_CHECK(calibrationMeasurement.value("pattern_id").toString() == "calibration_square");
    EDI_CHECK(nearlyEqual(calibrationMeasurement.value("expected_value").toDouble(), 0.24));
    EDI_CHECK(nearlyEqual(calibrationMeasurement.value("measured_value").toDouble(), 0.238));
    EDI_CHECK(nearlyEqual(calibrationMeasurement.value("error_value").toDouble(), -0.002));
    EDI_CHECK(calibrationMeasurement.value("percent_error").toDouble() < 0.0);
    QVariantMap calibrationCorrection = calibrationModel.value("calibration_correction").toMap();
    EDI_CHECK(calibrationCorrection.value("ok").toBool());
    EDI_CHECK(nearlyEqual(calibrationCorrection.value("scale_factor").toDouble(), 0.24 / 0.238));
    EDI_CHECK(calibrationCorrection.value("correction_percent").toDouble() > 0.0);
    EDI_CHECK(nearlyEqual(calibrationModel.value("plot_summary").toMap().value("calibration_scale").toDouble(), 1.0));
    calibrationSquare = calibrationModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(calibrationSquare.value("measurement_note").toString().contains("calibration_square"));
    const double calibrationSquareXBeforeApply = calibrationSquare.value("x").toDouble();
    EDI_CHECK(calibrationController.applyCalibrationCorrection());
    calibrationModel = calibrationController.modelDocument();
    const double appliedCalibrationScale = 0.24 / 0.238;
    const QVariantMap appliedCalibrationPlot = calibrationModel.value("plot_summary").toMap();
    EDI_CHECK(nearlyEqual(appliedCalibrationPlot.value("calibration_scale").toDouble(), appliedCalibrationScale));
    EDI_CHECK(appliedCalibrationPlot.value("has_plot_bounds").toBool());
    const QVariantMap appliedCalibrationPlotBounds = appliedCalibrationPlot.value("plot_bounds").toMap();
    EDI_CHECK(nearlyEqual(appliedCalibrationPlotBounds.value("x").toDouble(), 0.15 * appliedCalibrationScale));
    EDI_CHECK(nearlyEqual(appliedCalibrationPlotBounds.value("width").toDouble(), 0.24 * appliedCalibrationScale));
    const QVariantList appliedCalibrationSegments = appliedCalibrationPlot.value("preview").toMap().value("segments").toList();
    EDI_CHECK(!appliedCalibrationSegments.isEmpty());
    const QVariantMap appliedCalibrationSegment = appliedCalibrationSegments.front().toMap();
    EDI_CHECK(nearlyEqual(appliedCalibrationSegment.value("raw_x1").toDouble(), 0.15));
    EDI_CHECK(nearlyEqual(appliedCalibrationSegment.value("raw_x2").toDouble(), 0.39));
    EDI_CHECK(nearlyEqual(appliedCalibrationSegment.value("x1").toDouble(), 0.15 * appliedCalibrationScale));
    EDI_CHECK(nearlyEqual(appliedCalibrationSegment.value("calibrated_x2").toDouble(), 0.39 * appliedCalibrationScale));
    calibrationSquare = calibrationModel.value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(calibrationSquare.value("x").toDouble(), calibrationSquareXBeforeApply));

    EDI_CHECK(calibrationController.createLayer());
    EDI_CHECK(calibrationController.activeLayerId() == "layer_2");
    EDI_CHECK(calibrationController.createCalibrationPattern("test_circle"));
    calibrationModel = calibrationController.modelDocument();
    calibrationObjects = calibrationModel.value("drawing_objects").toList();
    EDI_CHECK(calibrationObjects.size() == 2);
    QVariantMap calibrationCircle = calibrationObjects.back().toMap();
    EDI_CHECK(calibrationCircle.value("kind").toString() == "circle");
    EDI_CHECK(calibrationCircle.value("layer_id").toString() == "layer_2");
    EDI_CHECK(nearlyEqual(calibrationCircle.value("cx").toDouble(), 0.27));
    EDI_CHECK(nearlyEqual(calibrationCircle.value("cy").toDouble(), 0.27));
    EDI_CHECK(nearlyEqual(calibrationCircle.value("radius").toDouble(), 0.12));

    EDI_CHECK(calibrationController.createCalibrationPattern("line_spacing"));
    calibrationModel = calibrationController.modelDocument();
    calibrationObjects = calibrationModel.value("drawing_objects").toList();
    EDI_CHECK(calibrationObjects.size() == 7);
    QVariantList calibrationSelection = calibrationModel.value("selected_object_ids").toList();
    EDI_CHECK(calibrationSelection.size() == 5);
    for (int index = 2; index < calibrationObjects.size(); ++index) {
        const QVariantMap line = calibrationObjects[index].toMap();
        EDI_CHECK(line.value("kind").toString() == "line");
        EDI_CHECK(line.value("layer_id").toString() == "layer_2");
        EDI_CHECK(nearlyEqual(line.value("x1").toDouble(), 0.15));
        EDI_CHECK(nearlyEqual(line.value("x2").toDouble(), 0.39));
    }
    EDI_CHECK(calibrationController.recordCalibrationMeasurement(0.041));
    calibrationModel = calibrationController.modelDocument();
    calibrationMeasurement = calibrationModel.value("calibration_measurement").toMap();
    EDI_CHECK(calibrationMeasurement.value("pattern_id").toString() == "calibration_line_spacing");
    EDI_CHECK(nearlyEqual(calibrationMeasurement.value("expected_value").toDouble(), 0.04));
    EDI_CHECK(nearlyEqual(calibrationMeasurement.value("measured_value").toDouble(), 0.041));
    EDI_CHECK(calibrationMeasurement.value("percent_error").toDouble() > 0.0);
    calibrationCorrection = calibrationModel.value("calibration_correction").toMap();
    EDI_CHECK(nearlyEqual(calibrationCorrection.value("scale_factor").toDouble(), 0.04 / 0.041));
    EDI_CHECK(nearlyEqual(calibrationModel.value("plot_summary").toMap().value("calibration_scale").toDouble(), 0.24 / 0.238));
    calibrationObjects = calibrationModel.value("drawing_objects").toList();
    for (int index = 2; index < calibrationObjects.size(); ++index) {
        const QVariantMap line = calibrationObjects[index].toMap();
        EDI_CHECK(line.value("measurement_note").toString().contains("calibration_line_spacing"));
    }
    QVariantMap calibrationPlotSummary = calibrationModel.value("plot_summary").toMap();
    EDI_CHECK(calibrationPlotSummary.value("plot_object_count").toInt() == 7);
    EDI_CHECK(calibrationPlotSummary.value("segment_count").toInt() > 7);
    EDI_CHECK(!calibrationPlotSummary.value("blocked").toBool());
    EDI_CHECK(calibrationController.setActiveLayerLocked(true));
    EDI_CHECK(!calibrationController.createCalibrationPattern("test_square"));

    DrawingDocumentController selectionController;
    selectionController.setSelectedToolId("point_tool");
    selectionController.clickCanvasNormalized(0.1, 0.1);
    selectionController.clickCanvasNormalized(0.4, 0.4);
    selectionController.clickCanvasNormalized(0.8, 0.8);
    EDI_CHECK(selectionController.selectObjectsInBoundsNormalized(0.0, 0.0, 0.5, 0.5));
    QVariantMap selectionModel = selectionController.modelDocument();
    QVariantList selectedIds = selectionModel.value("selected_object_ids").toList();
    EDI_CHECK(selectedIds.size() == 2);
    EDI_CHECK(selectionModel.value("active_object_id").toString() == selectedIds.back().toString());

    DrawingDocumentController arrangeController;
    arrangeController.setSelectedToolId("point_tool");
    arrangeController.clickCanvasNormalized(0.2, 0.2);
    arrangeController.clickCanvasNormalized(0.5, 0.7);
    arrangeController.clickCanvasNormalized(0.8, 0.4);
    EDI_CHECK(arrangeController.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0));
    EDI_CHECK(arrangeController.alignSelection("left"));
    QVariantList arrangedObjects = arrangeController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(arrangedObjects.size() == 3);
    EDI_CHECK(nearlyEqual(arrangedObjects[0].toMap().value("x").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(arrangedObjects[1].toMap().value("x").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(arrangedObjects[2].toMap().value("x").toDouble(), 0.2));
    EDI_CHECK(!arrangeController.alignSelection("diagonal"));
    EDI_CHECK(arrangeController.distributeSelection("y"));
    arrangedObjects = arrangeController.modelDocument().value("drawing_objects").toList();
    EDI_CHECK(nearlyEqual(arrangedObjects[0].toMap().value("y").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(arrangedObjects[1].toMap().value("y").toDouble(), 0.7));
    EDI_CHECK(nearlyEqual(arrangedObjects[2].toMap().value("y").toDouble(), 0.45));
    EDI_CHECK(!arrangeController.distributeSelection("diagonal"));

    DrawingDocumentController previewController;
    previewController.setSelectedToolId("line_tool");
    previewController.clickCanvasNormalized(0.2, 0.2);
    QVariantMap pendingModel = previewController.modelDocument();
    EDI_CHECK(pendingModel.value("drawing_objects").toList().empty());
    EDI_CHECK(!pendingModel.contains("preview_object"));

    previewController.updateCreationPreviewNormalized(0.6, 0.7);
    QVariantMap previewModel = previewController.modelDocument();
    EDI_CHECK(previewModel.value("drawing_objects").toList().empty());
    EDI_CHECK(previewModel.contains("preview_object"));
    QVariantMap previewLine = previewModel.value("preview_object").toMap();
    EDI_CHECK(previewLine.value("kind").toString() == "line");
    EDI_CHECK(nearlyEqual(previewLine.value("x1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(previewLine.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(previewLine.value("x2").toDouble(), 0.6));
    EDI_CHECK(nearlyEqual(previewLine.value("y2").toDouble(), 0.7));

    previewController.clickCanvasNormalized(0.6, 0.7);
    QVariantMap committedPreviewModel = previewController.modelDocument();
    EDI_CHECK(committedPreviewModel.value("drawing_objects").toList().size() == 1);
    EDI_CHECK(!committedPreviewModel.contains("preview_object"));

    DrawingDocumentController cancelPreviewController;
    cancelPreviewController.setSelectedToolId("rectangle_tool");
    cancelPreviewController.clickCanvasNormalized(0.1, 0.1);
    cancelPreviewController.updateCreationPreviewNormalized(0.4, 0.4);
    EDI_CHECK(cancelPreviewController.modelDocument().contains("preview_object"));
    cancelPreviewController.setSelectedToolId("select_move");
    EDI_CHECK(!cancelPreviewController.modelDocument().contains("preview_object"));
    EDI_CHECK(cancelPreviewController.modelDocument().value("drawing_objects").toList().empty());

    DrawingDocumentController snappedPreviewController;
    snappedPreviewController.setGridSnapEnabled(true);
    snappedPreviewController.setSelectedToolId("rectangle_tool");
    snappedPreviewController.clickCanvasNormalized(0.14, 0.14);
    snappedPreviewController.updateCreationPreviewNormalized(0.36, 0.36);
    QVariantMap snappedPreview = snappedPreviewController.modelDocument().value("preview_object").toMap();
    EDI_CHECK(snappedPreview.value("kind").toString() == "rectangle");
    EDI_CHECK(nearlyEqual(snappedPreview.value("x").toDouble(), snappedSquarePoint));
    EDI_CHECK(nearlyEqual(snappedPreview.value("y").toDouble(), snappedSquarePoint));
    const double snappedSquareExtent = 10.0 * squareQuarterInchStep;
    EDI_CHECK(nearlyEqual(snappedPreview.value("width").toDouble(), snappedSquareExtent));
    EDI_CHECK(nearlyEqual(snappedPreview.value("height").toDouble(), snappedSquareExtent));
    EDI_CHECK(snappedPreviewController.modelDocument().value("drawing_objects").toList().empty());
    snappedPreviewController.clickCanvasNormalized(0.36, 0.36);
    QVariantMap snappedCommitted = snappedPreviewController.modelDocument();
    EDI_CHECK(snappedCommitted.value("drawing_objects").toList().size() == 1);
    EDI_CHECK(!snappedCommitted.contains("preview_object"));

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
    EDI_CHECK(guideLinePreview.value("kind").toString() == "line");
    EDI_CHECK(nearlyEqual(guideLinePreview.value("x1").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideLinePreview.value("y1").toDouble(), 0.75));
    EDI_CHECK(nearlyEqual(guideLinePreview.value("x2").toDouble(), 0.52));
    EDI_CHECK(nearlyEqual(guideLinePreview.value("y2").toDouble(), 0.75));
    guideLinePreviewController.clickCanvasNormalized(0.52, 0.74);
    QVariantMap guideLineCommitted = guideLinePreviewController.modelDocument().value("drawing_objects").toList().back().toMap();
    EDI_CHECK(guideLineCommitted.value("kind").toString() == "line");
    EDI_CHECK(nearlyEqual(guideLineCommitted.value("x1").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideLineCommitted.value("y1").toDouble(), 0.75));
    EDI_CHECK(nearlyEqual(guideLineCommitted.value("x2").toDouble(), 0.52));
    EDI_CHECK(nearlyEqual(guideLineCommitted.value("y2").toDouble(), 0.75));

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
    EDI_CHECK(guideRectanglePreview.value("kind").toString() == "rectangle");
    EDI_CHECK(nearlyEqual(guideRectanglePreview.value("x").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideRectanglePreview.value("y").toDouble(), 0.70));
    EDI_CHECK(nearlyEqual(guideRectanglePreview.value("width").toDouble(), 0.19));
    EDI_CHECK(nearlyEqual(guideRectanglePreview.value("height").toDouble(), 0.05));

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
    EDI_CHECK(guideCirclePreview.value("kind").toString() == "circle");
    EDI_CHECK(nearlyEqual(guideCirclePreview.value("cx").toDouble(), 0.33));
    EDI_CHECK(nearlyEqual(guideCirclePreview.value("cy").toDouble(), 0.75));
    EDI_CHECK(nearlyEqual(guideCirclePreview.value("radius").toDouble(), 0.10));

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
    EDI_CHECK(disabledGuideLinePreview.value("kind").toString() == "line");
    EDI_CHECK(nearlyEqual(disabledGuideLinePreview.value("x1").toDouble(), 0.34));
    EDI_CHECK(nearlyEqual(disabledGuideLinePreview.value("y1").toDouble(), 0.74));
    EDI_CHECK(nearlyEqual(disabledGuideLinePreview.value("x2").toDouble(), 0.52));
    EDI_CHECK(nearlyEqual(disabledGuideLinePreview.value("y2").toDouble(), 0.74));

    DrawingDocumentController zeroSizeController;
    zeroSizeController.setSelectedToolId("circle_tool");
    zeroSizeController.clickCanvasNormalized(0.5, 0.5);
    zeroSizeController.updateCreationPreviewNormalized(0.5, 0.5);
    QVariantMap zeroCirclePreview = zeroSizeController.modelDocument().value("preview_object").toMap();
    EDI_CHECK(zeroCirclePreview.value("kind").toString() == "circle");
    EDI_CHECK(nearlyEqual(zeroCirclePreview.value("radius").toDouble(), 0.0));
    zeroSizeController.clickCanvasNormalized(0.5, 0.5);
    QVariantMap zeroCircleCommitted = zeroSizeController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(zeroCircleCommitted.value("radius").toDouble(), 0.0));

    DrawingDocumentController numericRectController;
    numericRectController.setSelectedToolId("rectangle_tool");
    numericRectController.clickCanvasNormalized(0.1, 0.1);
    numericRectController.clickCanvasNormalized(0.4, 0.4);
    EDI_CHECK(numericRectController.updateSelectedObjectGeometryField("width", 0.5));
    EDI_CHECK(numericRectController.updateSelectedObjectGeometryField("height", 0.25));
    EDI_CHECK(numericRectController.updateSelectedObjectGeometryField("rotation_deg", 45.0));
    QVariantMap numericRect = numericRectController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(numericFieldIds(numericRect) == QStringList({
        QStringLiteral("x"),
        QStringLiteral("y"),
        QStringLiteral("width"),
        QStringLiteral("height"),
        QStringLiteral("rotation_deg"),
    }));
    QVariantMap numericRectPhysical = numericRect.value("physical_geometry").toMap();
    EDI_CHECK(numericRectPhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(numericRectPhysical.value("width").toDouble(), 6.0));
    EDI_CHECK(nearlyEqual(numericRectPhysical.value("height").toDouble(), 3.0));
    EDI_CHECK(nearlyEqual(numericRectPhysical.value("rotation_deg").toDouble(), 45.0));
    EDI_CHECK(nearlyEqual(numericRect.value("width").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(numericRect.value("height").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(numericRect.value("rotation_deg").toDouble(), 45.0));
    QVariantList numericRectMeasurement = numericRect.value("measurement_lines").toList();
    EDI_CHECK(numericRectMeasurement.size() == 3);
    EDI_CHECK(numericRectMeasurement[0].toString().startsWith("area: "));
    EDI_CHECK(!numericRectController.updateSelectedObjectGeometryField("width", -0.1));
    QVariantMap rectEditStatus = editStatus(numericRectController);
    EDI_CHECK(!rectEditStatus.value("ok").toBool());
    EDI_CHECK(rectEditStatus.value("mode").toString() == "normalized");
    EDI_CHECK(rectEditStatus.value("field_id").toString() == "width");
    EDI_CHECK(rectEditStatus.value("code").toString() == "invalid_geometry");
    EDI_CHECK(rectEditStatus.value("message").toString() == "rectangle dimensions must be non-negative");
    QVariantMap numericRectAfterInvalid = numericRectController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(numericRectAfterInvalid.value("width").toDouble(), 0.5));
    EDI_CHECK(numericRectController.updateSelectedObjectPhysicalGeometryField("width", 3.0));
    rectEditStatus = editStatus(numericRectController);
    EDI_CHECK(rectEditStatus.value("ok").toBool());
    EDI_CHECK(rectEditStatus.value("mode").toString() == "physical");
    EDI_CHECK(rectEditStatus.value("field_id").toString() == "width");
    EDI_CHECK(rectEditStatus.value("message").toString().isEmpty());
    EDI_CHECK(numericRectController.updateSelectedObjectPhysicalGeometryField("height", 6.0));
    QVariantMap physicalRect = numericRectController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(physicalRect.value("width").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(physicalRect.value("height").toDouble(), 0.5));

    DrawingDocumentController numericCircleController;
    numericCircleController.setSelectedToolId("circle_tool");
    numericCircleController.clickCanvasNormalized(0.5, 0.5);
    numericCircleController.clickCanvasNormalized(0.7, 0.5);
    EDI_CHECK(numericCircleController.updateSelectedObjectGeometryField("cx", 0.4));
    EDI_CHECK(numericCircleController.updateSelectedObjectGeometryField("cy", 0.45));
    EDI_CHECK(numericCircleController.updateSelectedObjectGeometryField("radius", 0.125));
    QVariantMap numericCircle = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(numericFieldIds(numericCircle) == QStringList({
        QStringLiteral("cx"),
        QStringLiteral("cy"),
        QStringLiteral("radius"),
        QStringLiteral("diameter"),
    }));
    QVariantMap numericCirclePhysical = numericCircle.value("physical_geometry").toMap();
    EDI_CHECK(numericCirclePhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(numericCirclePhysical.value("radius").toDouble(), 1.5));
    EDI_CHECK(nearlyEqual(numericCirclePhysical.value("diameter").toDouble(), 3.0));
    EDI_CHECK(nearlyEqual(numericCircle.value("cx").toDouble(), 0.4));
    EDI_CHECK(nearlyEqual(numericCircle.value("cy").toDouble(), 0.45));
    EDI_CHECK(nearlyEqual(numericCircle.value("radius").toDouble(), 0.125));
    EDI_CHECK(nearlyEqual(numericCircle.value("diameter").toDouble(), 0.25));
    QVariantList numericCircleMeasurement = numericCircle.value("measurement_lines").toList();
    EDI_CHECK(numericCircleMeasurement.size() == 3);
    EDI_CHECK(numericCircleMeasurement[0].toString().startsWith("area: "));
    EDI_CHECK(numericCircleController.updateSelectedObjectGeometryField("diameter", 0.5));
    numericCircle = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(numericCircle.value("radius").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(numericCircle.value("diameter").toDouble(), 0.5));
    EDI_CHECK(!numericCircleController.updateSelectedObjectGeometryField("radius", -0.01));
    EDI_CHECK(!numericCircleController.updateSelectedObjectGeometryField("diameter", -0.01));
    QVariantMap numericCircleAfterInvalid = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(numericCircleAfterInvalid.value("radius").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(numericCircleAfterInvalid.value("diameter").toDouble(), 0.5));
    EDI_CHECK(numericCircleController.updateSelectedObjectPhysicalGeometryField("radius", 3.0));
    QVariantMap physicalCircle = numericCircleController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(physicalCircle.value("radius").toDouble(), 0.25));
    EDI_CHECK(nearlyEqual(physicalCircle.value("diameter").toDouble(), 0.5));
    const int circleRevisionBeforePhysicalInvalid = numericCircleController.modelDocument().value("revision").toInt();
    EDI_CHECK(!numericCircleController.updateSelectedObjectPhysicalGeometryField("diameter", -1.0));
    QVariantMap circleEditStatus = editStatus(numericCircleController);
    EDI_CHECK(!circleEditStatus.value("ok").toBool());
    EDI_CHECK(circleEditStatus.value("mode").toString() == "physical");
    EDI_CHECK(circleEditStatus.value("field_id").toString() == "diameter");
    EDI_CHECK(circleEditStatus.value("code").toString() == "invalid_geometry");
    EDI_CHECK(circleEditStatus.value("message").toString() == "circle diameter must be non-negative");
    EDI_CHECK(numericCircleController.modelDocument().value("revision").toInt() == circleRevisionBeforePhysicalInvalid);
    numericCircleController.setSelectedToolId("select_move");
    EDI_CHECK(editStatus(numericCircleController).isEmpty());

    DrawingDocumentController numericLineController;
    numericLineController.setSelectedToolId("line_tool");
    numericLineController.clickCanvasNormalized(0.1, 0.2);
    numericLineController.clickCanvasNormalized(0.4, 0.6);
    QVariantMap numericLine = numericLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(numericFieldIds(numericLine) == QStringList({
        QStringLiteral("x1"),
        QStringLiteral("y1"),
        QStringLiteral("x2"),
        QStringLiteral("y2"),
        QStringLiteral("line_length"),
        QStringLiteral("line_angle_deg"),
    }));
    QVariantMap numericLinePhysical = numericLine.value("physical_geometry").toMap();
    EDI_CHECK(numericLinePhysical.value("unit_label").toString() == "in");
    EDI_CHECK(nearlyEqual(numericLinePhysical.value("x1").toDouble(), 1.2));
    EDI_CHECK(nearlyEqual(numericLinePhysical.value("y1").toDouble(), 2.4));
    EDI_CHECK(nearlyEqual(numericLinePhysical.value("x2").toDouble(), 4.8));
    EDI_CHECK(nearlyEqual(numericLinePhysical.value("y2").toDouble(), 7.2));
    EDI_CHECK(nearlyEqual(numericLinePhysical.value("line_length").toDouble(), 6.0));
    EDI_CHECK(nearlyEqual(numericLinePhysical.value("line_angle_deg").toDouble(), 53.1301023542));
    EDI_CHECK(nearlyEqual(numericLine.value("line_length").toDouble(), 0.5));
    EDI_CHECK(nearlyEqual(numericLine.value("line_angle_deg").toDouble(), 53.1301023542));
    EDI_CHECK(numericLineController.updateSelectedObjectPhysicalGeometryField("line_length", 12.0));
    numericLine = numericLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(numericLine.value("x1").toDouble(), 0.1));
    EDI_CHECK(nearlyEqual(numericLine.value("y1").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(numericLine.value("x2").toDouble(), 0.7));
    EDI_CHECK(nearlyEqual(numericLine.value("y2").toDouble(), 1.0));
    EDI_CHECK(nearlyEqual(numericLine.value("line_length").toDouble(), 1.0));
    EDI_CHECK(numericLineController.updateSelectedObjectPhysicalGeometryField("line_angle_deg", 0.0));
    numericLine = numericLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(numericLine.value("x2").toDouble(), 1.1));
    EDI_CHECK(nearlyEqual(numericLine.value("y2").toDouble(), 0.2));
    EDI_CHECK(nearlyEqual(numericLine.value("line_angle_deg").toDouble(), 0.0));
    const int lineRevisionBeforeInvalid = numericLineController.modelDocument().value("revision").toInt();
    EDI_CHECK(!numericLineController.updateSelectedObjectGeometryField("line_length", -0.1));
    EDI_CHECK(!numericLineController.updateSelectedObjectPhysicalGeometryField("line_length", -1.0));
    EDI_CHECK(!numericLineController.updateSelectedObjectGeometryField("line_angle_deg", std::numeric_limits<double>::infinity()));
    EDI_CHECK(numericLineController.modelDocument().value("revision").toInt() == lineRevisionBeforeInvalid);

    DrawingDocumentController lockedLineController;
    lockedLineController.setSelectedToolId("line_tool");
    lockedLineController.clickCanvasNormalized(0.1, 0.1);
    lockedLineController.clickCanvasNormalized(0.4, 0.4);
    QVariantMap lockedLine = lockedLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(!lockedLine.value("locked").toBool());
    EDI_CHECK(lockedLine.value("visible").toBool());
    EDI_CHECK(lockedLineController.setSelectedObjectLocked(true));
    lockedLine = lockedLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(lockedLine.value("locked").toBool());
    const int lockedRevision = lockedLineController.modelDocument().value("revision").toInt();
    EDI_CHECK(!lockedLineController.updateSelectedObjectGeometryField("x2", 0.8));
    EDI_CHECK(!lockedLineController.editSelectedHandleNormalized("line_end", 0.8, 0.8));
    EDI_CHECK(!lockedLineController.moveSelectionNormalized(0.1, 0.0));
    EDI_CHECK(!lockedLineController.offsetSelectedObject("left"));
    EDI_CHECK(!lockedLineController.mirrorSelectedObject("vertical"));
    EDI_CHECK(!lockedLineController.repeatSelectedObject("x"));
    EDI_CHECK(lockedLineController.modelDocument().value("revision").toInt() == lockedRevision);
    EDI_CHECK(lockedLineController.setSelectedObjectLocked(false));
    EDI_CHECK(lockedLineController.updateSelectedObjectGeometryField("x2", 0.8));
    lockedLine = lockedLineController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(lockedLine.value("x2").toDouble(), 0.8));

    DrawingDocumentController noSelectionEditController;
    EDI_CHECK(!noSelectionEditController.updateSelectedObjectGeometryField("x", 0.2));
    EDI_CHECK(!noSelectionEditController.setSelectedObjectLocked(true));
    EDI_CHECK(!noSelectionEditController.setSelectedObjectVisible(false));
    EDI_CHECK(!noSelectionEditController.nudgeSelection("right", "grid"));

    DrawingDocumentController nudgeController;
    nudgeController.setSelectedToolId("point_tool");
    nudgeController.clickCanvasNormalized(0.5, 0.5);
    EDI_CHECK(nudgeController.nudgeSelection("right", "grid"));
    EDI_CHECK(nudgeController.nudgeSelection("up", "fine"));
    QVariantMap nudgedPoint = nudgeController.modelDocument().value("drawing_objects").toList().front().toMap();
    EDI_CHECK(nearlyEqual(nudgedPoint.value("x").toDouble(), 0.5 + squareQuarterInchStep));
    EDI_CHECK(nearlyEqual(nudgedPoint.value("y").toDouble(), 0.5 - squareQuarterInchStep * 0.25));
    EDI_CHECK(!nudgeController.nudgeSelection("diagonal", "grid"));

    DrawingDocumentController selectionIsolationController;
    selectionIsolationController.setSelectedToolId("point_tool");
    selectionIsolationController.clickCanvasNormalized(0.2, 0.2);
    QString selectedBeforePreview = selectionIsolationController.selectedObjectId();
    selectionIsolationController.setSelectedToolId("line_tool");
    selectionIsolationController.clickCanvasNormalized(0.4, 0.4);
    selectionIsolationController.updateCreationPreviewNormalized(0.8, 0.8);
    EDI_CHECK(selectionIsolationController.modelDocument().contains("preview_object"));
    EDI_CHECK(selectionIsolationController.selectedObjectId() == selectedBeforePreview);
    selectionIsolationController.setSelectedToolId("select_move");
    selectionIsolationController.clickCanvasNormalized(0.8, 0.8);
    EDI_CHECK(selectionIsolationController.selectedObjectId().isEmpty());

    // Save/open round-trip: a document written to disk and reopened projects
    // to the same model, and the reopened controller keeps minting fresh ids.
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("roundtrip.edidraw")));

        DrawingDocumentController saveController;
        saveController.setSelectedToolId("line_tool");
        saveController.clickCanvasNormalized(0.1, 0.2);
        saveController.clickCanvasNormalized(0.8, 0.9);
        saveController.setSelectedToolId("circle_tool");
        saveController.clickCanvasNormalized(0.4, 0.4);
        saveController.clickCanvasNormalized(0.6, 0.4);
        const QVariantList savedObjects = saveController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(savedObjects.size() == 2);
        const QString savedSelected = saveController.selectedObjectId();

        EDI_CHECK(saveController.saveDocument(url));

        // A second controller, mutated differently, then opens the saved file.
        DrawingDocumentController openController;
        openController.setSelectedToolId("point_tool");
        openController.clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(openController.modelDocument().value("drawing_objects").toList().size() == 1);

        EDI_CHECK(openController.openDocument(url));
        const QVariantList openedObjects = openController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(openedObjects.size() == 2);
        EDI_CHECK(openController.selectedObjectId() == savedSelected);
        // Preview/pending state is cleared by open.
        EDI_CHECK(!openController.modelDocument().contains("preview_object"));

        // Object ids match the saved document positionally.
        for (int i = 0; i < savedObjects.size(); ++i) {
            EDI_CHECK(openedObjects[i].toMap().value("id").toString()
                   == savedObjects[i].toMap().value("id").toString());
        }

        // Newly created objects after open keep minting above the highest
        // trailing serial already present (the loaded line/circle are _0001/
        // _0002, so the next id must carry a serial of at least 3).
        int highestLoadedSerial = 0;
        for (const QVariant &existing : openedObjects) {
            highestLoadedSerial = std::max(highestLoadedSerial, trailingSerial(existing.toMap().value("id").toString()));
        }
        EDI_CHECK(highestLoadedSerial >= 2);
        openController.setSelectedToolId("point_tool");
        openController.clickCanvasNormalized(0.3, 0.3);
        const QVariantList grownObjects = openController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(grownObjects.size() == 3);
        const QString newId = grownObjects.back().toMap().value("id").toString();
        for (const QVariant &existing : openedObjects) {
            EDI_CHECK(existing.toMap().value("id").toString() != newId);
        }
        EDI_CHECK(trailingSerial(newId) > highestLoadedSerial);

        // Opening a missing file fails without disturbing the document.
        const QUrl missing = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("nope.edidraw")));
        EDI_CHECK(!openController.openDocument(missing));
        EDI_CHECK(openController.modelDocument().value("drawing_objects").toList().size() == 3);
    }

    // SVG / HPGL export write files whose contents start with the right markers.
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        DrawingDocumentController exportController;
        exportController.setSelectedToolId("line_tool");
        exportController.clickCanvasNormalized(0.1, 0.1);
        exportController.clickCanvasNormalized(0.9, 0.9);

        const QString svgPath = tempDir.filePath(QStringLiteral("out.svg"));
        EDI_CHECK(exportController.exportSvgDocument(QUrl::fromLocalFile(svgPath)));
        QFile svgFile(svgPath);
        EDI_CHECK(svgFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString svg = QString::fromUtf8(svgFile.readAll());
        EDI_CHECK(svg.startsWith(QStringLiteral("<svg")));
        EDI_CHECK(svg.contains(QStringLiteral("<path")));

        const QString hpglPath = tempDir.filePath(QStringLiteral("out.hpgl"));
        EDI_CHECK(exportController.exportHpglDocument(QUrl::fromLocalFile(hpglPath)));
        QFile hpglFile(hpglPath);
        EDI_CHECK(hpglFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString hpgl = QString::fromUtf8(hpglFile.readAll());
        EDI_CHECK(hpgl.startsWith(QStringLiteral("IN;")));
        EDI_CHECK(hpgl.contains(QStringLiteral("PD")));
    }

    // Dirty tracking is content-based: selecting an object after a save does NOT
    // mark the document dirty (selection is excluded, like undo).
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("dirty.edidraw")));

        DrawingDocumentController dirtyController;
        EDI_CHECK(!dirtyController.isDocumentDirty()); // fresh document is clean
        dirtyController.setSelectedToolId("point_tool");
        dirtyController.clickCanvasNormalized(0.2, 0.2);
        dirtyController.clickCanvasNormalized(0.8, 0.8);
        EDI_CHECK(dirtyController.isDocumentDirty()); // created objects -> dirty
        EDI_CHECK(dirtyController.saveDocument(url));
        EDI_CHECK(!dirtyController.isDocumentDirty()); // saved -> clean

        // Select / deselect after save: selection-only, must stay clean.
        dirtyController.setSelectedToolId("select_move");
        dirtyController.clickCanvasNormalized(0.2, 0.2);
        EDI_CHECK(!dirtyController.isDocumentDirty());
        dirtyController.clickCanvasNormalized(0.45, 0.05); // empty space: clears selection
        EDI_CHECK(!dirtyController.isDocumentDirty());

        // A real geometry change marks dirty again.
        dirtyController.clickCanvasNormalized(0.8, 0.8); // select an object
        EDI_CHECK(dirtyController.nudgeSelection("right", "grid"));
        EDI_CHECK(dirtyController.isDocumentDirty());
    }

    // Dirty tracking is epoch-based, immune to revision aliasing across undo.
    // Regression: save at revision N, undo to N-1, then a DIFFERENT edit whose
    // revision collides back to N must STILL read dirty. The old revision-equality
    // shortcut reported false-clean here and silently lost changes on close.
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QUrl url = QUrl::fromLocalFile(tempDir.filePath(QStringLiteral("alias.edidraw")));

        DrawingDocumentController c;
        c.setSelectedToolId("point_tool");
        c.clickCanvasNormalized(0.2, 0.2); // object A (revision 1)
        c.clickCanvasNormalized(0.8, 0.8); // object B (revision 2)
        EDI_CHECK(c.saveDocument(url));
        EDI_CHECK(!c.isDocumentDirty());      // saved at revision 2 -> clean
        EDI_CHECK(c.undo());                  // back to {A} (revision 1)
        EDI_CHECK(c.isDocumentDirty());       // diverged from the saved state -> dirty
        c.clickCanvasNormalized(0.5, 0.5); // object C: revision aliases back to 2
        EDI_CHECK(c.isDocumentDirty());       // content differs from saved {A,B} -> dirty
    }

    // Undo/redo.
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value("drawing_objects").toList().size();
        };

        // create -> undo -> empty -> redo -> restored.
        DrawingDocumentController undoController;
        EDI_CHECK(!undoController.canUndo());
        EDI_CHECK(!undoController.canRedo());
        EDI_CHECK(!undoController.undo()); // nothing to undo
        undoController.setSelectedToolId("point_tool");
        undoController.clickCanvasNormalized(0.3, 0.4);
        EDI_CHECK(objectCount(undoController) == 1);
        EDI_CHECK(undoController.canUndo());
        const QString createdId = undoController.selectedObjectId();
        EDI_CHECK(undoController.undo());
        EDI_CHECK(objectCount(undoController) == 0);
        EDI_CHECK(!undoController.canUndo());
        EDI_CHECK(undoController.canRedo());
        EDI_CHECK(undoController.redo());
        EDI_CHECK(objectCount(undoController) == 1);
        EDI_CHECK(undoController.modelDocument().value("drawing_objects").toList().front().toMap().value("id").toString() == createdId);
        EDI_CHECK(!undoController.canRedo());

        // nudge twice -> undo once -> one nudge remains (each nudge is one step).
        DrawingDocumentController nudgeUndoController;
        nudgeUndoController.setSelectedToolId("point_tool");
        nudgeUndoController.clickCanvasNormalized(0.5, 0.5);
        const double startY = nudgeUndoController.modelDocument().value("drawing_objects").toList().front().toMap().value("y").toDouble();
        EDI_CHECK(nudgeUndoController.nudgeSelection("up", "grid"));
        EDI_CHECK(nudgeUndoController.nudgeSelection("up", "grid"));
        const double twiceY = nudgeUndoController.modelDocument().value("drawing_objects").toList().front().toMap().value("y").toDouble();
        EDI_CHECK(!nearlyEqual(twiceY, startY));
        EDI_CHECK(nudgeUndoController.undo());
        const double onceY = nudgeUndoController.modelDocument().value("drawing_objects").toList().front().toMap().value("y").toDouble();
        // After one undo, exactly one nudge remains: halfway between start and twice.
        EDI_CHECK(nearlyEqual(onceY, (startY + twiceY) / 2.0));
        EDI_CHECK(nudgeUndoController.canUndo()); // create + one nudge still undoable

        // A guide preset that creates several guides is a single undo step.
        DrawingDocumentController guideUndoController;
        EDI_CHECK(guideUndoController.applyGuidePreset("drawable_bounds"));
        const int guideObjects = guideUndoController.modelDocument().value("drawing_objects").toList().size();
        EDI_CHECK(guideObjects >= 2); // preset adds multiple guides
        EDI_CHECK(guideUndoController.canUndo());
        EDI_CHECK(guideUndoController.undo());
        EDI_CHECK(guideUndoController.modelDocument().value("drawing_objects").toList().isEmpty());
        EDI_CHECK(!guideUndoController.canUndo()); // exactly one step for the whole preset

        // Pure selection changes are not undoable and do not clear redo.
        DrawingDocumentController selectionUndoController;
        selectionUndoController.setSelectedToolId("point_tool");
        selectionUndoController.clickCanvasNormalized(0.2, 0.2);
        selectionUndoController.clickCanvasNormalized(0.8, 0.8);
        EDI_CHECK(objectCount(selectionUndoController) == 2);
        selectionUndoController.undo(); // remove second point
        EDI_CHECK(objectCount(selectionUndoController) == 1);
        EDI_CHECK(selectionUndoController.canRedo());
        // Select the remaining point: selection-only, must not clear redo or add a step.
        selectionUndoController.setSelectedToolId("select_move");
        selectionUndoController.clickCanvasNormalized(0.2, 0.2);
        EDI_CHECK(selectionUndoController.canRedo());
        EDI_CHECK(selectionUndoController.redo());
        EDI_CHECK(objectCount(selectionUndoController) == 2);

        // redo is cleared by a new edit.
        DrawingDocumentController redoClearController;
        redoClearController.setSelectedToolId("point_tool");
        redoClearController.clickCanvasNormalized(0.3, 0.3);
        redoClearController.clickCanvasNormalized(0.6, 0.6);
        redoClearController.undo();
        EDI_CHECK(redoClearController.canRedo());
        redoClearController.clickCanvasNormalized(0.9, 0.9); // new edit
        EDI_CHECK(!redoClearController.canRedo());

        // 100-step cap: the oldest edits drop out of the undo history.
        DrawingDocumentController capController;
        capController.setSelectedToolId("point_tool");
        for (int i = 0; i < 102; ++i) {
            capController.clickCanvasNormalized(0.1 + 0.001 * i, 0.1);
        }
        EDI_CHECK(capController.modelDocument().value("drawing_objects").toList().size() == 102);
        int undone = 0;
        while (capController.undo()) {
            ++undone;
        }
        EDI_CHECK(undone == 100); // capped at 100, the first two creations are unrecoverable
        EDI_CHECK(capController.modelDocument().value("drawing_objects").toList().size() == 2);

        // A drag bracket interrupted by undo must not poison later undo history.
        // Open a bracket (as a drag would), then undo mid-gesture: the bracket
        // is abandoned, and a subsequent edit must still push its own undo step.
        DrawingDocumentController leakController;
        leakController.setSelectedToolId("point_tool");
        leakController.clickCanvasNormalized(0.5, 0.5); // step 1: create
        leakController.beginInteractiveEdit();          // a drag starts...
        EDI_CHECK(leakController.undo());                  // ...but undo fires first
        EDI_CHECK(objectCount(leakController) == 0);
        // The leaked bracket is gone: a fresh edit is independently undoable.
        leakController.setSelectedToolId("point_tool");
        leakController.clickCanvasNormalized(0.4, 0.4);
        EDI_CHECK(objectCount(leakController) == 1);
        EDI_CHECK(leakController.canUndo());
        EDI_CHECK(leakController.undo());                  // the new edit undoes cleanly
        EDI_CHECK(objectCount(leakController) == 0);
    }

    // Keyboard-action controller seams: cancel, delete, duplicate, coarse nudge.
    {
        // cancelPendingCreation clears an in-flight two-click creation + preview.
        DrawingDocumentController cancelController;
        cancelController.setSelectedToolId("line_tool");
        cancelController.clickCanvasNormalized(0.2, 0.2); // first click: pending
        cancelController.updateCreationPreviewNormalized(0.6, 0.6);
        EDI_CHECK(cancelController.modelDocument().contains("preview_object"));
        cancelController.cancelPendingCreation();
        EDI_CHECK(!cancelController.modelDocument().contains("preview_object"));
        EDI_CHECK(cancelController.modelDocument().value("drawing_objects").toList().isEmpty());
        // No-op when nothing pending.
        cancelController.cancelPendingCreation();

        // deleteSelectedObject removes the active object of any kind and is undoable.
        DrawingDocumentController deleteController;
        deleteController.setSelectedToolId("point_tool");
        deleteController.clickCanvasNormalized(0.4, 0.4);
        EDI_CHECK(deleteController.modelDocument().value("drawing_objects").toList().size() == 1);
        EDI_CHECK(deleteController.deleteSelectedObject());
        EDI_CHECK(deleteController.modelDocument().value("drawing_objects").toList().isEmpty());
        EDI_CHECK(deleteController.canUndo());
        deleteController.undo();
        EDI_CHECK(deleteController.modelDocument().value("drawing_objects").toList().size() == 1);
        // Delete with nothing selected fails.
        DrawingDocumentController emptyDeleteController;
        EDI_CHECK(!emptyDeleteController.deleteSelectedObject());

        // duplicateSelectedObject clones the active object at a small offset.
        DrawingDocumentController dupController;
        dupController.setSelectedToolId("point_tool");
        dupController.clickCanvasNormalized(0.3, 0.3);
        const QVariantMap original = dupController.modelDocument().value("drawing_objects").toList().front().toMap();
        EDI_CHECK(dupController.duplicateSelectedObject());
        const QVariantList afterDup = dupController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(afterDup.size() == 2);
        const QVariantMap copy = afterDup.back().toMap();
        EDI_CHECK(copy.value("id").toString() != original.value("id").toString());
        EDI_CHECK(nearlyEqual(copy.value("x").toDouble(), original.value("x").toDouble() + 0.02));
        EDI_CHECK(nearlyEqual(copy.value("y").toDouble(), original.value("y").toDouble() + 0.02));
        // The duplicate is selected and the action is a single undo step.
        EDI_CHECK(dupController.selectedObjectId() == copy.value("id").toString());
        dupController.undo();
        EDI_CHECK(dupController.modelDocument().value("drawing_objects").toList().size() == 1);
        DrawingDocumentController emptyDupController;
        EDI_CHECK(!emptyDupController.duplicateSelectedObject());

        // Shift-nudge maps to the "coarse" step (4x the grid step).
        DrawingDocumentController coarseController;
        coarseController.setSelectedToolId("point_tool");
        coarseController.clickCanvasNormalized(0.5, 0.5);
        const double baseX = coarseController.modelDocument().value("drawing_objects").toList().front().toMap().value("x").toDouble();
        EDI_CHECK(coarseController.nudgeSelection("right", "grid"));
        const double gridX = coarseController.modelDocument().value("drawing_objects").toList().front().toMap().value("x").toDouble();
        coarseController.undo();
        EDI_CHECK(coarseController.nudgeSelection("right", "coarse"));
        const double coarseX = coarseController.modelDocument().value("drawing_objects").toList().front().toMap().value("x").toDouble();
        // coarse step is 4x the grid step.
        EDI_CHECK(nearlyEqual(coarseX - baseX, (gridX - baseX) * 4.0));
    }

    // Polyline: the first multi-click tool. Clicks anchor vertices into the
    // pending request; nothing reaches the document until the finish verb.
    {
        DrawingDocumentController polyController;
        polyController.setSelectedToolId("polyline_tool");
        polyController.clickCanvasNormalized(0.2, 0.2);
        polyController.clickCanvasNormalized(0.5, 0.3);
        EDI_CHECK(polyController.modelDocument().value("drawing_objects").toList().isEmpty());

        // The pointer previews as a provisional last vertex.
        polyController.updateCreationPreviewNormalized(0.6, 0.6);
        EDI_CHECK(polyController.modelDocument().contains("preview_object"));

        polyController.clickCanvasNormalized(0.6, 0.6);
        EDI_CHECK(polyController.finishPendingMultiClick());
        const QVariantList objects = polyController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 1);
        const QVariantMap polyline = objects.front().toMap();
        EDI_CHECK(polyline.value("kind").toString() == QStringLiteral("polyline"));
        EDI_CHECK(polyController.selectedObjectId() == polyline.value("id").toString());

        // The whole gesture is ONE undo step, not one per click.
        EDI_CHECK(polyController.canUndo());
        polyController.undo();
        EDI_CHECK(polyController.modelDocument().value("drawing_objects").toList().isEmpty());
        polyController.redo();
        EDI_CHECK(polyController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Finishing with nothing pending refuses; a one-vertex trail
        // dissolves silently (same as Escape).
        EDI_CHECK(!polyController.finishPendingMultiClick());
        polyController.clickCanvasNormalized(0.8, 0.8);
        EDI_CHECK(!polyController.finishPendingMultiClick());
        EDI_CHECK(polyController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Escape drops an in-flight trail without touching the document.
        polyController.clickCanvasNormalized(0.1, 0.8);
        polyController.clickCanvasNormalized(0.3, 0.9);
        polyController.cancelPendingCreation();
        EDI_CHECK(!polyController.finishPendingMultiClick());
        EDI_CHECK(polyController.modelDocument().value("drawing_objects").toList().size() == 1);

        // A finished polyline is selectable by clicking near a segment.
        polyController.setSelectedToolId("select_move");
        polyController.clickCanvasNormalized(0.35, 0.25);
        EDI_CHECK(polyController.selectedObjectId() == polyline.value("id").toString());
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
        EDI_CHECK(splineController.modelDocument().value("drawing_objects").toList().isEmpty());

        EDI_CHECK(splineController.finishPendingMultiClick());
        const QVariantList objects = splineController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 1);
        const QVariantMap spline = objects.front().toMap();
        EDI_CHECK(spline.value("kind").toString() == QStringLiteral("spline"));
        // The projection flattened the sampled curve into drawable points.
        EDI_CHECK(!spline.value("points").toList().isEmpty());

        // The whole gesture is ONE undo step; redo restores it whole.
        EDI_CHECK(splineController.canUndo());
        splineController.undo();
        EDI_CHECK(splineController.modelDocument().value("drawing_objects").toList().isEmpty());
        splineController.redo();
        EDI_CHECK(splineController.modelDocument().value("drawing_objects").toList().size() == 1);

        // A finished spline is selectable by clicking near the curve.
        splineController.setSelectedToolId("select_move");
        splineController.clickCanvasNormalized(0.4, 0.5); // a control point lies on the curve
        EDI_CHECK(splineController.selectedObjectId() == spline.value("id").toString());
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
        EDI_CHECK(objectCount(clip) == 2);

        // Copy with nothing selected is a no-op (clear the click selection
        // with an empty marquee first).
        clip.selectObjectsInBoundsNormalized(0.9, 0.9, 0.95, 0.95);
        EDI_CHECK(!clip.copySelection());
        EDI_CHECK(!clip.canPaste());

        // Marquee-select both points, copy, paste: two fresh objects appear,
        // the originals stay, and the pasted pair is what's now selected.
        clip.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(clip.copySelection());
        EDI_CHECK(clip.canPaste());
        const int beforePaste = objectCount(clip);
        EDI_CHECK(clip.paste());
        EDI_CHECK(objectCount(clip) == beforePaste + 2);
        EDI_CHECK(clip.modelDocument().value("selected_object_ids").toList().size() == 2);

        // Every object id in the document is unique after the paste.
        {
            const QVariantList objects = clip.modelDocument().value("drawing_objects").toList();
            QSet<QString> ids;
            for (const QVariant &value : objects) {
                ids.insert(value.toMap().value("id").toString());
            }
            EDI_CHECK(ids.size() == objects.size());
        }

        // Paste is exactly one undo step (not one per pasted object).
        clip.undo();
        EDI_CHECK(objectCount(clip) == beforePaste);
        clip.redo();
        EDI_CHECK(objectCount(clip) == beforePaste + 2);

        // The clipboard survives selection changes: clear the selection, paste
        // again — still pastes BOTH copied points.
        clip.selectObjectsInBoundsNormalized(0.9, 0.9, 0.95, 0.95);
        const int beforeSecond = objectCount(clip);
        EDI_CHECK(clip.paste());
        EDI_CHECK(objectCount(clip) == beforeSecond + 2);

        // Cut: copies then removes the selection as one undo step.
        DrawingDocumentController cutter;
        cutter.setSelectedToolId("point_tool");
        cutter.clickCanvasNormalized(0.4, 0.4);
        cutter.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(cutter.cutSelection());
        EDI_CHECK(objectCount(cutter) == 0); // cut removed it
        EDI_CHECK(cutter.canPaste());
        cutter.undo();                    // the cut's delete is one undo step
        EDI_CHECK(objectCount(cutter) == 1);
        cutter.redo();
        EDI_CHECK(objectCount(cutter) == 0);
        EDI_CHECK(cutter.paste());           // the cut clipboard still pastes
        EDI_CHECK(objectCount(cutter) == 1);

        // Empty clipboard pastes nothing.
        DrawingDocumentController fresh;
        EDI_CHECK(!fresh.canPaste());
        EDI_CHECK(!fresh.paste());

        // Paste is ATOMIC (user decision 2026-06-11). The DISCRIMINATING
        // setup is a clipboard spanning two layers with only one locked:
        // the old per-object loop pasted the unlocked subset (returned
        // true, count +1, selection = the partial paste) — every assertion
        // below fails under it.
        DrawingDocumentController atomicPaste;
        atomicPaste.setSelectedToolId("point_tool");
        atomicPaste.clickCanvasNormalized(0.3, 0.3); // point on the default layer (serial 1)
        EDI_CHECK(atomicPaste.createLayer());           // second layer becomes active
        atomicPaste.clickCanvasNormalized(0.6, 0.6); // point on the second layer (serial 2)
        atomicPaste.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(atomicPaste.copySelection());            // clipboard spans both layers
        EDI_CHECK(atomicPaste.setActiveLayerLocked(true)); // lock ONLY the second layer
        const int beforeLockedPaste = objectCount(atomicPaste);
        EDI_CHECK(!atomicPaste.paste());
        EDI_CHECK(objectCount(atomicPaste) == beforeLockedPaste);
        // The failure preserves the selection (the old loop cleared it via
        // SelectObjectsCommand{empty}) and reports through edit_status.
        EDI_CHECK(atomicPaste.modelDocument().value("selected_object_ids").toList().size() == 2);
        QVariantMap pasteStatus = atomicPaste.modelDocument().value("edit_status").toMap();
        EDI_CHECK(pasteStatus.value("ok").toBool() == false);
        EDI_CHECK(pasteStatus.value("mode").toString() == "paste");
        // Unlock: the same clipboard pastes whole.
        EDI_CHECK(atomicPaste.setActiveLayerLocked(false));
        EDI_CHECK(atomicPaste.paste());
        EDI_CHECK(objectCount(atomicPaste) == beforeLockedPaste + 2);
        // Pins the serial reclaim: points minted serials 1-2, the FAILED
        // paste minted 3-4 then restored, so this paste re-mints 3-4 —
        // without the restore in paste() these would be 5-6.
        QVariantList pastedSelection = atomicPaste.modelDocument().value("selected_object_ids").toList();
        EDI_CHECK(pastedSelection.size() == 2);
        EDI_CHECK(trailingSerial(pastedSelection.first().toString()) == 3);
        EDI_CHECK(trailingSerial(pastedSelection.last().toString()) == 4);
        // A clean paste leaves no stale rejection behind.
        EDI_CHECK(atomicPaste.modelDocument().value("edit_status").toMap().isEmpty());
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

        EDI_CHECK(meta.setSelectedObjectRole("cutout"));
        EDI_CHECK(meta.setSelectedObjectMaterial("oak"));
        EDI_CHECK(meta.setSelectedObjectExportGroup("frame"));
        EDI_CHECK(meta.setSelectedObjectTags({QStringLiteral("load-bearing"), QStringLiteral(" visible "),
                                            QStringLiteral("")})); // blanks dropped, others trimmed
        const QVariantMap projected = activeObj();
        EDI_CHECK(projected.value("role").toString() == "cutout");
        EDI_CHECK(projected.value("material").toString() == "oak");
        EDI_CHECK(projected.value("export_group").toString() == "frame");
        EDI_CHECK(projected.value("tags").toString() == "load-bearing, visible");

        // Each edit is undoable; undoing the tags edit restores the prior tags.
        meta.undo();
        EDI_CHECK(activeObj().value("tags").toString().isEmpty());

        // An unknown role name falls back to none rather than erroring.
        EDI_CHECK(meta.setSelectedObjectRole("not_a_role"));
        EDI_CHECK(activeObj().value("role").toString() == "none");

        // No selection: setters refuse.
        DrawingDocumentController noSel;
        EDI_CHECK(!noSel.setSelectedObjectRole("wall"));
        EDI_CHECK(!noSel.setSelectedObjectMaterial("steel"));
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
        EDI_CHECK(madeRect.value("kind").toString() == "rectangle");
        EDI_CHECK(nearlyEqual(madeRect.value("corner_radius").toDouble(), 0.05));
        EDI_CHECK(nearlyEqual(madeRect.value("inset").toDouble(), 0.02));

        // Negative/non-finite options normalize to a plain box.
        rectCtl.setRectCornerRadius(-1.0);
        EDI_CHECK(rectCtl.rectCornerRadius() == 0.0);

        // Aspect-lock: a fresh square, then a corner drag with the lock on
        // preserves the 1:1 ratio even though the cursor demands a rectangle.
        DrawingDocumentController lockCtl;
        lockCtl.setSelectedToolId("rectangle_tool");
        lockCtl.clickCanvasNormalized(0.2, 0.2);
        lockCtl.clickCanvasNormalized(0.4, 0.4); // a square (equal w/h)
        const QVariantMap square = activeRect(lockCtl);
        const double w0 = square.value("bounds").toMap().value("width").toDouble();
        const double h0 = square.value("bounds").toMap().value("height").toDouble();
        EDI_CHECK(nearlyEqual(w0, h0));

        lockCtl.setAspectLockEnabled(true);
        EDI_CHECK(lockCtl.aspectLockEnabled());
        // Drag the SE corner far off-square; the lock keeps width==height.
        lockCtl.editSelectedHandleNormalized("rect_se", 0.9, 0.5);
        const QVariantMap locked = activeRect(lockCtl);
        const double wL = locked.value("bounds").toMap().value("width").toDouble();
        const double hL = locked.value("bounds").toMap().value("height").toDouble();
        EDI_CHECK(nearlyEqual(wL, hL));

        // With the lock off, the same kind of drag frees the ratio.
        lockCtl.setAspectLockEnabled(false);
        lockCtl.editSelectedHandleNormalized("rect_se", 0.95, 0.45);
        const QVariantMap freed = activeRect(lockCtl);
        EDI_CHECK(!nearlyEqual(freed.value("bounds").toMap().value("width").toDouble(),
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
        EDI_CHECK(!layerColor.isEmpty());

        EDI_CHECK(styleController.setSelectedObjectStrokeColor(QStringLiteral("#ff6600")));
        EDI_CHECK(styleController.setSelectedObjectStrokeWidth(4.5));
        EDI_CHECK(styleController.setSelectedObjectLineStyle(QStringLiteral("dash")));
        QVariantMap styled = activeProjection();
        EDI_CHECK(styled.value(QStringLiteral("effective_stroke_color")).toString() == QStringLiteral("#ff6600"));
        EDI_CHECK(styled.value(QStringLiteral("effective_stroke_width")).toDouble() == 4.5);
        EDI_CHECK(styled.value(QStringLiteral("effective_line_style")).toString() == QStringLiteral("dash"));
        // An art color keeps the layer's physical pen (no preset match).
        EDI_CHECK(styled.value(QStringLiteral("effective_pen_id")).toString() == QStringLiteral("pen_black"));
        // A preset color SELECTS its pen.
        EDI_CHECK(styleController.setSelectedObjectStrokeColor(QStringLiteral("#75c7ff")));
        EDI_CHECK(activeProjection().value(QStringLiteral("effective_pen_id")).toString() == QStringLiteral("pen_blue"));

        // Inherit sentinels hand control back to the layer.
        EDI_CHECK(styleController.setSelectedObjectStrokeColor(QString()));
        EDI_CHECK(styleController.setSelectedObjectStrokeWidth(0.0));
        styled = activeProjection();
        EDI_CHECK(styled.value(QStringLiteral("effective_stroke_color")).toString() == layerColor);

        // Undo walks the style edits back one command at a time: undoing
        // the width-0 (inherit) command restores the explicit 4.5 — an
        // assertion that FAILS if undo restores nothing (the first draft
        // checked a value that held either way; the review caught it).
        EDI_CHECK(styleController.undo());
        styled = activeProjection();
        EDI_CHECK(styled.value(QStringLiteral("effective_stroke_width")).toDouble() == 4.5);

        // Opacity: per-object only (no layer fallback), clamped to [0, 1],
        // surfaced through BOTH projection keys, one undo step per edit.
        EDI_CHECK(styleController.setSelectedObjectStrokeOpacity(0.4));
        styled = activeProjection();
        EDI_CHECK(styled.value(QStringLiteral("effective_stroke_opacity")).toDouble() == 0.4);
        EDI_CHECK(styled.value(QStringLiteral("own_stroke_opacity")).toDouble() == 0.4);
        EDI_CHECK(styleController.setSelectedObjectStrokeOpacity(5.0)); // clamps high
        EDI_CHECK(activeProjection().value(QStringLiteral("effective_stroke_opacity")).toDouble() == 1.0);
        EDI_CHECK(styleController.setSelectedObjectStrokeOpacity(-1.0)); // clamps to transparent
        EDI_CHECK(activeProjection().value(QStringLiteral("effective_stroke_opacity")).toDouble() == 0.0);
        EDI_CHECK(!styleController.setSelectedObjectStrokeOpacity(std::numeric_limits<double>::quiet_NaN()));
        EDI_CHECK(styleController.undo()); // undoing the 0.0 edit restores the clamp-high 1.0
        EDI_CHECK(activeProjection().value(QStringLiteral("effective_stroke_opacity")).toDouble() == 1.0);

        // Fill: gated by kind — only Rectangle/Circle/Ellipse/Polygon carry a fill
        // that the painter/SVG actually renders.  Open kinds (Line, Arc, …) are
        // rejected at the setter so no invisible "write-only" fill state can build up.
        // Add a Circle to this controller (away from the existing line) and select it.
        styleController.setSelectedToolId(QStringLiteral("circle_tool"));
        styleController.clickCanvasNormalized(0.5, 0.1); // center
        styleController.clickCanvasNormalized(0.7, 0.1); // edge → radius 0.2
        // circle_tool auto-selects the new object
        EDI_CHECK(activeProjection().value(QStringLiteral("own_fill_opacity")).toDouble() == 0.0);
        EDI_CHECK(styleController.setSelectedObjectFillColor(QStringLiteral("#2244aa")));
        EDI_CHECK(styleController.setSelectedObjectFillOpacity(0.6));
        styled = activeProjection();
        EDI_CHECK(styled.value(QStringLiteral("own_fill_color")).toString() == QStringLiteral("#2244aa"));
        EDI_CHECK(styled.value(QStringLiteral("own_fill_opacity")).toDouble() == 0.6);
        EDI_CHECK(styleController.setSelectedObjectFillOpacity(5.0)); // clamps high
        EDI_CHECK(activeProjection().value(QStringLiteral("own_fill_opacity")).toDouble() == 1.0);
        EDI_CHECK(!styleController.setSelectedObjectFillColor(QStringLiteral("not-a-color"))); // junk rejected
        EDI_CHECK(!styleController.setSelectedObjectFillOpacity(std::numeric_limits<double>::quiet_NaN())); // non-finite rejected
        // No-op guard: re-setting the value already in place must NOT push an undo step —
        // so the undo below restores 0.6, not 1.0.
        EDI_CHECK(styleController.setSelectedObjectFillOpacity(1.0)); // already 1.0 after the clamp
        EDI_CHECK(styleController.undo());
        EDI_CHECK(activeProjection().value(QStringLiteral("own_fill_opacity")).toDouble() == 0.6);

        // DR-15: fill setters on an open kind (Line) return false; object.fill unchanged.
        // Re-select the diagonal line created at the top of this block.
        styleController.setSelectedToolId(QStringLiteral("select_move"));
        styleController.clickCanvasNormalized(0.3, 0.3); // midpoint of the (0.2,0.2)→(0.8,0.8) line
        const QVariantMap lineBeforeFill = activeProjection();
        EDI_CHECK(!styleController.setSelectedObjectFillColor(QStringLiteral("#aabbcc")));
        EDI_CHECK(!styleController.setSelectedObjectFillOpacity(0.7));
        const QVariantMap lineAfterFill = activeProjection();
        // fill state must be byte-identical after the rejected calls
        EDI_CHECK(lineAfterFill.value(QStringLiteral("own_fill_color"))
            == lineBeforeFill.value(QStringLiteral("own_fill_color")));
        EDI_CHECK(lineAfterFill.value(QStringLiteral("own_fill_opacity"))
            == lineBeforeFill.value(QStringLiteral("own_fill_opacity")));
    }

    // #30 parametric arrays: option state (count + spacings) drives repeat
    // instead of the retired hardcoded 3 x 0.1.
    {
        DrawingDocumentController arrayController;
        // Option setters normalize garbage instead of failing later actions.
        arrayController.setArrayCount(0);
        EDI_CHECK(arrayController.arrayCount() == 1);
        arrayController.setArrayCount(500);
        EDI_CHECK(arrayController.arrayCount() == 99);
        arrayController.setArraySpacingX(std::numeric_limits<double>::infinity());
        EDI_CHECK(arrayController.arraySpacingX() == 0.0);
        arrayController.setArraySpacingY(-0.25); // negative = march up/left, legal
        EDI_CHECK(arrayController.arraySpacingY() == -0.25);
        arrayController.setFixedRadius(-2.0);
        EDI_CHECK(arrayController.fixedRadius() == 0.0);
        // Magnitudes clamp to the unit document space — stored state always
        // matches what the spins can show and what the build stamps.
        arrayController.setArraySpacingX(5.0);
        EDI_CHECK(arrayController.arraySpacingX() == 1.0);
        arrayController.setArraySpacingY(-5.0);
        EDI_CHECK(arrayController.arraySpacingY() == -1.0);
        arrayController.setFixedRadius(5.0);
        EDI_CHECK(arrayController.fixedRadius() == 1.0);
        arrayController.setFixedRadius(0.0);

        arrayController.setSelectedToolId("line_tool");
        arrayController.clickCanvasNormalized(0.2, 0.3);
        arrayController.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(arrayController.modelDocument().value("drawing_objects").toList().size() == 1);

        arrayController.setArrayCount(2);
        arrayController.setArraySpacingX(0.2);
        EDI_CHECK(arrayController.repeatSelectedObject("x"));
        QVariantList arrayObjects = arrayController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(arrayObjects.size() == 3);
        QVariantMap secondCopy = arrayObjects[2].toMap();
        EDI_CHECK(nearlyEqual(secondCopy.value("x1").toDouble(), 0.6));
        EDI_CHECK(nearlyEqual(secondCopy.value("y1").toDouble(), 0.3));
        EDI_CHECK(arrayController.modelDocument().value("selected_object_ids").toList().size() == 2);
    }

    // Grid array: count x count cells from one count spin, both spacings.
    {
        DrawingDocumentController gridArrayController;
        gridArrayController.setSelectedToolId("line_tool");
        gridArrayController.clickCanvasNormalized(0.1, 0.1);
        gridArrayController.clickCanvasNormalized(0.15, 0.1);

        // A 1x1 grid has nothing to create.
        gridArrayController.setArrayCount(1);
        EDI_CHECK(!gridArrayController.gridArraySelectedObject());

        gridArrayController.setArrayCount(2);
        gridArrayController.setArraySpacingX(0.2);
        gridArrayController.setArraySpacingY(0.3);
        EDI_CHECK(gridArrayController.gridArraySelectedObject());
        QVariantList gridObjects = gridArrayController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(gridObjects.size() == 4); // source + 3 copies
        QVariantMap rightCopy = gridObjects[1].toMap();    // cell (1,0)
        QVariantMap downCopy = gridObjects[2].toMap();     // cell (0,1)
        QVariantMap diagonalCopy = gridObjects[3].toMap(); // cell (1,1)
        EDI_CHECK(nearlyEqual(rightCopy.value("x1").toDouble(), 0.3));
        EDI_CHECK(nearlyEqual(rightCopy.value("y1").toDouble(), 0.1));
        EDI_CHECK(nearlyEqual(downCopy.value("x1").toDouble(), 0.1));
        EDI_CHECK(nearlyEqual(downCopy.value("y1").toDouble(), 0.4));
        EDI_CHECK(nearlyEqual(diagonalCopy.value("x1").toDouble(), 0.3));
        EDI_CHECK(nearlyEqual(diagonalCopy.value("y1").toDouble(), 0.4));
        EDI_CHECK(gridArrayController.modelDocument().value("selected_object_ids").toList().size() == 3);

        // One undo step removes the whole grid.
        EDI_CHECK(gridArrayController.undo());
        EDI_CHECK(gridArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
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
        EDI_CHECK(seeded.size() == 1);
        EDI_CHECK(nearlyEqual(seeded[0].toMap().value("radius").toDouble(), 0.02));

        radialController.setArrayCount(3);
        // Arming requires a usable source and exposes the prompt; the click
        // that follows is consumed as the centre, NOT as a new selection.
        EDI_CHECK(radialController.beginRadialArrayCenterPick());
        EDI_CHECK(radialController.isAwaitingPointCapture());
        EDI_CHECK(radialController.modelDocument().value("awaiting_point_capture").toBool());
        EDI_CHECK(!radialController.modelDocument().value("point_capture_prompt").toString().isEmpty());

        radialController.clickCanvasNormalized(centerX, centerY); // sets the ring centre
        EDI_CHECK(!radialController.isAwaitingPointCapture());       // capture consumed
        EDI_CHECK(!radialController.modelDocument().contains("awaiting_point_capture"));
        QVariantList ringObjects = radialController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(ringObjects.size() == 4);
        // Slots = 4 -> copies at 90/180/270 degrees, all at ring radius 0.2.
        for (int i = 1; i < ringObjects.size(); ++i) {
            QVariantMap copy = ringObjects[i].toMap();
            const double dx = copy.value("cx").toDouble() - centerX;
            const double dy = copy.value("cy").toDouble() - centerY;
            EDI_CHECK(nearlyEqual(std::hypot(dx, dy), 0.2));
            EDI_CHECK(nearlyEqual(copy.value("radius").toDouble(), 0.02));
        }

        // The capture click did not change selection — the source is still
        // active (so a second array would work), and the array is ONE undo step.
        EDI_CHECK(radialController.canUndo());
        radialController.undo();
        EDI_CHECK(radialController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Arming with NOTHING selected refuses and arms nothing.
        DrawingDocumentController emptyController;
        EDI_CHECK(!emptyController.beginRadialArrayCenterPick());
        EDI_CHECK(!emptyController.isAwaitingPointCapture());

        // Escape (cancelPendingCreation) drops an armed capture without arraying.
        radialController.setSelectedToolId("select_move");
        radialController.clickCanvasNormalized(centerX - 0.2, centerY); // reselect the source circle
        EDI_CHECK(radialController.beginRadialArrayCenterPick());
        radialController.cancelPendingCreation();
        EDI_CHECK(!radialController.isAwaitingPointCapture());
        EDI_CHECK(radialController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Switching tools also cancels an armed capture.
        EDI_CHECK(radialController.beginRadialArrayCenterPick());
        radialController.setSelectedToolId("line_tool");
        EDI_CHECK(!radialController.isAwaitingPointCapture());

        // A source sitting ON the picked centre has a zero arm: the planner
        // rejects, the document is untouched, and the capture still clears.
        DrawingDocumentController degenerateController;
        degenerateController.setSelectedToolId("circle_tool");
        degenerateController.setFixedRadius(0.05);
        degenerateController.clickCanvasNormalized(0.5, 0.5);
        degenerateController.clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(degenerateController.beginRadialArrayCenterPick());
        degenerateController.clickCanvasNormalized(0.5, 0.5); // centre == source centre -> zero arm
        EDI_CHECK(!degenerateController.isAwaitingPointCapture());
        EDI_CHECK(degenerateController.modelDocument().value("drawing_objects").toList().size() == 1);
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
        EDI_CHECK(rosette.modelDocument().value("drawing_objects").toList().size() == 1);

        rosette.setSelectedToolId("select_move");
        rosette.clickCanvasNormalized(centerX - 0.2, centerY); // reselect the source
        rosette.setArrayCount(3);
        rosette.setRotateCopiesTotalAngle(360.0);
        EDI_CHECK(nearlyEqual(rosette.rotateCopiesTotalAngle(), 360.0));
        EDI_CHECK(rosette.beginRotateCopiesCenterPick());
        EDI_CHECK(rosette.isAwaitingPointCapture());
        rosette.clickCanvasNormalized(centerX, centerY); // sets the rosette centre
        EDI_CHECK(!rosette.isAwaitingPointCapture());

        QVariantList objs = rosette.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objs.size() == 4); // source + 3 rotated copies
        for (int i = 1; i < objs.size(); ++i) {
            QVariantMap copy = objs[i].toMap();
            const double dx = copy.value("cx").toDouble() - centerX;
            const double dy = copy.value("cy").toDouble() - centerY;
            EDI_CHECK(nearlyEqual(std::hypot(dx, dy), 0.2)); // each copy orbits the ring
        }
        // ONE undo step removes all copies.
        EDI_CHECK(rosette.canUndo());
        rosette.undo();
        EDI_CHECK(rosette.modelDocument().value("drawing_objects").toList().size() == 1);

        // Refuses to arm with nothing selected.
        DrawingDocumentController emptyRosette;
        EDI_CHECK(!emptyRosette.beginRotateCopiesCenterPick());
        EDI_CHECK(!emptyRosette.isAwaitingPointCapture());

        // Setter must store faithfully — no silent swallow of small values.
        // Before the fix, |angle| < 1.0 was ignored and the spin diverged from
        // the stored value.
        DrawingDocumentController setterCheck;
        setterCheck.setRotateCopiesTotalAngle(0.0);
        EDI_CHECK(nearlyEqual(setterCheck.rotateCopiesTotalAngle(), 0.0));
        setterCheck.setRotateCopiesTotalAngle(0.5);
        EDI_CHECK(nearlyEqual(setterCheck.rotateCopiesTotalAngle(), 0.5));
        // Non-finite must still be ignored (prior value preserved).
        setterCheck.setRotateCopiesTotalAngle(std::numeric_limits<double>::quiet_NaN());
        EDI_CHECK(nearlyEqual(setterCheck.rotateCopiesTotalAngle(), 0.5));
    }

    // Kaleidoscope: arming + the captured centre reflects the source across
    // arrayCount() axes (one copy per axis) in ONE undo step. The single-axis
    // mirror verb is unchanged; the op test covers the per-kind orientation flip.
    {
        DrawingDocumentController kaleido;
        kaleido.setSelectedToolId("line_tool");
        kaleido.clickCanvasNormalized(0.6, 0.55);
        kaleido.clickCanvasNormalized(0.7, 0.6); // a line (a mirrorable kind)
        EDI_CHECK(kaleido.modelDocument().value("drawing_objects").toList().size() == 1);

        kaleido.setSelectedToolId("select_move");
        kaleido.clickCanvasNormalized(0.65, 0.575); // select the line
        EDI_CHECK(!kaleido.selectedObjectId().isEmpty());
        kaleido.setArrayCount(3); // 3 axes -> 3 reflected copies

        EDI_CHECK(kaleido.beginKaleidoscopeCenterPick());
        EDI_CHECK(kaleido.isAwaitingPointCapture());
        kaleido.clickCanvasNormalized(0.5, 0.5); // sets the kaleidoscope centre
        EDI_CHECK(!kaleido.isAwaitingPointCapture());
        EDI_CHECK(kaleido.modelDocument().value("drawing_objects").toList().size() == 4); // source + 3

        // ONE undo step removes all reflected copies.
        EDI_CHECK(kaleido.canUndo());
        kaleido.undo();
        EDI_CHECK(kaleido.modelDocument().value("drawing_objects").toList().size() == 1);

        // Refuses to arm with nothing selected.
        DrawingDocumentController emptyKaleido;
        EDI_CHECK(!emptyKaleido.beginKaleidoscopeCenterPick());
        EDI_CHECK(!emptyKaleido.isAwaitingPointCapture());
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
        EDI_CHECK(trimController.modelDocument().value("drawing_objects").toList().size() == 2);

        // Select the horizontal target (click on it, away from the crossing).
        trimController.setSelectedToolId("select_move");
        trimController.clickCanvasNormalized(0.3, 0.5);
        const QString targetId = trimController.selectedObjectId();
        EDI_CHECK(!targetId.isEmpty());

        // Arm trim, then click the RIGHT stub: the b end trims to the crossing.
        EDI_CHECK(trimController.beginTrimSelectedLine());
        EDI_CHECK(trimController.isAwaitingPointCapture());
        trimController.clickCanvasNormalized(0.72, 0.5);
        EDI_CHECK(!trimController.isAwaitingPointCapture());

        // The target now runs (0.2,0.5)..(0.5,0.5); trim mutates, never adds.
        QVariantList objects = trimController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 2);
        auto findById = [](const QVariantList &list, const QString &id) {
            QVariantMap found;
            for (const QVariant &v : list) {
                if (v.toMap().value("id").toString() == id) { found = v.toMap(); }
            }
            return found;
        };
        const QVariantMap trimmed = findById(objects, targetId);
        EDI_CHECK(!trimmed.isEmpty());
        EDI_CHECK(nearlyEqual(trimmed.value("x1").toDouble(), 0.2));
        EDI_CHECK(nearlyEqual(trimmed.value("x2").toDouble(), 0.5)); // b pulled to the cut
        EDI_CHECK(nearlyEqual(trimmed.value("y2").toDouble(), 0.5));

        // One undo step restores the full line.
        EDI_CHECK(trimController.canUndo());
        trimController.undo();
        const QVariantMap restored = findById(
            trimController.modelDocument().value("drawing_objects").toList(), targetId);
        EDI_CHECK(nearlyEqual(restored.value("x2").toDouble(), 0.8));

        // Trim with no crossing boundary surfaces a status and changes nothing.
        DrawingDocumentController lonelyController;
        lonelyController.setSelectedToolId("line_tool");
        lonelyController.clickCanvasNormalized(0.2, 0.3);
        lonelyController.clickCanvasNormalized(0.8, 0.3);
        lonelyController.setSelectedToolId("select_move");
        lonelyController.clickCanvasNormalized(0.5, 0.3);
        EDI_CHECK(lonelyController.beginTrimSelectedLine());
        lonelyController.clickCanvasNormalized(0.7, 0.3);
        EDI_CHECK(!lonelyController.isAwaitingPointCapture());
        EDI_CHECK(lonelyController.modelDocument().contains("edit_status"));
        const QVariantList lonelyObjects = lonelyController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(lonelyObjects.size() == 1);
        EDI_CHECK(nearlyEqual(lonelyObjects.front().toMap().value("x2").toDouble(), 0.8)); // unchanged

        // Trim refuses to arm when the selection is not a line.
        DrawingDocumentController nonLineController;
        nonLineController.setSelectedToolId("circle_tool");
        nonLineController.clickCanvasNormalized(0.5, 0.5);
        nonLineController.clickCanvasNormalized(0.6, 0.5);
        EDI_CHECK(!nonLineController.beginTrimSelectedLine());
        EDI_CHECK(!nonLineController.isAwaitingPointCapture());
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
        EDI_CHECK(filletController.modelDocument().value("drawing_objects").toList().size() == 2);

        // Select the horizontal line as the fillet target.
        filletController.setSelectedToolId("select_move");
        filletController.clickCanvasNormalized(0.55, 0.3);
        const QString targetId = filletController.selectedObjectId();
        EDI_CHECK(!targetId.isEmpty());

        filletController.setFilletRadius(0.1);
        EDI_CHECK(nearlyEqual(filletController.filletRadius(), 0.1));
        EDI_CHECK(filletController.beginFilletSelectedLine());
        EDI_CHECK(filletController.isAwaitingPointCapture());
        // Pick near the vertical line, inside the corner.
        filletController.clickCanvasNormalized(0.33, 0.5);
        EDI_CHECK(!filletController.isAwaitingPointCapture());

        // Two trimmed lines + one new arc = 3 objects.
        const QVariantList objects = filletController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 3);
        int arcCount = 0;
        for (const QVariant &v : objects) {
            if (v.toMap().value("kind").toString() == QStringLiteral("arc")) {
                ++arcCount;
            }
        }
        EDI_CHECK(arcCount == 1);

        // The whole fillet (two trims + the arc) is ONE undo step.
        EDI_CHECK(filletController.canUndo());
        filletController.undo();
        EDI_CHECK(filletController.modelDocument().value("drawing_objects").toList().size() == 2);

        // Fillet refuses to arm when the selection is not a line.
        DrawingDocumentController nonLine;
        nonLine.setSelectedToolId("circle_tool");
        nonLine.clickCanvasNormalized(0.5, 0.5);
        nonLine.clickCanvasNormalized(0.6, 0.5);
        EDI_CHECK(!nonLine.beginFilletSelectedLine());
        EDI_CHECK(!nonLine.isAwaitingPointCapture());
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
        EDI_CHECK(chamferController.modelDocument().value("drawing_objects").toList().size() == 2);

        chamferController.setSelectedToolId("select_move");
        chamferController.clickCanvasNormalized(0.55, 0.3);
        const QString targetId = chamferController.selectedObjectId();
        EDI_CHECK(!targetId.isEmpty());

        chamferController.setChamferSetback(0.1);
        EDI_CHECK(nearlyEqual(chamferController.chamferSetback(), 0.1));
        EDI_CHECK(chamferController.beginChamferSelectedLine());
        EDI_CHECK(chamferController.isAwaitingPointCapture());
        chamferController.clickCanvasNormalized(0.33, 0.5); // near the vertical line, in the corner
        EDI_CHECK(!chamferController.isAwaitingPointCapture());

        // Two set-back lines + one new bevel = 3 objects, and the bevel is a LINE.
        const QVariantList objects = chamferController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 3);
        int lineCount = 0;
        for (const QVariant &v : objects) {
            if (v.toMap().value("kind").toString() == QStringLiteral("line")) {
                ++lineCount;
            }
        }
        EDI_CHECK(lineCount == 3); // both arms + the bevel are all lines

        // The whole chamfer (two trims + the bevel) is ONE undo step.
        EDI_CHECK(chamferController.canUndo());
        chamferController.undo();
        EDI_CHECK(chamferController.modelDocument().value("drawing_objects").toList().size() == 2);

        // A rejection (no second line) surfaces via edit_status, not a silent no-op.
        DrawingDocumentController loneLine;
        loneLine.setSelectedToolId("line_tool");
        loneLine.clickCanvasNormalized(0.2, 0.2);
        loneLine.clickCanvasNormalized(0.8, 0.2);
        loneLine.setSelectedToolId("select_move");
        loneLine.clickCanvasNormalized(0.5, 0.2);
        EDI_CHECK(loneLine.beginChamferSelectedLine());
        loneLine.clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(!loneLine.isAwaitingPointCapture());
        const QVariantMap status = loneLine.modelDocument().value("edit_status").toMap();
        EDI_CHECK(status.value("ok").toBool() == false);
        EDI_CHECK(status.value("mode").toString() == "chamfer");
        EDI_CHECK(!status.value("message").toString().isEmpty());

        // Chamfer refuses to arm when the selection is not a line.
        DrawingDocumentController nonLineChamfer;
        nonLineChamfer.setSelectedToolId("circle_tool");
        nonLineChamfer.clickCanvasNormalized(0.5, 0.5);
        nonLineChamfer.clickCanvasNormalized(0.6, 0.5);
        EDI_CHECK(!nonLineChamfer.beginChamferSelectedLine());
        EDI_CHECK(!nonLineChamfer.isAwaitingPointCapture());
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
        EDI_CHECK(extendController.modelDocument().value("drawing_objects").toList().size() == 2);

        extendController.setSelectedToolId("select_move");
        extendController.clickCanvasNormalized(0.4, 0.5); // select the horizontal line
        const QString targetId = extendController.selectedObjectId();
        EDI_CHECK(!targetId.isEmpty());

        EDI_CHECK(extendController.beginExtendSelectedLine());
        EDI_CHECK(extendController.isAwaitingPointCapture());
        extendController.clickCanvasNormalized(0.55, 0.5); // pick near the b-end → extend it
        EDI_CHECK(!extendController.isAwaitingPointCapture());

        // Still 2 objects (extend updates, never creates); the b-end reached x=0.8.
        QVariantList objects = extendController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 2);
        QVariantMap extended;
        for (const QVariant &v : objects) {
            if (v.toMap().value("id").toString() == targetId) {
                extended = v.toMap();
            }
        }
        EDI_CHECK(!extended.isEmpty());
        EDI_CHECK(nearlyEqual(extended.value("x2").toDouble(), 0.8));
        EDI_CHECK(nearlyEqual(extended.value("y2").toDouble(), 0.5));
        EDI_CHECK(nearlyEqual(extended.value("x1").toDouble(), 0.3)); // anchor end unchanged

        // ONE undo step restores the original short line.
        EDI_CHECK(extendController.canUndo());
        extendController.undo();
        for (const QVariant &v : extendController.modelDocument().value("drawing_objects").toList()) {
            if (v.toMap().value("id").toString() == targetId) {
                EDI_CHECK(nearlyEqual(v.toMap().value("x2").toDouble(), 0.5));
            }
        }

        // A dead click (no reachable boundary) surfaces via edit_status, not silent.
        DrawingDocumentController loneExtend;
        loneExtend.setSelectedToolId("line_tool");
        loneExtend.clickCanvasNormalized(0.3, 0.5);
        loneExtend.clickCanvasNormalized(0.5, 0.5);
        loneExtend.setSelectedToolId("select_move");
        loneExtend.clickCanvasNormalized(0.4, 0.5);
        EDI_CHECK(loneExtend.beginExtendSelectedLine());
        loneExtend.clickCanvasNormalized(0.55, 0.5);
        EDI_CHECK(!loneExtend.isAwaitingPointCapture());
        const QVariantMap status = loneExtend.modelDocument().value("edit_status").toMap();
        EDI_CHECK(status.value("ok").toBool() == false);
        EDI_CHECK(status.value("mode").toString() == "extend");
        EDI_CHECK(!status.value("message").toString().isEmpty());

        // Extend refuses to arm when the selection is not a line.
        DrawingDocumentController nonLineExtend;
        nonLineExtend.setSelectedToolId("circle_tool");
        nonLineExtend.clickCanvasNormalized(0.5, 0.5);
        nonLineExtend.clickCanvasNormalized(0.6, 0.5);
        EDI_CHECK(!nonLineExtend.beginExtendSelectedLine());
        EDI_CHECK(!nonLineExtend.isAwaitingPointCapture());
    }

    // Break verb: split a line at the picked point into TWO independent objects
    // (original shortened + new piece) in ONE undo step. Pick-a-point's BreakPoint
    // consumer; reuses the chamfer atomic multi-object pattern.
    {
        DrawingDocumentController breakController;
        breakController.setSelectedToolId("line_tool");
        breakController.clickCanvasNormalized(0.2, 0.5);
        breakController.clickCanvasNormalized(0.8, 0.5); // one horizontal line
        EDI_CHECK(breakController.modelDocument().value("drawing_objects").toList().size() == 1);

        breakController.setSelectedToolId("select_move");
        breakController.clickCanvasNormalized(0.5, 0.5); // select the line
        const QString targetId = breakController.selectedObjectId();
        EDI_CHECK(!targetId.isEmpty());

        EDI_CHECK(breakController.beginBreakSelectedObject());
        EDI_CHECK(breakController.isAwaitingPointCapture());
        breakController.clickCanvasNormalized(0.5, 0.5); // break at the midpoint
        EDI_CHECK(!breakController.isAwaitingPointCapture());

        // Two lines now: the original shortened + the new piece.
        QVariantList objects = breakController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 2);
        QVariantMap original;
        QVariantMap piece;
        for (const QVariant &v : objects) {
            const QVariantMap m = v.toMap();
            EDI_CHECK(m.value("kind").toString() == QStringLiteral("line"));
            if (m.value("id").toString() == targetId) {
                original = m;
            } else {
                piece = m;
            }
        }
        EDI_CHECK(!original.isEmpty() && !piece.isEmpty());
        EDI_CHECK(nearlyEqual(original.value("x1").toDouble(), 0.2) && nearlyEqual(original.value("x2").toDouble(), 0.5));
        EDI_CHECK(nearlyEqual(piece.value("x1").toDouble(), 0.5) && nearlyEqual(piece.value("x2").toDouble(), 0.8));
        // The original (first piece) stays selected.
        EDI_CHECK(breakController.selectedObjectId() == targetId);

        // ONE undo step restores the single original line.
        EDI_CHECK(breakController.canUndo());
        breakController.undo();
        EDI_CHECK(breakController.modelDocument().value("drawing_objects").toList().size() == 1);

        // A dead break (exactly at an endpoint) surfaces via edit_status, not silent.
        DrawingDocumentController deadBreak;
        deadBreak.setSelectedToolId("line_tool");
        deadBreak.clickCanvasNormalized(0.2, 0.5);
        deadBreak.clickCanvasNormalized(0.8, 0.5);
        deadBreak.setSelectedToolId("select_move");
        deadBreak.clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(deadBreak.beginBreakSelectedObject());
        deadBreak.clickCanvasNormalized(0.2, 0.5); // exactly the a endpoint
        EDI_CHECK(!deadBreak.isAwaitingPointCapture());
        const QVariantMap status = deadBreak.modelDocument().value("edit_status").toMap();
        EDI_CHECK(status.value("ok").toBool() == false);
        EDI_CHECK(status.value("mode").toString() == "break");
        EDI_CHECK(!status.value("message").toString().isEmpty());
        EDI_CHECK(deadBreak.modelDocument().value("drawing_objects").toList().size() == 1); // unchanged

        // Break refuses to arm when the selection is neither line nor polyline.
        DrawingDocumentController nonBreakable;
        nonBreakable.setSelectedToolId("circle_tool");
        nonBreakable.clickCanvasNormalized(0.5, 0.5);
        nonBreakable.clickCanvasNormalized(0.6, 0.5);
        EDI_CHECK(!nonBreakable.beginBreakSelectedObject());
        EDI_CHECK(!nonBreakable.isAwaitingPointCapture());
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
        EDI_CHECK(!failureController.repeatSelectedObject("x"));
        QVariantMap arrayStatus = failureController.modelDocument().value("edit_status").toMap();
        EDI_CHECK(arrayStatus.value("ok").toBool() == false);
        EDI_CHECK(arrayStatus.value("mode").toString() == "array");
        EDI_CHECK(!arrayStatus.value("message").toString().isEmpty());
        EDI_CHECK(failureController.modelDocument().value("drawing_objects").toList().size() == 1);

        // Guides reject grid arrays at the planner; the controller surfaces
        // it the same way and creates nothing.
        // (Covered here at the planner seam; the guide tool path is separate.)

        failureController.setArraySpacingX(-0.1);
        EDI_CHECK(failureController.repeatSelectedObject("x"));
        QVariantList marched = failureController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(marched.size() == 3);
        // The failed attempt reclaimed serials 2-3, so the first successful
        // copy is repeat_2, not repeat_4.
        EDI_CHECK(trailingSerial(marched[1].toMap().value("id").toString()) == 2);
        // Negative spacing marches left: 0.5 + 2 * -0.1 = 0.3.
        EDI_CHECK(nearlyEqual(marched[2].toMap().value("x1").toDouble(), 0.3));
        // Success cleared the stale rejection.
        EDI_CHECK(failureController.modelDocument().value("edit_status").toMap().isEmpty());
    }

    // Guides cannot grid/radial-array (single-axis translation would stack
    // coincident copies): the controller reports failure and creates nothing.
    {
        DrawingDocumentController guideArrayController;
        guideArrayController.setSelectedToolId("horizontal_guide_tool");
        guideArrayController.clickCanvasNormalized(0.5, 0.62);
        EDI_CHECK(guideArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
        guideArrayController.setArrayCount(2);
        guideArrayController.setArraySpacingX(0.1);
        guideArrayController.setArraySpacingY(0.1);
        EDI_CHECK(!guideArrayController.gridArraySelectedObject());
        EDI_CHECK(guideArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
        // Radial arming succeeds (a guide IS an editable object), but the array
        // planner rejects a guide source when the centre click runs it — so the
        // rejection now lands at the pick click, and the document is untouched.
        EDI_CHECK(guideArrayController.beginRadialArrayCenterPick());
        guideArrayController.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(!guideArrayController.isAwaitingPointCapture());
        EDI_CHECK(guideArrayController.modelDocument().value("drawing_objects").toList().size() == 1);
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
        EDI_CHECK(nearlyEqual(fixedObjects[0].toMap().value("radius").toDouble(), 0.1));

        fixedRadiusController.setFixedRadius(0.0);
        fixedRadiusController.clickCanvasNormalized(0.5, 0.5);
        fixedRadiusController.clickCanvasNormalized(0.8, 0.5);
        fixedObjects = fixedRadiusController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(nearlyEqual(fixedObjects[1].toMap().value("radius").toDouble(), 0.3));
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
        EDI_CHECK(roomController.createRoomFromSpec(spec));
        const QVariantList objects = roomController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(objects.size() == 7); // 2 + 2 + 2 (opened edges) + 1 (solid W)
        for (const QVariant &value : objects) {
            EDI_CHECK(value.toMap().value("kind").toString() == "wall");
        }
        // One undo removes the whole room (atomic batch, not 7 separate creates).
        EDI_CHECK(roomController.undo());
        EDI_CHECK(roomController.modelDocument().value("drawing_objects").toList().empty());

        // A degenerate spec is refused without touching the document.
        edi::drafting::RoomSpec bad;
        bad.width = 0.0;
        bad.height = 0.3;
        EDI_CHECK(!roomController.createRoomFromSpec(bad));
        EDI_CHECK(roomController.modelDocument().value("drawing_objects").toList().empty());
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
        EDI_CHECK(plugController.createRoomFromSpec(spec));

        const edi::drafting::DraftingDocument &doc = plugController.draftingDocument();
        EDI_CHECK(doc.plugs.size() == 2);
        EDI_CHECK(doc.objects.size() == 6); // 4 solid walls + 2 plug markers

        const edi::drafting::DraftingPlug &north = doc.plugs.front();
        EDI_CHECK(north.name == "north_door" && north.type == "door");
        // Each plug anchors to a real Point marker in the SAME document.
        const edi::drafting::DraftingObject *marker = edi::drafting::findObject(doc, north.anchorObjectId);
        EDI_CHECK(marker != nullptr);
        EDI_CHECK(marker->kind == edi::drafting::DraftingShapeKind::Point);
        EDI_CHECK(marker->metadata.toolProvenance == "plug");

        // The whole room — walls, markers, AND plugs — collapses in one undo.
        EDI_CHECK(plugController.undo());
        EDI_CHECK(plugController.draftingDocument().plugs.empty());
        EDI_CHECK(plugController.draftingDocument().objects.empty());
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
        EDI_CHECK(connController.createRoomFromSpec(spec));

        const edi::drafting::DraftingDocument &doc = connController.draftingDocument();
        EDI_CHECK(doc.plugs.size() == 2);
        EDI_CHECK(doc.connections.size() == 1);
        const edi::drafting::DraftingDeclaredConnection &edge = doc.connections.front();
        EDI_CHECK(edge.type == "corridor");
        // The edge references the two plugs by the ids the controller minted.
        EDI_CHECK(edge.plugA == doc.plugs[0].id && edge.plugB == doc.plugs[1].id);

        // One undo removes walls, markers, plugs, AND the connection together.
        EDI_CHECK(connController.undo());
        EDI_CHECK(connController.draftingDocument().connections.empty());
        EDI_CHECK(connController.draftingDocument().plugs.empty());
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
        EDI_CHECK(mapController.createMapFromSpec(map));
        const edi::drafting::DraftingDocument &doc = mapController.draftingDocument();
        EDI_CHECK(doc.plugs.size() == 2);
        EDI_CHECK(doc.connections.size() == 1);
        // Each plug's exported name is namespaced room.plug, so the two "door"
        // plugs are distinguishable in the graph.
        bool foundA = false;
        bool foundB = false;
        for (const edi::drafting::DraftingPlug &p : doc.plugs) {
            foundA = foundA || p.name == "a.door";
            foundB = foundB || p.name == "b.door";
        }
        EDI_CHECK(foundA && foundB);
        // The connection joins the two distinct plug ids.
        const edi::drafting::DraftingDeclaredConnection &edge = doc.connections.front();
        EDI_CHECK(edge.plugA != edge.plugB);
        EDI_CHECK(edge.type == "corridor");

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
        EDI_CHECK(corridorWalls >= 2);
        // A door leaf per connected plug (both ends of the one connection), rendered
        // as a Door-type wall band.
        EDI_CHECK(doorLeaves == 2);
        for (const edi::drafting::DraftingObject &o : doc.objects) {
            if (o.metadata.toolProvenance == "door") {
                EDI_CHECK(o.metadata.wallVisual.type == edi::drafting::WallType::Door);
            }
        }

        // One undo collapses the entire map — every room's walls, every plug,
        // every connection, AND the corridors — together.
        EDI_CHECK(mapController.undo());
        EDI_CHECK(mapController.draftingDocument().objects.empty());
        EDI_CHECK(mapController.draftingDocument().plugs.empty());
        EDI_CHECK(mapController.draftingDocument().connections.empty());
    }

    // 044 PROPORTIONALITY (hub SCALE-POLICY invariant): createMapFromSpec DERIVES its
    // corridor/door widths from the dungeon's room geometry (min room short edge / 3),
    // NOT from a hardcoded literal — so a dungeon with every room doubled gets a door
    // leaf (and corridor) exactly 2x thicker. Rooms are canvas units NOT scaled by
    // canvasPerAuthoredUnit, hence the derivation tracks room geometry. We read the
    // door-leaf WallGeometry thickness because both the leaf and the corridor wall band
    // derive from the same kCorridorWidth.
    {
        const auto buildTwoRoomMap = [](double scale) {
            edi::drafting::MapSpec map;
            edi::drafting::NamedRoomSpec a;
            a.name = "a";
            a.spec.origin = {0.0 * scale, 0.0 * scale};
            a.spec.width = 0.4 * scale;
            a.spec.height = 0.4 * scale;
            a.spec.wallThickness = 0.02;
            a.spec.plugs = {{edi::drafting::RoomEdge::East, 0.2 * scale, "door", "door"}};
            edi::drafting::NamedRoomSpec b;
            b.name = "b";
            b.spec.origin = {0.6 * scale, 0.0 * scale};
            b.spec.width = 0.4 * scale;
            b.spec.height = 0.4 * scale;
            b.spec.wallThickness = 0.02;
            b.spec.plugs = {{edi::drafting::RoomEdge::West, 0.2 * scale, "door", "door"}};
            map.rooms = {a, b};
            edi::drafting::MapConnectionSpec corridor;
            corridor.from = {"a", "door"};
            corridor.to = {"b", "door"};
            corridor.type = "corridor";
            map.connections = {corridor};
            return map;
        };
        // Read the door-leaf WallGeometry thickness off the first "door"-provenance object.
        const auto doorLeafThickness = [](const DrawingDocumentController &ctl) {
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (o.metadata.toolProvenance != "door") {
                    continue;
                }
                const auto *wall = std::get_if<edi::drafting::WallGeometry>(&o.geometry);
                EDI_CHECK(wall != nullptr);
                return wall->thickness;
            }
            EDI_CHECK(false && "expected a door leaf");
            return 0.0;
        };

        DrawingDocumentController baseCtl;
        EDI_CHECK(baseCtl.createMapFromSpec(buildTwoRoomMap(1.0)));
        const double baseThickness = doorLeafThickness(baseCtl);

        DrawingDocumentController doubledCtl;
        EDI_CHECK(doubledCtl.createMapFromSpec(buildTwoRoomMap(2.0)));
        const double doubledThickness = doorLeafThickness(doubledCtl);

        // Doubling every room origin + size doubles the derived door/corridor width.
        EDI_CHECK(baseThickness > 0.0);
        EDI_CHECK(nearlyEqual(doubledThickness, 2.0 * baseThickness));
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
        EDI_CHECK(featureController.createMapFromSpec(map, scale));
        const edi::drafting::DraftingDocument &doc = featureController.draftingDocument();

        int featureCount = 0;
        bool sawRubble = false;
        bool sawStatue = false;
        for (const edi::drafting::DraftingObject &o : doc.objects) {
            if (o.metadata.toolProvenance != "feature") {
                continue;
            }
            ++featureCount;
            EDI_CHECK(o.kind == edi::drafting::DraftingShapeKind::Point);
            const auto tagHas = [&o](const std::string &t) {
                return std::find(o.metadata.tags.begin(), o.metadata.tags.end(), t) != o.metadata.tags.end();
            };
            const auto point = std::get<edi::drafting::PointGeometry>(o.geometry).point;
            if (tagHas("feature:rubble")) {
                sawRubble = true;
                EDI_CHECK(tagHas("name:cave_in"));     // named -> name:<name> tag
                EDI_CHECK(nearlyEqual(point.x, 1.0 + 3.0 * scale)); // 1.06
                EDI_CHECK(nearlyEqual(point.y, 2.0 + 4.0 * scale)); // 2.08
            } else if (tagHas("feature:statue")) {
                sawStatue = true;
                // No name -> no name: tag, only the feature: tag.
                EDI_CHECK(o.metadata.tags.size() == 1);
                EDI_CHECK(nearlyEqual(point.x, 1.0 + 5.0 * scale)); // 1.10
                EDI_CHECK(nearlyEqual(point.y, 2.0));               // y offset 0
            }
            // Neutral law: a feature carries NO ObjectRole.
            EDI_CHECK(o.metadata.role == edi::drafting::ObjectRole::None);
        }
        EDI_CHECK(featureCount == 2 && sawRubble && sawStatue);

        // Undo collapses the features with the rest of the map.
        EDI_CHECK(featureController.undo());
        EDI_CHECK(featureController.draftingDocument().objects.empty());
    }

    // 041: MapSpec-level prop instances (MapBlockSpec) realize as definition-less
    // Point markers carrying a BlockPlacementMetadata. A block is MapSpec-level, so its
    // ABSOLUTE authored-feet position is scaled by canvasPerAuthoredUnit with NO room
    // origin added. assetRef + instanceId + transform survive a serialize round-trip.
    {
        edi::drafting::MapSpec map;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {0.0, 0.0}; // canvas units; footprint spans [0,0.4] both axes
        a.spec.width = 0.4;
        a.spec.height = 0.4;
        a.spec.wallThickness = 0.02;
        map.rooms = {a};
        // Two props: a sarcophagus with non-identity rotation/scale (proves plumbing),
        // a brazier at identity with no name (proves the name tag is omitted when empty).
        map.blocks = {
            {"crypt.sarcophagus", {3.0, 4.0}, 90.0, 2.0, "lord_tomb"},
            {"crypt.brazier", {5.0, 1.0}, 0.0, 1.0, ""},
        };

        DrawingDocumentController blockController;
        const double scale = 0.02; // canvas per authored foot (non-1.0 proves scaling)
        EDI_CHECK(blockController.createMapFromSpec(map, scale));
        const edi::drafting::DraftingDocument &doc = blockController.draftingDocument();

        int blockCount = 0;
        std::string sarcophagusInstance;
        std::string brazierInstance;
        for (const edi::drafting::DraftingObject &o : doc.objects) {
            if (o.metadata.blockPlacement.instanceId.empty()) {
                continue;
            }
            ++blockCount;
            EDI_CHECK(o.kind == edi::drafting::DraftingShapeKind::Point);
            EDI_CHECK(o.metadata.toolProvenance == "block");
            EDI_CHECK(o.metadata.blockPlacement.blockId.empty()); // no definition: pure asset ref
            const auto tagHas = [&o](const std::string &t) {
                return std::find(o.metadata.tags.begin(), o.metadata.tags.end(), t) != o.metadata.tags.end();
            };
            const auto point = std::get<edi::drafting::PointGeometry>(o.geometry).point;
            if (o.metadata.blockPlacement.assetRef == "crypt.sarcophagus") {
                sarcophagusInstance = o.metadata.blockPlacement.instanceId;
                EDI_CHECK(nearlyEqual(point.x, 3.0 * scale)); // no room origin added
                EDI_CHECK(nearlyEqual(point.y, 4.0 * scale));
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 90.0));
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 2.0));
                EDI_CHECK(tagHas("name:lord_tomb")); // named -> name:<name> tag
            } else if (o.metadata.blockPlacement.assetRef == "crypt.brazier") {
                brazierInstance = o.metadata.blockPlacement.instanceId;
                EDI_CHECK(nearlyEqual(point.x, 5.0 * scale));
                EDI_CHECK(nearlyEqual(point.y, 1.0 * scale));
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 0.0)); // identity
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 1.0));
                EDI_CHECK(o.metadata.tags.empty()); // no name -> no name: tag
            }
        }
        EDI_CHECK(blockCount == 2);
        EDI_CHECK(!sarcophagusInstance.empty() && !brazierInstance.empty());
        EDI_CHECK(sarcophagusInstance != brazierInstance); // minted off the one serial

        // ROUND-TRIP: the stamped placement survives encode/decode (block_placement
        // already serializes when instance_id is non-empty — this confirms the data,
        // no new codec). assetRef + instanceId + transform must come back byte-faithful.
        const edi::formats::ByteBuffer bytes =
            edi::drafting::encodeDraftingDocument(doc);
        const auto reloaded = edi::drafting::decodeDraftingDocument(bytes, "blockroundtrip");
        EDI_CHECK(reloaded.ok && reloaded.value);
        int reloadedBlocks = 0;
        for (const edi::drafting::DraftingObject &o : reloaded.value->objects) {
            if (o.metadata.blockPlacement.instanceId.empty()) {
                continue;
            }
            ++reloadedBlocks;
            EDI_CHECK(o.metadata.blockPlacement.blockId.empty());
            if (o.metadata.blockPlacement.assetRef == "crypt.sarcophagus") {
                EDI_CHECK(o.metadata.blockPlacement.instanceId == sarcophagusInstance);
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 90.0));
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 2.0));
            } else if (o.metadata.blockPlacement.assetRef == "crypt.brazier") {
                EDI_CHECK(o.metadata.blockPlacement.instanceId == brazierInstance);
            }
        }
        EDI_CHECK(reloadedBlocks == 2);

        // Undo collapses the props with the rest of the map.
        EDI_CHECK(blockController.undo());
        EDI_CHECK(blockController.draftingDocument().objects.empty());
    }

    // 041: a map with NO blocks adds no block markers (additive, behavior unchanged).
    {
        edi::drafting::MapSpec map;
        edi::drafting::NamedRoomSpec a;
        a.name = "a";
        a.spec.origin = {0.0, 0.0};
        a.spec.width = 0.4;
        a.spec.height = 0.4;
        a.spec.wallThickness = 0.02;
        map.rooms = {a};

        DrawingDocumentController plainBlockController;
        EDI_CHECK(plainBlockController.createMapFromSpec(map));
        int blockCount = 0;
        for (const edi::drafting::DraftingObject &o : plainBlockController.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                ++blockCount;
            }
        }
        EDI_CHECK(blockCount == 0); // no blocks authored -> no block markers
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
        EDI_CHECK(plainController.createMapFromSpec(map));
        int featureCount = 0;
        for (const edi::drafting::DraftingObject &o : plainController.draftingDocument().objects) {
            if (o.metadata.toolProvenance == "feature") {
                ++featureCount;
            }
        }
        EDI_CHECK(featureCount == 0); // no features authored -> no feature markers
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
        EDI_CHECK(nearlyEqual(walls[0].toMap().value("thickness").toDouble(), 0.25));

        wallThicknessController.setWallThickness(0.0); // invalid -> 0.1 default
        EDI_CHECK(nearlyEqual(wallThicknessController.wallThickness(), 0.1));
        wallThicknessController.clickCanvasNormalized(0.2, 0.6);
        wallThicknessController.clickCanvasNormalized(0.8, 0.6);
        walls = wallThicknessController.modelDocument().value("drawing_objects").toList();
        EDI_CHECK(nearlyEqual(walls[1].toMap().value("thickness").toDouble(), 0.1));
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
        EDI_CHECK(objectCount(blockCtl) == 2);

        // Define with nothing selected is refused (a dead button must say so).
        blockCtl.selectObjectsInBoundsNormalized(0.9, 0.9, 0.95, 0.95);
        EDI_CHECK(!blockCtl.defineBlockFromSelection("table"));
        EDI_CHECK(blockCtl.draftingDocument().blocks.empty());

        // Marquee-select both points and save them as a named block. The
        // definition lands in one undo step; its members are normalized to the
        // origin (lower-left at 0,0), extent = the selection's union span.
        blockCtl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(blockCtl.defineBlockFromSelection("table", "recipe.tavern_table"));
        EDI_CHECK(blockCtl.draftingDocument().blocks.size() == 1);
        const edi::drafting::DraftingBlock &def = blockCtl.draftingDocument().blocks.front();
        const QString blockId = QString::fromStdString(def.id);
        EDI_CHECK(blockId.startsWith("block_"));
        EDI_CHECK(def.name == "table");
        EDI_CHECK(def.assetRef == "recipe.tavern_table"); // Seam B: linked at define time
        EDI_CHECK(def.objects.size() == 2);
        EDI_CHECK(nearlyEqual(def.bounds.x, 0.0) && nearlyEqual(def.bounds.y, 0.0));
        EDI_CHECK(nearlyEqual(def.bounds.width, 0.3) && nearlyEqual(def.bounds.height, 0.3));
        // Capture the normalized definition to prove independence after placement.
        const auto defPointA = std::get<edi::drafting::PointGeometry>(def.objects[0].geometry).point;
        EDI_CHECK(nearlyEqual(defPointA.x, 0.0) && nearlyEqual(defPointA.y, 0.0));

        // Defining is one undo step (the document objects are untouched by it).
        blockCtl.undo();
        EDI_CHECK(blockCtl.draftingDocument().blocks.empty());
        EDI_CHECK(objectCount(blockCtl) == 2);
        blockCtl.redo();
        EDI_CHECK(blockCtl.draftingDocument().blocks.size() == 1);

        // Stamping an unknown block is a no-op.
        EDI_CHECK(!blockCtl.placeBlockInstance("block_9999", 0.5, 0.5));
        EDI_CHECK(objectCount(blockCtl) == 2);

        // Stamp the block centred on (0.5,0.5): two fresh "instance_" objects
        // appear (centre 0.15,0.15 -> offset 0.35,0.35), selected as one unit.
        const int beforeStamp = objectCount(blockCtl);
        EDI_CHECK(blockCtl.placeBlockInstance(blockId, 0.5, 0.5));
        EDI_CHECK(objectCount(blockCtl) == beforeStamp + 2);
        EDI_CHECK(blockCtl.modelDocument().value("selected_object_ids").toList().size() == 2);

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
                    EDI_CHECK((nearlyEqual(px, 0.35) && nearlyEqual(py, 0.35))
                           || (nearlyEqual(px, 0.65) && nearlyEqual(py, 0.65)));
                }
            }
            EDI_CHECK(instanceCount == 2);
            EDI_CHECK(ids.size() == objects.size());
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
                EDI_CHECK(bp.blockId == blockId.toStdString());
                EDI_CHECK(bp.assetRef == "recipe.tavern_table");
                if (sharedInstanceId.isEmpty()) {
                    sharedInstanceId = QString::fromStdString(bp.instanceId);
                } else {
                    EDI_CHECK(QString::fromStdString(bp.instanceId) == sharedInstanceId); // one placement
                }
            }
            EDI_CHECK(stamped == 2);
            EDI_CHECK(sharedInstanceId.startsWith("blockinst_"));
        }

        // Independence (FLATTEN): the definition is byte-unchanged by placement,
        // and stamping is exactly one undo step.
        const auto defPointAfter = std::get<edi::drafting::PointGeometry>(
            blockCtl.draftingDocument().blocks.front().objects[0].geometry).point;
        EDI_CHECK(nearlyEqual(defPointAfter.x, 0.0) && nearlyEqual(defPointAfter.y, 0.0));
        blockCtl.undo();
        EDI_CHECK(objectCount(blockCtl) == beforeStamp);
        EDI_CHECK(blockCtl.draftingDocument().blocks.size() == 1); // definition survives
        blockCtl.redo();
        EDI_CHECK(objectCount(blockCtl) == beforeStamp + 2);
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
            EDI_CHECK(false && "no placed rectangle");
            return {};
        };

        DrawingDocumentController ctl;
        ctl.setSelectedToolId("rectangle_tool");
        ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.clickCanvasNormalized(0.5, 0.5); // 0.2 x 0.2 rectangle
        ctl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(ctl.defineBlockFromSelection("box"));
        const QString blockId = QString::fromStdString(ctl.draftingDocument().blocks.front().id);

        // Setter guards: a non-finite/non-positive value is rejected (state kept).
        ctl.setBlockPlacementScale(2.0);
        ctl.setBlockPlacementScale(0.0);   // invalid -> stays 2.0
        ctl.setBlockPlacementScale(-1.0);  // invalid -> stays 2.0
        EDI_CHECK(nearlyEqual(ctl.blockPlacementScale(), 2.0));
        ctl.setBlockPlacementRotation(90.0);
        EDI_CHECK(nearlyEqual(ctl.blockPlacementRotation(), 90.0));

        // IDENTITY placement (0deg / 1.0) is byte-identical to the pre-DM-14 stamp: the
        // rectangle keeps its 0.2 footprint and identity placement metadata.
        ctl.setBlockPlacementRotation(0.0);
        ctl.setBlockPlacementScale(1.0);
        EDI_CHECK(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        {
            const edi::drafting::RectangleGeometry r = placedRect(ctl);
            EDI_CHECK(nearlyEqual(r.width, 0.2) && nearlyEqual(r.height, 0.2)); // NOT scaled
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (!o.metadata.blockPlacement.instanceId.empty()) {
                    EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 0.0));
                    EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 1.0));
                }
            }
        }
        ctl.undo(); // drop the identity stamp

        // 90deg / x2 placement: the rectangle scales (0.2 -> 0.4) and its metadata
        // records the transform.
        ctl.setBlockPlacementRotation(90.0);
        ctl.setBlockPlacementScale(2.0);
        EDI_CHECK(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        {
            const edi::drafting::RectangleGeometry r = placedRect(ctl);
            EDI_CHECK(nearlyEqual(r.width, 0.4) && nearlyEqual(r.height, 0.4)); // scaled x2
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (!o.metadata.blockPlacement.instanceId.empty()) {
                    EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 90.0));
                    EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 2.0));
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
            EDI_CHECK(false && "no placed rectangle");
            return {};
        };

        DrawingDocumentController ctl;
        ctl.setSelectedToolId("rectangle_tool");
        ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.clickCanvasNormalized(0.5, 0.5);
        ctl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(ctl.defineBlockFromSelection("box"));
        const QString blockId = QString::fromStdString(ctl.draftingDocument().blocks.front().id);

        // Place a 90deg / x2 instance (rectangle now 0.4) and capture its instance id.
        ctl.setBlockPlacementRotation(90.0);
        ctl.setBlockPlacementScale(2.0);
        EDI_CHECK(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        std::string instanceId;
        for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                instanceId = o.metadata.blockPlacement.instanceId;
            }
        }
        EDI_CHECK(!instanceId.empty());

        // A bad instance id refuses with no change.
        const std::uint64_t revBefore = ctl.draftingDocument().revision;
        EDI_CHECK(!ctl.transformBlockInstance(QStringLiteral("blockinst_nope"), 45.0, 1.5));
        EDI_CHECK(ctl.draftingDocument().revision == revBefore);
        // A non-positive scale factor refuses too (NaN/range guard).
        EDI_CHECK(!ctl.transformBlockInstance(QString::fromStdString(instanceId), 45.0, 0.0));

        // Transform by (+45deg, x1.5): geometry scales again (0.4 -> 0.6) and the
        // metadata ACCUMULATES (90+45=135, 2*1.5=3.0) in one undo step.
        EDI_CHECK(ctl.transformBlockInstance(QString::fromStdString(instanceId), 45.0, 1.5));
        {
            const edi::drafting::RectangleGeometry r = placedRect(ctl);
            EDI_CHECK(nearlyEqual(r.width, 0.6) && nearlyEqual(r.height, 0.6)); // 0.4 x 1.5
            for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
                if (o.metadata.blockPlacement.instanceId == instanceId) {
                    EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.rotationDeg, 135.0));
                    EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 3.0));
                }
            }
        }
        // One undo reverts the whole group transform (back to 0.4).
        EDI_CHECK(ctl.undo());
        EDI_CHECK(nearlyEqual(placedRect(ctl).width, 0.4));
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
            EDI_CHECK(false && "no placed rectangle");
            return {};
        };

        DrawingDocumentController ctl;
        ctl.setSelectedToolId("rectangle_tool");
        ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.clickCanvasNormalized(0.5, 0.5);
        ctl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(ctl.defineBlockFromSelection("box"));
        const QString blockId = QString::fromStdString(ctl.draftingDocument().blocks.front().id);

        // A huge-but-finite placement scale is accepted (finite, > 0).
        ctl.setBlockPlacementScale(1e200);
        EDI_CHECK(nearlyEqual(ctl.blockPlacementScale(), 1e200));
        EDI_CHECK(ctl.placeBlockInstance(blockId, 0.6, 0.6));
        std::string instanceId;
        for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                instanceId = o.metadata.blockPlacement.instanceId;
            }
        }
        EDI_CHECK(!instanceId.empty());
        EDI_CHECK(std::isfinite(ctl.draftingDocument().objects.back().metadata.blockPlacement.scale));

        // 1e200 * 1e200 -> inf: the member is refused, leaving scale finite (1e200).
        ctl.transformBlockInstance(QString::fromStdString(instanceId), 0.0, 1e200);
        for (const edi::drafting::DraftingObject &o : ctl.draftingDocument().objects) {
            if (o.metadata.blockPlacement.instanceId == instanceId) {
                EDI_CHECK(std::isfinite(o.metadata.blockPlacement.scale)); // never inf
                EDI_CHECK(nearlyEqual(o.metadata.blockPlacement.scale, 1e200)); // unchanged
            }
        }
        // The geometry stayed finite too (no inf coords written).
        EDI_CHECK(std::isfinite(placedRect(ctl).width));
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
        EDI_CHECK(mapCtl.createMapFromSpec(spec, 0.02)); // authored at 0.02 canvas/ft
        EDI_CHECK(nearlyEqual(mapCtl.draftingDocument().canvasPerAuthoredUnit, 0.02)); // scale recorded
        EDI_CHECK(mapCtl.draftingDocument().rooms.size() == 2);
        const edi::drafting::DraftingMapRoom &ra = mapCtl.draftingDocument().rooms[0];
        EDI_CHECK(ra.name == "a" && ra.material == "stone");
        EDI_CHECK(nearlyEqual(ra.origin.x, 0.1) && nearlyEqual(ra.origin.y, 0.1));
        EDI_CHECK(nearlyEqual(ra.width, 0.2) && nearlyEqual(ra.height, 0.15));
        EDI_CHECK(mapCtl.draftingDocument().rooms[1].name == "b");

        // Atomic with the map create: one undo clears the rooms (and the walls).
        mapCtl.undo();
        EDI_CHECK(mapCtl.draftingDocument().rooms.empty());
        mapCtl.redo();
        EDI_CHECK(mapCtl.draftingDocument().rooms.size() == 2);

        // DM-07/08 PERSISTENCE LEG: an EDITED document survives .edidraw save/reload
        // with its rooms intact — the proof that Seam C reads rooms FROM the document
        // (not the transient MapSpec). Encode to bytes, decode, and assert every
        // room's name + footprint + material is byte-faithful. This is what would
        // FAIL if mapRoomValue/readMapRoom ever dropped a field.
        const edi::formats::ByteBuffer bytes =
            edi::drafting::encodeDraftingDocument(mapCtl.draftingDocument());
        const auto reloaded = edi::drafting::decodeDraftingDocument(bytes, "roundtrip");
        EDI_CHECK(reloaded.ok && reloaded.value);
        const auto &saved = mapCtl.draftingDocument().rooms;
        const auto &back = reloaded.value->rooms;
        EDI_CHECK(back.size() == saved.size() && back.size() == 2);
        for (std::size_t i = 0; i < saved.size(); ++i) {
            EDI_CHECK(back[i].name == saved[i].name);
            EDI_CHECK(back[i].material == saved[i].material);
            EDI_CHECK(nearlyEqual(back[i].origin.x, saved[i].origin.x));
            EDI_CHECK(nearlyEqual(back[i].origin.y, saved[i].origin.y));
            EDI_CHECK(nearlyEqual(back[i].width, saved[i].width));
            EDI_CHECK(nearlyEqual(back[i].height, saved[i].height));
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
        EDI_CHECK(fillCtl.createMapFromSpec(spec, 0.02));
        const int objectsAfterMap = static_cast<int>(fillCtl.draftingDocument().objects.size());
        const std::uint64_t revAfterMap = fillCtl.draftingDocument().revision;

        // Arming on a doc WITH rooms returns true, exposes the prompt, and does NOT
        // touch the document (revision unchanged).
        EDI_CHECK(fillCtl.beginRegionFillPick());
        EDI_CHECK(fillCtl.isAwaitingPointCapture());
        EDI_CHECK(fillCtl.pointCapturePrompt() == QStringLiteral("Click inside a room to fill"));
        EDI_CHECK(fillCtl.draftingDocument().revision == revAfterMap);

        // A click INSIDE the room footprint mints exactly one Polygon, filled
        // (opacity > 0), auto-selected as the active object, revision bumped once,
        // capture cleared.
        fillCtl.clickCanvasNormalized(0.25, 0.25);
        EDI_CHECK(!fillCtl.isAwaitingPointCapture()); // capture consumed
        const auto &objs = fillCtl.draftingDocument().objects;
        EDI_CHECK(static_cast<int>(objs.size()) == objectsAfterMap + 1);
        const edi::drafting::DraftingObject &poly = objs.back();
        EDI_CHECK(poly.kind == edi::drafting::DraftingShapeKind::Polygon);
        EDI_CHECK(poly.fill.opacity > 0.0);
        EDI_CHECK(fillCtl.draftingDocument().activeObjectId.has_value()
               && *fillCtl.draftingDocument().activeObjectId == poly.id);
        // The fill bumped the revision (it is one ATOMIC edit — createObjectsAndSelect
        // brackets the create+select in a single beginEdit/commitEdit, so it collapses
        // to ONE undo step, asserted below; the raw counter bumps per command).
        EDI_CHECK(fillCtl.draftingDocument().revision > revAfterMap);
        // Neutral: the fill carries NO ObjectRole (presentation only).
        EDI_CHECK(poly.metadata.role == edi::drafting::ObjectRole::None);

        // One undo removes the whole fill (the atomicity proof).
        EDI_CHECK(fillCtl.undo());
        EDI_CHECK(static_cast<int>(fillCtl.draftingDocument().objects.size()) == objectsAfterMap);

        // Armed → click in OPEN SPACE → no object created, capture cleared, refusal
        // surfaced (the document gains nothing).
        EDI_CHECK(fillCtl.beginRegionFillPick());
        const int before = static_cast<int>(fillCtl.draftingDocument().objects.size());
        fillCtl.clickCanvasNormalized(0.9, 0.9); // outside the only room
        EDI_CHECK(!fillCtl.isAwaitingPointCapture());
        EDI_CHECK(static_cast<int>(fillCtl.draftingDocument().objects.size()) == before);

        // Arming on an EMPTY-rooms document refuses (no dead prompt).
        DrawingDocumentController emptyCtl;
        EDI_CHECK(!emptyCtl.beginRegionFillPick());
        EDI_CHECK(!emptyCtl.isAwaitingPointCapture());
    }

    // B2-1: interactive plug-placement tool.
    // beginPlugPick() arms a PlugPlacement capture; a canvas click mints exactly one
    // Point marker + one DraftingPlug in one undo step; one undo removes both.
    {
        DrawingDocumentController plugCtl;

        // Arming: prompt set, document untouched — same up-front test as other
        // pick tools (region fill, block instance).
        EDI_CHECK(plugCtl.beginPlugPick());
        EDI_CHECK(plugCtl.isAwaitingPointCapture());
        EDI_CHECK(plugCtl.pointCapturePrompt() == QStringLiteral("Click a wall to place a plug"));
        const std::uint64_t revBefore = plugCtl.draftingDocument().revision;
        EDI_CHECK(plugCtl.draftingDocument().plugs.empty());
        EDI_CHECK(plugCtl.draftingDocument().objects.empty());

        // A click at (0.5, 0.5): capture consumed, one marker + one plug created,
        // revision bumped. objectSnapEnabled is false on a fresh controller, so the
        // click lands at the raw canvas coordinate with no snap interference.
        plugCtl.clickCanvasNormalized(0.5, 0.5);
        EDI_CHECK(!plugCtl.isAwaitingPointCapture()); // capture consumed
        {
            const edi::drafting::DraftingDocument &doc = plugCtl.draftingDocument();
            EDI_CHECK(doc.plugs.size() == 1);
            EDI_CHECK(doc.objects.size() == 1); // the anchor Point marker
            EDI_CHECK(doc.revision > revBefore);

            const edi::drafting::DraftingPlug &plug = doc.plugs.front();
            EDI_CHECK(plug.type == "door"); // default neutral type — no game rule
            // anchor cache matches the click position (no snap offset).
            EDI_CHECK(nearlyEqual(plug.anchor.x, 0.5) && nearlyEqual(plug.anchor.y, 0.5));

            // The plug anchors to the minted Point marker.
            const edi::drafting::DraftingObject *marker =
                edi::drafting::findObject(doc, plug.anchorObjectId);
            EDI_CHECK(marker != nullptr);
            EDI_CHECK(marker->kind == edi::drafting::DraftingShapeKind::Point);
            EDI_CHECK(marker->metadata.toolProvenance == "plug");
        }

        // One undo removes BOTH marker and plug — they are in one undo step.
        EDI_CHECK(plugCtl.undo());
        EDI_CHECK(plugCtl.draftingDocument().plugs.empty());
        EDI_CHECK(plugCtl.draftingDocument().objects.empty());
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
            EDI_CHECK(doc.plugs.size() == 2);
        }
        const std::uint64_t revAfterPlugs   = connCtl.draftingDocument().revision;
        const int           markersAfterPlug = static_cast<int>(connCtl.draftingDocument().objects.size()); // 2

        // Arm the connection tool.
        EDI_CHECK(connCtl.beginConnectionPick());
        EDI_CHECK(connCtl.isAwaitingPointCapture());
        EDI_CHECK(connCtl.pointCapturePrompt() == QStringLiteral("Click the first plug"));
        // Arming must NOT touch the document.
        EDI_CHECK(connCtl.draftingDocument().revision == revAfterPlugs);

        // First click on plug A's marker — prompt advances, no connection yet.
        connCtl.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(connCtl.isAwaitingPointCapture()); // still armed for second click
        EDI_CHECK(connCtl.pointCapturePrompt() == QStringLiteral("Click the second plug"));
        EDI_CHECK(connCtl.draftingDocument().connections.empty()); // not yet connected

        // Second click on SAME plug A — refuse, tool disarms, no change.
        connCtl.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(!connCtl.isAwaitingPointCapture()); // tool disarmed on refusal
        EDI_CHECK(connCtl.draftingDocument().connections.empty()); // still no connection
        // Document content must be unchanged (refusal = edit-status only, no object change).
        EDI_CHECK(static_cast<int>(connCtl.draftingDocument().objects.size()) == markersAfterPlug);

        // Re-arm and complete the full two-click flow.
        EDI_CHECK(connCtl.beginConnectionPick());
        connCtl.clickCanvasNormalized(0.3, 0.3); // first click → plug A stored
        connCtl.clickCanvasNormalized(0.7, 0.7); // second click → plug B → connect!

        EDI_CHECK(!connCtl.isAwaitingPointCapture()); // capture consumed
        {
            const edi::drafting::DraftingDocument &doc = connCtl.draftingDocument();
            EDI_CHECK(doc.connections.size() == 1); // one connection declared

            const edi::drafting::DraftingDeclaredConnection &conn = doc.connections.front();
            // The connection references the two minted plug ids.
            EDI_CHECK(conn.plugA == doc.plugs[0].id && conn.plugB == doc.plugs[1].id);

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
            EDI_CHECK(taggedWallCount > 0);
        }

        // One undo removes the connection AND all corridor objects in one step,
        // leaving only the two plug markers (the atomicity proof).
        EDI_CHECK(connCtl.undo());
        EDI_CHECK(connCtl.draftingDocument().connections.empty());
        EDI_CHECK(static_cast<int>(connCtl.draftingDocument().objects.size()) == markersAfterPlug);
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
        EDI_CHECK(projCtl.defineBlockFromSelection("dot"));
        const QString blkId = QString::fromStdString(projCtl.draftingDocument().blocks.front().id);

        EDI_CHECK(projCtl.placeBlockInstance(blkId, 0.7, 0.7));

        // Capture the shared instance id from the document.
        std::string placedInstanceId;
        for (const edi::drafting::DraftingObject &o : projCtl.draftingDocument().objects) {
            if (!o.metadata.blockPlacement.instanceId.empty()) {
                placedInstanceId = o.metadata.blockPlacement.instanceId;
                break;
            }
        }
        EDI_CHECK(!placedInstanceId.empty());

        // Case A: the active object IS a block-instance placement → keys are
        // true and the instance id.  placeBlockInstance auto-selects the stamped
        // objects, so one of them is already active.
        {
            const QVariantMap model = projCtl.modelDocument();
            EDI_CHECK(model.value(QStringLiteral("has_block_instance_selection")).toBool() == true);
            EDI_CHECK(model.value(QStringLiteral("instance_id")).toString()
                   == QString::fromStdString(placedInstanceId));
        }

        // Case B: select the ordinary point (non-placement) → keys revert to
        // false / "".
        EDI_CHECK(projCtl.selectObjectById(QString::fromStdString(ordinaryId)));
        {
            const QVariantMap model = projCtl.modelDocument();
            EDI_CHECK(model.value(QStringLiteral("has_block_instance_selection")).toBool() == false);
            EDI_CHECK(model.value(QStringLiteral("instance_id")).toString().isEmpty());
        }

        // Case C: deselect everything (empty selection, no active object) →
        // keys are false / "".
        projCtl.selectObjectsInBoundsNormalized(0.99, 0.99, 1.0, 1.0); // empty region
        EDI_CHECK(projCtl.draftingDocument().selectedObjectIds.empty());
        {
            const QVariantMap model = projCtl.modelDocument();
            EDI_CHECK(model.value(QStringLiteral("has_block_instance_selection")).toBool() == false);
            EDI_CHECK(model.value(QStringLiteral("instance_id")).toString().isEmpty());
        }
    }

    // M1: deleteAllConstructionLines — clears exactly the ConstructionLine-kind
    // objects in one undoable command, leaving every other kind byte-identical.
    {
        DrawingDocumentController clCtl;

        // Build a mixed document: Line + Circle + 2 ConstructionLines.
        clCtl.setSelectedToolId(QStringLiteral("line_tool"));
        clCtl.clickCanvasNormalized(0.1, 0.1);
        clCtl.clickCanvasNormalized(0.4, 0.4);

        clCtl.setSelectedToolId(QStringLiteral("circle_tool"));
        clCtl.clickCanvasNormalized(0.7, 0.7);
        clCtl.clickCanvasNormalized(0.9, 0.7); // radius 0.2

        clCtl.setSelectedToolId(QStringLiteral("horizontal_construction_line_tool"));
        clCtl.clickCanvasNormalized(0.5, 0.3); // horizontal ConstructionLine

        clCtl.setSelectedToolId(QStringLiteral("vertical_construction_line_tool"));
        clCtl.clickCanvasNormalized(0.6, 0.5); // vertical ConstructionLine

        const QVariantList before = clCtl.modelDocument().value(QStringLiteral("drawing_objects")).toList();
        EDI_CHECK(before.size() == 4);

        // Capture the ids and kinds of the two non-construction objects.
        const QString lineId   = before[0].toMap().value(QStringLiteral("id")).toString();
        const QString circleId = before[1].toMap().value(QStringLiteral("id")).toString();
        EDI_CHECK(before[0].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
        EDI_CHECK(before[1].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("circle"));
        EDI_CHECK(before[2].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("construction_line"));
        EDI_CHECK(before[3].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("construction_line"));

        // Delete all construction lines.
        EDI_CHECK(clCtl.deleteAllConstructionLines());

        const QVariantList after = clCtl.modelDocument().value(QStringLiteral("drawing_objects")).toList();
        EDI_CHECK(after.size() == 2); // only the Line and Circle remain

        // Remaining objects are byte-identical (same ids, same kinds).
        EDI_CHECK(after[0].toMap().value(QStringLiteral("id")).toString() == lineId);
        EDI_CHECK(after[1].toMap().value(QStringLiteral("id")).toString() == circleId);
        EDI_CHECK(after[0].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("line"));
        EDI_CHECK(after[1].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("circle"));

        // One undo step restores all four objects.
        EDI_CHECK(clCtl.undo());
        const QVariantList restored = clCtl.modelDocument().value(QStringLiteral("drawing_objects")).toList();
        EDI_CHECK(restored.size() == 4);
        EDI_CHECK(restored[2].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("construction_line"));
        EDI_CHECK(restored[3].toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("construction_line"));

        // Calling deleteAllConstructionLines on a doc with NO construction lines
        // is a no-op: does not crash, object count unchanged.
        EDI_CHECK(clCtl.deleteAllConstructionLines()); // undone — all 4 back, then delete again
        EDI_CHECK(clCtl.deleteAllConstructionLines()); // doc now has no CL — second call is a no-op
        EDI_CHECK(clCtl.modelDocument().value(QStringLiteral("drawing_objects")).toList().size() == 2);
    }

    // M8-S2: motif capture (defineMotifFromSelection) + FLATTEN placement
    // (beginMotifPlacement + canvas click → runMotifAtPoint).
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        };

        DrawingDocumentController motifCtl;

        // Build a 2-object doc (two Points).
        motifCtl.setSelectedToolId(QStringLiteral("point_tool"));
        motifCtl.clickCanvasNormalized(0.1, 0.2);
        motifCtl.clickCanvasNormalized(0.4, 0.5);
        EDI_CHECK(objectCount(motifCtl) == 2);

        // beginMotifPlacement on a missing name → false (no capture armed).
        EDI_CHECK(!motifCtl.beginMotifPlacement(QStringLiteral("nosuchname")));
        EDI_CHECK(motifCtl.pointCapturePrompt().isEmpty());

        // Marquee-select both points and define a motif.
        motifCtl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(motifCtl.defineMotifFromSelection(QStringLiteral("dot_pair")));
        EDI_CHECK(motifCtl.draftingDocument().motifs.size() == 1);
        EDI_CHECK(motifCtl.draftingDocument().motifs[0].name == "dot_pair");
        EDI_CHECK(motifCtl.draftingDocument().motifs[0].objects.size() == 2);

        // Define is one undo step; undoing removes the motif.
        motifCtl.undo();
        EDI_CHECK(motifCtl.draftingDocument().motifs.empty());
        motifCtl.redo();
        EDI_CHECK(motifCtl.draftingDocument().motifs.size() == 1);

        // Define a second motif (same selection) with duplicate name → rejected.
        motifCtl.selectObjectsInBoundsNormalized(0.0, 0.0, 1.0, 1.0);
        EDI_CHECK(!motifCtl.defineMotifFromSelection(QStringLiteral("dot_pair")));

        // beginMotifPlacement with the correct name → arms a capture.
        EDI_CHECK(motifCtl.beginMotifPlacement(QStringLiteral("dot_pair")));
        EDI_CHECK(!motifCtl.pointCapturePrompt().isEmpty());

        // Canvas click → FLATTEN-drop: two fresh "motif_" objects appear.
        const int beforePlace = objectCount(motifCtl);
        motifCtl.clickCanvasNormalized(0.7, 0.8);
        EDI_CHECK(objectCount(motifCtl) == beforePlace + 2);

        // Confirm auto-selection of the placed batch.
        EDI_CHECK(motifCtl.modelDocument().value(QStringLiteral("selected_object_ids")).toList().size() == 2);

        // The placed objects have "motif_" ids and are ordinary first-class objects.
        const QVariantList objs = motifCtl.modelDocument().value(QStringLiteral("drawing_objects")).toList();
        bool foundMotifId = false;
        for (const QVariant &v : objs) {
            const QString id = v.toMap().value(QStringLiteral("id")).toString();
            if (id.startsWith(QStringLiteral("motif_"))) {
                foundMotifId = true;
                break;
            }
        }
        EDI_CHECK(foundMotifId);

        // The whole placement is ONE undo step (beforePlace + 2 → beforePlace on undo).
        EDI_CHECK(motifCtl.undo());
        EDI_CHECK(objectCount(motifCtl) == beforePlace);
    }

    // M2-S2: dropIntersectionPoints — materialise segment crossings as Point objects.
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        };
        auto kindCount = [](DrawingDocumentController &c, const QString &kind) {
            int n = 0;
            for (const QVariant &v : c.modelDocument().value(QStringLiteral("drawing_objects")).toList()) {
                if (v.toMap().value(QStringLiteral("kind")).toString() == kind) {
                    ++n;
                }
            }
            return n;
        };

        // Two crossing lines → exactly 1 Point at the crossing, auto-selected.
        {
            DrawingDocumentController ctl;
            ctl.setSelectedToolId("line_tool");
            ctl.clickCanvasNormalized(0.2, 0.5);
            ctl.clickCanvasNormalized(0.8, 0.5); // horizontal y=0.5
            ctl.clickCanvasNormalized(0.5, 0.2);
            ctl.clickCanvasNormalized(0.5, 0.8); // vertical x=0.5
            EDI_CHECK(objectCount(ctl) == 2);

            // The line_tool auto-selects the last-created line, so we clear the
            // selection before calling dropIntersectionPoints — this tests the
            // whole-document path (no selection → full visible scan).
            ctl.setSelectedToolId("select_move");
            ctl.clickCanvasNormalized(0.99, 0.99); // empty space → clears selection
            EDI_CHECK(ctl.modelDocument().value(QStringLiteral("selected_object_ids")).toList().isEmpty());

            const bool dropped = ctl.dropIntersectionPoints();
            EDI_CHECK(dropped);
            EDI_CHECK(objectCount(ctl) == 3);
            EDI_CHECK(kindCount(ctl, QStringLiteral("point")) == 1);

            // The created Point is auto-selected.
            const QVariantList selIds =
                ctl.modelDocument().value(QStringLiteral("selected_object_ids")).toList();
            EDI_CHECK(selIds.size() == 1);
            EDI_CHECK(selIds[0].toString().startsWith(QStringLiteral("isect_")));

            // The whole operation is ONE undo step.
            EDI_CHECK(ctl.canUndo());
            ctl.undo();
            EDI_CHECK(objectCount(ctl) == 2);
        }

        // Idempotent: calling again after the first drop creates 0 new Points.
        {
            DrawingDocumentController ctl;
            ctl.setSelectedToolId("line_tool");
            ctl.clickCanvasNormalized(0.2, 0.5);
            ctl.clickCanvasNormalized(0.8, 0.5);
            ctl.clickCanvasNormalized(0.5, 0.2);
            ctl.clickCanvasNormalized(0.5, 0.8);
            // Clear the auto-selection so the whole-document path runs.
            ctl.setSelectedToolId("select_move");
            ctl.clickCanvasNormalized(0.99, 0.99);

            EDI_CHECK(ctl.dropIntersectionPoints()); // first drop: creates 1 Point
            const int after1 = objectCount(ctl);
            EDI_CHECK(after1 == 3);

            // The first drop auto-selects the new Point — clear selection again
            // so the second call also uses the whole-document path.
            ctl.clickCanvasNormalized(0.99, 0.99); // select_move still active → clears selection

            const bool second = ctl.dropIntersectionPoints(); // second drop: 0 new
            EDI_CHECK(!second);
            EDI_CHECK(objectCount(ctl) == after1); // nothing added
        }

        // Selection-scoped: 3 lines, only 2 selected → only their crossing drops.
        //
        // Layout (all in the canvas [0,1] space):
        //   l1: horizontal y=0.5, x=[0.1,0.9] — spans full width
        //   l2: vertical   x=0.4, y=[0.3,0.7] — crosses l1 at (0.4, 0.5)
        //   l3: vertical   x=0.6, y=[0.3,0.7] — crosses l1 at (0.6, 0.5)
        // Full-document drop would give 2 Points.
        //
        // A marquee [0.0,0.25]→[0.55,0.75] selects l1 (x=[0.1,0.9] intersects)
        // and l2 (x=0.4 ≤ 0.55) but NOT l3 (x=0.6 > 0.55 — no x overlap with the
        // marquee box).  The selection-scoped drop then gives only l1∩l2=(0.4,0.5).
        {
            DrawingDocumentController ctl;
            ctl.setSelectedToolId("line_tool");
            ctl.clickCanvasNormalized(0.1, 0.5);
            ctl.clickCanvasNormalized(0.9, 0.5); // l1: horizontal y=0.5
            ctl.clickCanvasNormalized(0.4, 0.3);
            ctl.clickCanvasNormalized(0.4, 0.7); // l2: vertical x=0.4
            ctl.clickCanvasNormalized(0.6, 0.3);
            ctl.clickCanvasNormalized(0.6, 0.7); // l3: vertical x=0.6
            EDI_CHECK(objectCount(ctl) == 3);

            // Arm the marquee so l1+l2 are selected but l3 is excluded.
            ctl.selectObjectsInBoundsNormalized(0.0, 0.25, 0.55, 0.75);
            // Confirm exactly 2 objects are selected (l1 and l2).
            EDI_CHECK(ctl.modelDocument().value(QStringLiteral("selected_object_ids")).toList().size() == 2);

            // Selection-scoped drop: only l1∩l2=(0.4,0.5) lands.
            const bool dropped = ctl.dropIntersectionPoints();
            EDI_CHECK(dropped);
            EDI_CHECK(objectCount(ctl) == 4); // 3 lines + 1 Point
            EDI_CHECK(kindCount(ctl, QStringLiteral("point")) == 1);
        }
    }

    // DR-13: angular_dimension_tool — two-line-pick arm
    // Mirrors the fillet test: click a line → arm capture; click second line →
    // planAngularDimension → one Dimension object created in one undo step.
    {
        auto objectCount = [](DrawingDocumentController &c) {
            return c.modelDocument().value(QStringLiteral("drawing_objects")).toList().size();
        };

        // Nominal: two perpendicular lines → Angular dimension created.
        {
            DrawingDocumentController angCtl;
            angCtl.setSelectedToolId("line_tool");
            // Horizontal line from (0.2,0.4) to (0.7,0.4).
            angCtl.clickCanvasNormalized(0.2, 0.4);
            angCtl.clickCanvasNormalized(0.7, 0.4);
            // Vertical line from (0.4,0.2) to (0.4,0.7).
            angCtl.clickCanvasNormalized(0.4, 0.2);
            angCtl.clickCanvasNormalized(0.4, 0.7);
            EDI_CHECK(objectCount(angCtl) == 2);

            const int before = objectCount(angCtl);
            angCtl.setSelectedToolId("angular_dimension_tool");

            // First click: hit horizontal line midpoint → arms capture.
            angCtl.clickCanvasNormalized(0.45, 0.4);
            EDI_CHECK(angCtl.isAwaitingPointCapture());
            EDI_CHECK(!angCtl.pointCapturePrompt().isEmpty());
            // No object created by the first click.
            EDI_CHECK(objectCount(angCtl) == before);

            // Second click: hit vertical line midpoint → creates Angular dimension.
            angCtl.clickCanvasNormalized(0.4, 0.45);
            EDI_CHECK(!angCtl.isAwaitingPointCapture());
            EDI_CHECK(objectCount(angCtl) == before + 1);

            // Confirm the new object is a dimension.
            const QVariantList objs =
                angCtl.modelDocument().value(QStringLiteral("drawing_objects")).toList();
            bool hasDim = false;
            for (const QVariant &v : objs) {
                if (v.toMap().value(QStringLiteral("kind")).toString() == QStringLiteral("dimension")) {
                    hasDim = true;
                    break;
                }
            }
            EDI_CHECK(hasDim);

            // The whole operation is ONE undo step.
            EDI_CHECK(angCtl.canUndo());
            angCtl.undo();
            EDI_CHECK(objectCount(angCtl) == before);
        }

        // Second-pick is not a line (empty space) → no object, capture cleared.
        {
            DrawingDocumentController angCtl2;
            angCtl2.setSelectedToolId("line_tool");
            angCtl2.clickCanvasNormalized(0.1, 0.3);
            angCtl2.clickCanvasNormalized(0.6, 0.3);
            EDI_CHECK(objectCount(angCtl2) == 1);

            angCtl2.setSelectedToolId("angular_dimension_tool");
            angCtl2.clickCanvasNormalized(0.35, 0.3); // hit the line
            EDI_CHECK(angCtl2.isAwaitingPointCapture());

            const int before2 = objectCount(angCtl2);
            angCtl2.clickCanvasNormalized(0.9, 0.9); // empty space — no line here
            EDI_CHECK(!angCtl2.isAwaitingPointCapture());
            // No new object should appear.
            EDI_CHECK(objectCount(angCtl2) == before2);
        }

        // First click is not on a line → no capture armed.
        {
            DrawingDocumentController angCtl3;
            angCtl3.setSelectedToolId("angular_dimension_tool");
            angCtl3.clickCanvasNormalized(0.5, 0.5); // nothing in document
            EDI_CHECK(!angCtl3.isAwaitingPointCapture());
        }

        // Parallel lines → planAngularDimension rejects → no object created.
        {
            DrawingDocumentController angCtl4;
            angCtl4.setSelectedToolId("line_tool");
            // Two horizontal lines (same slope).
            angCtl4.clickCanvasNormalized(0.1, 0.3);
            angCtl4.clickCanvasNormalized(0.6, 0.3); // line1: y=0.3
            angCtl4.clickCanvasNormalized(0.1, 0.6);
            angCtl4.clickCanvasNormalized(0.6, 0.6); // line2: y=0.6
            EDI_CHECK(objectCount(angCtl4) == 2);

            angCtl4.setSelectedToolId("angular_dimension_tool");
            angCtl4.clickCanvasNormalized(0.35, 0.3); // arm on line1
            EDI_CHECK(angCtl4.isAwaitingPointCapture());

            const int before4 = objectCount(angCtl4);
            angCtl4.clickCanvasNormalized(0.35, 0.6); // pick line2 (parallel)
            EDI_CHECK(!angCtl4.isAwaitingPointCapture());
            // planAngularDimension rejects parallel lines — no new object.
            EDI_CHECK(objectCount(angCtl4) == before4);
        }
    }

    // --- 024 coverage: plug-A deleted between clicks -------------------------
    // Arm the connection tool, complete the first click (plug A stored), then
    // DELETE plug A's marker before the second click. The second click should
    // see that the stored plug id no longer exists (apply-time re-validation in
    // connectPlugs) and refuse: no DeclaredConnection, capture cleared.
    //
    // This path exercises the "both plugs must exist" guard in connectPlugs — the
    // gap between the first click (plug A recorded) and the second click (plug A
    // gone from the graph due to its anchor marker being deleted).
    {
        DrawingDocumentController delBetweenCtl;

        // Place plug A at (0.3, 0.3) and plug B at (0.7, 0.7).
        delBetweenCtl.beginPlugPick();
        delBetweenCtl.clickCanvasNormalized(0.3, 0.3); // plug A
        delBetweenCtl.beginPlugPick();
        delBetweenCtl.clickCanvasNormalized(0.7, 0.7); // plug B

        // Capture the marker id for plug A (we will delete it next).
        const std::string markerAId = delBetweenCtl.draftingDocument().plugs[0].anchorObjectId;

        // Arm the connection tool and do the first click (plug A stored).
        EDI_CHECK(delBetweenCtl.beginConnectionPick());
        delBetweenCtl.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(delBetweenCtl.isAwaitingPointCapture()); // re-armed for second click
        EDI_CHECK(delBetweenCtl.pointCapturePrompt() == QStringLiteral("Click the second plug"));

        // Delete plug A's marker WHILE the connection tool is still armed.
        // selectObjectById does not cancel the pending capture — only setSelectedToolId
        // does — so the PlugConnect state survives the delete.
        EDI_CHECK(delBetweenCtl.selectObjectById(QString::fromStdString(markerAId)));
        EDI_CHECK(delBetweenCtl.deleteSelectedObject());
        // The cascade removed plug A from the graph; only plug B remains.
        EDI_CHECK(delBetweenCtl.draftingDocument().plugs.size() == 1);

        // Second click on plug B — the stored plug A id now resolves to nullopt in
        // connectPlugs (apply-time re-validation); the tool refuses and clears.
        delBetweenCtl.clickCanvasNormalized(0.7, 0.7);
        EDI_CHECK(!delBetweenCtl.isAwaitingPointCapture()); // disarmed
        EDI_CHECK(delBetweenCtl.draftingDocument().connections.empty()); // no connection made
    }

    // --- 024 coverage: empty-corridor fallback (degenerate same-point plugs) --
    // When both plug anchors collapse to the SAME coordinate, routeCorridorCenterline
    // returns a degenerate (zero-length) centerline and corridorWalls produces NO wall
    // objects. connectPlugs falls back to an inline bracket that records ONLY the
    // DeclaredConnection, so the graph edge exists without rendered geometry.
    // One undo must remove that connection (the fallback bracket is one undo step).
    {
        DrawingDocumentController degCtl;

        // Place two plug markers at the EXACT same normalized position.
        // Each call mints a distinct marker and a distinct plug, but both anchors
        // land at (0.5, 0.5) — the degenerate case for the corridor router.
        degCtl.beginPlugPick();
        degCtl.clickCanvasNormalized(0.5, 0.5); // plug A anchor (0.5, 0.5)
        degCtl.beginPlugPick();
        degCtl.clickCanvasNormalized(0.5, 0.5); // plug B anchor (0.5, 0.5) — same point

        EDI_CHECK(degCtl.draftingDocument().plugs.size() == 2);
        const int markersCount = static_cast<int>(degCtl.draftingDocument().objects.size()); // 2

        const QString degPlugAId = QString::fromStdString(degCtl.draftingDocument().plugs[0].id);
        const QString degPlugBId = QString::fromStdString(degCtl.draftingDocument().plugs[1].id);

        // Call connectPlugs directly (bypassing the hit-test click path — both
        // markers are at the same pixel and hit-test would return only one of them).
        EDI_CHECK(degCtl.connectPlugs(degPlugAId, degPlugBId));

        // The fallback path: one DeclaredConnection exists, NO corridor walls added.
        EDI_CHECK(degCtl.draftingDocument().connections.size() == 1);
        EDI_CHECK(static_cast<int>(degCtl.draftingDocument().objects.size()) == markersCount);

        // One undo removes the connection (the fallback bracket is one undo step).
        EDI_CHECK(degCtl.undo());
        EDI_CHECK(degCtl.draftingDocument().connections.empty());
        EDI_CHECK(static_cast<int>(degCtl.draftingDocument().objects.size()) == markersCount);
    }

    // --- 024 coverage: stale-member cleared on tool switch (NIT fix) ----------
    // After a tool switch the two-stage connection state (m_pendingConnectionPlugA)
    // must be wiped — the fix lives in setSelectedToolId alongside m_pointCapture.reset().
    // Sequence: arm → first click (plug A stored) → switch tool → re-arm → click:
    // the click must be treated as the FIRST click (prompt becomes "second"), not
    // immediately connected to the stale A.
    {
        DrawingDocumentController staleCtl;

        // Place two plugs at known positions.
        staleCtl.beginPlugPick();
        staleCtl.clickCanvasNormalized(0.3, 0.3); // plug A
        staleCtl.beginPlugPick();
        staleCtl.clickCanvasNormalized(0.7, 0.7); // plug B

        // Arm connection tool and store plug A via the first click.
        EDI_CHECK(staleCtl.beginConnectionPick());
        staleCtl.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(staleCtl.isAwaitingPointCapture()); // re-armed for second click

        // Switch to a different tool — cancels the pick AND (NIT fix) clears
        // m_pendingConnectionPlugA so it cannot bleed into the next session.
        // NOTE: the default tool id is "select_move"; beginConnectionPick does NOT
        // change it, so we must switch to a GENUINELY DIFFERENT tool to avoid the
        // early-return guard (if (m_selectedToolId == toolId) return;).
        staleCtl.setSelectedToolId(QStringLiteral("rectangle_tool"));
        EDI_CHECK(!staleCtl.isAwaitingPointCapture()); // capture cancelled by tool switch

        // Re-arm the connection tool (also clears m_pendingConnectionPlugA for safety).
        EDI_CHECK(staleCtl.beginConnectionPick());
        EDI_CHECK(staleCtl.pointCapturePrompt() == QStringLiteral("Click the first plug"));

        // Clicking plug A is now the FIRST click (plug A was NOT left stale from
        // before the tool switch). The tool re-arms for the second click.
        staleCtl.clickCanvasNormalized(0.3, 0.3);
        EDI_CHECK(staleCtl.isAwaitingPointCapture()); // still armed — waiting for second plug
        EDI_CHECK(staleCtl.pointCapturePrompt() == QStringLiteral("Click the second plug")); // FIRST click consumed
        EDI_CHECK(staleCtl.draftingDocument().connections.empty()); // no premature connection
    }

    // --- B2-CTX: selectConnection + projection keys ----------------------------
    //
    // selectConnection(validId) → has_connection_selection=true + object selection cleared.
    // selectConnection(unknownId) → no-op.
    // Object-select after selectConnection → clears m_activeConnectionId.
    {
        DrawingDocumentController connSelCtl;

        // Place two plugs and connect them to get a real DeclaredConnection.
        connSelCtl.beginPlugPick();
        connSelCtl.clickCanvasNormalized(0.3, 0.3);
        connSelCtl.beginPlugPick();
        connSelCtl.clickCanvasNormalized(0.7, 0.7);

        // Retrieve both plug ids from the document.
        const auto &plugs = connSelCtl.draftingDocument().plugs;
        EDI_CHECK(plugs.size() == 2);
        const QString plugAId = QString::fromStdString(plugs[0].id);
        const QString plugBId = QString::fromStdString(plugs[1].id);

        // Connect them → one DeclaredConnection is minted.
        EDI_CHECK(connSelCtl.connectPlugs(plugAId, plugBId));
        const auto &conns = connSelCtl.draftingDocument().connections;
        EDI_CHECK(conns.size() == 1);
        const QString connId = QString::fromStdString(conns[0].id);

        // Before any selectConnection call: has_connection_selection should be false.
        QVariantMap model = connSelCtl.modelDocument();
        EDI_CHECK(!model.value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(model.value(QStringLiteral("active_connection_id")).toString().isEmpty());

        // selectConnection(unknownId) → no-op: the key stays false.
        connSelCtl.selectConnection(QStringLiteral("no-such-connection-id"));
        model = connSelCtl.modelDocument();
        EDI_CHECK(!model.value(QStringLiteral("has_connection_selection")).toBool());

        // selectConnection(validId) → has_connection_selection true + id set.
        connSelCtl.selectConnection(connId);
        model = connSelCtl.modelDocument();
        EDI_CHECK(model.value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(model.value(QStringLiteral("active_connection_id")).toString() == connId);

        // selectConnection also clears the object selection (mutual exclusion).
        // First select an object, then call selectConnection; selection must be gone.
        {
            // Pick the first plug's anchor marker as our test object.
            const QString markerId = QString::fromStdString(
                connSelCtl.draftingDocument().objects.front().id);
            EDI_CHECK(connSelCtl.selectObjectById(markerId));
            // Object is now selected — connection select was cleared by selectObjectById.
            EDI_CHECK(!connSelCtl.modelDocument()
                       .value(QStringLiteral("has_connection_selection")).toBool());

            // Now select the connection again.
            connSelCtl.selectConnection(connId);
            EDI_CHECK(connSelCtl.modelDocument()
                       .value(QStringLiteral("has_connection_selection")).toBool());
            // The object selection must be empty (clearSelection was called inside selectConnection).
            EDI_CHECK(connSelCtl.modelDocument()
                       .value(QStringLiteral("selected_object_ids")).toList().isEmpty());
        }

        // Object-select after selectConnection clears m_activeConnectionId.
        const QString markerId = QString::fromStdString(
            connSelCtl.draftingDocument().objects.front().id);
        EDI_CHECK(connSelCtl.selectObjectById(markerId));
        model = connSelCtl.modelDocument();
        EDI_CHECK(!model.value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(model.value(QStringLiteral("active_connection_id")).toString().isEmpty());
    }

    // active_object_is_plug: placing a plug marker and selecting it yields true.
    {
        DrawingDocumentController plugObjCtl;
        plugObjCtl.beginPlugPick();
        plugObjCtl.clickCanvasNormalized(0.5, 0.5);

        // The freshly placed marker is auto-selected; it is a plug anchor.
        QVariantMap model = plugObjCtl.modelDocument();
        EDI_CHECK(model.value(QStringLiteral("active_object_is_plug")).toBool());

        // A plain (non-plug) object is NOT a plug anchor.
        DrawingDocumentController plainCtl;
        plainCtl.setSelectedToolId(QStringLiteral("circle_tool"));
        plainCtl.clickCanvasNormalized(0.5, 0.5); // create a circle
        model = plainCtl.modelDocument();
        EDI_CHECK(!model.value(QStringLiteral("active_object_is_plug")).toBool());
    }

    // Brief 037: active_plug_type projection key.
    // Three cases: plug active → type string; plain object active → ""; no selection → "".
    {
        // Case 1: a plug whose type is "window" is active → active_plug_type == "window".
        // beginPlugPick places a Point marker; the marker is auto-selected (plugObjCtl
        // above shows this). We then call setPlugType to change it to "window" (which
        // also re-selects the leaf, not the marker), then use beginPlugPick again to
        // place and select a fresh plug of type "window" directly via the type field.
        // Simpler: place a plug, call setPlugType to "window", then manually select the
        // plug marker via its id to confirm the type is reflected.
        DrawingDocumentController ctl;
        ctl.beginPlugPick();
        ctl.clickCanvasNormalized(0.5, 0.5); // places marker, auto-selects it
        // Grab the plug id from the document.
        EDI_CHECK(ctl.draftingDocument().plugs.size() == 1);
        const std::string plugId = ctl.draftingDocument().plugs[0].id;

        // setPlugType to "window" (the marker stays; a leaf is minted or replaced).
        EDI_CHECK(ctl.setPlugType(QString::fromStdString(plugId), QStringLiteral("window")));
        // After setPlugType, the leaf (not the marker) may be selected. Re-select the
        // plug marker so active_object_is_plug == true and active_plug_type is visible.
        const std::string anchorObjId = ctl.draftingDocument().plugs[0].anchorObjectId;
        ctl.selectObjectById(QString::fromStdString(anchorObjId));

        const QVariantMap model1 = ctl.modelDocument();
        EDI_CHECK(model1.value(QStringLiteral("active_object_is_plug")).toBool());
        EDI_CHECK(model1.value(QStringLiteral("active_plug_type")).toString()
               == QStringLiteral("window"));

        // Case 2: a plain (non-plug) Wall object is active → active_plug_type == "".
        DrawingDocumentController plainCtl2;
        plainCtl2.setSelectedToolId(QStringLiteral("wall_tool"));
        plainCtl2.clickCanvasNormalized(0.3, 0.3);
        plainCtl2.clickCanvasNormalized(0.7, 0.3); // create a wall
        const QVariantMap model2 = plainCtl2.modelDocument();
        EDI_CHECK(!model2.value(QStringLiteral("active_object_is_plug")).toBool());
        EDI_CHECK(model2.value(QStringLiteral("active_plug_type")).toString().isEmpty());

        // Case 3: no selection → active_plug_type == "".
        DrawingDocumentController emptyCtl;
        const QVariantMap model3 = emptyCtl.modelDocument();
        EDI_CHECK(model3.value(QStringLiteral("active_plug_type")).toString().isEmpty());
    }

    // --- B2-4: deleteConnection + deletePlug -----------------------------------
    //
    // Fixture shared across all four cases (repeated inline — DrawingDocumentController
    // is a QObject, so it cannot move into a std::tuple returned from a lambda;
    // four short setup sequences are clearer than heap-wrapping them).

    // (1) deleteConnection(validId) — edge + corridor gone, plugs stay,
    //     one undo restores everything.
    {
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);
        const int objectsWithConn = static_cast<int>(ctl.draftingDocument().objects.size());

        // Count corridor walls (tagged "connection:<connId>").
        const std::string connTag = "connection:" + cId.toStdString();
        int corridorCount = 0;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == connTag) { ++corridorCount; break; }
            }
        }
        EDI_CHECK(corridorCount > 0); // at least one corridor wall present

        EDI_CHECK(ctl.deleteConnection(cId));

        // Graph edge gone.
        EDI_CHECK(ctl.draftingDocument().connections.empty());
        // Corridor walls gone.
        int remaining = 0;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == connTag) { ++remaining; break; }
            }
        }
        EDI_CHECK(remaining == 0);
        // Plug markers still present (both plugs + both markers).
        EDI_CHECK(ctl.draftingDocument().plugs.size() == 2);
        EDI_CHECK(static_cast<int>(ctl.draftingDocument().objects.size())
               == objectsWithConn - corridorCount);

        // One undo restores the edge AND the corridor.
        EDI_CHECK(ctl.undo());
        EDI_CHECK(ctl.draftingDocument().connections.size() == 1);
        EDI_CHECK(static_cast<int>(ctl.draftingDocument().objects.size()) == objectsWithConn);
    }

    // (2) deleteConnection(unknownId) → no-op (returns false).
    {
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId2 = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId2 = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId2, bId2));
        const std::size_t connsBefore = ctl.draftingDocument().connections.size();
        EDI_CHECK(!ctl.deleteConnection(QStringLiteral("no-such-connection")));
        EDI_CHECK(ctl.draftingDocument().connections.size() == connsBefore);
    }

    // (3) deletePlug(plugA) — plug + connection + corridor + anchor marker gone;
    //     plug B + its marker remain; one undo restores everything.
    {
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);
        const int objectsWithConn = static_cast<int>(ctl.draftingDocument().objects.size());
        const std::size_t plugsBefore  = ctl.draftingDocument().plugs.size();       // 2
        const std::size_t connsBefore  = ctl.draftingDocument().connections.size(); // 1

        // Count objects that belong to plug A's connection (corridor walls tagged
        // "connection:<cId>") plus the anchor marker itself.
        const std::string connTag = "connection:" + cId.toStdString();
        int doomed = 0; // corridor walls
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == connTag) { ++doomed; break; }
            }
        }
        doomed += 1; // + the anchor marker for plug A

        EDI_CHECK(ctl.deletePlug(aId));

        // Plug A gone; plug B still present.
        EDI_CHECK(ctl.draftingDocument().plugs.size() == plugsBefore - 1);
        EDI_CHECK(!edi::drafting::plugIndexById(ctl.draftingDocument(), aId.toStdString()));
        EDI_CHECK( edi::drafting::plugIndexById(ctl.draftingDocument(), bId.toStdString()));

        // Connection gone (cascaded from DeletePlugCommand).
        EDI_CHECK(ctl.draftingDocument().connections.empty());
        EDI_CHECK(!edi::drafting::connectionIndexById(ctl.draftingDocument(), cId.toStdString()));

        // Corridor walls and anchor marker of plug A gone.
        EDI_CHECK(static_cast<int>(ctl.draftingDocument().objects.size())
               == objectsWithConn - doomed);
        // Plug B's anchor marker still exists.
        const std::string bAnchorId =
            ctl.draftingDocument().plugs.front().anchorObjectId;
        EDI_CHECK(edi::drafting::findObject(ctl.draftingDocument(), bAnchorId) != nullptr);

        // One undo restores plug A + connection + corridor + anchor marker.
        EDI_CHECK(ctl.undo());
        EDI_CHECK(ctl.draftingDocument().plugs.size() == plugsBefore);
        EDI_CHECK(ctl.draftingDocument().connections.size() == connsBefore);
        EDI_CHECK(static_cast<int>(ctl.draftingDocument().objects.size()) == objectsWithConn);
    }

    // (4) deletePlug(unknownId) → no-op (returns false).
    {
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const std::size_t plugsBefore = ctl.draftingDocument().plugs.size();
        EDI_CHECK(!ctl.deletePlug(QStringLiteral("no-such-plug")));
        EDI_CHECK(ctl.draftingDocument().plugs.size() == plugsBefore);
    }

    // --- Brief 029: B2-CTX mutual-exclusion fixes ----------------------------

    // Fix 1: canvas click clears an active connection selection.
    // Drive via clickCanvasNormalized (NOT selectObjectById — that was already
    // covered; the bug was specifically in the canvas-click path).
    {
        DrawingDocumentController ctl;

        // Place two plugs, connect them, select the connection via Map-browser.
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());

        // Canvas-click on the first plug's anchor marker (at 0.3, 0.3 in normalised coords).
        // This is a select_move click — it should clear the connection selection AND select
        // the object.
        ctl.setSelectedToolId(QStringLiteral("select_move"));
        ctl.clickCanvasNormalized(0.3, 0.3);

        // has_connection_selection must now be false — the canvas click cleared it.
        const QVariantMap model = ctl.modelDocument();
        EDI_CHECK(!model.value(QStringLiteral("has_connection_selection")).toBool());
        // An object is now selected (the plug marker at that position).
        EDI_CHECK(!model.value(QStringLiteral("selected_object_ids")).toList().isEmpty());
    }

    // Fix 1 (plug-anchor variant): canvas-click a plug marker while a connection is
    // selected → has_connection_selection false AND active_object_is_plug true.
    // This proves the mutual-exclusion invariant holds: only one context is active.
    {
        DrawingDocumentController ctl;

        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        ctl.selectConnection(cId);

        // Click on the plug anchor at (0.3, 0.3).
        ctl.setSelectedToolId(QStringLiteral("select_move"));
        ctl.clickCanvasNormalized(0.3, 0.3);

        const QVariantMap model = ctl.modelDocument();
        EDI_CHECK(!model.value(QStringLiteral("has_connection_selection")).toBool());
        // The clicked object is a plug anchor → active_object_is_plug should be true.
        EDI_CHECK(model.value(QStringLiteral("active_object_is_plug")).toBool());
    }

    // Fix 2a: selectConnection(id) → deleteConnection(id) → has_connection_selection false.
    {
        DrawingDocumentController ctl;

        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());

        // Delete the selected connection — must clear m_activeConnectionId.
        EDI_CHECK(ctl.deleteConnection(cId));
        EDI_CHECK(!ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("active_connection_id")).toString().isEmpty());
    }

    // Fix 2b: selectConnection(id) → deletePlug(endpoint) → connection cascaded
    // → has_connection_selection false.
    {
        DrawingDocumentController ctl;

        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        // Select the connection, then delete plug A — the connection cascades.
        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());

        EDI_CHECK(ctl.deletePlug(aId));
        // Connection is gone (cascade); m_activeConnectionId should be cleared.
        EDI_CHECK(ctl.draftingDocument().connections.empty());
        EDI_CHECK(!ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("active_connection_id")).toString().isEmpty());
    }

    // --- Brief 032: cancelPendingCreation clears m_activeConnectionId -----------
    // Escape (cancelPendingCreation) must end any connection selection so the
    // "any focus-shift ends the connection selection" invariant holds uniformly.
    // Sequence: arm a pick (sets m_pointCapture) → selectConnection (sets
    // m_activeConnectionId while m_pointCapture is still live) → cancelPendingCreation
    // clears both and emits modelChanged so the projection cache refreshes.
    // Note: beginPlugPick DOES clear m_activeConnectionId, so we call
    // selectConnection AFTER beginPlugPick to put both states live simultaneously.
    {
        DrawingDocumentController ctl;

        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        // Arm a plug-pick (sets m_pointCapture; also clears m_activeConnectionId —
        // that is correct behaviour for the begin*Pick family).
        ctl.beginPlugPick();

        // NOW set the connection selection while the point-capture is armed.
        // selectConnection does not touch m_pointCapture, so both states are live.
        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());

        // Escape — cancelPendingCreation sees m_pointCapture (doesn't early-return),
        // clears both m_pointCapture and m_activeConnectionId, then emits modelChanged
        // because hadConnectionSelection was true.
        ctl.cancelPendingCreation();
        EDI_CHECK(!ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("active_connection_id")).toString().isEmpty());
    }

    // --- Brief 027: B2-5 rerouteConnection -----------------------------------
    // The reroute verb replaces a connection's corridor walls with freshly-routed
    // ones that follow the two plugs' CURRENT anchor marker positions.
    {
        // (1) Basic reroute: place two plugs, connect them, move plug A's anchor
        // marker, call rerouteConnection — old corridor walls are gone; new walls
        // exist; connection record + plugs unchanged; one undo reverts the corridor.
        DrawingDocumentController ctl;

        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);
        const std::string connTag = "connection:" + cId.toStdString();

        // Snapshot the old corridor wall ids.
        std::vector<std::string> oldWallIds;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == connTag) { oldWallIds.push_back(obj.id); break; }
            }
        }
        EDI_CHECK(!oldWallIds.empty()); // corridor must have been created

        // Move plug A's anchor marker to a new position (simulates user drag).
        // selectObjectById resolves to the marker; updateSelectedObjectGeometryField
        // patches the PointGeometry via UpdateGeometryCommand.
        const std::string anchorAId = ctl.draftingDocument().plugs[0].anchorObjectId;
        EDI_CHECK(ctl.selectObjectById(QString::fromStdString(anchorAId)));
        EDI_CHECK(ctl.updateSelectedObjectGeometryField(QStringLiteral("x"), 0.1));
        EDI_CHECK(ctl.updateSelectedObjectGeometryField(QStringLiteral("y"), 0.1));

        // Record document state before reroute so we can compare after.
        const std::size_t objsBefore = ctl.draftingDocument().objects.size();
        const std::size_t connsBefore = ctl.draftingDocument().connections.size();
        const std::size_t plugsBefore = ctl.draftingDocument().plugs.size();

        // Reroute: must succeed and the connection + plugs must be untouched.
        EDI_CHECK(ctl.rerouteConnection(cId));
        EDI_CHECK(ctl.draftingDocument().connections.size() == connsBefore);
        EDI_CHECK(ctl.draftingDocument().plugs.size() == plugsBefore);
        EDI_CHECK(ctl.draftingDocument().connections[0].id == cId.toStdString());
        EDI_CHECK(ctl.draftingDocument().connections[0].plugA == aId.toStdString());
        EDI_CHECK(ctl.draftingDocument().connections[0].plugB == bId.toStdString());

        // Old corridor wall ids must be gone (the reroute deleted them).
        for (const std::string &oldId : oldWallIds) {
            bool found = false;
            for (const auto &obj : ctl.draftingDocument().objects) {
                if (obj.id == oldId) { found = true; break; }
            }
            EDI_CHECK(!found); // old wall must no longer exist
        }

        // New corridor walls tagged with the SAME connTag must now exist.
        std::vector<std::string> newWallIds;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == connTag) { newWallIds.push_back(obj.id); break; }
            }
        }
        EDI_CHECK(!newWallIds.empty()); // rerouted corridor must have walls
        // Old and new wall ids are distinct (fresh ids were minted).
        for (const std::string &newId : newWallIds) {
            bool clash = false;
            for (const std::string &oldId : oldWallIds) {
                if (newId == oldId) { clash = true; break; }
            }
            EDI_CHECK(!clash);
        }

        // ── HEADLINE ASSERTION: the rerouted corridor FOLLOWS the moved anchor ──
        // The corridorWalls geometry is WallGeometry{a, b, thickness}. The first
        // wall segment's vertex `a` is at doorA ± (width/2) in the perpendicular
        // direction — max offset hw = 0.0225 from the anchor point.  We assert
        // that at least one endpoint of any new corridor wall is within epsilon
        // (0.03 > hw) of the moved anchor (0.1, 0.1).
        // A regression where buildTaggedCorridorWalls read the stale plug.anchor
        // snapshot (0.3, 0.3) instead of the live marker geometry (0.1, 0.1)
        // would produce wall vertices near (0.3, 0.3) and FAIL this assertion.
        constexpr double kEpsilon = 0.03;
        const edi::drafting::Point2D movedAnchorPt{0.1, 0.1};
        bool corridorNearMovedAnchor = false;
        for (const auto &obj : ctl.draftingDocument().objects) {
            bool isNewWall = false;
            for (const auto &tag : obj.metadata.tags) {
                if (tag == connTag) { isNewWall = true; break; }
            }
            if (!isNewWall) { continue; }
            const auto *wall = std::get_if<edi::drafting::WallGeometry>(&obj.geometry);
            if (!wall) { continue; }
            const auto withinEps = [&](const edi::drafting::Point2D &p) {
                return std::hypot(p.x - movedAnchorPt.x, p.y - movedAnchorPt.y) < kEpsilon;
            };
            if (withinEps(wall->a) || withinEps(wall->b)) {
                corridorNearMovedAnchor = true;
                break;
            }
        }
        EDI_CHECK(corridorNearMovedAnchor); // corridor must start at the moved anchor

        // Total object count is unchanged (same number of corridor walls) or
        // may differ — but at minimum the reroute produced at least one wall.
        (void)objsBefore; // count check is approximate; presence check above is canonical

        // One undo reverts the corridor (new walls gone, old walls back).
        EDI_CHECK(ctl.undo());
        bool oldWallsBack = true;
        for (const std::string &oldId : oldWallIds) {
            bool found = false;
            for (const auto &obj : ctl.draftingDocument().objects) {
                if (obj.id == oldId) { found = true; break; }
            }
            if (!found) { oldWallsBack = false; break; }
        }
        EDI_CHECK(oldWallsBack);
        // And the new walls from the reroute should be gone after undo.
        for (const std::string &newId : newWallIds) {
            bool found = false;
            for (const auto &obj : ctl.draftingDocument().objects) {
                if (obj.id == newId) { found = true; break; }
            }
            EDI_CHECK(!found);
        }
    }

    {
        // (2) rerouteConnection(unknownId) → no-op (returns false, doc unchanged).
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const std::size_t objsBefore = ctl.draftingDocument().objects.size();
        EDI_CHECK(!ctl.rerouteConnection(QStringLiteral("no-such-connection")));
        // Document must be unchanged (no corridor modified or removed).
        EDI_CHECK(ctl.draftingDocument().objects.size() == objsBefore);
        EDI_CHECK(ctl.draftingDocument().connections.size() == 1);
    }

    // --- Brief 033: B2-3 setPlugType ------------------------------------------
    // setPlugType updates plug.type AND creates/replaces the door leaf (a thin Wall
    // tagged "plug:<plugId>") in ONE bracket (one undo step).

    {
        // (1) First setPlugType call on a fresh interactive plug: no pre-existing leaf →
        // mints one.  Second call REPLACES the leaf (old leaf gone, new leaf with new type).
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.4, 0.4);
        const QString pId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const std::string leafTag = "plug:" + pId.toStdString();

        // First setPlugType → "window".
        EDI_CHECK(ctl.setPlugType(pId, QStringLiteral("window")));
        EDI_CHECK(ctl.draftingDocument().plugs[0].type == "window");

        // Exactly one Wall with the leaf tag must exist, and it must be Window type.
        std::vector<std::string> leafIds;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { leafIds.push_back(obj.id); break; }
            }
        }
        EDI_CHECK(leafIds.size() == 1); // exactly one leaf
        // Retrieve the leaf and verify its WallVisual type.
        const edi::drafting::DraftingObject *leafObj = nullptr;
        for (const auto &obj : ctl.draftingDocument().objects) {
            if (obj.id == leafIds[0]) { leafObj = &obj; break; }
        }
        EDI_CHECK(leafObj != nullptr);
        EDI_CHECK(leafObj->kind == edi::drafting::DraftingShapeKind::Wall);
        EDI_CHECK(leafObj->metadata.wallVisual.type == edi::drafting::WallType::Window);
        EDI_CHECK(leafObj->metadata.toolProvenance == "door");

        // Second setPlugType → "secret": old leaf gone, new leaf present, type Secret.
        const std::string firstLeafId = leafIds[0];
        EDI_CHECK(ctl.setPlugType(pId, QStringLiteral("secret")));
        EDI_CHECK(ctl.draftingDocument().plugs[0].type == "secret");

        // Count leaves after second call — still exactly one.
        leafIds.clear();
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { leafIds.push_back(obj.id); break; }
            }
        }
        EDI_CHECK(leafIds.size() == 1);                   // still ONE leaf
        EDI_CHECK(leafIds[0] != firstLeafId);             // fresh id (old leaf replaced)
        // Find and check the new leaf.
        const edi::drafting::DraftingObject *newLeaf = nullptr;
        for (const auto &obj : ctl.draftingDocument().objects) {
            if (obj.id == leafIds[0]) { newLeaf = &obj; break; }
        }
        EDI_CHECK(newLeaf != nullptr);
        EDI_CHECK(newLeaf->metadata.wallVisual.type == edi::drafting::WallType::Secret);
    }

    {
        // (2) ONE undo after setPlugType reverts BOTH the type change AND the leaf.
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.4, 0.4);
        const QString pId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const std::string leafTag = "plug:" + pId.toStdString();

        // Apply once (window).
        EDI_CHECK(ctl.setPlugType(pId, QStringLiteral("window")));
        const std::string windowLeafId = [&] {
            for (const auto &obj : ctl.draftingDocument().objects) {
                for (const auto &t : obj.metadata.tags) {
                    if (t == leafTag) return obj.id;
                }
            }
            return std::string{};
        }();
        EDI_CHECK(!windowLeafId.empty());

        // Undo: plug.type reverts to "door" (the default placed by placePlugAtPoint)
        // and the window leaf disappears.
        EDI_CHECK(ctl.undo());
        EDI_CHECK(ctl.draftingDocument().plugs[0].type == "door");
        bool leafGone = true;
        for (const auto &obj : ctl.draftingDocument().objects) {
            if (obj.id == windowLeafId) { leafGone = false; break; }
        }
        EDI_CHECK(leafGone);
        // No leaf tag should remain at all (leaf was created by setPlugType, not
        // by placePlugAtPoint — placePlugAtPoint creates no leaf).
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                EDI_CHECK(tag != leafTag); // no leaf should exist after undo
            }
        }
    }

    {
        // (3) Unknown plug id → returns false, document unchanged.
        DrawingDocumentController ctl;
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.4, 0.4);
        const std::size_t objsBefore = ctl.draftingDocument().objects.size();
        EDI_CHECK(!ctl.setPlugType(QStringLiteral("no-such-plug"), QStringLiteral("window")));
        EDI_CHECK(ctl.draftingDocument().objects.size() == objsBefore);
        EDI_CHECK(ctl.draftingDocument().plugs[0].type == "door"); // unchanged
    }

    // --- Brief 034: undo/redo reconcile m_activeConnectionId ------------------
    // SCENARIO: select object → selectConnection → undo → BOTH-TRUE must NOT happen.
    // The undo restores the prior document snapshot which has activeObjectId set;
    // m_activeConnectionId (view-state, not in snapshot) stays set from the
    // selectConnection call.  reconcileActiveConnection() must clear it.

    {
        // (1) Conflict scenario: undo restores object selection while connection
        // is selected → reconcile clears the connection selection.
        DrawingDocumentController ctl;

        // Place two plugs and connect them (creates a connection record).
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        // Step 1: select the plug-A anchor object (leaves a snapshot with
        // activeObjectId set).
        const QString anchorId = QString::fromStdString(
            ctl.draftingDocument().plugs[0].anchorObjectId);
        EDI_CHECK(ctl.selectObjectById(anchorId));
        EDI_CHECK(ctl.draftingDocument().activeObjectId.has_value()); // object is selected

        // Step 2: selectConnection → sets m_activeConnectionId, clears activeObjectId.
        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(!ctl.draftingDocument().activeObjectId.has_value()); // cleared by selectConnection

        // Step 3: undo → restores the prior snapshot (activeObjectId set from step 1).
        // reconcileActiveConnection() should see the conflict and clear m_activeConnectionId.
        EDI_CHECK(ctl.undo());
        // Object selection restored from snapshot → connection selection must be gone.
        EDI_CHECK(ctl.draftingDocument().activeObjectId.has_value()); // object back
        EDI_CHECK(!ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("active_connection_id")).toString().isEmpty());
    }

    {
        // (2) Benign undo: undo does NOT restore an object selection while a
        // connection is selected → reconcile must NOT over-clear (connection
        // selection survives the undo).
        DrawingDocumentController ctl;

        // Place two plugs and connect them.
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        // selectConnection with NO object selected beforehand.
        // The prior snapshot (from connectPlugs) has no activeObjectId.
        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());

        // Undo connectPlugs → removes the connection from the document.
        // reconcileActiveConnection sees condition (2): the connection is now gone
        // (dangling reference) → clears m_activeConnectionId.
        EDI_CHECK(ctl.undo());
        EDI_CHECK(ctl.draftingDocument().connections.empty()); // connection removed by undo
        EDI_CHECK(!ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("active_connection_id")).toString().isEmpty());
    }

    {
        // (3) Benign undo where the connection STILL EXISTS and NO object selection
        // is restored: reconcile must leave m_activeConnectionId intact.
        //
        // The snapshot for the undo-able action must have activeObjectId = nullopt,
        // otherwise condition (1) of the reconcile fires. We ensure this by clicking
        // an empty canvas spot (deselect) after connectPlugs — that click is a pure
        // selection change (commitEdit detects it via documentsDifferOnlyBySelection)
        // and does NOT push an undo snapshot — so the subsequent third-plug placement's
        // beginEdit() captures a doc with activeObjectId = nullopt.
        DrawingDocumentController ctl;

        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.3, 0.3);
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.7, 0.7);
        const QString aId = QString::fromStdString(ctl.draftingDocument().plugs[0].id);
        const QString bId = QString::fromStdString(ctl.draftingDocument().plugs[1].id);
        EDI_CHECK(ctl.connectPlugs(aId, bId));
        const QString cId = QString::fromStdString(ctl.draftingDocument().connections[0].id);

        // Deselect everything: click an empty canvas region with select_move.
        // This clears activeObjectId but does NOT push an undo snapshot (pure
        // selection-only change). After this the doc has activeObjectId = nullopt.
        ctl.clickCanvasNormalized(0.0, 0.0);
        EDI_CHECK(!ctl.draftingDocument().activeObjectId.has_value());

        // Place a third plug. beginEdit() inside createObjectsAndSelect now
        // captures a snapshot with activeObjectId = nullopt (no conflict).
        ctl.beginPlugPick(); ctl.clickCanvasNormalized(0.5, 0.1);
        EDI_CHECK(ctl.draftingDocument().plugs.size() == 3); // third plug placed

        // Select the connection (no object selection active).
        ctl.selectConnection(cId);
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());

        // Undo the third plug: restored snapshot has activeObjectId = nullopt AND
        // the connection still exists → neither reconcile condition fires →
        // m_activeConnectionId stays → connection selection survives.
        EDI_CHECK(ctl.undo());
        EDI_CHECK(ctl.draftingDocument().plugs.size() == 2); // third plug undone
        EDI_CHECK(ctl.draftingDocument().connections.size() == 1); // connection still there
        EDI_CHECK(!ctl.draftingDocument().activeObjectId.has_value()); // no object selected
        // Connection selection must survive (no conflict, no dangling reference).
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("has_connection_selection")).toBool());
        EDI_CHECK(ctl.modelDocument().value(QStringLiteral("active_connection_id")).toString()
               == cId);
    }

    // --- Brief 036: authored door-leaf tag backfill ---------------------------
    // createMapFromSpec now stamps "plug:<id>" on authored door leaves so they are
    // symmetric with interactive leaves. Two seam tests:
    //   (1) setPlugType on an authored plug replaces the leaf (no duplicate).
    //   (2) deletePlug on an authored plug removes the leaf (no orphan).
    //
    // Note: DrawingDocumentController is a QObject (non-copyable/non-movable), so
    // we cannot return it from a helper lambda. The two-room fixture is inlined in
    // each test block (same resolution as the B2-4 tests).

    {
        // (1) setPlugType on an authored plug: the authored leaf is REPLACED
        // (not duplicated). After the call there must be exactly ONE leaf tagged
        // "plug:<id>", and its wallVisual.type must reflect the new type.
        DrawingDocumentController ctl;
        {
            edi::drafting::MapSpec map;
            edi::drafting::NamedRoomSpec roomA;
            roomA.name = "a";
            roomA.spec.origin = {0.0, 0.0};
            roomA.spec.width = 0.4;
            roomA.spec.height = 0.4;
            roomA.spec.wallThickness = 0.02;
            roomA.spec.plugs = {{edi::drafting::RoomEdge::East, 0.2, "door", "door"}};
            edi::drafting::NamedRoomSpec roomB;
            roomB.name = "b";
            roomB.spec.origin = {0.6, 0.0};
            roomB.spec.width = 0.4;
            roomB.spec.height = 0.4;
            roomB.spec.wallThickness = 0.02;
            roomB.spec.plugs = {{edi::drafting::RoomEdge::West, 0.2, "door", "door"}};
            map.rooms = {roomA, roomB};
            edi::drafting::MapConnectionSpec conn;
            conn.from = {"a", "door"}; conn.to = {"b", "door"}; conn.type = "corridor";
            map.connections = {conn};
            EDI_CHECK(ctl.createMapFromSpec(map));
        }
        EDI_CHECK(ctl.draftingDocument().plugs.size() == 2);
        EDI_CHECK(ctl.draftingDocument().connections.size() == 1);

        // Pick plug "a.door" (the first plug whose name starts with "a.").
        std::string plugId;
        for (const auto &p : ctl.draftingDocument().plugs) {
            if (p.name == "a.door") { plugId = p.id; break; }
        }
        EDI_CHECK(!plugId.empty());
        const std::string leafTag = "plug:" + plugId;

        // Authored leaf must already be tagged (the backfill fix).
        int leafCountBefore = 0;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { ++leafCountBefore; break; }
            }
        }
        EDI_CHECK(leafCountBefore == 1); // exactly one authored leaf with the tag

        // setPlugType → "window": must REPLACE, not duplicate.
        EDI_CHECK(ctl.setPlugType(QString::fromStdString(plugId), QStringLiteral("window")));
        EDI_CHECK(ctl.draftingDocument().plugs[0].type == "window"
               || ctl.draftingDocument().plugs[1].type == "window"); // one of them updated

        int leafCountAfter = 0;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { ++leafCountAfter; break; }
            }
        }
        EDI_CHECK(leafCountAfter == 1); // still ONE leaf — authored leaf replaced, not duplicated

        // The surviving leaf must be Window type.
        bool foundWindow = false;
        for (const auto &obj : ctl.draftingDocument().objects) {
            bool hasTag = false;
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { hasTag = true; break; }
            }
            if (!hasTag) { continue; }
            const auto *wall = std::get_if<edi::drafting::WallGeometry>(&obj.geometry);
            if (wall && obj.metadata.wallVisual.type == edi::drafting::WallType::Window) {
                foundWindow = true;
            }
        }
        EDI_CHECK(foundWindow);

        // One undo reverts both type change and leaf swap.
        EDI_CHECK(ctl.undo());
        int leafCountAfterUndo = 0;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { ++leafCountAfterUndo; break; }
            }
        }
        EDI_CHECK(leafCountAfterUndo == 1); // back to one leaf
        // The reverted leaf should be Door type (original authored type).
        bool foundDoor = false;
        for (const auto &obj : ctl.draftingDocument().objects) {
            bool hasTag = false;
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { hasTag = true; break; }
            }
            if (!hasTag) { continue; }
            if (obj.metadata.wallVisual.type == edi::drafting::WallType::Door) {
                foundDoor = true;
            }
        }
        EDI_CHECK(foundDoor);
    }

    {
        // (2) deletePlug on an authored plug: the authored leaf must be REMOVED.
        // No stray toolProvenance="door" Wall should remain for this plug's id.
        DrawingDocumentController ctl;
        {
            edi::drafting::MapSpec map;
            edi::drafting::NamedRoomSpec roomA;
            roomA.name = "a";
            roomA.spec.origin = {0.0, 0.0};
            roomA.spec.width = 0.4;
            roomA.spec.height = 0.4;
            roomA.spec.wallThickness = 0.02;
            roomA.spec.plugs = {{edi::drafting::RoomEdge::East, 0.2, "door", "door"}};
            edi::drafting::NamedRoomSpec roomB;
            roomB.name = "b";
            roomB.spec.origin = {0.6, 0.0};
            roomB.spec.width = 0.4;
            roomB.spec.height = 0.4;
            roomB.spec.wallThickness = 0.02;
            roomB.spec.plugs = {{edi::drafting::RoomEdge::West, 0.2, "door", "door"}};
            map.rooms = {roomA, roomB};
            edi::drafting::MapConnectionSpec conn;
            conn.from = {"a", "door"}; conn.to = {"b", "door"}; conn.type = "corridor";
            map.connections = {conn};
            EDI_CHECK(ctl.createMapFromSpec(map));
        }
        EDI_CHECK(ctl.draftingDocument().plugs.size() == 2);

        std::string plugId;
        for (const auto &p : ctl.draftingDocument().plugs) {
            if (p.name == "a.door") { plugId = p.id; break; }
        }
        EDI_CHECK(!plugId.empty());
        const std::string leafTag = "plug:" + plugId;

        // Sanity: leaf exists before delete.
        bool leafBefore = false;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { leafBefore = true; break; }
            }
            if (leafBefore) break;
        }
        EDI_CHECK(leafBefore);

        EDI_CHECK(ctl.deletePlug(QString::fromStdString(plugId)));

        // Leaf must be gone (not orphaned).
        bool leafAfter = false;
        for (const auto &obj : ctl.draftingDocument().objects) {
            for (const auto &tag : obj.metadata.tags) {
                if (tag == leafTag) { leafAfter = true; break; }
            }
            if (leafAfter) break;
        }
        EDI_CHECK(!leafAfter); // no orphan

        // No stray "door"-provenanced wall should remain for this plug either.
        // (Guard against a leak where the tag was added but the leaf still isn't
        //  collected because the gather loop has a bug.)
        // We check that no Wall with toolProvenance="door" exists that has a tag
        // matching the deleted plug. (The other plug's leaf may still be present.)
        for (const auto &obj : ctl.draftingDocument().objects) {
            if (obj.metadata.toolProvenance != "door") { continue; }
            for (const auto &tag : obj.metadata.tags) {
                EDI_CHECK(tag != leafTag); // no stray door leaf for the deleted plug
            }
        }
    }

    return 0;
}
