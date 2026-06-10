#include "drafting/DraftingPhysicalEdit.h"
#include "drafting/DraftingStore.h"

#include <cassert>
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
    assert(built.ok);
    return built.object;
}

const NumericGeometryEditCommand *numericCommand(const DraftingPhysicalGeometryEditPlan &plan)
{
    assert(plan.ok);
    assert(plan.command);
    return std::get_if<NumericGeometryEditCommand>(&*plan.command);
}

const UpdateGeometryCommand *geometryCommand(const DraftingPhysicalGeometryEditPlan &plan)
{
    assert(plan.ok);
    assert(plan.command);
    return std::get_if<UpdateGeometryCommand>(&*plan.command);
}

} // namespace

int main()
{
    DraftingObject point = object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}});
    const DraftingPhysicalGeometryEditPlan pointX = planPhysicalGeometryEdit(point, grid(), "x", 6.0);
    const NumericGeometryEditCommand *pointXCommand = numericCommand(pointX);
    assert(pointXCommand != nullptr);
    assert(pointXCommand->objectId == "point_1");
    assert(pointXCommand->fieldId == "x");
    assert(near(pointXCommand->value, 0.5));

    DraftingDocument document = makeDraftingDocument("physical_edit_doc");
    assert(addObject(document, point).ok);
    const auto revisionBeforePointEdit = document.revision;
    assert(applyDraftingCommand(document, *pointX.command).ok);
    const auto *editedPointObject = findObject(document, "point_1");
    assert(editedPointObject != nullptr);
    const auto *editedPoint = std::get_if<PointGeometry>(&editedPointObject->geometry);
    assert(editedPoint != nullptr);
    assert(near(editedPoint->point.x, 0.5));
    assert(near(editedPoint->point.y, 0.2));
    assert(document.revision == revisionBeforePointEdit + 1);

    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.1, 0.2}, {0.4, 0.6}});
    const DraftingPhysicalGeometryEditPlan lineLength = planPhysicalGeometryEdit(line, grid(), "line_length", 12.0);
    const UpdateGeometryCommand *lineLengthCommand = geometryCommand(lineLength);
    assert(lineLengthCommand != nullptr);
    assert(lineLengthCommand->objectId == "line_1");
    const auto *lineLengthGeometry = std::get_if<LineGeometry>(&lineLengthCommand->geometry);
    assert(lineLengthGeometry != nullptr);
    assert(near(lineLengthGeometry->a.x, 0.1));
    assert(near(lineLengthGeometry->a.y, 0.2));
    assert(near(lineLengthGeometry->b.x, 0.7));
    assert(near(lineLengthGeometry->b.y, 1.0));

    const DraftingPhysicalGeometryEditPlan lineAngle = planPhysicalGeometryEdit(line, grid(), "line_angle_deg", 0.0);
    const UpdateGeometryCommand *lineAngleCommand = geometryCommand(lineAngle);
    assert(lineAngleCommand != nullptr);
    const auto *lineAngleGeometry = std::get_if<LineGeometry>(&lineAngleCommand->geometry);
    assert(lineAngleGeometry != nullptr);
    assert(near(lineAngleGeometry->b.y, 0.2));

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.1, 0.2}, 0.3, 0.4});
    const DraftingPhysicalGeometryEditPlan rectWidth = planPhysicalGeometryEdit(rect, grid(), "width", 3.0);
    const NumericGeometryEditCommand *rectWidthCommand = numericCommand(rectWidth);
    assert(rectWidthCommand != nullptr);
    assert(rectWidthCommand->fieldId == "width");
    assert(near(rectWidthCommand->value, 0.25));

    const DraftingPhysicalGeometryEditPlan rectRotation = planPhysicalGeometryEdit(rect, grid(), "rotation_deg", 45.0);
    const NumericGeometryEditCommand *rectRotationCommand = numericCommand(rectRotation);
    assert(rectRotationCommand != nullptr);
    assert(rectRotationCommand->fieldId == "rotation_deg");
    assert(near(rectRotationCommand->value, 45.0));

    // Arc angles are unit-independent: the physical edit passes the degrees
    // through unchanged (regression: these used to be rejected).
    DraftingObject arc = object("arc_1", DraftingShapeKind::Arc, ArcGeometry{{0.3, 0.3}, 0.1, 15.0, 120.0});
    const DraftingPhysicalGeometryEditPlan arcStart = planPhysicalGeometryEdit(arc, grid(), "start_angle_deg", 30.0);
    const NumericGeometryEditCommand *arcStartCommand = numericCommand(arcStart);
    assert(arcStartCommand != nullptr);
    assert(arcStartCommand->fieldId == "start_angle_deg");
    assert(near(arcStartCommand->value, 30.0));
    const DraftingPhysicalGeometryEditPlan arcEnd = planPhysicalGeometryEdit(arc, grid(), "end_angle_deg", 200.0);
    const NumericGeometryEditCommand *arcEndCommand = numericCommand(arcEnd);
    assert(arcEndCommand != nullptr);
    assert(near(arcEndCommand->value, 200.0));
    const DraftingPhysicalGeometryEditPlan arcRadius = planPhysicalGeometryEdit(arc, grid(), "radius", 4.0);
    assert(numericCommand(arcRadius) != nullptr); // radius still scales by unit

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    const DraftingPhysicalGeometryEditPlan circleRadius = planPhysicalGeometryEdit(circle, grid(), "radius", 3.0);
    const NumericGeometryEditCommand *circleRadiusCommand = numericCommand(circleRadius);
    assert(circleRadiusCommand != nullptr);
    assert(circleRadiusCommand->fieldId == "radius");
    assert(near(circleRadiusCommand->value, 0.25));

    const DraftingPhysicalGeometryEditPlan circleDiameter = planPhysicalGeometryEdit(circle, grid(), "diameter", 6.0);
    const NumericGeometryEditCommand *circleDiameterCommand = numericCommand(circleDiameter);
    assert(circleDiameterCommand != nullptr);
    assert(circleDiameterCommand->fieldId == "diameter");
    assert(near(circleDiameterCommand->value, 0.5));

    DraftingObject guide = object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25});
    const DraftingPhysicalGeometryEditPlan guidePosition = planPhysicalGeometryEdit(guide, grid(), "position", 6.0);
    const NumericGeometryEditCommand *guidePositionCommand = numericCommand(guidePosition);
    assert(guidePositionCommand != nullptr);
    assert(guidePositionCommand->fieldId == "position");
    assert(near(guidePositionCommand->value, 0.5));

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.6}, 0.04});
    const DraftingPhysicalGeometryEditPlan dimensionLength = planPhysicalGeometryEdit(dimension, grid(), "dimension_length", 12.0);
    const UpdateGeometryCommand *dimensionLengthCommand = geometryCommand(dimensionLength);
    assert(dimensionLengthCommand != nullptr);
    const auto *dimensionLengthGeometry = std::get_if<DimensionGeometry>(&dimensionLengthCommand->geometry);
    assert(dimensionLengthGeometry != nullptr);
    assert(near(dimensionLengthGeometry->b.x, 0.807106781186548));
    assert(near(dimensionLengthGeometry->b.y, 0.907106781186548));

    const DraftingPhysicalGeometryEditPlan dimensionOffset = planPhysicalGeometryEdit(dimension, grid(), "offset", 1.2);
    const NumericGeometryEditCommand *dimensionOffsetCommand = numericCommand(dimensionOffset);
    assert(dimensionOffsetCommand != nullptr);
    assert(dimensionOffsetCommand->fieldId == "offset");
    assert(near(dimensionOffsetCommand->value, 0.1));

    const DraftingPhysicalGeometryEditPlan badField = planPhysicalGeometryEdit(circle, grid(), "missing", 1.0);
    assert(!badField.ok);
    assert(badField.code == DraftingResultCode::InvalidGeometry);
    assert(!badField.command);

    const DraftingPhysicalGeometryEditPlan badValue = planPhysicalGeometryEdit(circle, grid(), "radius", std::numeric_limits<double>::infinity());
    assert(!badValue.ok);
    assert(badValue.code == DraftingResultCode::InvalidGeometry);
    assert(!badValue.command);

    const DraftingPhysicalGeometryEditPlan negativeRadius = planPhysicalGeometryEdit(circle, grid(), "radius", -1.0);
    assert(!negativeRadius.ok);
    assert(negativeRadius.code == DraftingResultCode::InvalidGeometry);
    assert(!negativeRadius.command);

    DraftingGridProjection badGrid = grid();
    badGrid.settings.width = 0.0;
    const DraftingPhysicalGeometryEditPlan invalidGrid = planPhysicalGeometryEdit(point, badGrid, "x", 1.0);
    assert(!invalidGrid.ok);
    assert(invalidGrid.code == DraftingResultCode::InvalidGeometry);
    assert(!invalidGrid.command);

    return 0;
}
