#include "drafting/DraftingNumericEdit.h"

#include <cassert>
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
    assert(built.ok);
    return built.object;
}

} // namespace

int main()
{
    DraftingObject line = object("line_1", DraftingShapeKind::Line, LineGeometry{{0.0, 0.0}, {3.0, 4.0}});
    const auto *sourceLine = std::get_if<LineGeometry>(&line.geometry);
    assert(sourceLine != nullptr);
    assert(nearlyEqual(lineAngleDegrees(*sourceLine), 53.1301023542));

    auto lengthEdit = applyNumericGeometryEdit(line, "line_length", 10.0);
    assert(lengthEdit.ok);
    const auto *lengthLine = std::get_if<LineGeometry>(&lengthEdit.geometry);
    assert(lengthLine != nullptr);
    assert(nearlyEqual(lengthLine->a.x, 0.0));
    assert(nearlyEqual(lengthLine->a.y, 0.0));
    assert(nearlyEqual(lengthLine->b.x, 6.0));
    assert(nearlyEqual(lengthLine->b.y, 8.0));

    auto angleEdit = applyNumericGeometryEdit(line, "line_angle_deg", 0.0);
    assert(angleEdit.ok);
    const auto *angleLine = std::get_if<LineGeometry>(&angleEdit.geometry);
    assert(angleLine != nullptr);
    assert(nearlyEqual(angleLine->b.x, 5.0));
    assert(nearlyEqual(angleLine->b.y, 0.0));

    auto negativeLength = applyNumericGeometryEdit(line, "line_length", -1.0);
    assert(!negativeLength.ok);
    assert(negativeLength.code == DraftingResultCode::InvalidGeometry);

    DraftingObject circle = object("circle_1", DraftingShapeKind::Circle, CircleGeometry{{0.25, 0.25}, 0.1});
    auto diameterEdit = applyNumericGeometryEdit(circle, "diameter", 0.5);
    assert(diameterEdit.ok);
    const auto *diameterCircle = std::get_if<CircleGeometry>(&diameterEdit.geometry);
    assert(diameterCircle != nullptr);
    assert(nearlyEqual(diameterCircle->center.x, 0.25));
    assert(nearlyEqual(diameterCircle->radius, 0.25));

    auto negativeDiameter = applyNumericGeometryEdit(circle, "diameter", -0.5);
    assert(!negativeDiameter.ok);
    assert(negativeDiameter.code == DraftingResultCode::InvalidGeometry);

    auto negativeRadius = applyNumericGeometryEdit(circle, "radius", -0.1);
    assert(!negativeRadius.ok);
    assert(negativeRadius.code == DraftingResultCode::InvalidGeometry);

    DraftingObject rect = object("rect_1", DraftingShapeKind::Rectangle, RectangleGeometry{{0.1, 0.2}, 0.3, 0.4});
    auto negativeWidth = applyNumericGeometryEdit(rect, "width", -0.1);
    assert(!negativeWidth.ok);
    assert(negativeWidth.code == DraftingResultCode::InvalidGeometry);

    DraftingObject guide = object("guide_1", DraftingShapeKind::Guide, GuideGeometry{GuideOrientation::Vertical, 0.25});
    auto guidePosition = applyNumericGeometryEdit(guide, "position", 0.75);
    assert(guidePosition.ok);
    const auto *editedGuide = std::get_if<GuideGeometry>(&guidePosition.geometry);
    assert(editedGuide != nullptr);
    assert(editedGuide->orientation == GuideOrientation::Vertical);
    assert(nearlyEqual(editedGuide->position, 0.75));
    auto invalidGuidePosition = applyNumericGeometryEdit(guide, "position", 1.5);
    assert(!invalidGuidePosition.ok);
    assert(invalidGuidePosition.code == DraftingResultCode::InvalidGeometry);

    DraftingObject construction = object("construction_1", DraftingShapeKind::ConstructionLine, ConstructionLineGeometry{{0.1, 0.2}, {0.6, 0.7}});
    auto constructionEnd = applyNumericGeometryEdit(construction, "x2", 0.9);
    assert(constructionEnd.ok);
    const auto *editedConstruction = std::get_if<ConstructionLineGeometry>(&constructionEnd.geometry);
    assert(editedConstruction != nullptr);
    assert(nearlyEqual(editedConstruction->a.x, 0.1));
    assert(nearlyEqual(editedConstruction->b.x, 0.9));
    auto collapsedConstruction = applyNumericGeometryEdit(construction, "x2", 0.1);
    assert(collapsedConstruction.ok);
    DraftingObject partiallyCollapsed = construction;
    partiallyCollapsed.geometry = collapsedConstruction.geometry;
    auto fullyCollapsedConstruction = applyNumericGeometryEdit(partiallyCollapsed, "y2", 0.2);
    assert(!fullyCollapsedConstruction.ok);
    assert(fullyCollapsedConstruction.code == DraftingResultCode::InvalidGeometry);

    DraftingObject dimension = object("dimension_1", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Distance, {0.1, 0.2}, {0.5, 0.6}, 0.04});
    auto dimensionOffset = applyNumericGeometryEdit(dimension, "offset", -0.08);
    assert(dimensionOffset.ok);
    const auto *editedDimension = std::get_if<DimensionGeometry>(&dimensionOffset.geometry);
    assert(editedDimension != nullptr);
    assert(nearlyEqual(editedDimension->offset, -0.08));
    auto dimensionEnd = applyNumericGeometryEdit(dimension, "y2", 0.6);
    assert(dimensionEnd.ok);
    const auto *dimensionWithNewEnd = std::get_if<DimensionGeometry>(&dimensionEnd.geometry);
    assert(dimensionWithNewEnd != nullptr);
    assert(nearlyEqual(dimensionWithNewEnd->b.y, 0.6));
    auto dimensionLength = applyNumericGeometryEdit(dimension, "dimension_length", 1.0);
    assert(dimensionLength.ok);
    const auto *dimensionWithNewLength = std::get_if<DimensionGeometry>(&dimensionLength.geometry);
    assert(dimensionWithNewLength != nullptr);
    assert(nearlyEqual(dimensionWithNewLength->a.x, 0.1));
    assert(nearlyEqual(dimensionWithNewLength->a.y, 0.2));
    assert(nearlyEqual(std::hypot(dimensionWithNewLength->b.x - dimensionWithNewLength->a.x, dimensionWithNewLength->b.y - dimensionWithNewLength->a.y), 1.0));
    auto dimensionAngle = applyNumericGeometryEdit(dimension, "dimension_angle_deg", 0.0);
    assert(dimensionAngle.ok);
    const auto *dimensionWithNewAngle = std::get_if<DimensionGeometry>(&dimensionAngle.geometry);
    assert(dimensionWithNewAngle != nullptr);
    assert(nearlyEqual(dimensionWithNewAngle->b.y, 0.2));
    assert(nearlyEqual(dimensionWithNewAngle->b.x, 0.1 + std::hypot(0.4, 0.4)));
    auto negativeDimensionLength = applyNumericGeometryEdit(dimension, "dimension_length", -1.0);
    assert(!negativeDimensionLength.ok);
    assert(negativeDimensionLength.code == DraftingResultCode::InvalidGeometry);

    DraftingObject widthDimension = object("dimension_width", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Width, {0.1, 0.2}, {0.5, 0.2}, 0.04});
    auto editedWidthDimension = applyNumericGeometryEdit(widthDimension, "dimension_length", 0.7);
    assert(editedWidthDimension.ok);
    const auto *widthGeometry = std::get_if<DimensionGeometry>(&editedWidthDimension.geometry);
    assert(widthGeometry != nullptr);
    assert(widthGeometry->kind == DimensionKind::Width);
    assert(nearlyEqual(widthGeometry->b.x, 0.8));
    assert(nearlyEqual(widthGeometry->b.y, 0.2));
    assert(!applyNumericGeometryEdit(widthDimension, "dimension_angle_deg", 15.0).ok);

    DraftingObject heightDimension = object("dimension_height", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Height, {0.1, 0.2}, {0.1, 0.5}, 0.04});
    auto editedHeightDimension = applyNumericGeometryEdit(heightDimension, "dimension_length", 0.6);
    assert(editedHeightDimension.ok);
    const auto *heightGeometry = std::get_if<DimensionGeometry>(&editedHeightDimension.geometry);
    assert(heightGeometry != nullptr);
    assert(heightGeometry->kind == DimensionKind::Height);
    assert(nearlyEqual(heightGeometry->b.x, 0.1));
    assert(nearlyEqual(heightGeometry->b.y, 0.8));

    DraftingObject diameterDimension = object("dimension_diameter", DraftingShapeKind::Dimension, DimensionGeometry{DimensionKind::Diameter, {0.1, 0.2}, {0.4, 0.2}, 0.04});
    auto editedDiameterDimension = applyNumericGeometryEdit(diameterDimension, "dimension_length", 1.0);
    assert(editedDiameterDimension.ok);
    const auto *diameterGeometry = std::get_if<DimensionGeometry>(&editedDiameterDimension.geometry);
    assert(diameterGeometry != nullptr);
    assert(diameterGeometry->kind == DimensionKind::Diameter);
    assert(nearlyEqual(diameterGeometry->b.x, 0.6));
    assert(nearlyEqual(diameterGeometry->b.y, 0.2));
    auto collapsedDimension = applyNumericGeometryEdit(dimension, "x2", 0.1);
    assert(collapsedDimension.ok);
    DraftingObject partiallyCollapsedDimension = dimension;
    partiallyCollapsedDimension.geometry = collapsedDimension.geometry;
    auto fullyCollapsedDimension = applyNumericGeometryEdit(partiallyCollapsedDimension, "y2", 0.2);
    assert(!fullyCollapsedDimension.ok);
    assert(fullyCollapsedDimension.code == DraftingResultCode::InvalidGeometry);

    auto pointX = applyNumericGeometryEdit(object("point_1", DraftingShapeKind::Point, PointGeometry{{0.1, 0.2}}), "x", 0.5);
    assert(pointX.ok);
    const auto *point = std::get_if<PointGeometry>(&pointX.geometry);
    assert(point != nullptr);
    assert(nearlyEqual(point->point.x, 0.5));
    assert(nearlyEqual(point->point.y, 0.2));

    auto badField = applyNumericGeometryEdit(line, "missing", 1.0);
    assert(!badField.ok);
    assert(badField.code == DraftingResultCode::InvalidGeometry);

    auto badValue = applyNumericGeometryEdit(line, "x1", std::numeric_limits<double>::infinity());
    assert(!badValue.ok);
    assert(badValue.code == DraftingResultCode::InvalidGeometry);

    DraftingObject mismatched = line;
    mismatched.kind = DraftingShapeKind::Circle;
    auto mismatch = applyNumericGeometryEdit(mismatched, "line_length", 2.0);
    assert(!mismatch.ok);
    assert(mismatch.code == DraftingResultCode::KindGeometryMismatch);

    return 0;
}
