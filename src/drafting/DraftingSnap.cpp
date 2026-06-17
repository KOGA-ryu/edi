#include "drafting/DraftingSnap.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGrid.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
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

bool guideSnapCandidate(const DraftingObject &object, Point2D point, DraftingSnapCandidate &candidate, double &candidateDistance)
{
    if (!object.visible || object.kind != DraftingShapeKind::Guide || !kindMatchesGeometry(object.kind, object.geometry)) {
        return false;
    }
    const auto *guide = std::get_if<GuideGeometry>(&object.geometry);
    if (guide == nullptr || !std::isfinite(guide->position)) {
        return false;
    }

    if (guide->orientation == GuideOrientation::Horizontal) {
        candidate = {
            normalizeDraftingPoint({point.x, guide->position}),
            DraftingSnapSourceKind::Guide,
            draftingSnapSourceKindName(DraftingSnapSourceKind::Guide),
            object.id,
        };
        candidateDistance = std::abs(point.y - guide->position);
        return true;
    }

    candidate = {
        normalizeDraftingPoint({guide->position, point.y}),
        DraftingSnapSourceKind::Guide,
        draftingSnapSourceKindName(DraftingSnapSourceKind::Guide),
        object.id,
    };
    candidateDistance = std::abs(point.x - guide->position);
    return true;
}

DraftingSnapResult nearestGuideSnap(Point2D point, const DraftingDocument &document, const DraftingSnapSettings &settings)
{
    if (!settings.objectSnapEnabled || !settings.guideEnabled || !std::isfinite(settings.objectTolerance) || settings.objectTolerance < 0.0) {
        return noneSnap(point);
    }

    bool hasVertical = false;
    bool hasHorizontal = false;
    DraftingSnapCandidate bestVertical;
    DraftingSnapCandidate bestHorizontal;
    double bestVerticalDistance = settings.objectTolerance;
    double bestHorizontalDistance = settings.objectTolerance;
    for (const DraftingObject &object : document.objects) {
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !layer->visible) {
            continue;
        }

        DraftingSnapCandidate candidate;
        double candidateDistance = 0.0;
        if (!guideSnapCandidate(object, point, candidate, candidateDistance) || candidateDistance > settings.objectTolerance) {
            continue;
        }

        const auto *guide = std::get_if<GuideGeometry>(&object.geometry);
        if (guide->orientation == GuideOrientation::Vertical && candidateDistance <= bestVerticalDistance) {
            bestVerticalDistance = candidateDistance;
            bestVertical = candidate;
            hasVertical = true;
        } else if (guide->orientation == GuideOrientation::Horizontal && candidateDistance <= bestHorizontalDistance) {
            bestHorizontalDistance = candidateDistance;
            bestHorizontal = candidate;
            hasHorizontal = true;
        }
    }

    if (hasVertical && hasHorizontal) {
        const double intersectionDistance = std::hypot(bestVerticalDistance, bestHorizontalDistance);
        if (intersectionDistance <= settings.objectTolerance) {
            return {
                normalizeDraftingPoint({bestVertical.point.x, bestHorizontal.point.y}),
                DraftingSnapKind::Object,
                DraftingSnapSourceKind::Guide,
                draftingSnapSourceKindName(DraftingSnapSourceKind::Guide),
                bestVertical.sourceObjectId.empty() ? bestHorizontal.sourceObjectId : bestVertical.sourceObjectId,
            };
        }
    }

    if (hasVertical && (!hasHorizontal || bestVerticalDistance <= bestHorizontalDistance)) {
        return candidateSnap(bestVertical);
    }
    if (hasHorizontal) {
        return candidateSnap(bestHorizontal);
    }
    return noneSnap(point);
}

// Clamp the cursor's foot-of-perpendicular onto the segment [a,b]. Mirrors the
// hit-test's distanceToSegment math, but returns the POINT (not the distance) —
// there is no public segment-projection helper to reuse.
Point2D projectOnSegment(Point2D a, Point2D b, Point2D point)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length2 = dx * dx + dy * dy;
    if (length2 <= 0.0) {
        return a; // degenerate segment: the single point
    }
    const double t = clamp01(((point.x - a.x) * dx + (point.y - a.y) * dy) / length2);
    return {a.x + t * dx, a.y + t * dy};
}

// The UNCLAMPED foot of perpendicular from `point` onto the segment's line, but
// ONLY when it lands on the segment (t in [0,1], small epsilon). nullopt
// otherwise — unlike projectOnSegment this never clamps to an endpoint, because a
// "perpendicular" candidate past a segment end is not actually perpendicular (and
// the endpoint is already an Endpoint snap, so clamping would mislabel it).
std::optional<Point2D> perpendicularFootOnSegment(Point2D a, Point2D b, Point2D point)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length2 = dx * dx + dy * dy;
    if (length2 <= 0.0) {
        return std::nullopt; // degenerate segment has no perpendicular foot
    }
    constexpr double epsilon = 0.000001;
    const double t = ((point.x - a.x) * dx + (point.y - a.y) * dy) / length2;
    if (t < -epsilon || t > 1.0 + epsilon) {
        return std::nullopt;
    }
    return Point2D{a.x + t * dx, a.y + t * dy};
}

// Nearest point on a chain of segments (closed adds the wrap-around edge). nullopt
// when there is no edge to project onto (< 2 vertices — those are vertex snaps).
std::optional<Point2D> nearestOnVertexList(const std::vector<Point2D> &vertices, Point2D point, bool closed)
{
    if (vertices.size() < 2) {
        return std::nullopt;
    }
    std::optional<Point2D> best;
    double bestDistance = std::numeric_limits<double>::max();
    auto consider = [&](Point2D a, Point2D b) {
        const Point2D projected = projectOnSegment(a, b, point);
        const double candidateDistance = distance(projected, point);
        if (candidateDistance < bestDistance) {
            bestDistance = candidateDistance;
            best = projected;
        }
    };
    for (std::size_t i = 0; i + 1 < vertices.size(); ++i) {
        consider(vertices[i], vertices[i + 1]);
    }
    if (closed) {
        consider(vertices.back(), vertices.front());
    }
    return best;
}

// The cursor's nearest point ON an object's curve, for the v1 OnCurve kinds:
// line, circle, arc, and polyline/polygon edges. Other kinds (wall, construction
// line, dimension, ellipse, spline) have no OnCurve candidate yet -> nullopt.
std::optional<Point2D> nearestPointOnCurve(const DraftingObject &object, Point2D point)
{
    if (const auto *line = std::get_if<LineGeometry>(&object.geometry)) {
        return projectOnSegment(line->a, line->b, point);
    }
    if (const auto *circle = std::get_if<CircleGeometry>(&object.geometry)) {
        const double centerDistance = distance(circle->center, point);
        if (centerDistance <= 0.0) {
            return std::nullopt; // cursor exactly at the center — direction undefined
        }
        const double scale = circle->radius / centerDistance;
        return Point2D{
            circle->center.x + (point.x - circle->center.x) * scale,
            circle->center.y + (point.y - circle->center.y) * scale,
        };
    }
    if (const auto *arc = std::get_if<ArcGeometry>(&object.geometry)) {
        // Same sweep convention as the arc hit-test / quadrant gate above.
        constexpr double pi = 3.14159265358979323846;
        const double lo = std::min(arc->startAngleDeg, arc->endAngleDeg);
        const double hi = std::max(arc->startAngleDeg, arc->endAngleDeg);
        double angle = std::atan2(point.y - arc->center.y, point.x - arc->center.x) * 180.0 / pi;
        while (angle < lo) {
            angle += 360.0;
        }
        if (angle <= hi) {
            return arcPointAtAngle(arc->center, arc->radius, angle);
        }
        // Outside the sweep: the nearer endpoint is the closest on-curve point.
        const Point2D start = arcPointAtAngle(arc->center, arc->radius, arc->startAngleDeg);
        const Point2D end = arcPointAtAngle(arc->center, arc->radius, arc->endAngleDeg);
        return distance(start, point) <= distance(end, point) ? start : end;
    }
    if (const auto *polygon = std::get_if<PolygonGeometry>(&object.geometry)) {
        return nearestOnVertexList(polygon->vertices, point, true);
    }
    if (const auto *polyline = std::get_if<PolylineGeometry>(&object.geometry)) {
        return nearestOnVertexList(polyline->vertices, point, false);
    }
    return std::nullopt;
}

DraftingSnapResult nearestObjectSnap(Point2D point, const DraftingDocument &document, const DraftingSnapSettings &settings)
{
    if (!settings.objectSnapEnabled || !std::isfinite(settings.objectTolerance) || settings.objectTolerance < 0.0) {
        return noneSnap(point);
    }

    bool found = false;
    DraftingSnapCandidate best;
    double bestDistance = settings.objectTolerance;
    DraftingSnapResult guide = nearestGuideSnap(point, document, settings);
    if (guide.kind == DraftingSnapKind::Object) {
        best = {guide.point, guide.sourceKind, guide.label, guide.sourceObjectId};
        bestDistance = distance(point, guide.point);
        found = true;
    }
    for (const DraftingSnapCandidate &candidate : snapCandidatesForDocument(document, settings)) {
        const double candidateDistance = distance(point, candidate.point);
        if (candidateDistance <= bestDistance) {
            bestDistance = candidateDistance;
            best = candidate;
            found = true;
        }
    }
    // OnCurve is the LOWEST-priority object snap: the nearest projection of the
    // cursor onto a curve's body. It is a FALLBACK TIER — consulted only when no
    // keypoint/guide/intersection snapped at all (`!found`), so EVERY keypoint
    // wins whenever one is in range, even when the curve body happens to be a
    // hair closer (the standard CAD "nearest" precedence, and what keeps the
    // existing keypoint/intersection snaps byte-unchanged). It is computed here,
    // not as a static per-object candidate, because it depends on the cursor.
    // (The brief sketched "append after the keypoints"; under the existing <=
    // tie-rule that would let a closer OnCurve outrank an intersection — e.g. a
    // cursor nearer a line body than to a crossing — so the precedence is realized
    // as this fallback tier instead. Same intent: keypoints always win.)
    if (!found && settings.onCurveEnabled) {
        for (const DraftingObject &object : document.objects) {
            if (!object.visible || !kindMatchesGeometry(object.kind, object.geometry)) {
                continue;
            }
            const DraftingLayer *layer = findLayer(document, object.layerId);
            if (layer == nullptr || !layer->visible) {
                continue;
            }
            const std::optional<Point2D> projected = nearestPointOnCurve(object, point);
            if (!projected || !isFinite(*projected)) {
                continue;
            }
            const double candidateDistance = distance(point, *projected);
            if (candidateDistance <= settings.objectTolerance && candidateDistance < bestDistance) {
                bestDistance = candidateDistance;
                best = {
                    normalizeDraftingPoint(*projected),
                    DraftingSnapSourceKind::OnCurve,
                    draftingSnapSourceKindName(DraftingSnapSourceKind::OnCurve),
                    object.id,
                };
                found = true;
            }
        }
    }
    return found ? candidateSnap(best) : noneSnap(point);
}

// The up-to-two tangent CONTACT angles (degrees, the model's atan2 convention)
// from an external point to a circle. The classic Thales construction: a tangent
// touches where the radius is perpendicular to the tangent line, so in the right
// triangle (center, fromPoint, contact) the right angle is at the contact and the
// angle at the center is acos(radius / |fromPoint-center|). The contacts therefore
// sit at the center→fromPoint bearing ± that angle. Empty when fromPoint is inside
// or on the circle (no external tangent) or the radius is degenerate.
std::vector<double> tangentContactAnglesDeg(Point2D center, double radius, Point2D fromPoint)
{
    constexpr double pi = 3.14159265358979323846;
    const double centerDistance = distance(center, fromPoint);
    if (!(radius > 0.0) || centerDistance <= radius) {
        return {};
    }
    const double bearing = std::atan2(fromPoint.y - center.y, fromPoint.x - center.x);
    const double half = std::acos(radius / centerDistance); // radius/dist in (0,1)
    return {
        (bearing + half) * 180.0 / pi,
        (bearing - half) * 180.0 / pi,
    };
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
    case DraftingSnapSourceKind::Guide:
        return "guide";
    case DraftingSnapSourceKind::Intersection:
        return "intersection";
    case DraftingSnapSourceKind::Quadrant:
        return "quadrant";
    case DraftingSnapSourceKind::OnCurve:
        return "on_curve";
    case DraftingSnapSourceKind::Tangent:
        return "tangent";
    case DraftingSnapSourceKind::Perpendicular:
        return "perpendicular";
    }
    return "unknown";
}

const char *draftingSnapTolerancePresetId(double tolerance)
{
    if (tolerance <= 0.015) {
        return "tight";
    }
    if (tolerance >= 0.06) {
        return "loose";
    }
    return "normal";
}

double draftingSnapToleranceForPreset(const std::string &presetId)
{
    if (presetId == "tight") {
        return 0.015;
    }
    if (presetId == "loose") {
        return 0.06;
    }
    return 0.03;
}

void applyDraftingGridToSnapSettings(DraftingSnapSettings &snapSettings, const DraftingGridSettings &gridSettings)
{
    const DraftingGridSettings safe = sanitizeDraftingGridSettings(gridSettings);
    snapSettings.gridStepX = safe.minorStep / safe.width;
    snapSettings.gridStepY = safe.minorStep / safe.height;
    snapSettings.gridStep = snapSettings.gridStepX;
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
            // The 4 cardinal points on the perimeter (0/90/180/270°). arcPointAtAngle
            // is the shared center+radius→point helper, so a full circle is just the
            // four quadrant angles with no sweep gate.
            if (settings.quadrantEnabled) {
                for (const double angle : {0.0, 90.0, 180.0, 270.0}) {
                    addCandidate(candidates, object, arcPointAtAngle(geometry.center, geometry.radius, angle), DraftingSnapSourceKind::Quadrant);
                }
            }
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            if (settings.centerEnabled) {
                addCandidate(candidates, object, geometry.center, DraftingSnapSourceKind::Center);
            }
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, arcPointAtAngle(geometry.center, geometry.radius, geometry.startAngleDeg), DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, arcPointAtAngle(geometry.center, geometry.radius, geometry.endAngleDeg), DraftingSnapSourceKind::Endpoint);
            }
            // Only the cardinal angles that lie WITHIN the arc's sweep are real
            // perimeter points — a 0→90° arc has no 180/270° quadrant. The sweep
            // test mirrors the arc hit-test convention exactly (lo/hi = min/max of
            // the endpoints, wrap the candidate angle up to ≥ lo, in-sweep if ≤ hi).
            if (settings.quadrantEnabled) {
                const double lo = std::min(geometry.startAngleDeg, geometry.endAngleDeg);
                const double hi = std::max(geometry.startAngleDeg, geometry.endAngleDeg);
                for (double angle : {0.0, 90.0, 180.0, 270.0}) {
                    while (angle < lo) {
                        angle += 360.0;
                    }
                    if (angle <= hi) {
                        addCandidate(candidates, object, arcPointAtAngle(geometry.center, geometry.radius, angle), DraftingSnapSourceKind::Quadrant);
                    }
                }
            }
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            return;
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
        } else if constexpr (std::is_same_v<Geometry, PolylineGeometry>) {
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
        } else if constexpr (std::is_same_v<Geometry, EllipseGeometry>) {
            if (settings.centerEnabled) {
                addCandidate(candidates, object, geometry.center, DraftingSnapSourceKind::Center);
            }
        } else if constexpr (std::is_same_v<Geometry, TextAnnotationGeometry>) {
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, geometry.position, DraftingSnapSourceKind::Endpoint);
            }
        } else if constexpr (std::is_same_v<Geometry, SplineGeometry>) {
            // Snap to the control points (which lie on the curve): first/last as
            // endpoints, the interior knots as vertices. Mirrors the polyline
            // arm but reads controlPoints, not vertices.
            if (settings.endpointEnabled && !geometry.controlPoints.empty()) {
                addCandidate(candidates, object, geometry.controlPoints.front(), DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, geometry.controlPoints.back(), DraftingSnapSourceKind::Endpoint);
            }
            if (settings.vertexEnabled && geometry.controlPoints.size() > 2) {
                for (std::size_t i = 1; i + 1 < geometry.controlPoints.size(); ++i) {
                    addCandidate(candidates, object, geometry.controlPoints[i], DraftingSnapSourceKind::Vertex);
                }
            }
        } else if constexpr (std::is_same_v<Geometry, WallGeometry>) {
            // A wall snaps exactly like a line: its two centerline endpoints and
            // the centerline midpoint, each gated by its setting flag. The
            // thickness does not add snap targets in v1.
            if (settings.endpointEnabled) {
                addCandidate(candidates, object, geometry.a, DraftingSnapSourceKind::Endpoint);
                addCandidate(candidates, object, geometry.b, DraftingSnapSourceKind::Endpoint);
            }
            if (settings.midpointEnabled) {
                addCandidate(candidates, object, {(geometry.a.x + geometry.b.x) / 2.0, (geometry.a.y + geometry.b.y) / 2.0}, DraftingSnapSourceKind::Midpoint);
            }
        } else {
            static_assert(always_false_v<Geometry>, "snapCandidatesForObject: unhandled geometry kind — add an arm");
        }
    }, object.geometry);

    return candidates;
}

std::vector<DraftingSnapCandidate> snapCandidatesForDocument(const DraftingDocument &document, const DraftingSnapSettings &settings)
{
    std::vector<DraftingSnapCandidate> candidates;
    // Per-object candidates (endpoints, midpoints, centres, …), one object at a
    // time. Collect the visible line objects on the way so the pairwise
    // intersection pass below does not re-scan the document.
    struct VisibleLine {
        const LineGeometry *geometry;
        DraftingObjectId id;
    };
    std::vector<VisibleLine> lines;
    for (const DraftingObject &object : document.objects) {
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !layer->visible) {
            continue;
        }
        const std::vector<DraftingSnapCandidate> objectCandidates = snapCandidatesForObject(object, settings);
        candidates.insert(candidates.end(), objectCandidates.begin(), objectCandidates.end());
        if (const auto *line = std::get_if<LineGeometry>(&object.geometry)) {
            lines.push_back({line, object.id});
        }
    }

    // Intersection candidates are PAIRWISE — they belong to no single object, so
    // they cannot come from snapCandidatesForObject. For every pair of visible
    // line segments that actually cross within both segments, the crossing is a
    // snap target (the most-used CAD snap, and it reuses the same
    // segmentIntersection the trim/fillet verbs are built on). Lines only for
    // now; circle/arc/construction-line crossings are a later slice. O(n^2) over
    // line objects — fine for drawing-sized documents.
    if (settings.intersectionEnabled) {
        for (std::size_t i = 0; i < lines.size(); ++i) {
            for (std::size_t j = i + 1; j < lines.size(); ++j) {
                const std::optional<Point2D> crossing = segmentIntersection(*lines[i].geometry, *lines[j].geometry);
                if (crossing) {
                    candidates.push_back({*crossing, DraftingSnapSourceKind::Intersection,
                                          draftingSnapSourceKindName(DraftingSnapSourceKind::Intersection),
                                          lines[i].id});
                }
            }
        }
    }
    return candidates;
}

std::vector<DraftingSnapCandidate> relativeSnapCandidatesForDocument(const DraftingDocument &document, Point2D fromPoint, const DraftingSnapSettings &settings)
{
    std::vector<DraftingSnapCandidate> candidates;
    if (!isFinite(fromPoint)) {
        return candidates;
    }

    auto addPerpendicularFoot = [&](const DraftingObject &object, Point2D a, Point2D b) {
        // A Perpendicular candidate must be a GENUINE perpendicular foot — emit it
        // only when the unclamped projection lands on the segment; never clamp to an
        // endpoint (the "deferred perpendicular" past a segment end is a future
        // variant, not built here).
        if (const std::optional<Point2D> foot = perpendicularFootOnSegment(a, b, fromPoint)) {
            addCandidate(candidates, object, *foot, DraftingSnapSourceKind::Perpendicular);
        }
    };

    for (const DraftingObject &object : document.objects) {
        if (!object.visible || !kindMatchesGeometry(object.kind, object.geometry)) {
            continue;
        }
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (layer == nullptr || !layer->visible) {
            continue;
        }

        // TANGENT contacts from the anchor to circles/arcs. For a full circle both
        // contacts apply; for an arc keep only contacts whose angle lies within the
        // sweep (same lo/hi wrap convention as the quadrant gate / arc hit-test).
        if (settings.tangentEnabled) {
            if (const auto *circle = std::get_if<CircleGeometry>(&object.geometry)) {
                for (const double angle : tangentContactAnglesDeg(circle->center, circle->radius, fromPoint)) {
                    addCandidate(candidates, object, arcPointAtAngle(circle->center, circle->radius, angle), DraftingSnapSourceKind::Tangent);
                }
            } else if (const auto *arc = std::get_if<ArcGeometry>(&object.geometry)) {
                const double lo = std::min(arc->startAngleDeg, arc->endAngleDeg);
                const double hi = std::max(arc->startAngleDeg, arc->endAngleDeg);
                for (double angle : tangentContactAnglesDeg(arc->center, arc->radius, fromPoint)) {
                    double swept = angle;
                    while (swept < lo) {
                        swept += 360.0;
                    }
                    if (swept <= hi) {
                        addCandidate(candidates, object, arcPointAtAngle(arc->center, arc->radius, angle), DraftingSnapSourceKind::Tangent);
                    }
                }
            }
        }

        // PERPENDICULAR feet from the anchor onto straight lines / edges: line and
        // construction-line segments, the wall centerline, and polyline/polygon
        // edges. (Curved kinds use the tangent constraint above instead.)
        if (settings.perpendicularEnabled) {
            if (const auto *line = std::get_if<LineGeometry>(&object.geometry)) {
                addPerpendicularFoot(object, line->a, line->b);
            } else if (const auto *cline = std::get_if<ConstructionLineGeometry>(&object.geometry)) {
                addPerpendicularFoot(object, cline->a, cline->b);
            } else if (const auto *wall = std::get_if<WallGeometry>(&object.geometry)) {
                addPerpendicularFoot(object, wall->a, wall->b);
            } else if (const auto *polyline = std::get_if<PolylineGeometry>(&object.geometry)) {
                for (std::size_t i = 0; i + 1 < polyline->vertices.size(); ++i) {
                    addPerpendicularFoot(object, polyline->vertices[i], polyline->vertices[i + 1]);
                }
            } else if (const auto *polygon = std::get_if<PolygonGeometry>(&object.geometry)) {
                const std::vector<Point2D> &v = polygon->vertices;
                for (std::size_t i = 0; i + 1 < v.size(); ++i) {
                    addPerpendicularFoot(object, v[i], v[i + 1]);
                }
                if (v.size() >= 2) {
                    addPerpendicularFoot(object, v.back(), v.front()); // closing edge
                }
            }
        }
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
