#include "drafting/DraftingNumericEdit.h"

#include "drafting/DraftingGeometry.h"

#include "EdiAssert.h"
#include <cmath>
#include <limits>
#include <string>
#include <utility>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    EDI_CHECK(built.ok);
    return built.object;
}

} // namespace

int main()
{
    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {3.0, 4.0}});
    const auto *sourceLine = std::get_if<LineGeometry>(&line.geometry);
    EDI_CHECK(sourceLine != nullptr);
    EDI_CHECK(nearlyEqual(lineAngleDegrees(*sourceLine), 53.1301023542));

    auto lengthEdit = applyNumericGeometryEdit(line, "line_length", 10.0);
    EDI_CHECK(lengthEdit.ok);
    const auto *lengthLine = std::get_if<LineGeometry>(&lengthEdit.geometry);
    EDI_CHECK(lengthLine != nullptr);
    EDI_CHECK(nearlyEqual(lengthLine->a.x, 0.0));
    EDI_CHECK(nearlyEqual(lengthLine->a.y, 0.0));
    EDI_CHECK(nearlyEqual(lengthLine->b.x, 6.0));
    EDI_CHECK(nearlyEqual(lengthLine->b.y, 8.0));

    auto angleEdit = applyNumericGeometryEdit(line, "line_angle_deg", 0.0);
    EDI_CHECK(angleEdit.ok);
    const auto *angleLine = std::get_if<LineGeometry>(&angleEdit.geometry);
    EDI_CHECK(angleLine != nullptr);
    EDI_CHECK(nearlyEqual(angleLine->b.x, 5.0));
    EDI_CHECK(nearlyEqual(angleLine->b.y, 0.0));

    auto negativeLength = applyNumericGeometryEdit(line, "line_length", -1.0);
    EDI_CHECK(!negativeLength.ok);
    EDI_CHECK(negativeLength.code == DraftingResultCode::InvalidGeometry);

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    auto diameterEdit = applyNumericGeometryEdit(circle, "diameter", 0.5);
    EDI_CHECK(diameterEdit.ok);
    const auto *diameterCircle = std::get_if<CircleGeometry>(&diameterEdit.geometry);
    EDI_CHECK(diameterCircle != nullptr);
    EDI_CHECK(nearlyEqual(diameterCircle->center.x, 0.25));
    EDI_CHECK(nearlyEqual(diameterCircle->radius, 0.25));

    auto negativeDiameter = applyNumericGeometryEdit(circle, "diameter", -0.5);
    EDI_CHECK(!negativeDiameter.ok);
    EDI_CHECK(negativeDiameter.code == DraftingResultCode::InvalidGeometry);

    // Arc numeric fields: cx/cy/radius/start_angle_deg/end_angle_deg.
    DraftingObject arc = object("arc_1", DraftingShapeKind::Arc, ArcGeometry{{0.4, 0.4}, 0.15, 15.0, 120.0});
    auto arcRadiusEdit = applyNumericGeometryEdit(arc, "radius", 0.3);
    EDI_CHECK(arcRadiusEdit.ok);
    EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(arcRadiusEdit.geometry).radius, 0.3));
    auto arcStartEdit = applyNumericGeometryEdit(arc, "start_angle_deg", 45.0);
    EDI_CHECK(arcStartEdit.ok);
    EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(arcStartEdit.geometry).startAngleDeg, 45.0));
    auto arcEndEdit = applyNumericGeometryEdit(arc, "end_angle_deg", 200.0);
    EDI_CHECK(arcEndEdit.ok);
    EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(arcEndEdit.geometry).endAngleDeg, 200.0));
    auto arcBadField = applyNumericGeometryEdit(arc, "diameter", 0.5);
    EDI_CHECK(!arcBadField.ok); // arc has no diameter field

    auto negativeRadius = applyNumericGeometryEdit(circle, "radius", -0.1);
    EDI_CHECK(!negativeRadius.ok);
    EDI_CHECK(negativeRadius.code == DraftingResultCode::InvalidGeometry);

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.1, 0.2}, 0.3, 0.4});
    auto negativeWidth = applyNumericGeometryEdit(rect, "width", -0.1);
    EDI_CHECK(!negativeWidth.ok);
    EDI_CHECK(negativeWidth.code == DraftingResultCode::InvalidGeometry);

    DraftingObject guide = object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25});
    auto guidePosition = applyNumericGeometryEdit(guide, "position", 0.75);
    EDI_CHECK(guidePosition.ok);
    const auto *editedGuide = std::get_if<GuideGeometry>(&guidePosition.geometry);
    EDI_CHECK(editedGuide != nullptr);
    EDI_CHECK(editedGuide->orientation == GuideOrientation::Vertical);
    EDI_CHECK(nearlyEqual(editedGuide->position, 0.75));
    auto invalidGuidePosition = applyNumericGeometryEdit(guide, "position", 1.5);
    EDI_CHECK(!invalidGuidePosition.ok);
    EDI_CHECK(invalidGuidePosition.code == DraftingResultCode::InvalidGeometry);

    DraftingObject construction = object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.1, 0.2}, {0.6, 0.7}});
    auto constructionEnd = applyNumericGeometryEdit(construction, "x2", 0.9);
    EDI_CHECK(constructionEnd.ok);
    const auto *editedConstruction = std::get_if<ConstructionLineGeometry>(&constructionEnd.geometry);
    EDI_CHECK(editedConstruction != nullptr);
    EDI_CHECK(nearlyEqual(editedConstruction->a.x, 0.1));
    EDI_CHECK(nearlyEqual(editedConstruction->b.x, 0.9));
    auto collapsedConstruction = applyNumericGeometryEdit(construction, "x2", 0.1);
    EDI_CHECK(collapsedConstruction.ok);
    DraftingObject partiallyCollapsed = construction;
    partiallyCollapsed.geometry = collapsedConstruction.geometry;
    auto fullyCollapsedConstruction = applyNumericGeometryEdit(partiallyCollapsed, "y2", 0.2);
    EDI_CHECK(!fullyCollapsedConstruction.ok);
    EDI_CHECK(fullyCollapsedConstruction.code == DraftingResultCode::InvalidGeometry);

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.6}, 0.04});
    auto dimensionOffset = applyNumericGeometryEdit(dimension, "offset", -0.08);
    EDI_CHECK(dimensionOffset.ok);
    const auto *editedDimension = std::get_if<DimensionGeometry>(&dimensionOffset.geometry);
    EDI_CHECK(editedDimension != nullptr);
    EDI_CHECK(nearlyEqual(editedDimension->offset, -0.08));
    auto dimensionEnd = applyNumericGeometryEdit(dimension, "y2", 0.6);
    EDI_CHECK(dimensionEnd.ok);
    const auto *dimensionWithNewEnd = std::get_if<DimensionGeometry>(&dimensionEnd.geometry);
    EDI_CHECK(dimensionWithNewEnd != nullptr);
    EDI_CHECK(nearlyEqual(dimensionWithNewEnd->b.y, 0.6));
    auto dimensionLength = applyNumericGeometryEdit(dimension, "dimension_length", 1.0);
    EDI_CHECK(dimensionLength.ok);
    const auto *dimensionWithNewLength = std::get_if<DimensionGeometry>(&dimensionLength.geometry);
    EDI_CHECK(dimensionWithNewLength != nullptr);
    EDI_CHECK(nearlyEqual(dimensionWithNewLength->a.x, 0.1));
    EDI_CHECK(nearlyEqual(dimensionWithNewLength->a.y, 0.2));
    EDI_CHECK(nearlyEqual(std::hypot(dimensionWithNewLength->b.x - dimensionWithNewLength->a.x, dimensionWithNewLength->b.y - dimensionWithNewLength->a.y), 1.0));
    auto dimensionAngle = applyNumericGeometryEdit(dimension, "dimension_angle_deg", 0.0);
    EDI_CHECK(dimensionAngle.ok);
    const auto *dimensionWithNewAngle = std::get_if<DimensionGeometry>(&dimensionAngle.geometry);
    EDI_CHECK(dimensionWithNewAngle != nullptr);
    EDI_CHECK(nearlyEqual(dimensionWithNewAngle->b.y, 0.2));
    EDI_CHECK(nearlyEqual(dimensionWithNewAngle->b.x, 0.1 + std::hypot(0.4, 0.4)));
    auto negativeDimensionLength = applyNumericGeometryEdit(dimension, "dimension_length", -1.0);
    EDI_CHECK(!negativeDimensionLength.ok);
    EDI_CHECK(negativeDimensionLength.code == DraftingResultCode::InvalidGeometry);

    DraftingObject widthDimension = object("dimension_width", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Width, {0.1, 0.2}, {0.5, 0.2}, 0.04});
    auto editedWidthDimension = applyNumericGeometryEdit(widthDimension, "dimension_length", 0.7);
    EDI_CHECK(editedWidthDimension.ok);
    const auto *widthGeometry = std::get_if<DimensionGeometry>(&editedWidthDimension.geometry);
    EDI_CHECK(widthGeometry != nullptr);
    EDI_CHECK(widthGeometry->kind == DimensionKind::Width);
    EDI_CHECK(nearlyEqual(widthGeometry->b.x, 0.8));
    EDI_CHECK(nearlyEqual(widthGeometry->b.y, 0.2));
    EDI_CHECK(!applyNumericGeometryEdit(widthDimension, "dimension_angle_deg", 15.0).ok);

    DraftingObject heightDimension = object("dimension_height", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Height, {0.1, 0.2}, {0.1, 0.5}, 0.04});
    auto editedHeightDimension = applyNumericGeometryEdit(heightDimension, "dimension_length", 0.6);
    EDI_CHECK(editedHeightDimension.ok);
    const auto *heightGeometry = std::get_if<DimensionGeometry>(&editedHeightDimension.geometry);
    EDI_CHECK(heightGeometry != nullptr);
    EDI_CHECK(heightGeometry->kind == DimensionKind::Height);
    EDI_CHECK(nearlyEqual(heightGeometry->b.x, 0.1));
    EDI_CHECK(nearlyEqual(heightGeometry->b.y, 0.8));

    DraftingObject diameterDimension = object("dimension_diameter", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Diameter, {0.1, 0.2}, {0.4, 0.2}, 0.04});
    auto editedDiameterDimension = applyNumericGeometryEdit(diameterDimension, "dimension_length", 1.0);
    EDI_CHECK(editedDiameterDimension.ok);
    const auto *diameterGeometry = std::get_if<DimensionGeometry>(&editedDiameterDimension.geometry);
    EDI_CHECK(diameterGeometry != nullptr);
    EDI_CHECK(diameterGeometry->kind == DimensionKind::Diameter);
    EDI_CHECK(nearlyEqual(diameterGeometry->b.x, 0.6));
    EDI_CHECK(nearlyEqual(diameterGeometry->b.y, 0.2));
    auto collapsedDimension = applyNumericGeometryEdit(dimension, "x2", 0.1);
    EDI_CHECK(collapsedDimension.ok);
    DraftingObject partiallyCollapsedDimension = dimension;
    partiallyCollapsedDimension.geometry = collapsedDimension.geometry;
    auto fullyCollapsedDimension = applyNumericGeometryEdit(partiallyCollapsedDimension, "y2", 0.2);
    EDI_CHECK(!fullyCollapsedDimension.ok);
    EDI_CHECK(fullyCollapsedDimension.code == DraftingResultCode::InvalidGeometry);

    auto pointX = applyNumericGeometryEdit(object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}}), "x", 0.5);
    EDI_CHECK(pointX.ok);
    const auto *point = std::get_if<PointGeometry>(&pointX.geometry);
    EDI_CHECK(point != nullptr);
    EDI_CHECK(nearlyEqual(point->point.x, 0.5));
    EDI_CHECK(nearlyEqual(point->point.y, 0.2));

    auto badField = applyNumericGeometryEdit(line, "missing", 1.0);
    EDI_CHECK(!badField.ok);
    EDI_CHECK(badField.code == DraftingResultCode::InvalidGeometry);

    auto badValue = applyNumericGeometryEdit(line, "x1", std::numeric_limits<double>::infinity());
    EDI_CHECK(!badValue.ok);
    EDI_CHECK(badValue.code == DraftingResultCode::InvalidGeometry);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Circle;
    auto mismatch = applyNumericGeometryEdit(mismatched, "line_length", 2.0);
    EDI_CHECK(!mismatch.ok);
    EDI_CHECK(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    return 0;
}
