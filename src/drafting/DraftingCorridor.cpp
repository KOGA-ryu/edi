#include "drafting/DraftingCorridor.h"

#include "drafting/DraftingGeometry.h"

#include <cmath>
#include <cstddef>
#include <utility>

namespace edi::drafting {

namespace {

constexpr double kEps = 1e-9;

// Outward (away-from-room-interior) unit normal of a wall edge. North is the top
// edge (min y) so its outward normal points up = -y (y increases downward here).
Point2D outwardNormal(RoomEdge edge)
{
    switch (edge) {
    case RoomEdge::North: return {0.0, -1.0};
    case RoomEdge::South: return {0.0, 1.0};
    case RoomEdge::East:  return {1.0, 0.0};
    case RoomEdge::West:  return {-1.0, 0.0};
    }
    return {0.0, -1.0};
}

bool normalIsVertical(const Point2D &n)
{
    return std::abs(n.y) > 0.5;
}

bool samePoint(const Point2D &p, const Point2D &q)
{
    return std::abs(p.x - q.x) < kEps && std::abs(p.y - q.y) < kEps;
}

// Drop duplicate and colinear-interior points, so a degenerate L/Z collapses to a
// straight run (fewer, cleaner wall segments).
std::vector<Point2D> simplifyOrthogonal(const std::vector<Point2D> &in)
{
    std::vector<Point2D> out;
    for (const Point2D &p : in) {
        if (!out.empty() && samePoint(out.back(), p)) {
            continue;
        }
        while (out.size() >= 2) {
            const Point2D &a = out[out.size() - 2];
            const Point2D &b = out[out.size() - 1];
            const bool colinearX = std::abs(a.x - b.x) < kEps && std::abs(b.x - p.x) < kEps;
            const bool colinearY = std::abs(a.y - b.y) < kEps && std::abs(b.y - p.y) < kEps;
            if (colinearX || colinearY) {
                out.pop_back(); // b is redundant between a and p
            } else {
                break;
            }
        }
        out.push_back(p);
    }
    return out;
}

// The orthogonal centerline, leaving each door along its wall normal:
//  - both doors on vertical-normal walls (N/S): vertical-jog-vertical (collapses
//    to straight if the doors share an x);
//  - both horizontal (E/W): horizontal-jog-horizontal;
//  - mixed: a single-bend L.
std::vector<Point2D> corridorCenterline(const CorridorSpec &spec)
{
    const Point2D a = spec.doorA;
    const Point2D b = spec.doorB;
    const Point2D nA = outwardNormal(spec.edgeA);
    const Point2D nB = outwardNormal(spec.edgeB);

    std::vector<Point2D> pts;
    if (normalIsVertical(nA) && normalIsVertical(nB)) {
        const double ym = (a.y + b.y) / 2.0;
        pts = {a, {a.x, ym}, {b.x, ym}, b};
    } else if (!normalIsVertical(nA) && !normalIsVertical(nB)) {
        const double xm = (a.x + b.x) / 2.0;
        pts = {a, {xm, a.y}, {xm, b.y}, b};
    } else if (normalIsVertical(nA)) {
        pts = {a, {a.x, b.y}, b}; // leave A vertically, enter B horizontally
    } else {
        pts = {a, {b.x, a.y}, b}; // leave A horizontally, enter B vertically
    }
    return simplifyOrthogonal(pts);
}

Point2D unitDirection(const Point2D &from, const Point2D &to)
{
    const double dx = to.x - from.x;
    const double dy = to.y - from.y;
    const double len = std::hypot(dx, dy);
    if (len < 1e-12) {
        return {0.0, 0.0};
    }
    return {dx / len, dy / len};
}

Point2D leftNormal(const Point2D &d)
{
    return {-d.y, d.x};
}

} // namespace

std::vector<DraftingObject> planCorridor(const CorridorSpec &spec,
                                         const std::function<DraftingObjectId()> &mintId)
{
    std::vector<DraftingObject> walls;
    const std::vector<Point2D> center = corridorCenterline(spec);
    if (center.size() < 2) {
        return walls; // doors coincide — nothing to route
    }

    const double hw = spec.width / 2.0;
    const std::size_t segments = center.size() - 1;

    std::vector<Point2D> leftN(segments);
    for (std::size_t i = 0; i < segments; ++i) {
        leftN[i] = leftNormal(unitDirection(center[i], center[i + 1]));
    }

    // Offset a centerline vertex onto one side (sign +1 / -1). Interior corners use
    // the orthogonal miter point — the intersection of the two offset lines, which
    // for a 90° turn is corner + sign*(n_prev + n_next)*hw. Endpoints offset by the
    // single adjacent normal. Adjacent segments thus SHARE the mitered vertex, so
    // the wall-join pass cleans the corner for free.
    const auto offsetVertex = [&](std::size_t v, double sign) -> Point2D {
        const Point2D &c = center[v];
        if (v == 0) {
            return {c.x + sign * leftN[0].x * hw, c.y + sign * leftN[0].y * hw};
        }
        if (v == segments) {
            return {c.x + sign * leftN[segments - 1].x * hw, c.y + sign * leftN[segments - 1].y * hw};
        }
        const Point2D miter{leftN[v - 1].x + leftN[v].x, leftN[v - 1].y + leftN[v].y};
        return {c.x + sign * miter.x * hw, c.y + sign * miter.y * hw};
    };

    const auto emitSide = [&](double sign) -> bool {
        for (std::size_t i = 0; i < segments; ++i) {
            const Point2D a = offsetVertex(i, sign);
            const Point2D b = offsetVertex(i + 1, sign);
            DraftingObjectBuildResult built = buildDraftingObject(
                mintId(), DraftingShapeKind::Wall, WallGeometry{a, b, spec.wallThickness});
            if (!built.ok) {
                return false;
            }
            built.object.bounds = computeBounds(built.object.geometry);
            built.object.metadata.toolProvenance = "corridor"; // neutral classification
            walls.push_back(std::move(built.object));
        }
        return true;
    };

    if (!emitSide(1.0) || !emitSide(-1.0)) {
        return {};
    }
    return walls;
}

} // namespace edi::drafting
