#include "drafting/DraftingPlotPlan.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <vector>

namespace edi::drafting {
namespace {

int layerOrderForObject(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return layer == nullptr ? std::numeric_limits<int>::max() : layer->order;
}

double pointDistance(Point2D a, Point2D b)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

void appendSegment(
    DraftingPlotPlan &plan,
    const DraftingObject &object,
    const DraftingLayer &layer,
    Point2D a,
    Point2D b)
{
    plan.segments.push_back({
        object.id,
        object.layerId,
        a,
        b,
        layer.plot.penId,
        layer.plot.strokeColor,
        layer.plot.strokeWidth,
    });
}

void appendVertexSegments(
    DraftingPlotPlan &plan,
    const DraftingObject &object,
    const DraftingLayer &layer,
    const std::vector<Point2D> &vertices,
    bool closed)
{
    if (vertices.size() < 2) {
        return;
    }
    for (std::size_t index = 0; index + 1 < vertices.size(); ++index) {
        appendSegment(plan, object, layer, vertices[index], vertices[index + 1]);
    }
    if (closed) {
        appendSegment(plan, object, layer, vertices.back(), vertices.front());
    }
}

void appendPlotSegments(DraftingPlotPlan &plan, const DraftingObject &object, const DraftingLayer &layer)
{
    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            constexpr double halfSize = 0.005;
            appendSegment(
                plan,
                object,
                layer,
                {geometry.point.x - halfSize, geometry.point.y},
                {geometry.point.x + halfSize, geometry.point.y});
            appendSegment(
                plan,
                object,
                layer,
                {geometry.point.x, geometry.point.y - halfSize},
                {geometry.point.x, geometry.point.y + halfSize});
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            appendSegment(plan, object, layer, geometry.a, geometry.b);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const Point2D a = geometry.origin;
            const Point2D b{geometry.origin.x + geometry.width, geometry.origin.y};
            const Point2D c{geometry.origin.x + geometry.width, geometry.origin.y + geometry.height};
            const Point2D d{geometry.origin.x, geometry.origin.y + geometry.height};
            appendSegment(plan, object, layer, a, b);
            appendSegment(plan, object, layer, b, c);
            appendSegment(plan, object, layer, c, d);
            appendSegment(plan, object, layer, d, a);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            constexpr int circleSegments = 32;
            constexpr double pi = 3.14159265358979323846;
            Point2D previous{geometry.center.x + geometry.radius, geometry.center.y};
            for (int index = 1; index <= circleSegments; ++index) {
                const double angle = 2.0 * pi * static_cast<double>(index) / static_cast<double>(circleSegments);
                const Point2D next{
                    geometry.center.x + std::cos(angle) * geometry.radius,
                    geometry.center.y + std::sin(angle) * geometry.radius,
                };
                appendSegment(plan, object, layer, previous, next);
                previous = next;
            }
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            appendVertexSegments(plan, object, layer, geometry.vertices, true);
        } else if constexpr (std::is_same_v<Geometry, PolylineGeometry>) {
            appendVertexSegments(plan, object, layer, geometry.vertices, false);
        }
    }, object.geometry);
}

void appendTravelSegments(DraftingPlotPlan &plan)
{
    constexpr double minimumTravel = 0.000001;
    if (plan.segments.size() < 2) {
        return;
    }

    for (std::size_t index = 0; index + 1 < plan.segments.size(); ++index) {
        const DraftingPlotSegment &from = plan.segments[index];
        const DraftingPlotSegment &to = plan.segments[index + 1];
        const double travelDistance = pointDistance(from.b, to.a);
        if (!std::isfinite(travelDistance) || travelDistance <= minimumTravel) {
            continue;
        }

        plan.travelSegments.push_back({
            from.objectId,
            to.objectId,
            from.b,
            to.a,
            travelDistance,
        });
        plan.travelDistance += travelDistance;
    }
}

} // namespace

bool draftingShapeCanPlot(DraftingShapeKind kind)
{
    return kind != DraftingShapeKind::Guide
        && kind != DraftingShapeKind::ConstructionLine
        && kind != DraftingShapeKind::Dimension;
}

DraftingPlotPlan buildDraftingPlotPlan(const DraftingDocument &document, const DraftingGridProjection &grid)
{
    DraftingPlotPlan plan;

    std::vector<const DraftingObject *> sortedObjects;
    sortedObjects.reserve(document.objects.size());
    for (const DraftingObject &object : document.objects) {
        sortedObjects.push_back(&object);
    }
    std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [&](const DraftingObject *a, const DraftingObject *b) {
        return layerOrderForObject(document, *a) < layerOrderForObject(document, *b);
    });

    for (const DraftingObject *objectPointer : sortedObjects) {
        const DraftingObject &object = *objectPointer;
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !layer->visible || !layer->plot.plotEnabled || !object.visible || !draftingShapeCanPlot(object.kind)) {
            continue;
        }

        plan.objects.push_back({
            object.id,
            object.layerId,
            layer->plot.penId,
            layer->plot.strokeColor,
            layer->plot.strokeWidth,
        });
        appendPlotSegments(plan, object, *layer);

        if (boundsOutsideDrawableArea(object.bounds, grid)) {
            plan.warnings.push_back({
                object.id,
                "out_of_drawable_bounds",
                "plot object is outside drawable bounds",
            });
        }
    }

    appendTravelSegments(plan);

    return plan;
}

} // namespace edi::drafting
