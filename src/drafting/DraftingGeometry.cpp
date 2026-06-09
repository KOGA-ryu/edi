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
    case DraftingShapeKind::Polygon:
        return "polygon";
    case DraftingShapeKind::Polyline:
        return "polyline";
    case DraftingShapeKind::Guide:
        return "guide";
    }
    return "unknown";
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

const char *draftingResultCodeName(DraftingResultCode code)
{
    switch (code) {
    case DraftingResultCode::None:
        return "none";
    case DraftingResultCode::EmptyObjectId:
        return "empty_object_id";
    case DraftingResultCode::DuplicateObjectId:
        return "duplicate_object_id";
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
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            return DraftingShapeKind::Polygon;
        } else if constexpr (std::is_same_v<Geometry, PolylineGeometry>) {
            return DraftingShapeKind::Polyline;
        } else {
            return DraftingShapeKind::Guide;
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
                || !std::isfinite(typedGeometry.rotationDeg)) {
                return GeometryValidationResult::rejected("rectangle fields must be finite");
            }
            if (typedGeometry.width < 0.0 || typedGeometry.height < 0.0) {
                return GeometryValidationResult::rejected("rectangle dimensions must be non-negative");
            }
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            if (!isFinite(typedGeometry.center) || !std::isfinite(typedGeometry.radius)) {
                return GeometryValidationResult::rejected("circle fields must be finite");
            }
            if (typedGeometry.radius < 0.0) {
                return GeometryValidationResult::rejected("circle radius must be non-negative");
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
        } else {
            if (!std::isfinite(typedGeometry.position)) {
                return GeometryValidationResult::rejected("guide position must be finite");
            }
            if (typedGeometry.position < 0.0 || typedGeometry.position > 1.0) {
                return GeometryValidationResult::rejected("guide position must be normalized");
            }
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
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                return {0.0, typedGeometry.position, 1.0, 0.0};
            }
            return {typedGeometry.position, 0.0, 0.0, 1.0};
        } else {
            return boundsFromPoints(typedGeometry.vertices);
        }
    }, geometry);
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
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                typedGeometry.position += dy;
            } else {
                typedGeometry.position += dx;
            }
        } else {
            for (Point2D &point : typedGeometry.vertices) {
                point = translatePoint(point, dx, dy);
            }
        }
        return typedGeometry;
    }, geometry);
}

double distance(Point2D a, Point2D b)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
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
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                handles.push_back({"guide", {0.5, typedGeometry.position}});
            } else {
                handles.push_back({"guide", {typedGeometry.position, 0.5}});
            }
        } else {
            for (std::size_t i = 0; i < typedGeometry.vertices.size(); ++i) {
                handles.push_back({"vertex_" + std::to_string(i), typedGeometry.vertices[i]});
            }
        }
        return handles;
    }, geometry);
}

} // namespace edi::drafting
