#include "drafting/DraftingToolCreation.h"

#include "EdiAssert.h"
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
    EDI_CHECK(draftingToolKindFromId("point_tool") == DraftingToolKind::Point);
    EDI_CHECK(draftingToolKindFromId("line_tool") == DraftingToolKind::Line);
    EDI_CHECK(draftingToolKindFromId("horizontal_guide_tool") == DraftingToolKind::HorizontalGuide);
    EDI_CHECK(draftingToolKindFromId("angled_construction_line_tool") == DraftingToolKind::AngledConstructionLine);
    EDI_CHECK(draftingToolKindFromId("distance_dimension_tool") == DraftingToolKind::DistanceDimension);
    EDI_CHECK(draftingToolKindFromId("width_dimension_tool") == DraftingToolKind::WidthDimension);
    EDI_CHECK(draftingToolKindFromId("height_dimension_tool") == DraftingToolKind::HeightDimension);
    EDI_CHECK(draftingToolKindFromId("radius_dimension_tool") == DraftingToolKind::RadiusDimension);
    EDI_CHECK(draftingToolKindFromId("diameter_dimension_tool") == DraftingToolKind::DiameterDimension);
    // DR-13: Angular dimension is NOT a drag-created tool — the controller uses the
    // two-line-pick arm instead.  The tool-kind round-trip must still work so the
    // controller can dispatch on it, and buildDraftingObjectForTool must fail cleanly.
    EDI_CHECK(draftingToolKindFromId("angular_dimension_tool") == DraftingToolKind::AngularDimension);
    EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::AngularDimension)) == "angular_dimension");
    {
        const auto bad = build("ang_1", DraftingToolKind::AngularDimension, {0.1, 0.1}, {0.5, 0.5});
        EDI_CHECK(!bad.ok);
        EDI_CHECK(bad.code == DraftingResultCode::InvalidGeometry);
    }
    EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::Circle)) == "circle");

    // Double-arrow: a Line-geometry variant carrying BOTH arrowheads as metadata
    // flags. The single arrow carries only the end head; a plain line carries
    // neither — so a start/end mix-up fails one of these assertions.
    {
        EDI_CHECK(draftingToolKindFromId("double_arrow_tool") == DraftingToolKind::DoubleArrow);
        EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::DoubleArrow)) == "double_arrow");
        const auto da = build("da_1", DraftingToolKind::DoubleArrow, {0.1, 0.1}, {0.5, 0.5});
        EDI_CHECK(da.ok);
        EDI_CHECK(da.object.kind == DraftingShapeKind::Line);
        EDI_CHECK(da.object.metadata.lineVisual.startArrow && da.object.metadata.lineVisual.endArrow);
        const auto arrow = build("ar_1", DraftingToolKind::Arrow, {0.1, 0.1}, {0.5, 0.5});
        EDI_CHECK(!arrow.object.metadata.lineVisual.startArrow && arrow.object.metadata.lineVisual.endArrow);
        const auto line = build("ln_1", DraftingToolKind::Line, {0.1, 0.1}, {0.5, 0.5});
        EDI_CHECK(!line.object.metadata.lineVisual.startArrow && !line.object.metadata.lineVisual.endArrow);
    }

    auto point = build("point_1", DraftingToolKind::Point, {0.1, 0.2}, {0.3, 0.4});
    EDI_CHECK(point.ok);
    EDI_CHECK(point.object.kind == DraftingShapeKind::Point);
    EDI_CHECK(point.object.metadata.toolProvenance == "point");
    const auto *pointGeometry = std::get_if<PointGeometry>(&point.object.geometry);
    EDI_CHECK(pointGeometry != nullptr);
    EDI_CHECK(pointGeometry->point.x == 0.3);
    EDI_CHECK(pointGeometry->point.y == 0.4);

    auto line = build("line_1", DraftingToolKind::Line, {0.1, 0.2}, {0.8, 0.9});
    EDI_CHECK(line.ok);
    const auto *lineGeometry = std::get_if<LineGeometry>(&line.object.geometry);
    EDI_CHECK(lineGeometry != nullptr);
    EDI_CHECK(lineGeometry->a.x == 0.1);
    EDI_CHECK(lineGeometry->b.y == 0.9);

    auto rect = build("rect_1", DraftingToolKind::Rectangle, {0.8, 0.9}, {0.1, 0.2});
    EDI_CHECK(rect.ok);
    const auto *rectGeometry = std::get_if<RectangleGeometry>(&rect.object.geometry);
    EDI_CHECK(rectGeometry != nullptr);
    EDI_CHECK(rectGeometry->origin.x == 0.1);
    EDI_CHECK(rectGeometry->origin.y == 0.2);
    EDI_CHECK(nearlyEqual(rectGeometry->width, 0.7));
    EDI_CHECK(nearlyEqual(rectGeometry->height, 0.7));

    auto circle = build("circle_1", DraftingToolKind::Circle, {0.5, 0.5}, {0.5, 0.75});
    EDI_CHECK(circle.ok);
    const auto *circleGeometry = std::get_if<CircleGeometry>(&circle.object.geometry);
    EDI_CHECK(circleGeometry != nullptr);
    EDI_CHECK(nearlyEqual(circleGeometry->radius, 0.25));

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
        EDI_CHECK(fixed.ok);
        EDI_CHECK(nearlyEqual(std::get<CircleGeometry>(fixed.object.geometry).radius, 0.1));
        EDI_CHECK(nearlyEqual(std::get<CircleGeometry>(fixed.object.geometry).center.x, 0.5));

        request.fixedRadius = 5.0; // clamps to the unit document space, like the gesture
        auto clamped = buildDraftingObjectForTool(request);
        EDI_CHECK(nearlyEqual(std::get<CircleGeometry>(clamped.object.geometry).radius, 1.0));

        request.fixedRadius = -0.2; // invalid option -> gesture-sized
        auto negative = buildDraftingObjectForTool(request);
        EDI_CHECK(nearlyEqual(std::get<CircleGeometry>(negative.object.geometry).radius, 0.25));

        request.fixedRadius = std::numeric_limits<double>::quiet_NaN();
        auto nan = buildDraftingObjectForTool(request);
        EDI_CHECK(nearlyEqual(std::get<CircleGeometry>(nan.object.geometry).radius, 0.25));
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
        EDI_CHECK(polygon.ok);
        EDI_CHECK(polygon.object.kind == DraftingShapeKind::Polygon);
        const auto *polygonGeometry = std::get_if<PolygonGeometry>(&polygon.object.geometry);
        EDI_CHECK(polygonGeometry != nullptr);
        EDI_CHECK(polygonGeometry->vertices.size() == 6); // default sides
        // Every vertex lies on the circumscribed circle of radius 0.3.
        for (const Point2D &v : polygonGeometry->vertices) {
            const double r = std::hypot(v.x - 0.5, v.y - 0.5);
            EDI_CHECK(nearlyEqual(r, 0.3));
        }
        // First vertex is at the rotation angle (default 30deg).
        const double firstAngle = std::atan2(polygonGeometry->vertices[0].y - 0.5,
                                             polygonGeometry->vertices[0].x - 0.5);
        EDI_CHECK(nearlyEqual(firstAngle, 30.0 * M_PI / 180.0));

        // Custom side count is honoured and clamped to [3, 24].
        request.polygonSides = 3;
        auto triangle = buildDraftingObjectForTool(request);
        EDI_CHECK(std::get<PolygonGeometry>(triangle.object.geometry).vertices.size() == 3);
        request.polygonSides = 100;
        auto clamped = buildDraftingObjectForTool(request);
        EDI_CHECK(std::get<PolygonGeometry>(clamped.object.geometry).vertices.size() == 24);
        request.polygonSides = 1;
        auto clampedLow = buildDraftingObjectForTool(request);
        EDI_CHECK(std::get<PolygonGeometry>(clampedLow.object.geometry).vertices.size() == 3);

        // fixedRadius reaches the polygon's circumscribed circle too.
        request.polygonSides = 6;
        request.fixedRadius = 0.15;
        auto fixedPolygon = buildDraftingObjectForTool(request);
        for (const Point2D &v : std::get<PolygonGeometry>(fixedPolygon.object.geometry).vertices) {
            EDI_CHECK(nearlyEqual(std::hypot(v.x - 0.5, v.y - 0.5), 0.15));
        }
    }
    EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::RegularPolygon)) == "regular_polygon");
    EDI_CHECK(draftingToolKindFromId("regular_polygon_tool") == DraftingToolKind::RegularPolygon);

    // Arc: centre + a point giving radius and start angle; end = start + sweep.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Arc;
        request.objectId = "arc_1";
        request.start = {0.5, 0.5};   // centre
        request.end = {0.8, 0.5};     // radius 0.3, start angle 0deg
        request.toolProvenance = "arc";
        auto arc = buildDraftingObjectForTool(request);
        EDI_CHECK(arc.ok);
        EDI_CHECK(arc.object.kind == DraftingShapeKind::Arc);
        const auto *arcGeometry = std::get_if<ArcGeometry>(&arc.object.geometry);
        EDI_CHECK(arcGeometry != nullptr);
        EDI_CHECK(nearlyEqual(arcGeometry->radius, 0.3));
        EDI_CHECK(nearlyEqual(arcGeometry->startAngleDeg, 0.0));
        EDI_CHECK(nearlyEqual(arcGeometry->endAngleDeg, 105.0)); // default sweep
        // A custom sweep is honoured.
        request.arcSweepDeg = 60.0;
        auto narrow = buildDraftingObjectForTool(request);
        EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(narrow.object.geometry).endAngleDeg, 60.0));

        // fixedRadius replaces the gesture distance but the click still aims
        // the arc: the start angle keeps coming from the second click.
        request.fixedRadius = 0.05;
        request.end = {0.5, 0.8}; // 90deg, gesture distance 0.3
        auto fixedArc = buildDraftingObjectForTool(request);
        EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(fixedArc.object.geometry).radius, 0.05));
        EDI_CHECK(nearlyEqual(std::get<ArcGeometry>(fixedArc.object.geometry).startAngleDeg, 90.0));
    }
    EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::Arc)) == "arc");
    EDI_CHECK(draftingToolKindFromId("arc_tool") == DraftingToolKind::Arc);

    auto horizontalGuide = build("guide_h", DraftingToolKind::HorizontalGuide, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(horizontalGuide.ok);
    EDI_CHECK(horizontalGuide.object.kind == DraftingShapeKind::Guide);
    const auto *horizontalGuideGeometry = std::get_if<GuideGeometry>(&horizontalGuide.object.geometry);
    EDI_CHECK(horizontalGuideGeometry != nullptr);
    EDI_CHECK(horizontalGuideGeometry->orientation == GuideOrientation::Horizontal);
    EDI_CHECK(nearlyEqual(horizontalGuideGeometry->position, 0.4));

    auto verticalGuide = build("guide_v", DraftingToolKind::VerticalGuide, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(verticalGuide.ok);
    const auto *verticalGuideGeometry = std::get_if<GuideGeometry>(&verticalGuide.object.geometry);
    EDI_CHECK(verticalGuideGeometry != nullptr);
    EDI_CHECK(verticalGuideGeometry->orientation == GuideOrientation::Vertical);
    EDI_CHECK(nearlyEqual(verticalGuideGeometry->position, 0.7));

    auto horizontalConstruction = build("construction_h", DraftingToolKind::HorizontalConstructionLine, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(horizontalConstruction.ok);
    EDI_CHECK(horizontalConstruction.object.kind == DraftingShapeKind::ConstructionLine);
    EDI_CHECK(horizontalConstruction.object.metadata.toolProvenance == "horizontal_construction_line");
    const auto *horizontalConstructionGeometry = std::get_if<ConstructionLineGeometry>(&horizontalConstruction.object.geometry);
    EDI_CHECK(horizontalConstructionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(horizontalConstructionGeometry->a.x, 0.0));
    EDI_CHECK(nearlyEqual(horizontalConstructionGeometry->a.y, 0.4));
    EDI_CHECK(nearlyEqual(horizontalConstructionGeometry->b.x, 1.0));
    EDI_CHECK(nearlyEqual(horizontalConstructionGeometry->b.y, 0.4));

    auto verticalConstruction = build("construction_v", DraftingToolKind::VerticalConstructionLine, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(verticalConstruction.ok);
    const auto *verticalConstructionGeometry = std::get_if<ConstructionLineGeometry>(&verticalConstruction.object.geometry);
    EDI_CHECK(verticalConstructionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(verticalConstructionGeometry->a.x, 0.7));
    EDI_CHECK(nearlyEqual(verticalConstructionGeometry->a.y, 0.0));
    EDI_CHECK(nearlyEqual(verticalConstructionGeometry->b.x, 0.7));
    EDI_CHECK(nearlyEqual(verticalConstructionGeometry->b.y, 1.0));

    auto angledConstruction = build("construction_a", DraftingToolKind::AngledConstructionLine, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(angledConstruction.ok);
    const auto *angledConstructionGeometry = std::get_if<ConstructionLineGeometry>(&angledConstruction.object.geometry);
    EDI_CHECK(angledConstructionGeometry != nullptr);
    EDI_CHECK(nearlyEqual(angledConstructionGeometry->a.x, 0.1));
    EDI_CHECK(nearlyEqual(angledConstructionGeometry->a.y, 0.2));
    EDI_CHECK(nearlyEqual(angledConstructionGeometry->b.x, 0.7));
    EDI_CHECK(nearlyEqual(angledConstructionGeometry->b.y, 0.4));

    auto zeroConstruction = build("construction_zero", DraftingToolKind::AngledConstructionLine, {0.2, 0.2}, {0.2, 0.2});
    EDI_CHECK(!zeroConstruction.ok);
    EDI_CHECK(zeroConstruction.code == DraftingResultCode::InvalidGeometry);

    auto distanceDimension = build("dimension_1", DraftingToolKind::DistanceDimension, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(distanceDimension.ok);
    EDI_CHECK(distanceDimension.object.kind == DraftingShapeKind::Dimension);
    EDI_CHECK(distanceDimension.object.metadata.toolProvenance == "distance_dimension");
    const auto *dimensionGeometry = std::get_if<DimensionGeometry>(&distanceDimension.object.geometry);
    EDI_CHECK(dimensionGeometry != nullptr);
    EDI_CHECK(dimensionGeometry->kind == DimensionKind::Distance);
    EDI_CHECK(nearlyEqual(dimensionGeometry->a.x, 0.1));
    EDI_CHECK(nearlyEqual(dimensionGeometry->a.y, 0.2));
    EDI_CHECK(nearlyEqual(dimensionGeometry->b.x, 0.7));
    EDI_CHECK(nearlyEqual(dimensionGeometry->b.y, 0.4));
    EDI_CHECK(nearlyEqual(dimensionGeometry->offset, 0.04));

    auto widthDimension = build("dimension_width", DraftingToolKind::WidthDimension, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(widthDimension.ok);
    const auto *widthDimensionGeometry = std::get_if<DimensionGeometry>(&widthDimension.object.geometry);
    EDI_CHECK(widthDimensionGeometry != nullptr);
    EDI_CHECK(widthDimensionGeometry->kind == DimensionKind::Width);
    EDI_CHECK(nearlyEqual(widthDimensionGeometry->a.x, 0.1));
    EDI_CHECK(nearlyEqual(widthDimensionGeometry->a.y, 0.2));
    EDI_CHECK(nearlyEqual(widthDimensionGeometry->b.x, 0.7));
    EDI_CHECK(nearlyEqual(widthDimensionGeometry->b.y, 0.2));

    auto heightDimension = build("dimension_height", DraftingToolKind::HeightDimension, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(heightDimension.ok);
    const auto *heightDimensionGeometry = std::get_if<DimensionGeometry>(&heightDimension.object.geometry);
    EDI_CHECK(heightDimensionGeometry != nullptr);
    EDI_CHECK(heightDimensionGeometry->kind == DimensionKind::Height);
    EDI_CHECK(nearlyEqual(heightDimensionGeometry->a.x, 0.1));
    EDI_CHECK(nearlyEqual(heightDimensionGeometry->a.y, 0.2));
    EDI_CHECK(nearlyEqual(heightDimensionGeometry->b.x, 0.1));
    EDI_CHECK(nearlyEqual(heightDimensionGeometry->b.y, 0.4));

    auto radiusDimension = build("dimension_radius", DraftingToolKind::RadiusDimension, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(radiusDimension.ok);
    const auto *radiusDimensionGeometry = std::get_if<DimensionGeometry>(&radiusDimension.object.geometry);
    EDI_CHECK(radiusDimensionGeometry != nullptr);
    EDI_CHECK(radiusDimensionGeometry->kind == DimensionKind::Radius);

    auto diameterDimension = build("dimension_diameter", DraftingToolKind::DiameterDimension, {0.1, 0.2}, {0.7, 0.4});
    EDI_CHECK(diameterDimension.ok);
    const auto *diameterDimensionGeometry = std::get_if<DimensionGeometry>(&diameterDimension.object.geometry);
    EDI_CHECK(diameterDimensionGeometry != nullptr);
    EDI_CHECK(diameterDimensionGeometry->kind == DimensionKind::Diameter);

    auto zeroDimension = build("dimension_zero", DraftingToolKind::DistanceDimension, {0.2, 0.2}, {0.2, 0.2});
    EDI_CHECK(!zeroDimension.ok);
    EDI_CHECK(zeroDimension.code == DraftingResultCode::InvalidGeometry);

    auto unknown = build("bad_1", DraftingToolKind::Unknown, {0.0, 0.0}, {1.0, 1.0});
    EDI_CHECK(!unknown.ok);
    EDI_CHECK(unknown.code == DraftingResultCode::InvalidGeometry);

    auto emptyId = build("", DraftingToolKind::Point, {0.0, 0.0}, {1.0, 1.0});
    EDI_CHECK(!emptyId.ok);
    EDI_CHECK(emptyId.code == DraftingResultCode::EmptyObjectId);

    // Polyline: a multi-click tool — the request carries its whole click
    // trail in `vertices`; start/end are ignored for this kind.
    {
        EDI_CHECK(draftingToolKindFromId("polyline_tool") == DraftingToolKind::Polyline);
        EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::Polyline)) == "polyline");

        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Polyline;
        request.objectId = "poly_1";
        request.vertices = {{0.1, 0.1}, {0.4, 0.2}, {0.5, 0.6}};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        EDI_CHECK(built.ok);
        EDI_CHECK(built.object.kind == DraftingShapeKind::Polyline);
        const auto *polyline = std::get_if<PolylineGeometry>(&built.object.geometry);
        EDI_CHECK(polyline != nullptr && polyline->vertices.size() == 3);
        EDI_CHECK(polyline->vertices[2].y == 0.6);

        // The geometry validation gate holds: one vertex is not a polyline.
        DraftingToolCreationRequest tooShort = request;
        tooShort.vertices = {{0.1, 0.1}};
        EDI_CHECK(!buildDraftingObjectForTool(tooShort).ok);
    }

    // Arrow: the line tool's variant — same Line geometry/kind, distinguished
    // only by the endArrow metadata flag. A plain line never carries it.
    {
        EDI_CHECK(draftingToolKindFromId("arrow_tool") == DraftingToolKind::Arrow);
        EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::Arrow)) == "arrow");

        DraftingToolCreationRequest arrow;
        arrow.tool = DraftingToolKind::Arrow;
        arrow.objectId = "arrow_1";
        arrow.start = {0.1, 0.1};
        arrow.end = {0.6, 0.4};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(arrow);
        EDI_CHECK(built.ok);
        EDI_CHECK(built.object.kind == DraftingShapeKind::Line); // geometry is a line
        EDI_CHECK(built.object.metadata.lineVisual.endArrow);
        EDI_CHECK(std::get_if<LineGeometry>(&built.object.geometry) != nullptr);

        DraftingToolCreationRequest plainLine = arrow;
        plainLine.tool = DraftingToolKind::Line;
        plainLine.objectId = "line_x";
        const DraftingObjectBuildResult line = buildDraftingObjectForTool(plainLine);
        EDI_CHECK(line.ok && !line.object.metadata.lineVisual.endArrow);
    }

    // Wall: a two-click tool (click a, click b) building a thick line. Like
    // circle's fixedRadius, the band thickness is a tool option (wallThickness)
    // defaulting to 0.1; request.start -> a, request.end -> b.
    {
        EDI_CHECK(draftingToolKindFromId("wall_tool") == DraftingToolKind::Wall);
        EDI_CHECK(std::string(draftingToolKindName(DraftingToolKind::Wall)) == "wall");

        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::Wall;
        request.objectId = "wall_1";
        request.start = {0.2, 0.5};
        request.end = {0.8, 0.5};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        EDI_CHECK(built.ok);
        EDI_CHECK(built.object.kind == DraftingShapeKind::Wall);
        const auto *wall = std::get_if<WallGeometry>(&built.object.geometry);
        EDI_CHECK(wall != nullptr);
        EDI_CHECK(nearlyEqual(wall->a.x, 0.2) && nearlyEqual(wall->a.y, 0.5)); // start -> a
        EDI_CHECK(nearlyEqual(wall->b.x, 0.8) && nearlyEqual(wall->b.y, 0.5)); // end -> b
        EDI_CHECK(nearlyEqual(wall->thickness, 0.1)); // default option, a visible band

        // A custom thickness option overrides the default; the endpoints are
        // untouched (thickness is not a gesture distance).
        request.wallThickness = 0.25;
        const DraftingObjectBuildResult thick = buildDraftingObjectForTool(request);
        EDI_CHECK(nearlyEqual(std::get<WallGeometry>(thick.object.geometry).thickness, 0.25));
        EDI_CHECK(nearlyEqual(std::get<WallGeometry>(thick.object.geometry).a.x, 0.2)); // a/b unchanged
    }

    return 0;
}
