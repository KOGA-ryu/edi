#include "drafting/DraftingPhysicalEdit.h"
#include "drafting/DraftingStore.h"

#include "EdiAssert.h"
#include <cmath>
#include <limits>
#include <utility>
#include <variant>

using namespace edi::drafting;

namespace {

bool near(double a, double b, double epsilon = 0.000001)
{
    return std::abs(a - b) <= epsilon;
}

DraftingGridProjection grid(double width = 12.0, double height = 12.0)
{
    DraftingGridSettings settings = defaultDraftingGridSettings();
    settings.width = width;
    settings.height = height;
    return projectDraftingGrid(settings);
}

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

const NumericGeometryEditCommand *numericCommand(const DraftingPhysicalGeometryEditPlan &plan)
{
    EDI_CHECK(plan.ok);
    EDI_CHECK(plan.command);
    return std::get_if<NumericGeometryEditCommand>(&*plan.command);
}

const UpdateGeometryCommand *geometryCommand(const DraftingPhysicalGeometryEditPlan &plan)
{
    EDI_CHECK(plan.ok);
    EDI_CHECK(plan.command);
    return std::get_if<UpdateGeometryCommand>(&*plan.command);
}

} // namespace

int main()
{
    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}});
    const DraftingPhysicalGeometryEditPlan pointX = planPhysicalGeometryEdit(point, grid(), "x", 6.0);
    const NumericGeometryEditCommand *pointXCommand = numericCommand(pointX);
    EDI_CHECK(pointXCommand != nullptr);
    EDI_CHECK(pointXCommand->objectId == "point_1");
    EDI_CHECK(pointXCommand->fieldId == "x");
    EDI_CHECK(near(pointXCommand->value, 0.5));

    DraftingDocument document = makeDraftingDocument("physical_edit_doc");
    EDI_CHECK(addObject(document, point).ok);
    const auto revisionBeforePointEdit = document.revision;
    EDI_CHECK(applyDraftingCommand(document, *pointX.command).ok);
    const auto *editedPointObject = findObject(document, "point_1");
    EDI_CHECK(editedPointObject != nullptr);
    const auto *editedPoint = std::get_if<PointGeometry>(&editedPointObject->geometry);
    EDI_CHECK(editedPoint != nullptr);
    EDI_CHECK(near(editedPoint->point.x, 0.5));
    EDI_CHECK(near(editedPoint->point.y, 0.2));
    EDI_CHECK(document.revision == revisionBeforePointEdit + 1);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.2}, {0.4, 0.6}});
    const DraftingPhysicalGeometryEditPlan lineLength = planPhysicalGeometryEdit(line, grid(), "line_length", 12.0);
    const UpdateGeometryCommand *lineLengthCommand = geometryCommand(lineLength);
    EDI_CHECK(lineLengthCommand != nullptr);
    EDI_CHECK(lineLengthCommand->objectId == "line_1");
    const auto *lineLengthGeometry = std::get_if<LineGeometry>(&lineLengthCommand->geometry);
    EDI_CHECK(lineLengthGeometry != nullptr);
    EDI_CHECK(near(lineLengthGeometry->a.x, 0.1));
    EDI_CHECK(near(lineLengthGeometry->a.y, 0.2));
    EDI_CHECK(near(lineLengthGeometry->b.x, 0.7));
    EDI_CHECK(near(lineLengthGeometry->b.y, 1.0));

    const DraftingPhysicalGeometryEditPlan lineAngle = planPhysicalGeometryEdit(line, grid(), "line_angle_deg", 0.0);
    const UpdateGeometryCommand *lineAngleCommand = geometryCommand(lineAngle);
    EDI_CHECK(lineAngleCommand != nullptr);
    const auto *lineAngleGeometry = std::get_if<LineGeometry>(&lineAngleCommand->geometry);
    EDI_CHECK(lineAngleGeometry != nullptr);
    EDI_CHECK(near(lineAngleGeometry->b.y, 0.2));

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.1, 0.2}, 0.3, 0.4});
    const DraftingPhysicalGeometryEditPlan rectWidth = planPhysicalGeometryEdit(rect, grid(), "width", 3.0);
    const NumericGeometryEditCommand *rectWidthCommand = numericCommand(rectWidth);
    EDI_CHECK(rectWidthCommand != nullptr);
    EDI_CHECK(rectWidthCommand->fieldId == "width");
    EDI_CHECK(near(rectWidthCommand->value, 0.25));

    const DraftingPhysicalGeometryEditPlan rectRotation = planPhysicalGeometryEdit(rect, grid(), "rotation_deg", 45.0);
    const NumericGeometryEditCommand *rectRotationCommand = numericCommand(rectRotation);
    EDI_CHECK(rectRotationCommand != nullptr);
    EDI_CHECK(rectRotationCommand->fieldId == "rotation_deg");
    EDI_CHECK(near(rectRotationCommand->value, 45.0));

    // Arc angles are unit-independent: the physical edit passes the degrees
    // through unchanged (regression: these used to be rejected).
    DraftingObject arc = object("arc_1", DraftingShapeKind::Arc, ArcGeometry{{0.3, 0.3}, 0.1, 15.0, 120.0});
    const DraftingPhysicalGeometryEditPlan arcStart = planPhysicalGeometryEdit(arc, grid(), "start_angle_deg", 30.0);
    const NumericGeometryEditCommand *arcStartCommand = numericCommand(arcStart);
    EDI_CHECK(arcStartCommand != nullptr);
    EDI_CHECK(arcStartCommand->fieldId == "start_angle_deg");
    EDI_CHECK(near(arcStartCommand->value, 30.0));
    const DraftingPhysicalGeometryEditPlan arcEnd = planPhysicalGeometryEdit(arc, grid(), "end_angle_deg", 200.0);
    const NumericGeometryEditCommand *arcEndCommand = numericCommand(arcEnd);
    EDI_CHECK(arcEndCommand != nullptr);
    EDI_CHECK(near(arcEndCommand->value, 200.0));
    const DraftingPhysicalGeometryEditPlan arcRadius = planPhysicalGeometryEdit(arc, grid(), "radius", 4.0);
    EDI_CHECK(numericCommand(arcRadius) != nullptr); // radius still scales by unit

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    const DraftingPhysicalGeometryEditPlan circleRadius = planPhysicalGeometryEdit(circle, grid(), "radius", 3.0);
    const NumericGeometryEditCommand *circleRadiusCommand = numericCommand(circleRadius);
    EDI_CHECK(circleRadiusCommand != nullptr);
    EDI_CHECK(circleRadiusCommand->fieldId == "radius");
    EDI_CHECK(near(circleRadiusCommand->value, 0.25));

    const DraftingPhysicalGeometryEditPlan circleDiameter = planPhysicalGeometryEdit(circle, grid(), "diameter", 6.0);
    const NumericGeometryEditCommand *circleDiameterCommand = numericCommand(circleDiameter);
    EDI_CHECK(circleDiameterCommand != nullptr);
    EDI_CHECK(circleDiameterCommand->fieldId == "diameter");
    EDI_CHECK(near(circleDiameterCommand->value, 0.5));

    DraftingObject guide = object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25});
    const DraftingPhysicalGeometryEditPlan guidePosition = planPhysicalGeometryEdit(guide, grid(), "position", 6.0);
    const NumericGeometryEditCommand *guidePositionCommand = numericCommand(guidePosition);
    EDI_CHECK(guidePositionCommand != nullptr);
    EDI_CHECK(guidePositionCommand->fieldId == "position");
    EDI_CHECK(near(guidePositionCommand->value, 0.5));

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.6}, 0.04});
    const DraftingPhysicalGeometryEditPlan dimensionLength = planPhysicalGeometryEdit(dimension, grid(), "dimension_length", 12.0);
    const UpdateGeometryCommand *dimensionLengthCommand = geometryCommand(dimensionLength);
    EDI_CHECK(dimensionLengthCommand != nullptr);
    const auto *dimensionLengthGeometry = std::get_if<DimensionGeometry>(&dimensionLengthCommand->geometry);
    EDI_CHECK(dimensionLengthGeometry != nullptr);
    EDI_CHECK(near(dimensionLengthGeometry->b.x, 0.807106781186548));
    EDI_CHECK(near(dimensionLengthGeometry->b.y, 0.907106781186548));

    const DraftingPhysicalGeometryEditPlan dimensionOffset = planPhysicalGeometryEdit(dimension, grid(), "offset", 1.2);
    const NumericGeometryEditCommand *dimensionOffsetCommand = numericCommand(dimensionOffset);
    EDI_CHECK(dimensionOffsetCommand != nullptr);
    EDI_CHECK(dimensionOffsetCommand->fieldId == "offset");
    EDI_CHECK(near(dimensionOffsetCommand->value, 0.1));

    // Wall: the band's endpoints + thickness are physically editable. ax/bx
    // normalize by width, ay/by by height, and thickness by width (a width-kind
    // length, like radius). Forward projection advertises these fields editable;
    // this pins the INVERSE actually applies them (regression: ax/ay/bx/by/
    // thickness were originally missing from planPhysicalGeometryEdit, so every
    // physical spinbox was dead and errored on commit).
    DraftingObject wall = object("wall_1", DraftingShapeKind::Wall, WallGeometry{{0.2, 0.5}, {0.8, 0.5}, 0.1});
    const DraftingPhysicalGeometryEditPlan wallAx = planPhysicalGeometryEdit(wall, grid(), "ax", 6.0);
    const NumericGeometryEditCommand *wallAxCommand = numericCommand(wallAx);
    EDI_CHECK(wallAxCommand != nullptr);
    EDI_CHECK(wallAxCommand->fieldId == "ax");
    EDI_CHECK(near(wallAxCommand->value, 0.5)); // 6.0 / width 12.0

    const DraftingPhysicalGeometryEditPlan wallBy = planPhysicalGeometryEdit(wall, grid(), "by", 3.0);
    const NumericGeometryEditCommand *wallByCommand = numericCommand(wallBy);
    EDI_CHECK(wallByCommand != nullptr);
    EDI_CHECK(wallByCommand->fieldId == "by");
    EDI_CHECK(near(wallByCommand->value, 0.25)); // 3.0 / height 12.0

    const DraftingPhysicalGeometryEditPlan wallThickness = planPhysicalGeometryEdit(wall, grid(), "thickness", 1.2);
    const NumericGeometryEditCommand *wallThicknessCommand = numericCommand(wallThickness);
    EDI_CHECK(wallThicknessCommand != nullptr);
    EDI_CHECK(wallThicknessCommand->fieldId == "thickness");
    EDI_CHECK(near(wallThicknessCommand->value, 0.1)); // 1.2 / width 12.0 (width-kind)

    // The plan applies cleanly to a real document, mutating the wall geometry.
    DraftingDocument wallDoc = makeDraftingDocument("wall_physical_doc");
    EDI_CHECK(addObject(wallDoc, wall).ok);
    EDI_CHECK(applyDraftingCommand(wallDoc, *wallThickness.command).ok);
    const auto *editedWall = findObject(wallDoc, "wall_1");
    EDI_CHECK(editedWall != nullptr);
    EDI_CHECK(near(std::get<WallGeometry>(editedWall->geometry).thickness, 0.1));

    const DraftingPhysicalGeometryEditPlan badField = planPhysicalGeometryEdit(circle, grid(), "missing", 1.0);
    EDI_CHECK(!badField.ok);
    EDI_CHECK(badField.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(!badField.command);

    const DraftingPhysicalGeometryEditPlan badValue = planPhysicalGeometryEdit(circle, grid(), "radius", std::numeric_limits<double>::infinity());
    EDI_CHECK(!badValue.ok);
    EDI_CHECK(badValue.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(!badValue.command);

    const DraftingPhysicalGeometryEditPlan negativeRadius = planPhysicalGeometryEdit(circle, grid(), "radius", -1.0);
    EDI_CHECK(!negativeRadius.ok);
    EDI_CHECK(negativeRadius.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(!negativeRadius.command);

    DraftingGridProjection badGrid = grid();
    badGrid.settings.width = 0.0;
    const DraftingPhysicalGeometryEditPlan invalidGrid = planPhysicalGeometryEdit(point, badGrid, "x", 1.0);
    EDI_CHECK(!invalidGrid.ok);
    EDI_CHECK(invalidGrid.code == DraftingResultCode::InvalidGeometry);
    EDI_CHECK(!invalidGrid.command);

    return 0;
}
