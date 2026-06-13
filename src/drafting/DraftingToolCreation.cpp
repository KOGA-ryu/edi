#include "drafting/DraftingToolCreation.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingToolKind draftingToolKindFromId(const std::string &toolId)
{
    if (toolId == "select_move") {
        return DraftingToolKind::SelectMove;
    }
    if (toolId == "point_tool") {
        return DraftingToolKind::Point;
    }
    if (toolId == "line_tool") {
        return DraftingToolKind::Line;
    }
    if (toolId == "arrow_tool") {
        return DraftingToolKind::Arrow;
    }
    if (toolId == "double_arrow_tool") {
        return DraftingToolKind::DoubleArrow;
    }
    if (toolId == "rectangle_tool") {
        return DraftingToolKind::Rectangle;
    }
    if (toolId == "circle_tool") {
        return DraftingToolKind::Circle;
    }
    if (toolId == "arc_tool") {
        return DraftingToolKind::Arc;
    }
    if (toolId == "ellipse_tool") {
        return DraftingToolKind::Ellipse;
    }
    if (toolId == "regular_polygon_tool") {
        return DraftingToolKind::RegularPolygon;
    }
    if (toolId == "polyline_tool") {
        return DraftingToolKind::Polyline;
    }
    if (toolId == "text_tool") {
        return DraftingToolKind::TextAnnotation;
    }
    if (toolId == "horizontal_guide_tool") {
        return DraftingToolKind::HorizontalGuide;
    }
    if (toolId == "vertical_guide_tool") {
        return DraftingToolKind::VerticalGuide;
    }
    if (toolId == "horizontal_construction_line_tool") {
        return DraftingToolKind::HorizontalConstructionLine;
    }
    if (toolId == "vertical_construction_line_tool") {
        return DraftingToolKind::VerticalConstructionLine;
    }
    if (toolId == "angled_construction_line_tool") {
        return DraftingToolKind::AngledConstructionLine;
    }
    if (toolId == "distance_dimension_tool") {
        return DraftingToolKind::DistanceDimension;
    }
    if (toolId == "width_dimension_tool") {
        return DraftingToolKind::WidthDimension;
    }
    if (toolId == "height_dimension_tool") {
        return DraftingToolKind::HeightDimension;
    }
    if (toolId == "radius_dimension_tool") {
        return DraftingToolKind::RadiusDimension;
    }
    if (toolId == "diameter_dimension_tool") {
        return DraftingToolKind::DiameterDimension;
    }
    return DraftingToolKind::Unknown;
}

const char *draftingToolKindName(DraftingToolKind kind)
{
    switch (kind) {
    case DraftingToolKind::SelectMove:
        return "select_move";
    case DraftingToolKind::Point:
        return "point";
    case DraftingToolKind::Line:
        return "line";
    case DraftingToolKind::Arrow:
        return "arrow";
    case DraftingToolKind::DoubleArrow:
        return "double_arrow";
    case DraftingToolKind::Rectangle:
        return "rectangle";
    case DraftingToolKind::Circle:
        return "circle";
    case DraftingToolKind::Arc:
        return "arc";
    case DraftingToolKind::Ellipse:
        return "ellipse";
    case DraftingToolKind::RegularPolygon:
        return "regular_polygon";
    case DraftingToolKind::Polyline:
        return "polyline";
    case DraftingToolKind::TextAnnotation:
        return "text";
    case DraftingToolKind::HorizontalGuide:
        return "horizontal_guide";
    case DraftingToolKind::VerticalGuide:
        return "vertical_guide";
    case DraftingToolKind::HorizontalConstructionLine:
        return "horizontal_construction_line";
    case DraftingToolKind::VerticalConstructionLine:
        return "vertical_construction_line";
    case DraftingToolKind::AngledConstructionLine:
        return "angled_construction_line";
    case DraftingToolKind::DistanceDimension:
        return "distance_dimension";
    case DraftingToolKind::WidthDimension:
        return "width_dimension";
    case DraftingToolKind::HeightDimension:
        return "height_dimension";
    case DraftingToolKind::RadiusDimension:
        return "radius_dimension";
    case DraftingToolKind::DiameterDimension:
        return "diameter_dimension";
    case DraftingToolKind::Unknown:
        return "unknown";
    }
    return "unknown";
}

namespace {

// Radius for the gesture-radius tools (circle/arc/regular polygon): a
// positive fixedRadius overrides the click distance so a parametric draw
// loop stamps identical shapes from rough clicks. Non-finite or negative
// options fall back to the gesture; both paths keep the legacy clamp to
// the unit document space.
double resolveToolRadius(const DraftingToolCreationRequest &request)
{
    const bool useFixed = std::isfinite(request.fixedRadius) && request.fixedRadius > 0.0;
    const double radius = useFixed ? request.fixedRadius : distance(request.start, request.end);
    return std::min(1.0, radius);
}

} // namespace

DraftingObjectBuildResult buildDraftingObjectForTool(const DraftingToolCreationRequest &request)
{
    DraftingShapeKind kind = DraftingShapeKind::Point;
    DraftingGeometry geometry = PointGeometry{request.start};
    if (request.tool == DraftingToolKind::Point) {
        kind = DraftingShapeKind::Point;
        geometry = PointGeometry{request.end};
    } else if (request.tool == DraftingToolKind::Line || request.tool == DraftingToolKind::Arrow
               || request.tool == DraftingToolKind::DoubleArrow) {
        // Arrow is the line tool's variant: same LineGeometry (kind stays
        // Line), the arrowhead is a metadata flag applied after the build.
        kind = DraftingShapeKind::Line;
        geometry = LineGeometry{request.start, request.end};
    } else if (request.tool == DraftingToolKind::Rectangle) {
        const double left = std::min(request.start.x, request.end.x);
        const double top = std::min(request.start.y, request.end.y);
        const double right = std::max(request.start.x, request.end.x);
        const double bottom = std::max(request.start.y, request.end.y);
        kind = DraftingShapeKind::Rectangle;
        RectangleGeometry box{{left, top}, right - left, bottom - top};
        box.cornerRadius = std::max(0.0, request.rectCornerRadius);
        box.inset = std::max(0.0, request.rectInset);
        geometry = box;
    } else if (request.tool == DraftingToolKind::Circle) {
        kind = DraftingShapeKind::Circle;
        geometry = CircleGeometry{request.start, resolveToolRadius(request)};
    } else if (request.tool == DraftingToolKind::Arc) {
        // Two clicks: centre (start) then a point giving radius and start angle.
        // The end angle is start + sweep (legacy default 105deg -> 15..120).
        // A fixed radius replaces the distance but the click still aims the arc.
        const double radius = resolveToolRadius(request);
        const double startAngle = std::atan2(request.end.y - request.start.y,
                                             request.end.x - request.start.x) * 180.0 / M_PI;
        kind = DraftingShapeKind::Arc;
        geometry = ArcGeometry{request.start, radius, startAngle, startAngle + request.arcSweepDeg};
    } else if (request.tool == DraftingToolKind::Ellipse) {
        // Two clicks: centre (start) then a point whose offsets from the centre
        // give the two radii. Mirrors the Circle arm but keeps the axes
        // independent, so x/y span separately instead of a single radius.
        kind = DraftingShapeKind::Ellipse;
        geometry = EllipseGeometry{request.start,
                                   std::min(1.0, std::abs(request.end.x - request.start.x)),
                                   std::min(1.0, std::abs(request.end.y - request.start.y))};
    } else if (request.tool == DraftingToolKind::Polyline) {
        kind = DraftingShapeKind::Polyline;
        PolylineGeometry polyline;
        polyline.vertices = request.vertices;
        geometry = polyline;
    } else if (request.tool == DraftingToolKind::RegularPolygon) {
        // Two clicks: centre (start) then a radius point (end). Vertices sit on
        // the circumscribed circle starting at rotationDeg, stepping by 360/sides.
        const int sides = std::clamp(request.polygonSides, 3, 24);
        const double radius = resolveToolRadius(request);
        const double rotationRad = request.polygonRotationDeg * M_PI / 180.0;
        PolygonGeometry polygon;
        polygon.vertices.reserve(static_cast<std::size_t>(sides));
        for (int i = 0; i < sides; ++i) {
            const double angle = rotationRad + (2.0 * M_PI * i) / sides;
            polygon.vertices.push_back({
                request.start.x + radius * std::cos(angle),
                request.start.y + radius * std::sin(angle),
            });
        }
        kind = DraftingShapeKind::Polygon;
        geometry = std::move(polygon);
    } else if (request.tool == DraftingToolKind::TextAnnotation) {
        // Single click: the controller passes start == end, so the placement
        // point is request.start. Content/height start at defaults; the user
        // edits them after the object exists (mirrors Point's one-point arm).
        kind = DraftingShapeKind::TextAnnotation;
        geometry = TextAnnotationGeometry{request.start, "Text", 0.04};
    } else if (request.tool == DraftingToolKind::HorizontalGuide) {
        kind = DraftingShapeKind::Guide;
        geometry = GuideGeometry{GuideOrientation::Horizontal, request.end.y};
    } else if (request.tool == DraftingToolKind::VerticalGuide) {
        kind = DraftingShapeKind::Guide;
        geometry = GuideGeometry{GuideOrientation::Vertical, request.end.x};
    } else if (request.tool == DraftingToolKind::HorizontalConstructionLine) {
        kind = DraftingShapeKind::ConstructionLine;
        geometry = ConstructionLineGeometry{{0.0, request.end.y}, {1.0, request.end.y}};
    } else if (request.tool == DraftingToolKind::VerticalConstructionLine) {
        kind = DraftingShapeKind::ConstructionLine;
        geometry = ConstructionLineGeometry{{request.end.x, 0.0}, {request.end.x, 1.0}};
    } else if (request.tool == DraftingToolKind::AngledConstructionLine) {
        kind = DraftingShapeKind::ConstructionLine;
        geometry = ConstructionLineGeometry{request.start, request.end};
    } else if (request.tool == DraftingToolKind::DistanceDimension) {
        kind = DraftingShapeKind::Dimension;
        geometry = DimensionGeometry{DimensionKind::Distance, request.start, request.end, 0.04};
    } else if (request.tool == DraftingToolKind::WidthDimension) {
        kind = DraftingShapeKind::Dimension;
        geometry = DimensionGeometry{DimensionKind::Width, request.start, {request.end.x, request.start.y}, 0.04};
    } else if (request.tool == DraftingToolKind::HeightDimension) {
        kind = DraftingShapeKind::Dimension;
        geometry = DimensionGeometry{DimensionKind::Height, request.start, {request.start.x, request.end.y}, 0.04};
    } else if (request.tool == DraftingToolKind::RadiusDimension) {
        kind = DraftingShapeKind::Dimension;
        geometry = DimensionGeometry{DimensionKind::Radius, request.start, request.end, 0.04};
    } else if (request.tool == DraftingToolKind::DiameterDimension) {
        kind = DraftingShapeKind::Dimension;
        geometry = DimensionGeometry{DimensionKind::Diameter, request.start, request.end, 0.04};
    } else {
        return DraftingObjectBuildResult::rejected(DraftingResultCode::InvalidGeometry, "tool cannot create a drafting object");
    }

    auto built = buildDraftingObject(request.objectId, kind, std::move(geometry));
    if (!built.ok) {
        return built;
    }
    built.object.layerId = request.layerId;
    built.object.metadata.toolProvenance = request.toolProvenance.empty()
        ? draftingToolKindName(request.tool)
        : request.toolProvenance;
    if (request.tool == DraftingToolKind::Arrow) {
        built.object.metadata.lineVisual.endArrow = true;
    }
    if (request.tool == DraftingToolKind::DoubleArrow) {
        built.object.metadata.lineVisual.startArrow = true;
        built.object.metadata.lineVisual.endArrow = true;
    }
    return built;
}

} // namespace edi::drafting
