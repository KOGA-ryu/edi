#include "drafting/DraftingToolCreation.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

DraftingObjectBuildResult build(std::string id, DraftingToolKind tool, Point2D start, Point2D end)
{
    DraftingToolCreationRequest request;
    request.tool = tool;
    request.objectId = std::move(id);
    request.layerId = "default";
    request.start = start;
    request.end = end;
    request.toolProvenance = draftingToolKindName(tool);
    return buildDraftingObjectForTool(request);
}

} // namespace

int main()
{
    assert(draftingToolKindFromId("point_tool") == DraftingToolKind::Point);
    assert(draftingToolKindFromId("line_tool") == DraftingToolKind::Line);
    assert(draftingToolKindFromId("horizontal_guide_tool") == DraftingToolKind::HorizontalGuide);
    assert(draftingToolKindFromId("angled_construction_line_tool") == DraftingToolKind::AngledConstructionLine);
    assert(draftingToolKindFromId("distance_dimension_tool") == DraftingToolKind::DistanceDimension);
    assert(draftingToolKindFromId("width_dimension_tool") == DraftingToolKind::WidthDimension);
    assert(draftingToolKindFromId("height_dimension_tool") == DraftingToolKind::HeightDimension);
    assert(draftingToolKindFromId("radius_dimension_tool") == DraftingToolKind::RadiusDimension);
    assert(draftingToolKindFromId("diameter_dimension_tool") == DraftingToolKind::DiameterDimension);
    assert(std::string(draftingToolKindName(DraftingToolKind::Circle)) == "circle");

    auto point = build("point_1", DraftingToolKind::Point, {0.1, 0.2}, {0.3, 0.4});
    assert(point.ok);
    assert(point.object.kind == DraftingShapeKind::Point);
    assert(point.object.metadata.toolProvenance == "point");
    const auto *pointGeometry = std::get_if<PointGeometry>(&point.object.geometry);
    assert(pointGeometry != nullptr);
    assert(pointGeometry->point.x == 0.3);
    assert(pointGeometry->point.y == 0.4);

    auto line = build("line_1", DraftingToolKind::Line, {0.1, 0.2}, {0.8, 0.9});
    assert(line.ok);
    const auto *lineGeometry = std::get_if<LineGeometry>(&line.object.geometry);
    assert(lineGeometry != nullptr);
    assert(lineGeometry->a.x == 0.1);
    assert(lineGeometry->b.y == 0.9);

    auto rect = build("rect_1", DraftingToolKind::Rectangle, {0.8, 0.9}, {0.1, 0.2});
    assert(rect.ok);
    const auto *rectGeometry = std::get_if<RectangleGeometry>(&rect.object.geometry);
    assert(rectGeometry != nullptr);
    assert(rectGeometry->origin.x == 0.1);
    assert(rectGeometry->origin.y == 0.2);
    assert(nearlyEqual(rectGeometry->width, 0.7));
    assert(nearlyEqual(rectGeometry->height, 0.7));

    auto circle = build("circle_1", DraftingToolKind::Circle, {0.5, 0.5}, {0.5, 0.75});
    assert(circle.ok);
    const auto *circleGeometry = std::get_if<CircleGeometry>(&circle.object.geometry);
    assert(circleGeometry != nullptr);
    assert(nearlyEqual(circleGeometry->radius, 0.25));

    auto horizontalGuide = build("guide_h", DraftingToolKind::HorizontalGuide, {0.1, 0.2}, {0.7, 0.4});
    assert(horizontalGuide.ok);
    assert(horizontalGuide.object.kind == DraftingShapeKind::Guide);
    const auto *horizontalGuideGeometry = std::get_if<GuideGeometry>(&horizontalGuide.object.geometry);
    assert(horizontalGuideGeometry != nullptr);
    assert(horizontalGuideGeometry->orientation == GuideOrientation::Horizontal);
    assert(nearlyEqual(horizontalGuideGeometry->position, 0.4));

    auto verticalGuide = build("guide_v", DraftingToolKind::VerticalGuide, {0.1, 0.2}, {0.7, 0.4});
    assert(verticalGuide.ok);
    const auto *verticalGuideGeometry = std::get_if<GuideGeometry>(&verticalGuide.object.geometry);
    assert(verticalGuideGeometry != nullptr);
    assert(verticalGuideGeometry->orientation == GuideOrientation::Vertical);
    assert(nearlyEqual(verticalGuideGeometry->position, 0.7));

    auto horizontalConstruction = build("construction_h", DraftingToolKind::HorizontalConstructionLine, {0.1, 0.2}, {0.7, 0.4});
    assert(horizontalConstruction.ok);
    assert(horizontalConstruction.object.kind == DraftingShapeKind::ConstructionLine);
    assert(horizontalConstruction.object.metadata.toolProvenance == "horizontal_construction_line");
    const auto *horizontalConstructionGeometry = std::get_if<ConstructionLineGeometry>(&horizontalConstruction.object.geometry);
    assert(horizontalConstructionGeometry != nullptr);
    assert(nearlyEqual(horizontalConstructionGeometry->a.x, 0.0));
    assert(nearlyEqual(horizontalConstructionGeometry->a.y, 0.4));
    assert(nearlyEqual(horizontalConstructionGeometry->b.x, 1.0));
    assert(nearlyEqual(horizontalConstructionGeometry->b.y, 0.4));

    auto verticalConstruction = build("construction_v", DraftingToolKind::VerticalConstructionLine, {0.1, 0.2}, {0.7, 0.4});
    assert(verticalConstruction.ok);
    const auto *verticalConstructionGeometry = std::get_if<ConstructionLineGeometry>(&verticalConstruction.object.geometry);
    assert(verticalConstructionGeometry != nullptr);
    assert(nearlyEqual(verticalConstructionGeometry->a.x, 0.7));
    assert(nearlyEqual(verticalConstructionGeometry->a.y, 0.0));
    assert(nearlyEqual(verticalConstructionGeometry->b.x, 0.7));
    assert(nearlyEqual(verticalConstructionGeometry->b.y, 1.0));

    auto angledConstruction = build("construction_a", DraftingToolKind::AngledConstructionLine, {0.1, 0.2}, {0.7, 0.4});
    assert(angledConstruction.ok);
    const auto *angledConstructionGeometry = std::get_if<ConstructionLineGeometry>(&angledConstruction.object.geometry);
    assert(angledConstructionGeometry != nullptr);
    assert(nearlyEqual(angledConstructionGeometry->a.x, 0.1));
    assert(nearlyEqual(angledConstructionGeometry->a.y, 0.2));
    assert(nearlyEqual(angledConstructionGeometry->b.x, 0.7));
    assert(nearlyEqual(angledConstructionGeometry->b.y, 0.4));

    auto zeroConstruction = build("construction_zero", DraftingToolKind::AngledConstructionLine, {0.2, 0.2}, {0.2, 0.2});
    assert(!zeroConstruction.ok);
    assert(zeroConstruction.code == DraftingResultCode::InvalidGeometry);

    auto distanceDimension = build("dimension_1", DraftingToolKind::DistanceDimension, {0.1, 0.2}, {0.7, 0.4});
    assert(distanceDimension.ok);
    assert(distanceDimension.object.kind == DraftingShapeKind::Dimension);
    assert(distanceDimension.object.metadata.toolProvenance == "distance_dimension");
    const auto *dimensionGeometry = std::get_if<DimensionGeometry>(&distanceDimension.object.geometry);
    assert(dimensionGeometry != nullptr);
    assert(dimensionGeometry->kind == DimensionKind::Distance);
    assert(nearlyEqual(dimensionGeometry->a.x, 0.1));
    assert(nearlyEqual(dimensionGeometry->a.y, 0.2));
    assert(nearlyEqual(dimensionGeometry->b.x, 0.7));
    assert(nearlyEqual(dimensionGeometry->b.y, 0.4));
    assert(nearlyEqual(dimensionGeometry->offset, 0.04));

    auto widthDimension = build("dimension_width", DraftingToolKind::WidthDimension, {0.1, 0.2}, {0.7, 0.4});
    assert(widthDimension.ok);
    const auto *widthDimensionGeometry = std::get_if<DimensionGeometry>(&widthDimension.object.geometry);
    assert(widthDimensionGeometry != nullptr);
    assert(widthDimensionGeometry->kind == DimensionKind::Width);
    assert(nearlyEqual(widthDimensionGeometry->a.x, 0.1));
    assert(nearlyEqual(widthDimensionGeometry->a.y, 0.2));
    assert(nearlyEqual(widthDimensionGeometry->b.x, 0.7));
    assert(nearlyEqual(widthDimensionGeometry->b.y, 0.2));

    auto heightDimension = build("dimension_height", DraftingToolKind::HeightDimension, {0.1, 0.2}, {0.7, 0.4});
    assert(heightDimension.ok);
    const auto *heightDimensionGeometry = std::get_if<DimensionGeometry>(&heightDimension.object.geometry);
    assert(heightDimensionGeometry != nullptr);
    assert(heightDimensionGeometry->kind == DimensionKind::Height);
    assert(nearlyEqual(heightDimensionGeometry->a.x, 0.1));
    assert(nearlyEqual(heightDimensionGeometry->a.y, 0.2));
    assert(nearlyEqual(heightDimensionGeometry->b.x, 0.1));
    assert(nearlyEqual(heightDimensionGeometry->b.y, 0.4));

    auto radiusDimension = build("dimension_radius", DraftingToolKind::RadiusDimension, {0.1, 0.2}, {0.7, 0.4});
    assert(radiusDimension.ok);
    const auto *radiusDimensionGeometry = std::get_if<DimensionGeometry>(&radiusDimension.object.geometry);
    assert(radiusDimensionGeometry != nullptr);
    assert(radiusDimensionGeometry->kind == DimensionKind::Radius);

    auto diameterDimension = build("dimension_diameter", DraftingToolKind::DiameterDimension, {0.1, 0.2}, {0.7, 0.4});
    assert(diameterDimension.ok);
    const auto *diameterDimensionGeometry = std::get_if<DimensionGeometry>(&diameterDimension.object.geometry);
    assert(diameterDimensionGeometry != nullptr);
    assert(diameterDimensionGeometry->kind == DimensionKind::Diameter);

    auto zeroDimension = build("dimension_zero", DraftingToolKind::DistanceDimension, {0.2, 0.2}, {0.2, 0.2});
    assert(!zeroDimension.ok);
    assert(zeroDimension.code == DraftingResultCode::InvalidGeometry);

    auto unknown = build("bad_1", DraftingToolKind::Unknown, {0.0, 0.0}, {1.0, 1.0});
    assert(!unknown.ok);
    assert(unknown.code == DraftingResultCode::InvalidGeometry);

    auto emptyId = build("", DraftingToolKind::Point, {0.0, 0.0}, {1.0, 1.0});
    assert(!emptyId.ok);
    assert(emptyId.code == DraftingResultCode::EmptyObjectId);

    return 0;
}
