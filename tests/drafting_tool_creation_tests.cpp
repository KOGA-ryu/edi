#include "drafting/DraftingToolCreation.h"

#include <cassert>
#include <cmath>
#include <limits>
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

    // Double-arrow: a Line-geometry variant carrying BOTH arrowheads as metadata
    // flags. The single arrow carries only the end head; a plain line carries
    // neither — so a start/end mix-up fails one of these assertions.
    {
        assert(draftingToolKindFromId("double_arrow_tool") == DraftingToolKind::DoubleArrow);
        assert(std::string(draftingToolKindName(DraftingToolKind::DoubleArrow)) == "double_arrow");
        const auto da = build("da_1", DraftingToolKind::DoubleArrow, {0.1, 0.1}, {0.5, 0.5});
        assert(da.ok);
        assert(da.object.kind == DraftingShapeKind::Line);
        assert(da.object.metadata.lineVisual.startArrow && da.object.metadata.lineVisual.endArrow);
        const auto arrow = build("ar_1", DraftingToolKind::Arrow, {0.1, 0.1}, {0.5, 0.5});
        assert(!arrow.object.metadata.lineVisual.startArrow && arrow.object.metadata.lineVisual.endArrow);
        const auto line = build("ln_1", DraftingToolKind::Line, {0.1, 0.1}, {0.5, 0.5});
        assert(!line.object.metadata.lineVisual.startArrow && !line.object.metadata.lineVisual.endArrow);
    }

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

    // Parametric radius (#30): a positive fixedRadius overrides the gesture
    // for every radius-from-gesture tool; zero/negative/non-finite fall back.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Circle;
        request.objectId = "circle_fixed";
        request.start = {0.5, 0.5};
        request.end = {0.5, 0.75}; // gesture says 0.25...
        request.fixedRadius = 0.1; // ...the option wins
        auto fixed = buildDraftingObjectForTool(request);
        assert(fixed.ok);
        assert(nearlyEqual(std::get<CircleGeometry>(fixed.object.geometry).radius, 0.1));
        assert(nearlyEqual(std::get<CircleGeometry>(fixed.object.geometry).center.x, 0.5));

        request.fixedRadius = 5.0; // clamps to the unit document space, like the gesture
        auto clamped = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<CircleGeometry>(clamped.object.geometry).radius, 1.0));

        request.fixedRadius = -0.2; // invalid option -> gesture-sized
        auto negative = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<CircleGeometry>(negative.object.geometry).radius, 0.25));

        request.fixedRadius = std::numeric_limits<double>::quiet_NaN();
        auto nan = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<CircleGeometry>(nan.object.geometry).radius, 0.25));
    }

    // Regular polygon: centre + radius point, default 6 sides at 30deg.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::RegularPolygon;
        request.objectId = "polygon_1";
        request.start = {0.5, 0.5};   // centre
        request.end = {0.8, 0.5};     // radius 0.3 to the right
        request.toolProvenance = "regular_polygon";
        auto polygon = buildDraftingObjectForTool(request);
        assert(polygon.ok);
        assert(polygon.object.kind == DraftingShapeKind::Polygon);
        const auto *polygonGeometry = std::get_if<PolygonGeometry>(&polygon.object.geometry);
        assert(polygonGeometry != nullptr);
        assert(polygonGeometry->vertices.size() == 6); // default sides
        // Every vertex lies on the circumscribed circle of radius 0.3.
        for (const Point2D &v : polygonGeometry->vertices) {
            const double r = std::hypot(v.x - 0.5, v.y - 0.5);
            assert(nearlyEqual(r, 0.3));
        }
        // First vertex is at the rotation angle (default 30deg).
        const double firstAngle = std::atan2(polygonGeometry->vertices[0].y - 0.5,
                                             polygonGeometry->vertices[0].x - 0.5);
        assert(nearlyEqual(firstAngle, 30.0 * M_PI / 180.0));

        // Custom side count is honoured and clamped to [3, 24].
        request.polygonSides = 3;
        auto triangle = buildDraftingObjectForTool(request);
        assert(std::get<PolygonGeometry>(triangle.object.geometry).vertices.size() == 3);
        request.polygonSides = 100;
        auto clamped = buildDraftingObjectForTool(request);
        assert(std::get<PolygonGeometry>(clamped.object.geometry).vertices.size() == 24);
        request.polygonSides = 1;
        auto clampedLow = buildDraftingObjectForTool(request);
        assert(std::get<PolygonGeometry>(clampedLow.object.geometry).vertices.size() == 3);

        // fixedRadius reaches the polygon's circumscribed circle too.
        request.polygonSides = 6;
        request.fixedRadius = 0.15;
        auto fixedPolygon = buildDraftingObjectForTool(request);
        for (const Point2D &v : std::get<PolygonGeometry>(fixedPolygon.object.geometry).vertices) {
            assert(nearlyEqual(std::hypot(v.x - 0.5, v.y - 0.5), 0.15));
        }
    }
    assert(std::string(draftingToolKindName(DraftingToolKind::RegularPolygon)) == "regular_polygon");
    assert(draftingToolKindFromId("regular_polygon_tool") == DraftingToolKind::RegularPolygon);

    // Arc: centre + a point giving radius and start angle; end = start + sweep.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Arc;
        request.objectId = "arc_1";
        request.start = {0.5, 0.5};   // centre
        request.end = {0.8, 0.5};     // radius 0.3, start angle 0deg
        request.toolProvenance = "arc";
        auto arc = buildDraftingObjectForTool(request);
        assert(arc.ok);
        assert(arc.object.kind == DraftingShapeKind::Arc);
        const auto *arcGeometry = std::get_if<ArcGeometry>(&arc.object.geometry);
        assert(arcGeometry != nullptr);
        assert(nearlyEqual(arcGeometry->radius, 0.3));
        assert(nearlyEqual(arcGeometry->startAngleDeg, 0.0));
        assert(nearlyEqual(arcGeometry->endAngleDeg, 105.0)); // default sweep
        // A custom sweep is honoured.
        request.arcSweepDeg = 60.0;
        auto narrow = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<ArcGeometry>(narrow.object.geometry).endAngleDeg, 60.0));

        // fixedRadius replaces the gesture distance but the click still aims
        // the arc: the start angle keeps coming from the second click.
        request.fixedRadius = 0.05;
        request.end = {0.5, 0.8}; // 90deg, gesture distance 0.3
        auto fixedArc = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<ArcGeometry>(fixedArc.object.geometry).radius, 0.05));
        assert(nearlyEqual(std::get<ArcGeometry>(fixedArc.object.geometry).startAngleDeg, 90.0));
    }
    assert(std::string(draftingToolKindName(DraftingToolKind::Arc)) == "arc");
    assert(draftingToolKindFromId("arc_tool") == DraftingToolKind::Arc);

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

    // Polyline: a multi-click tool — the request carries its whole click
    // trail in `vertices`; start/end are ignored for this kind.
    {
        assert(draftingToolKindFromId("polyline_tool") == DraftingToolKind::Polyline);
        assert(std::string(draftingToolKindName(DraftingToolKind::Polyline)) == "polyline");

        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Polyline;
        request.objectId = "poly_1";
        request.vertices = {{0.1, 0.1}, {0.4, 0.2}, {0.5, 0.6}};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        assert(built.ok);
        assert(built.object.kind == DraftingShapeKind::Polyline);
        const auto *polyline = std::get_if<PolylineGeometry>(&built.object.geometry);
        assert(polyline != nullptr && polyline->vertices.size() == 3);
        assert(polyline->vertices[2].y == 0.6);

        // The geometry validation gate holds: one vertex is not a polyline.
        DraftingToolCreationRequest tooShort = request;
        tooShort.vertices = {{0.1, 0.1}};
        assert(!buildDraftingObjectForTool(tooShort).ok);
    }

    // Arrow: the line tool's variant — same Line geometry/kind, distinguished
    // only by the endArrow metadata flag. A plain line never carries it.
    {
        assert(draftingToolKindFromId("arrow_tool") == DraftingToolKind::Arrow);
        assert(std::string(draftingToolKindName(DraftingToolKind::Arrow)) == "arrow");

        DraftingToolCreationRequest arrow;
        arrow.tool = DraftingToolKind::Arrow;
        arrow.objectId = "arrow_1";
        arrow.start = {0.1, 0.1};
        arrow.end = {0.6, 0.4};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(arrow);
        assert(built.ok);
        assert(built.object.kind == DraftingShapeKind::Line); // geometry is a line
        assert(built.object.metadata.lineVisual.endArrow);
        assert(std::get_if<LineGeometry>(&built.object.geometry) != nullptr);

        DraftingToolCreationRequest plainLine = arrow;
        plainLine.tool = DraftingToolKind::Line;
        plainLine.objectId = "line_x";
        const DraftingObjectBuildResult line = buildDraftingObjectForTool(plainLine);
        assert(line.ok && !line.object.metadata.lineVisual.endArrow);
    }

    // Wall: a two-click tool (click a, click b) building a thick line. Like
    // circle's fixedRadius, the band thickness is a tool option (wallThickness)
    // defaulting to 0.1; request.start -> a, request.end -> b.
    {
        assert(draftingToolKindFromId("wall_tool") == DraftingToolKind::Wall);
        assert(std::string(draftingToolKindName(DraftingToolKind::Wall)) == "wall");

        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Wall;
        request.objectId = "wall_1";
        request.start = {0.2, 0.5};
        request.end = {0.8, 0.5};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        assert(built.ok);
        assert(built.object.kind == DraftingShapeKind::Wall);
        const auto *wall = std::get_if<WallGeometry>(&built.object.geometry);
        assert(wall != nullptr);
        assert(nearlyEqual(wall->a.x, 0.2) && nearlyEqual(wall->a.y, 0.5)); // start -> a
        assert(nearlyEqual(wall->b.x, 0.8) && nearlyEqual(wall->b.y, 0.5)); // end -> b
        assert(nearlyEqual(wall->thickness, 0.1)); // default option, a visible band

        // A custom thickness option overrides the default; the endpoints are
        // untouched (thickness is not a gesture distance).
        request.wallThickness = 0.25;
        const DraftingObjectBuildResult thick = buildDraftingObjectForTool(request);
        assert(nearlyEqual(std::get<WallGeometry>(thick.object.geometry).thickness, 0.25));
        assert(nearlyEqual(std::get<WallGeometry>(thick.object.geometry).a.x, 0.2)); // a/b unchanged
    }

    return 0;
}
