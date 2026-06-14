#include "drafting/DraftingCorridor.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingPathfind.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace edi::drafting {

namespace {

constexpr double kEps = 1e-9;

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
                out.pop_back();
            } else {
                break;
            }
        }
        out.push_back(p);
    }
    return out;
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

// Does the axis-aligned segment p->q pass through the interior of rect [x0,x1]x[y0,y1]?
bool segmentCrossesRect(const Point2D &p, const Point2D &q,
                        double x0, double y0, double x1, double y1)
{
    if (std::abs(p.y - q.y) < kEps) { // horizontal at y
        const double y = p.y;
        if (y <= y0 + kEps || y >= y1 - kEps) {
            return false;
        }
        const double lo = std::min(p.x, q.x);
        const double hi = std::max(p.x, q.x);
        return std::max(lo, x0) < std::min(hi, x1) - kEps;
    }
    if (std::abs(p.x - q.x) < kEps) { // vertical at x
        const double x = p.x;
        if (x <= x0 + kEps || x >= x1 - kEps) {
            return false;
        }
        const double lo = std::min(p.y, q.y);
        const double hi = std::max(p.y, q.y);
        return std::max(lo, y0) < std::min(hi, y1) - kEps;
    }
    return false;
}

bool centerlineCrossesAny(const std::vector<Point2D> &center,
                          const std::vector<CorridorObstacle> &obstacles)
{
    for (std::size_t i = 1; i < center.size(); ++i) {
        for (const CorridorObstacle &o : obstacles) {
            if (segmentCrossesRect(center[i - 1], center[i],
                                   o.origin.x, o.origin.y, o.origin.x + o.width, o.origin.y + o.height)) {
                return true;
            }
        }
    }
    return false;
}

// Connect `from` to `to` orthogonally, leaving `from` along `normal` (vertical or
// horizontal). Returns the elbow point (may be redundant; simplify cleans it).
Point2D orthoElbow(const Point2D &from, const Point2D &normal, const Point2D &to)
{
    if (normalIsVertical(normal)) {
        return {from.x, to.y}; // move along y first
    }
    return {to.x, from.y}; // move along x first
}

} // namespace

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
        pts = {a, {a.x, b.y}, b};
    } else {
        pts = {a, {b.x, a.y}, b};
    }
    return simplifyOrthogonal(pts);
}

std::vector<Point2D> routeCorridorCenterline(const CorridorSpec &spec,
                                             const std::vector<CorridorObstacle> &obstacles)
{
    const std::vector<Point2D> direct = corridorCenterline(spec);
    if (obstacles.empty() || !centerlineCrossesAny(direct, obstacles)) {
        return direct; // the straight L/Z is already clear
    }

    // Re-route with A*: a coarse grid where rooms are expensive, doors are the
    // endpoints, a turn penalty keeps it few-cornered.
    constexpr double kCell = 0.02;
    constexpr double kInflate = 0.03;   // keep the corridor body off room walls
    constexpr double kRoomCost = 1000.0;
    constexpr double kTurn = 3.0;

    double minX = std::min(spec.doorA.x, spec.doorB.x);
    double minY = std::min(spec.doorA.y, spec.doorB.y);
    double maxX = std::max(spec.doorA.x, spec.doorB.x);
    double maxY = std::max(spec.doorA.y, spec.doorB.y);
    for (const CorridorObstacle &o : obstacles) {
        minX = std::min(minX, o.origin.x);
        minY = std::min(minY, o.origin.y);
        maxX = std::max(maxX, o.origin.x + o.width);
        maxY = std::max(maxY, o.origin.y + o.height);
    }
    const double margin = 0.06;
    minX -= margin;
    minY -= margin;
    maxX += margin;
    maxY += margin;

    DraftingPathGrid grid;
    grid.cols = std::max(1, static_cast<int>(std::ceil((maxX - minX) / kCell)));
    grid.rows = std::max(1, static_cast<int>(std::ceil((maxY - minY) / kCell)));
    grid.cellCost.assign(static_cast<std::size_t>(grid.cols) * grid.rows, 1.0);

    const auto cellCenter = [&](int col, int row) {
        return Point2D{minX + (col + 0.5) * kCell, minY + (row + 0.5) * kCell};
    };
    for (int row = 0; row < grid.rows; ++row) {
        for (int col = 0; col < grid.cols; ++col) {
            const Point2D c = cellCenter(col, row);
            for (const CorridorObstacle &o : obstacles) {
                if (c.x > o.origin.x - kInflate && c.x < o.origin.x + o.width + kInflate
                    && c.y > o.origin.y - kInflate && c.y < o.origin.y + o.height + kInflate) {
                    grid.cellCost[static_cast<std::size_t>(row) * grid.cols + col] = kRoomCost;
                    break;
                }
            }
        }
    }

    const auto toCell = [&](const Point2D &p) {
        int col = static_cast<int>(std::floor((p.x - minX) / kCell));
        int row = static_cast<int>(std::floor((p.y - minY) / kCell));
        col = std::clamp(col, 0, grid.cols - 1);
        row = std::clamp(row, 0, grid.rows - 1);
        return GridCell{col, row};
    };

    const std::vector<GridCell> path =
        findGridPath(grid, toCell(spec.doorA), toCell(spec.doorB), kTurn);
    if (path.size() < 2) {
        return direct; // no better route found — keep the direct line
    }

    // Cell centers → world polyline, with orthogonal elbows snapping each door (which
    // is off-grid) onto the path while leaving along its wall normal.
    std::vector<Point2D> routed;
    routed.push_back(spec.doorA);
    const Point2D firstCenter = cellCenter(path.front().col, path.front().row);
    routed.push_back(orthoElbow(spec.doorA, outwardNormal(spec.edgeA), firstCenter));
    for (const GridCell &c : path) {
        routed.push_back(cellCenter(c.col, c.row));
    }
    const Point2D lastCenter = cellCenter(path.back().col, path.back().row);
    routed.push_back(orthoElbow(spec.doorB, outwardNormal(spec.edgeB), lastCenter));
    routed.push_back(spec.doorB);
    return simplifyOrthogonal(routed);
}

std::vector<DraftingObject> corridorWalls(const std::vector<Point2D> &center,
                                          double width, double wallThickness,
                                          const std::function<DraftingObjectId()> &mintId)
{
    std::vector<DraftingObject> walls;
    if (center.size() < 2) {
        return walls;
    }

    const double hw = width / 2.0;
    const std::size_t segments = center.size() - 1;

    std::vector<Point2D> leftN(segments);
    for (std::size_t i = 0; i < segments; ++i) {
        leftN[i] = leftNormal(unitDirection(center[i], center[i + 1]));
    }

    // Interior corners use the orthogonal miter point — the intersection of the two
    // offset lines — so adjacent segments' side walls SHARE the corner and the
    // wall-join pass cleans it. Endpoints offset by the single adjacent normal.
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
                mintId(), DraftingShapeKind::Wall, WallGeometry{a, b, wallThickness});
            if (!built.ok) {
                return false;
            }
            built.object.bounds = computeBounds(built.object.geometry);
            built.object.metadata.toolProvenance = "corridor";
            walls.push_back(std::move(built.object));
        }
        return true;
    };

    if (!emitSide(1.0) || !emitSide(-1.0)) {
        return {};
    }
    return walls;
}

std::vector<DraftingObject> planCorridor(const CorridorSpec &spec,
                                         const std::function<DraftingObjectId()> &mintId)
{
    return corridorWalls(corridorCenterline(spec), spec.width, spec.wallThickness, mintId);
}

} // namespace edi::drafting
