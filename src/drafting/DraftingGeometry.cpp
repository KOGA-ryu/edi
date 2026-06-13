#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace edi::drafting {

namespace {

Bounds2D boundsFromPoints(const std::vector<Point2D> &points)
{
    if (points.empty()) {
        return {};
    }

    double minX = points.front().x;
    double minY = points.front().y;
    double maxX = points.front().x;
    double maxY = points.front().y;

    for (const Point2D &point : points) {
        minX = std::min(minX, point.x);
        minY = std::min(minY, point.y);
        maxX = std::max(maxX, point.x);
        maxY = std::max(maxY, point.y);
    }

    return {minX, minY, maxX - minX, maxY - minY};
}

std::vector<Point2D> rectangleCorners(const RectangleGeometry &rect)
{
    std::vector<Point2D> corners = {
        rect.origin,
        {rect.origin.x + rect.width, rect.origin.y},
        {rect.origin.x + rect.width, rect.origin.y + rect.height},
        {rect.origin.x, rect.origin.y + rect.height},
    };
    if (rect.rotationDeg == 0.0) {
        return corners;
    }

    const Point2D center {
        rect.origin.x + rect.width / 2.0,
        rect.origin.y + rect.height / 2.0,
    };
    const double angle = rect.rotationDeg * 3.14159265358979323846 / 180.0;
    const double cosA = std::cos(angle);
    const double sinA = std::sin(angle);
    for (Point2D &corner : corners) {
        const double dx = corner.x - center.x;
        const double dy = corner.y - center.y;
        corner = {
            center.x + dx * cosA - dy * sinA,
            center.y + dx * sinA + dy * cosA,
        };
    }
    return corners;
}

// Exact axis-aligned bounds of a circular arc: the two endpoints plus any of the
// four cardinal directions (0/90/180/270 deg) whose angle falls within the span.
Bounds2D arcBounds(const ArcGeometry &arc)
{
    constexpr double pi = 3.14159265358979323846;
    const double radius = std::max(0.0, arc.radius);
    const double lo = std::min(arc.startAngleDeg, arc.endAngleDeg);
    const double hi = std::max(arc.startAngleDeg, arc.endAngleDeg);

    auto pointAt = [&](double deg) -> Point2D {
        const double rad = deg * pi / 180.0;
        return {arc.center.x + radius * std::cos(rad), arc.center.y + radius * std::sin(rad)};
    };

    std::vector<Point2D> extremes{pointAt(arc.startAngleDeg), pointAt(arc.endAngleDeg)};
    // Walk every cardinal angle in [lo, hi] (offset by multiples of 360).
    for (double cardinal = std::floor(lo / 90.0) * 90.0; cardinal <= hi; cardinal += 90.0) {
        if (cardinal >= lo && cardinal <= hi) {
            extremes.push_back(pointAt(cardinal));
        }
    }
    return boundsFromPoints(extremes);
}

} // namespace

const char *shapeKindName(DraftingShapeKind kind)
{
    switch (kind) {
    case DraftingShapeKind::Point:
        return "point";
    case DraftingShapeKind::Line:
        return "line";
    case DraftingShapeKind::Rectangle:
        return "rectangle";
    case DraftingShapeKind::Circle:
        return "circle";
    case DraftingShapeKind::Arc:
        return "arc";
    case DraftingShapeKind::Polygon:
        return "polygon";
    case DraftingShapeKind::Polyline:
        return "polyline";
    case DraftingShapeKind::Guide:
        return "guide";
    case DraftingShapeKind::ConstructionLine:
        return "construction_line";
    case DraftingShapeKind::Dimension:
        return "dimension";
    }
    return "unknown";
}

DraftingShapeKind shapeKindFromName(const std::string &name)
{
    if (name == "point") return DraftingShapeKind::Point;
    if (name == "line") return DraftingShapeKind::Line;
    if (name == "rectangle") return DraftingShapeKind::Rectangle;
    if (name == "circle") return DraftingShapeKind::Circle;
    if (name == "arc") return DraftingShapeKind::Arc;
    if (name == "polygon") return DraftingShapeKind::Polygon;
    if (name == "polyline") return DraftingShapeKind::Polyline;
    if (name == "guide") return DraftingShapeKind::Guide;
    if (name == "construction_line") return DraftingShapeKind::ConstructionLine;
    if (name == "dimension") return DraftingShapeKind::Dimension;
    return DraftingShapeKind::Point;
}

const char *guideOrientationName(GuideOrientation orientation)
{
    switch (orientation) {
    case GuideOrientation::Horizontal:
        return "horizontal";
    case GuideOrientation::Vertical:
        return "vertical";
    }
    return "horizontal";
}

const char *dimensionKindName(DimensionKind kind)
{
    switch (kind) {
    case DimensionKind::Distance:
        return "distance";
    case DimensionKind::Width:
        return "width";
    case DimensionKind::Height:
        return "height";
    case DimensionKind::Radius:
        return "radius";
    case DimensionKind::Diameter:
        return "diameter";
    }
    return "distance";
}

const char *objectRoleName(ObjectRole role)
{
    switch (role) {
    case ObjectRole::None:
        return "none";
    case ObjectRole::Wall:
        return "wall";
    case ObjectRole::Floor:
        return "floor";
    case ObjectRole::Cutout:
        return "cutout";
    case ObjectRole::Collider:
        return "collider";
    }
    return "none";
}

ObjectRole objectRoleFromName(const std::string &name)
{
    if (name == "wall") return ObjectRole::Wall;
    if (name == "floor") return ObjectRole::Floor;
    if (name == "cutout") return ObjectRole::Cutout;
    if (name == "collider") return ObjectRole::Collider;
    return ObjectRole::None;
}

const char *draftingResultCodeName(DraftingResultCode code)
{
    switch (code) {
    case DraftingResultCode::None:
        return "none";
    case DraftingResultCode::EmptyObjectId:
        return "empty_object_id";
    case DraftingResultCode::DuplicateObjectId:
        return "duplicate_object_id";
    case DraftingResultCode::DuplicateLayerId:
        return "duplicate_layer_id";
    case DraftingResultCode::ObjectNotFound:
        return "object_not_found";
    case DraftingResultCode::LayerNotFound:
        return "layer_not_found";
    case DraftingResultCode::KindGeometryMismatch:
        return "kind_geometry_mismatch";
    case DraftingResultCode::InvalidGeometry:
        return "invalid_geometry";
    case DraftingResultCode::InvalidSelectionTarget:
        return "invalid_selection_target";
    case DraftingResultCode::InvalidMetadata:
        return "invalid_metadata";
    }
    return "unknown";
}

DraftingShapeKind geometryKind(const DraftingGeometry &geometry)
{
    return std::visit([](const auto &typedGeometry) {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            return DraftingShapeKind::Point;
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            return DraftingShapeKind::Line;
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            return DraftingShapeKind::Rectangle;
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            return DraftingShapeKind::Circle;
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            return DraftingShapeKind::Arc;
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            return DraftingShapeKind::Polygon;
        } else if constexpr (std::is_same_v<Geometry, PolylineGeometry>) {
            return DraftingShapeKind::Polyline;
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            return DraftingShapeKind::Guide;
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            return DraftingShapeKind::ConstructionLine;
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            return DraftingShapeKind::Dimension;
        } else {
            static_assert(always_false_v<Geometry>, "geometryKind: unhandled geometry kind — add an arm");
        }
    }, geometry);
}

bool kindMatchesGeometry(DraftingShapeKind kind, const DraftingGeometry &geometry)
{
    return kind == geometryKind(geometry);
}

bool isValidDraftingObjectId(const DraftingObjectId &id)
{
    return !id.empty();
}

bool isValidDraftingDocumentId(const DraftingDocumentId &id)
{
    return !id.empty();
}

bool isValidDraftingDocumentTitle(const std::string &title)
{
    return !title.empty();
}

bool isValidLayerId(const LayerId &id)
{
    return !id.empty();
}

bool isValidLayerName(const std::string &name)
{
    return !name.empty();
}

GeometryValidationResult GeometryValidationResult::accepted()
{
    return {true, DraftingResultCode::None, {}};
}

GeometryValidationResult GeometryValidationResult::rejected(std::string message)
{
    return {false, DraftingResultCode::InvalidGeometry, std::move(message)};
}

bool isFinite(Point2D point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool isFinite(const Bounds2D &bounds)
{
    return std::isfinite(bounds.x)
        && std::isfinite(bounds.y)
        && std::isfinite(bounds.width)
        && std::isfinite(bounds.height);
}

GeometryValidationResult validateGeometry(const DraftingGeometry &geometry)
{
    return std::visit([](const auto &typedGeometry) -> GeometryValidationResult {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            if (!isFinite(typedGeometry.point)) {
                return GeometryValidationResult::rejected("point coordinates must be finite");
            }
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            if (!isFinite(typedGeometry.a) || !isFinite(typedGeometry.b)) {
                return GeometryValidationResult::rejected("line endpoints must be finite");
            }
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            if (!isFinite(typedGeometry.origin)
                || !std::isfinite(typedGeometry.width)
                || !std::isfinite(typedGeometry.height)
                || !std::isfinite(typedGeometry.rotationDeg)
                || !std::isfinite(typedGeometry.cornerRadius)
                || !std::isfinite(typedGeometry.inset)) {
                return GeometryValidationResult::rejected("rectangle fields must be finite");
            }
            if (typedGeometry.width < 0.0 || typedGeometry.height < 0.0
                || typedGeometry.cornerRadius < 0.0 || typedGeometry.inset < 0.0) {
                return GeometryValidationResult::rejected("rectangle dimensions must be non-negative");
            }
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            if (!isFinite(typedGeometry.center) || !std::isfinite(typedGeometry.radius)) {
                return GeometryValidationResult::rejected("circle fields must be finite");
            }
            if (typedGeometry.radius < 0.0) {
                return GeometryValidationResult::rejected("circle radius must be non-negative");
            }
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            if (!isFinite(typedGeometry.center)
                || !std::isfinite(typedGeometry.radius)
                || !std::isfinite(typedGeometry.startAngleDeg)
                || !std::isfinite(typedGeometry.endAngleDeg)) {
                return GeometryValidationResult::rejected("arc fields must be finite");
            }
            if (typedGeometry.radius < 0.0) {
                return GeometryValidationResult::rejected("arc radius must be non-negative");
            }
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            if (typedGeometry.vertices.size() < 3) {
                return GeometryValidationResult::rejected("polygon requires at least three vertices");
            }
            for (const Point2D &point : typedGeometry.vertices) {
                if (!isFinite(point)) {
                    return GeometryValidationResult::rejected("polygon vertices must be finite");
                }
            }
        } else if constexpr (std::is_same_v<Geometry, PolylineGeometry>) {
            if (typedGeometry.vertices.size() < 2) {
                return GeometryValidationResult::rejected("polyline requires at least two vertices");
            }
            for (const Point2D &point : typedGeometry.vertices) {
                if (!isFinite(point)) {
                    return GeometryValidationResult::rejected("polyline vertices must be finite");
                }
            }
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (!std::isfinite(typedGeometry.position)) {
                return GeometryValidationResult::rejected("guide position must be finite");
            }
            if (typedGeometry.position < 0.0 || typedGeometry.position > 1.0) {
                return GeometryValidationResult::rejected("guide position must be normalized");
            }
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            if (!isFinite(typedGeometry.a) || !isFinite(typedGeometry.b)) {
                return GeometryValidationResult::rejected("construction line endpoints must be finite");
            }
            if (distance(typedGeometry.a, typedGeometry.b) <= 0.000001) {
                return GeometryValidationResult::rejected("construction line requires two distinct points");
            }
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            if (!isFinite(typedGeometry.a) || !isFinite(typedGeometry.b) || !std::isfinite(typedGeometry.offset)) {
                return GeometryValidationResult::rejected("dimension fields must be finite");
            }
            if (distance(typedGeometry.a, typedGeometry.b) <= 0.000001) {
                return GeometryValidationResult::rejected("dimension requires two distinct points");
            }
        } else {
            static_assert(always_false_v<Geometry>, "validateGeometry: unhandled geometry kind — add an arm");
        }
        return GeometryValidationResult::accepted();
    }, geometry);
}

Bounds2D computeBounds(const DraftingGeometry &geometry)
{
    return std::visit([](const auto &typedGeometry) -> Bounds2D {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            return {typedGeometry.point.x, typedGeometry.point.y, 0.0, 0.0};
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            return boundsFromPoints({typedGeometry.a, typedGeometry.b});
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            return boundsFromPoints(rectangleCorners(typedGeometry));
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            const double radius = std::max(0.0, typedGeometry.radius);
            return {
                typedGeometry.center.x - radius,
                typedGeometry.center.y - radius,
                radius * 2.0,
                radius * 2.0,
            };
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            return arcBounds(typedGeometry);
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                return {0.0, typedGeometry.position, 1.0, 0.0};
            }
            return {typedGeometry.position, 0.0, 0.0, 1.0};
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            return boundsFromPoints({typedGeometry.a, typedGeometry.b});
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            const double length = distance(typedGeometry.a, typedGeometry.b);
            if (length <= 0.000001) {
                return boundsFromPoints({typedGeometry.a, typedGeometry.b});
            }
            const double nx = -(typedGeometry.b.y - typedGeometry.a.y) / length * typedGeometry.offset;
            const double ny = (typedGeometry.b.x - typedGeometry.a.x) / length * typedGeometry.offset;
            return boundsFromPoints({
                typedGeometry.a,
                typedGeometry.b,
                {typedGeometry.a.x + nx, typedGeometry.a.y + ny},
                {typedGeometry.b.x + nx, typedGeometry.b.y + ny},
            });
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>
                             || std::is_same_v<Geometry, PolylineGeometry>) {
            return boundsFromPoints(typedGeometry.vertices);
        } else {
            static_assert(always_false_v<Geometry>, "computeBounds: unhandled geometry kind — add an arm");
        }
    }, geometry);
}

Bounds2D includeBounds(Bounds2D bounds, Bounds2D next)
{
    const double left = std::min(bounds.x, next.x);
    const double top = std::min(bounds.y, next.y);
    const double right = std::max(bounds.x + bounds.width, next.x + next.width);
    const double bottom = std::max(bounds.y + bounds.height, next.y + next.height);
    return {left, top, right - left, bottom - top};
}

bool boundsContainsPoint(Bounds2D bounds, Point2D point)
{
    return point.x >= bounds.x
        && point.y >= bounds.y
        && point.x <= bounds.x + bounds.width
        && point.y <= bounds.y + bounds.height;
}

Point2D translatePoint(Point2D point, double dx, double dy)
{
    point.x += dx;
    point.y += dy;
    return point;
}

DraftingGeometry translateGeometry(const DraftingGeometry &geometry, double dx, double dy)
{
    return std::visit([dx, dy](auto typedGeometry) -> DraftingGeometry {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            typedGeometry.point = translatePoint(typedGeometry.point, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            typedGeometry.a = translatePoint(typedGeometry.a, dx, dy);
            typedGeometry.b = translatePoint(typedGeometry.b, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            typedGeometry.origin = translatePoint(typedGeometry.origin, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            typedGeometry.center = translatePoint(typedGeometry.center, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            typedGeometry.center = translatePoint(typedGeometry.center, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                typedGeometry.position += dy;
            } else {
                typedGeometry.position += dx;
            }
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            typedGeometry.a = translatePoint(typedGeometry.a, dx, dy);
            typedGeometry.b = translatePoint(typedGeometry.b, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            typedGeometry.a = translatePoint(typedGeometry.a, dx, dy);
            typedGeometry.b = translatePoint(typedGeometry.b, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>
                             || std::is_same_v<Geometry, PolylineGeometry>) {
            for (Point2D &point : typedGeometry.vertices) {
                point = translatePoint(point, dx, dy);
            }
        } else {
            static_assert(always_false_v<Geometry>, "translateGeometry: unhandled geometry kind — add an arm");
        }
        return typedGeometry;
    }, geometry);
}

bool translationHasEffect(double dx, double dy)
{
    constexpr double epsilon = 0.0000001;
    return std::isfinite(dx)
        && std::isfinite(dy)
        && (std::abs(dx) >= epsilon || std::abs(dy) >= epsilon);
}

double distance(Point2D a, Point2D b)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

Point2D arcPointAtAngle(Point2D center, double radius, double angleDeg)
{
    constexpr double pi = 3.14159265358979323846;
    const double rad = angleDeg * pi / 180.0;
    return {center.x + radius * std::cos(rad), center.y + radius * std::sin(rad)};
}

double arcMidAngleDeg(const ArcGeometry &arc)
{
    return (arc.startAngleDeg + arc.endAngleDeg) * 0.5;
}

std::vector<Point2D> sampleArc(const ArcGeometry &arc, double maxStepDeg)
{
    std::vector<Point2D> points;
    const double span = arc.endAngleDeg - arc.startAngleDeg;
    const double step = (std::isfinite(maxStepDeg) && maxStepDeg > 0.0) ? maxStepDeg : 2.0;
    const int segments = std::max(1, static_cast<int>(std::ceil(std::abs(span) / step)));
    points.reserve(static_cast<std::size_t>(segments) + 1);
    for (int i = 0; i <= segments; ++i) {
        const double angle = arc.startAngleDeg + span * (static_cast<double>(i) / segments);
        points.push_back(arcPointAtAngle(arc.center, arc.radius, angle));
    }
    return points;
}

double lineAngleDegrees(const LineGeometry &line)
{
    constexpr double pi = 3.14159265358979323846;
    return std::atan2(line.b.y - line.a.y, line.b.x - line.a.x) * 180.0 / pi;
}

double dimensionAngleDegrees(const DimensionGeometry &dimension)
{
    constexpr double pi = 3.14159265358979323846;
    return std::atan2(dimension.b.y - dimension.a.y, dimension.b.x - dimension.a.x) * 180.0 / pi;
}

double displayedDimensionLength(double storedLength, DimensionKind kind)
{
    return kind == DimensionKind::Diameter ? storedLength * 2.0 : storedLength;
}

double storedDimensionLength(double displayedLength, DimensionKind kind)
{
    return kind == DimensionKind::Diameter ? displayedLength / 2.0 : displayedLength;
}

double area(const DraftingGeometry &geometry)
{
    if (const auto *rect = std::get_if<RectangleGeometry>(&geometry)) {
        return std::abs(rect->width * rect->height);
    }
    if (const auto *circle = std::get_if<CircleGeometry>(&geometry)) {
        constexpr double pi = 3.14159265358979323846;
        return pi * circle->radius * circle->radius;
    }
    if (const auto *polygon = std::get_if<PolygonGeometry>(&geometry)) {
        if (polygon->vertices.size() < 3) {
            return 0.0;
        }
        double twiceArea = 0.0;
        for (std::size_t i = 0; i < polygon->vertices.size(); ++i) {
            const Point2D &a = polygon->vertices[i];
            const Point2D &b = polygon->vertices[(i + 1) % polygon->vertices.size()];
            twiceArea += a.x * b.y - b.x * a.y;
        }
        return std::abs(twiceArea) / 2.0;
    }
    return 0.0;
}

std::vector<HandleAnchor> handleAnchors(const DraftingGeometry &geometry)
{
    return std::visit([](const auto &typedGeometry) {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        std::vector<HandleAnchor> handles;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            handles.push_back({"point", typedGeometry.point});
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            handles.push_back({"line_start", typedGeometry.a});
            handles.push_back({"line_end", typedGeometry.b});
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const auto corners = rectangleCorners(typedGeometry);
            handles.push_back({"rect_nw", corners[0]});
            handles.push_back({"rect_ne", corners[1]});
            handles.push_back({"rect_se", corners[2]});
            handles.push_back({"rect_sw", corners[3]});
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            handles.push_back({"circle_center", typedGeometry.center});
            handles.push_back({"circle_radius", {typedGeometry.center.x + typedGeometry.radius, typedGeometry.center.y}});
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            handles.push_back({"arc_center", typedGeometry.center});
            handles.push_back({"arc_radius", arcPointAtAngle(typedGeometry.center, typedGeometry.radius, arcMidAngleDeg(typedGeometry))});
            handles.push_back({"arc_start", arcPointAtAngle(typedGeometry.center, typedGeometry.radius, typedGeometry.startAngleDeg)});
            handles.push_back({"arc_end", arcPointAtAngle(typedGeometry.center, typedGeometry.radius, typedGeometry.endAngleDeg)});
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                handles.push_back({"guide", {0.5, typedGeometry.position}});
            } else {
                handles.push_back({"guide", {typedGeometry.position, 0.5}});
            }
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            handles.push_back({"construction_start", typedGeometry.a});
            handles.push_back({"construction_midpoint", {(typedGeometry.a.x + typedGeometry.b.x) / 2.0, (typedGeometry.a.y + typedGeometry.b.y) / 2.0}});
            handles.push_back({"construction_end", typedGeometry.b});
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            handles.push_back({"dimension_start", typedGeometry.a});
            handles.push_back({"dimension_midpoint", {(typedGeometry.a.x + typedGeometry.b.x) / 2.0, (typedGeometry.a.y + typedGeometry.b.y) / 2.0}});
            handles.push_back({"dimension_end", typedGeometry.b});
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>
                             || std::is_same_v<Geometry, PolylineGeometry>) {
            for (std::size_t i = 0; i < typedGeometry.vertices.size(); ++i) {
                handles.push_back({"vertex_" + std::to_string(i), typedGeometry.vertices[i]});
            }
        } else {
            static_assert(always_false_v<Geometry>, "handleAnchors: unhandled geometry kind — add an arm");
        }
        return handles;
    }, geometry);
}

} // namespace edi::drafting
