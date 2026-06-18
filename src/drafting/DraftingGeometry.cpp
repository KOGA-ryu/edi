#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace edi::drafting {

namespace {

struct LineParams {
    double t; // position along the first line (p.a + t*(p.b-p.a))
    double u; // position along the second line
};

// Solve p.a + t*r = q.a + u*s for the two parameters. nullopt when the lines are
// ~parallel/collinear or a segment is degenerate. The determinant is
// |r||s|*sin(theta), so it shrinks with the segment LENGTHS as well as the
// angle; an ABSOLUTE threshold would mistake short segments for parallel ones.
// Normalising by the lengths makes the test on sin(theta) — a genuine small
// angle — independent of how long the segments are.
std::optional<LineParams> solveLineParams(const LineGeometry &p, const LineGeometry &q)
{
    const double rX = p.b.x - p.a.x;
    const double rY = p.b.y - p.a.y;
    const double sX = q.b.x - q.a.x;
    const double sY = q.b.y - q.a.y;
    const double denominator = rX * sY - rY * sX;
    const double rLength = std::sqrt(rX * rX + rY * rY);
    const double sLength = std::sqrt(sX * sX + sY * sY);
    if (rLength == 0.0 || sLength == 0.0
        || std::abs(denominator) <= 1e-9 * rLength * sLength) {
        return std::nullopt;
    }
    const double dX = q.a.x - p.a.x;
    const double dY = q.a.y - p.a.y;
    return LineParams{(dX * sY - dY * sX) / denominator, (dX * rY - dY * rX) / denominator};
}

Point2D pointAlongLine(const LineGeometry &line, double t)
{
    return {line.a.x + t * (line.b.x - line.a.x), line.a.y + t * (line.b.y - line.a.y)};
}

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

// One Catmull-Rom segment: interpolate between p1 and p2, with p0 and p3 the
// neighbours that set the tangents. t in [0,1] returns p1 at 0 and p2 at 1, so
// the assembled curve passes through every control point. The 0.5 factor is the
// standard uniform (centripetal-free) tension — a learner reading this can map
// each coefficient to a term of the cubic basis.
Point2D catmullRomPoint(Point2D p0, Point2D p1, Point2D p2, Point2D p3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    const auto axis = [&](double a0, double a1, double a2, double a3) {
        return 0.5 * ((2.0 * a1)
                      + (-a0 + a2) * t
                      + (2.0 * a0 - 5.0 * a1 + 4.0 * a2 - a3) * t2
                      + (-a0 + 3.0 * a1 - 3.0 * a2 + a3) * t3);
    };
    return {axis(p0.x, p1.x, p2.x, p3.x), axis(p0.y, p1.y, p2.y, p3.y)};
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
    case DraftingShapeKind::Ellipse:
        return "ellipse";
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
    case DraftingShapeKind::TextAnnotation:
        return "text";
    case DraftingShapeKind::Spline:
        return "spline";
    case DraftingShapeKind::Wall:
        return "wall";
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
    if (name == "ellipse") return DraftingShapeKind::Ellipse;
    if (name == "polygon") return DraftingShapeKind::Polygon;
    if (name == "polyline") return DraftingShapeKind::Polyline;
    if (name == "guide") return DraftingShapeKind::Guide;
    if (name == "construction_line") return DraftingShapeKind::ConstructionLine;
    if (name == "dimension") return DraftingShapeKind::Dimension;
    if (name == "text") return DraftingShapeKind::TextAnnotation;
    if (name == "spline") return DraftingShapeKind::Spline;
    if (name == "wall") return DraftingShapeKind::Wall;
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
    case DimensionKind::Angular:
        return "angular";
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

const char *wallTypeName(WallType type)
{
    switch (type) {
    case WallType::Solid:
        return "solid";
    case WallType::Door:
        return "door";
    case WallType::Window:
        return "window";
    case WallType::Secret:
        return "secret";
    }
    return "solid";
}

WallType wallTypeFromName(const std::string &name)
{
    if (name == "door") return WallType::Door;
    if (name == "window") return WallType::Window;
    if (name == "secret") return WallType::Secret;
    return WallType::Solid;
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
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            return DraftingShapeKind::Ellipse;
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
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            return DraftingShapeKind::TextAnnotation;
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            return DraftingShapeKind::Spline;
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            return DraftingShapeKind::Wall;
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
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            if (!isFinite(typedGeometry.center)
                || !std::isfinite(typedGeometry.rx)
                || !std::isfinite(typedGeometry.ry)) {
                return GeometryValidationResult::rejected("ellipse fields must be finite");
            }
            if (typedGeometry.rx < 0.0 || typedGeometry.ry < 0.0) {
                return GeometryValidationResult::rejected("ellipse radii must be non-negative");
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
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            if (!isFinite(typedGeometry.position) || !std::isfinite(typedGeometry.height)) {
                return GeometryValidationResult::rejected("text fields must be finite");
            }
            if (typedGeometry.height < 0.0) {
                return GeometryValidationResult::rejected("text height must be non-negative");
            }
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            if (typedGeometry.controlPoints.size() < 2) {
                return GeometryValidationResult::rejected("spline requires at least two control points");
            }
            for (const Point2D &point : typedGeometry.controlPoints) {
                if (!isFinite(point)) {
                    return GeometryValidationResult::rejected("spline control points must be finite");
                }
            }
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // A wall is a thick line: finite endpoints (like LineGeometry) plus a
            // non-negative thickness (like circle's radius). A zero-length
            // centerline is allowed here — the tool/hit paths handle the
            // degenerate band — but thickness must not be negative.
            if (!isFinite(typedGeometry.a) || !isFinite(typedGeometry.b)
                || !std::isfinite(typedGeometry.thickness)) {
                return GeometryValidationResult::rejected("wall fields must be finite");
            }
            if (typedGeometry.thickness < 0.0) {
                return GeometryValidationResult::rejected("wall thickness must be non-negative");
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
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            const double rx = std::max(0.0, typedGeometry.rx);
            const double ry = std::max(0.0, typedGeometry.ry);
            return {typedGeometry.center.x - rx, typedGeometry.center.y - ry, rx * 2.0, ry * 2.0};
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
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            // Approximate box from the content; the painter draws real glyphs, but
            // selection/bounds only need an honest rectangle around them.
            const double h = std::max(0.0, typedGeometry.height);
            const double w = std::max(h * 0.3, static_cast<double>(typedGeometry.content.size()) * h * 0.55);
            return {typedGeometry.position.x, typedGeometry.position.y, w, h};
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            // Bounds of the DRAWN curve, not the control polygon: a Catmull-Rom
            // can bow outside its knots, so selection/bounds must measure the
            // sampled points the user actually sees.
            return boundsFromPoints(sampleSpline(typedGeometry));
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // Centerline bbox (like LineGeometry) EXPANDED by half the band
            // thickness on every side, so the band's painted footprint is fully
            // enclosed regardless of the segment's orientation. This over-covers
            // the rotated band's corners slightly (an axis-aligned pad of a
            // diagonal band), which is the safe direction for selection/bounds.
            const Bounds2D core = boundsFromPoints({typedGeometry.a, typedGeometry.b});
            const double pad = std::max(0.0, typedGeometry.thickness) / 2.0;
            return {core.x - pad, core.y - pad, core.width + pad * 2.0, core.height + pad * 2.0};
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
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
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
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            typedGeometry.position = translatePoint(typedGeometry.position, dx, dy);
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            // Moving the whole curve = moving every control point; the sampled
            // form follows because it is recomputed from these.
            for (Point2D &point : typedGeometry.controlPoints) {
                point = translatePoint(point, dx, dy);
            }
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // Move both centerline endpoints (like LineGeometry); thickness is a
            // band width, unaffected by translation.
            typedGeometry.a = translatePoint(typedGeometry.a, dx, dy);
            typedGeometry.b = translatePoint(typedGeometry.b, dx, dy);
        } else {
            static_assert(always_false_v<Geometry>, "translateGeometry: unhandled geometry kind — add an arm");
        }
        return typedGeometry;
    }, geometry);
}

namespace {

// Rotate+uniform-scale a point about a pivot — THE one place trig lives for the
// transform (deg->rad once). Uniform scale commutes with rotation, so
// scale-then-rotate == rotate-then-scale (no ambiguity). The matrix here
// (dx*c - dy*s, dx*s + dy*c) is the SAME one the model already uses for
// field-rotation (rectangleCorners and the arc angle convention), so a point
// rotated by mapPoint and a field rotated by `+= rotationDeg` agree in sense —
// no sign flip is needed despite the y-down canvas.
Point2D mapPoint(Point2D p, Point2D pivot, double rotationDeg, double scale)
{
    const double rad = rotationDeg * 3.14159265358979323846 / 180.0;
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    const double dx = (p.x - pivot.x) * scale;
    const double dy = (p.y - pivot.y) * scale;
    return Point2D{pivot.x + dx * c - dy * s, pivot.y + dx * s + dy * c};
}

} // namespace

DraftingGeometry transformGeometry(const DraftingGeometry &geometry, Point2D pivot, double rotationDeg, double scale)
{
    return std::visit([&](auto typedGeometry) -> DraftingGeometry {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            typedGeometry.point = mapPoint(typedGeometry.point, pivot, rotationDeg, scale);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            typedGeometry.a = mapPoint(typedGeometry.a, pivot, rotationDeg, scale);
            typedGeometry.b = mapPoint(typedGeometry.b, pivot, rotationDeg, scale);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            // rotationDeg is about the rectangle's CENTER (matches computeBounds/
            // painter), so the transform anchors on the center and re-derives
            // origin — Rectangle is the lone kind whose defining point (origin) !=
            // its rotation anchor (center). Mapping origin directly (the literal
            // contract sketch) would leave the center mis-placed by R(theta) on the
            // half-extent and the box would spin in place instead of orbiting the
            // pivot; anchoring on the center makes the rendered corners equal
            // mapPoint(original corners) exactly.
            const Point2D center{
                typedGeometry.origin.x + typedGeometry.width / 2.0,
                typedGeometry.origin.y + typedGeometry.height / 2.0,
            };
            const Point2D mappedCenter = mapPoint(center, pivot, rotationDeg, scale);
            typedGeometry.width *= scale;
            typedGeometry.height *= scale;
            typedGeometry.cornerRadius *= scale;
            typedGeometry.inset *= scale;
            typedGeometry.rotationDeg += rotationDeg;
            typedGeometry.origin = {
                mappedCenter.x - typedGeometry.width / 2.0,
                mappedCenter.y - typedGeometry.height / 2.0,
            };
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            // Uniform scale keeps it a circle; rotation only moves the center.
            typedGeometry.center = mapPoint(typedGeometry.center, pivot, rotationDeg, scale);
            typedGeometry.radius *= scale;
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            // Map the rotation anchor (center) and accumulate the angle on both
            // ends — sweep magnitude preserved, orientation rotates.
            typedGeometry.center = mapPoint(typedGeometry.center, pivot, rotationDeg, scale);
            typedGeometry.radius *= scale;
            typedGeometry.startAngleDeg += rotationDeg;
            typedGeometry.endAngleDeg += rotationDeg;
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            // v1 LIMITATION: the axis TILT from rotation is DROPPED — an
            // axis-aligned ellipse has no field to store it, so rotation only moves
            // the center while rx/ry stay axis-aligned (correct for circles rx==ry
            // and for pure translate/scale; lossy only for a rotated non-circular
            // ellipse). DEFERRED lossless fix: add an additive rotationDeg field to
            // EllipseGeometry (mirroring Rectangle) — a separate model slice that
            // touches every ellipse visit + serialize + painter; not built here.
            typedGeometry.center = mapPoint(typedGeometry.center, pivot, rotationDeg, scale);
            typedGeometry.rx *= scale;
            typedGeometry.ry *= scale;
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            // IDENTITY no-op: a guide is an infinite axis-aligned reference line, so
            // rotating/scaling it about an arbitrary pivot has no meaning in the v1
            // model. A deliberate arm (returns unchanged), NOT a fall-through.
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            typedGeometry.a = mapPoint(typedGeometry.a, pivot, rotationDeg, scale);
            typedGeometry.b = mapPoint(typedGeometry.b, pivot, rotationDeg, scale);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            typedGeometry.a = mapPoint(typedGeometry.a, pivot, rotationDeg, scale);
            typedGeometry.b = mapPoint(typedGeometry.b, pivot, rotationDeg, scale);
            typedGeometry.offset *= scale;
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>
                             || std::is_same_v<Geometry, PolylineGeometry>) {
            for (Point2D &point : typedGeometry.vertices) {
                point = mapPoint(point, pivot, rotationDeg, scale);
            }
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            // v1 LIMITATION: the baseline ANGLE is DROPPED — text has no angle
            // field, so rotation only moves the anchor while text stays upright
            // (correct for the common unrotated case). DEFERRED lossless fix: add
            // an additive rotationDeg field to TextAnnotationGeometry (mirroring
            // Rectangle) — a separate model slice touching every text visit +
            // serialize + painter; not built here.
            typedGeometry.position = mapPoint(typedGeometry.position, pivot, rotationDeg, scale);
            typedGeometry.height *= scale;
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            // Transform the whole curve = transform every control point; the
            // sampled form follows because it is recomputed from these.
            for (Point2D &point : typedGeometry.controlPoints) {
                point = mapPoint(point, pivot, rotationDeg, scale);
            }
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // Map both centerline endpoints (like LineGeometry); thickness is a
            // band width, so it scales but does not rotate.
            typedGeometry.a = mapPoint(typedGeometry.a, pivot, rotationDeg, scale);
            typedGeometry.b = mapPoint(typedGeometry.b, pivot, rotationDeg, scale);
            typedGeometry.thickness *= scale;
        } else {
            static_assert(always_false_v<Geometry>, "transformGeometry: unhandled DraftingGeometry arm");
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

std::optional<Point2D> segmentIntersection(const LineGeometry &a, const LineGeometry &b)
{
    const std::optional<LineParams> params = solveLineParams(a, b);
    if (!params) {
        return std::nullopt; // a zero-length segment or an ~parallel/collinear pair
    }
    // The crossing of the two infinite lines must fall within BOTH finite
    // segments; otherwise the lines only meet beyond an endpoint.
    if (params->t < 0.0 || params->t > 1.0 || params->u < 0.0 || params->u > 1.0) {
        return std::nullopt;
    }
    return pointAlongLine(a, params->t);
}

std::optional<Point2D> lineIntersection(const LineGeometry &a, const LineGeometry &b)
{
    const std::optional<LineParams> params = solveLineParams(a, b);
    if (!params) {
        return std::nullopt; // parallel/collinear or degenerate: no single vertex
    }
    return pointAlongLine(a, params->t); // no [0,1] clamp: the infinite lines' crossing
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

std::vector<Point2D> sampleEllipse(const EllipseGeometry &ellipse, int segments)
{
    constexpr double pi = 3.14159265358979323846;
    const int count = std::max(8, segments);
    std::vector<Point2D> points;
    points.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const double angle = 2.0 * pi * (static_cast<double>(i) / count);
        points.push_back({ellipse.center.x + ellipse.rx * std::cos(angle),
                          ellipse.center.y + ellipse.ry * std::sin(angle)});
    }
    return points;
}

std::vector<Point2D> sampleSpline(const SplineGeometry &spline, int stepsPerSegment)
{
    const std::vector<Point2D> &cp = spline.controlPoints;
    // 0/1 points cannot make a curve; 2 points are a single straight segment.
    // Both cases return the raw points so callers (bounds, hit, plot) still get
    // an honest chain without special-casing.
    if (cp.size() < 3) {
        return cp;
    }
    const int steps = std::max(1, stepsPerSegment);
    std::vector<Point2D> out;
    out.reserve(cp.size() * static_cast<std::size_t>(steps));
    out.push_back(cp.front());
    for (std::size_t i = 0; i + 1 < cp.size(); ++i) {
        // Endpoints have no outside neighbour, so duplicate them to clamp the
        // tangent — the curve starts and ends exactly at the first/last knot.
        const Point2D p0 = (i == 0) ? cp[i] : cp[i - 1];
        const Point2D p1 = cp[i];
        const Point2D p2 = cp[i + 1];
        const Point2D p3 = (i + 2 < cp.size()) ? cp[i + 2] : cp[i + 1];
        for (int s = 1; s <= steps; ++s) {
            out.push_back(catmullRomPoint(p0, p1, p2, p3, static_cast<double>(s) / steps));
        }
    }
    return out;
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

double dimensionMeasuredAngle(const DimensionGeometry &dimension)
{
    // Angular dimensions repurpose `offset` as the signed included angle (degrees);
    // for every other kind `offset` is a standoff length, and the measured quantity
    // is the base-ray direction.
    if (dimension.kind == DimensionKind::Angular) {
        return dimension.offset;
    }
    return dimensionAngleDegrees(dimension);
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
    if (const auto *ellipse = std::get_if<EllipseGeometry>(&geometry)) {
        constexpr double pi = 3.14159265358979323846;
        return pi * ellipse->rx * ellipse->ry;
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
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            handles.push_back({"ellipse_center", typedGeometry.center});
            handles.push_back({"ellipse_rx", {typedGeometry.center.x + typedGeometry.rx, typedGeometry.center.y}});
            handles.push_back({"ellipse_ry", {typedGeometry.center.x, typedGeometry.center.y + typedGeometry.ry}});
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
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            handles.push_back({"text_position", typedGeometry.position});
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            // One anchor per control point — the curve is edited by its knots,
            // which (Catmull-Rom) lie on the curve itself.
            for (std::size_t i = 0; i < typedGeometry.controlPoints.size(); ++i) {
                handles.push_back({"control_" + std::to_string(i), typedGeometry.controlPoints[i]});
            }
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // Two endpoint handles (like a line's a/b) plus one thickness handle.
            // The thickness handle sits at the centerline MIDPOINT, pushed
            // perpendicular to the a->b direction by half the thickness — so it
            // rides the edge of the band the way circle's radius handle rides the
            // rim. The perpendicular of (dx,dy) is (-dy,dx) normalized; a
            // zero-length wall degenerates the direction, so fall back to the
            // midpoint itself (no offset) when the segment has no length.
            const Point2D mid{(typedGeometry.a.x + typedGeometry.b.x) / 2.0,
                              (typedGeometry.a.y + typedGeometry.b.y) / 2.0};
            const double dx = typedGeometry.b.x - typedGeometry.a.x;
            const double dy = typedGeometry.b.y - typedGeometry.a.y;
            const double len = std::sqrt(dx * dx + dy * dy);
            const double half = std::max(0.0, typedGeometry.thickness) / 2.0;
            Point2D thicknessHandle = mid;
            if (len > 0.0) {
                thicknessHandle = {mid.x - dy / len * half, mid.y + dx / len * half};
            }
            handles.push_back({"wall_start", typedGeometry.a});
            handles.push_back({"wall_end", typedGeometry.b});
            handles.push_back({"wall_thickness", thicknessHandle});
        } else {
            static_assert(always_false_v<Geometry>, "handleAnchors: unhandled geometry kind — add an arm");
        }
        return handles;
    }, geometry);
}

} // namespace edi::drafting
