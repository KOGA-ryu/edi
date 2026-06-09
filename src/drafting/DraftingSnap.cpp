#include "drafting/DraftingSnap.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace edi::drafting {
namespace {

double clamp01(double value)
{
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

double safeGridStep(const DraftingSnapSettings &settings)
{
    if (!std::isfinite(settings.gridStep) || settings.gridStep <= 0.0) {
        return 1.0 / 16.0;
    }
    return std::clamp(settings.gridStep, 0.000001, 1.0);
}

double safeGridStepX(const DraftingSnapSettings &settings)
{
    if (std::isfinite(settings.gridStepX) && settings.gridStepX > 0.0) {
        return std::clamp(settings.gridStepX, 0.000001, 1.0);
    }
    return safeGridStep(settings);
}

double safeGridStepY(const DraftingSnapSettings &settings)
{
    if (std::isfinite(settings.gridStepY) && settings.gridStepY > 0.0) {
        return std::clamp(settings.gridStepY, 0.000001, 1.0);
    }
    return safeGridStep(settings);
}

void addCandidate(std::vector<DraftingSnapCandidate> &candidates,
    const DraftingObject &object,
    Point2D point,
    DraftingSnapSourceKind sourceKind)
{
    if (!isFinite(point)) {
        return;
    }
    candidates.push_back({
        normalizeDraftingPoint(point),
        sourceKind,
        draftingSnapSourceKindName(sourceKind),
        object.id,
    });
}

DraftingSnapResult candidateSnap(const DraftingSnapCandidate &candidate)
{
    return {
        normalizeDraftingPoint(candidate.point),
        DraftingSnapKind::Object,
        candidate.sourceKind,
        candidate.label,
        candidate.sourceObjectId,
    };
}

DraftingSnapResult nearestObjectSnap(Point2D point, const DraftingDocument &document, const DraftingSnapSettings &settings)
{
    if (!settings.objectSnapEnabled || !std::isfinite(settings.objectTolerance) || settings.objectTolerance < 0.0) {
        return noneSnap(point);
    }

    bool found = false;
    DraftingSnapCandidate best;
    double bestDistance = settings.objectTolerance;
    for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(document, settings)) {
        const double candidateDistance = distance(point, candidate.point);
        if (candidateDistance <= bestDistance) {
            bestDistance = candidateDistance;
            best = candidate;
            found = true;
        }
    }
    return found ? candidateSnap(best) : noneSnap(point);
}

} // namespace

Point2D normalizeDraftingPoint(Point2D point)
{
    return {clamp01(point.x), clamp01(point.y)};
}

const char *draftingSnapKindName(DraftingSnapKind kind)
{
    switch (kind) {
    case DraftingSnapKind::None:
        return "none";
    case DraftingSnapKind::Grid:
        return "grid";
    case DraftingSnapKind::Object:
        return "object";
    }
    return "unknown";
}

const char *draftingSnapSourceKindName(DraftingSnapSourceKind kind)
{
    switch (kind) {
    case DraftingSnapSourceKind::None:
        return "none";
    case DraftingSnapSourceKind::Endpoint:
        return "endpoint";
    case DraftingSnapSourceKind::Vertex:
        return "vertex";
    case DraftingSnapSourceKind::Midpoint:
        return "midpoint";
    case DraftingSnapSourceKind::Center:
        return "center";
    }
    return "unknown";
}

std::vector<DraftingSnapCandidate> snapCandidatesForObject(const DraftingObject &object, const DraftingSnapSettings &settings)
{
    std::vector<DraftingSnapCandidate> candidates;
    if (!object.visible || !kindMatchesGeometry(object.kind, object.geometry)) {
        return candidates;
    }

    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, geometry.point, DraftingSnapSourceKind::Endpoint);
            }
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, geometry.a, DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, geometry.b, DraftingSnapSourceKind::Endpoint);
            }
            if (settings.midpointEnabled) {
                addCandidate(candidates, object, {(geometry.a.x + geometry.b.x) / 2.0, (geometry.a.y + geometry.b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
            }
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            const std::vector<HandleAnchor> corners = handleAnchors(geometry);
            if (settings.vertexEnabled) {
                for (const HandleAnchor &corner : corners) {
                    addCandidate(candidates, object, corner.point, DraftingSnapSourceKind::Vertex);
                }
            }
            if (corners.size() == 4 && settings.midpointEnabled) {
                for (std::size_t i = 0; i < corners.size(); ++i) {
                    const Point2D a = corners[i].point;
                    const Point2D b = corners[(i + 1) % corners.size()].point;
                    addCandidate(candidates, object, {(a.x + b.x) / 2.0, (a.y + b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
                }
            }
            if (corners.size() == 4 && settings.centerEnabled) {
                double sumX = 0.0;
                double sumY = 0.0;
                for (const HandleAnchor &corner : corners) {
                    sumX += corner.point.x;
                    sumY += corner.point.y;
                }
                addCandidate(candidates, object, {sumX / 4.0, sumY / 4.0}, DraftingSnapSourceKind::Center);
            }
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            if (settings.centerEnabled) {
                addCandidate(candidates, object, geometry.center, DraftingSnapSourceKind::Center);
            }
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (settings.centerEnabled) {
                if (geometry.orientation == GuideOrientation::Horizontal) {
                    addCandidate(candidates, object, {0.5, geometry.position}, DraftingSnapSourceKind::Center);
                } else {
                    addCandidate(candidates, object, {geometry.position, 0.5}, DraftingSnapSourceKind::Center);
                }
            }
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, geometry.a, DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, geometry.b, DraftingSnapSourceKind::Endpoint);
            }
            if (settings.midpointEnabled) {
                addCandidate(candidates, object, {(geometry.a.x + geometry.b.x) / 2.0, (geometry.a.y + geometry.b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
            }
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, geometry.a, DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, geometry.b, DraftingSnapSourceKind::Endpoint);
            }
            if (settings.midpointEnabled) {
                addCandidate(candidates, object, {(geometry.a.x + geometry.b.x) / 2.0, (geometry.a.y + geometry.b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
            }
        } else if constexpr (std::is_same_v<Geometry, PolygonGeometry>) {
            if (settings.vertexEnabled) {
                for (Point2D point : geometry.vertices) {
                    addCandidate(candidates, object, point, DraftingSnapSourceKind::Vertex);
                }
            }
            if (settings.midpointEnabled && geometry.vertices.size() >= 2) {
                for (std::size_t i = 0; i < geometry.vertices.size(); ++i) {
                    const Point2D a = geometry.vertices[i];
                    const Point2D b = geometry.vertices[(i + 1) % geometry.vertices.size()];
                    addCandidate(candidates, object, {(a.x + b.x) / 2.0, (a.y + b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
                }
            }
            if (settings.centerEnabled && !geometry.vertices.empty()) {
                double sumX = 0.0;
                double sumY = 0.0;
                for (Point2D point : geometry.vertices) {
                    sumX += point.x;
                    sumY += point.y;
                }
                addCandidate(candidates, object, {sumX / static_cast<double>(geometry.vertices.size()), sumY / static_cast<double>(geometry.vertices.size())}, DraftingSnapSourceKind::Center);
            }
        } else {
            if (settings.endpointEnabled && !geometry.vertices.empty()) {
                addCandidate(candidates, object, geometry.vertices.front(), DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, geometry.vertices.back(), DraftingSnapSourceKind::Endpoint);
            }
            if (settings.vertexEnabled && geometry.vertices.size() > 2) {
                for (std::size_t i = 1; i + 1 < geometry.vertices.size(); ++i) {
                    addCandidate(candidates, object, geometry.vertices[i], DraftingSnapSourceKind::Vertex);
                }
            }
            if (settings.midpointEnabled && geometry.vertices.size() >= 2) {
                for (std::size_t i = 0; i + 1 < geometry.vertices.size(); ++i) {
                    const Point2D a = geometry.vertices[i];
                    const Point2D b = geometry.vertices[i + 1];
                    addCandidate(candidates, object, {(a.x + b.x) / 2.0, (a.y + b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
                }
            }
        }
    }, object.geometry);

    return candidates;
}

std::vector<DraftingSnapCandidate> snapCandidatesForDocument(const DraftingDocument &document, const DraftingSnapSettings &settings)
{
    std::vector<DraftingSnapCandidate> candidates;
    for (const DraftingObject &object : document.objects) {
        const std::vector<DraftingSnapCandidate> objectCandidates = snapCandidatesForObject(object, settings);
        candidates.insert(candidates.end(), objectCandidates.begin(), objectCandidates.end());
    }
    return candidates;
}

DraftingSnapResult noneSnap(Point2D point)
{
    return {
        normalizeDraftingPoint(point),
        DraftingSnapKind::None,
        DraftingSnapSourceKind::None,
        "none",
        {},
    };
}

DraftingSnapResult gridSnap(Point2D point, const DraftingSnapSettings &settings)
{
    const Point2D safe = normalizeDraftingPoint(point);
    const double stepX = safeGridStepX(settings);
    const double stepY = safeGridStepY(settings);
    return {
        normalizeDraftingPoint({std::round(safe.x / stepX) * stepX, std::round(safe.y / stepY) * stepY}),
        DraftingSnapKind::Grid,
        DraftingSnapSourceKind::None,
        "grid",
        {},
    };
}

DraftingSnapResult resolveSnap(Point2D rawPoint, const DraftingDocument &document, const DraftingSnapSettings &settings)
{
    const Point2D point = normalizeDraftingPoint(rawPoint);
    if (settings.objectPriorityBeforeGrid) {
        DraftingSnapResult object = nearestObjectSnap(point, document, settings);
        if (object.kind == DraftingSnapKind::Object) {
            return object;
        }
    }

    if (settings.gridEnabled) {
        return gridSnap(point, settings);
    }

    if (!settings.objectPriorityBeforeGrid) {
        DraftingSnapResult object = nearestObjectSnap(point, document, settings);
        if (object.kind == DraftingSnapKind::Object) {
            return object;
        }
    }

    return noneSnap(point);
}

} // namespace edi::drafting
