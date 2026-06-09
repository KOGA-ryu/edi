#include "drafting/DraftingHitTest.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace edi::drafting {
namespace {

double sqr(double value)
{
    return value * value;
}

double distanceToSegment(Point2D a, Point2D b, Point2D point)
{
    const double length2 = sqr(b.x - a.x) + sqr(b.y - a.y);
    if (length2 <= 0.000001) {
        return distance(a, point);
    }
    const double t = std::clamp(((point.x - a.x) * (b.x - a.x) + (point.y - a.y) * (b.y - a.y)) / length2, 0.0, 1.0);
    return distance({a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)}, point);
}

double distanceToVertexList(const std::vector<Point2D> &vertices, Point2D point, bool closed)
{
    if (vertices.empty()) {
        return std::numeric_limits<double>::max();
    }
    if (vertices.size() == 1) {
        return distance(vertices.front(), point);
    }

    double best = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i + 1 < vertices.size(); ++i) {
        best = std::min(best, distanceToSegment(vertices[i], vertices[i + 1], point));
    }
    if (closed) {
        best = std::min(best, distanceToSegment(vertices.back(), vertices.front(), point));
    }
    return best;
}

} // namespace

double hitDistance(const DraftingGeometry &geometry, Point2D point)
{
    if (!isFinite(point)) {
        return std::numeric_limits<double>::max();
    }

    return std::visit([&](const auto &typedGeometry) -> double {
        using Geometry = std::decay_t<decltype(typedGeometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            return distance(typedGeometry.point, point);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            return distanceToSegment(typedGeometry.a, typedGeometry.b, point);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const Bounds2D bounds = computeBounds(typedGeometry);
            const double nearestX = std::clamp(point.x, bounds.x, bounds.x + bounds.width);
            const double nearestY = std::clamp(point.y, bounds.y, bounds.y + bounds.height);
            return distance({nearestX, nearestY}, point);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            return std::abs(distance(typedGeometry.center, point) - typedGeometry.radius);
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            return distanceToVertexList(typedGeometry.vertices, point, true);
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (typedGeometry.orientation == GuideOrientation::Horizontal) {
                return std::abs(point.y - typedGeometry.position);
            }
            return std::abs(point.x - typedGeometry.position);
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            return distanceToSegment(typedGeometry.a, typedGeometry.b, point);
        } else {
            return distanceToVertexList(typedGeometry.vertices, point, false);
        }
    }, geometry);
}

DraftingHitTestResult hitTestObject(const DraftingObject &object, Point2D point)
{
    if (!object.visible || !kindMatchesGeometry(object.kind, object.geometry)) {
        return {};
    }

    DraftingHitTestResult result;
    result.ok = true;
    result.objectId = object.id;
    result.kind = object.kind;
    result.distance = hitDistance(object.geometry, point);
    return result;
}

DraftingHitTestResult hitTestDocument(const DraftingDocument &document, Point2D point, DraftingHitTestSettings settings)
{
    if (!isFinite(point) || !std::isfinite(settings.tolerance) || settings.tolerance < 0.0) {
        return {};
    }

    DraftingHitTestResult best;
    best.distance = settings.tolerance;
    for (const DraftingObject &object : document.objects) {
        const DraftingHitTestResult candidate = hitTestObject(object, point);
        if (candidate.ok && candidate.distance <= best.distance) {
            best = candidate;
        }
    }
    return best;
}

} // namespace edi::drafting
