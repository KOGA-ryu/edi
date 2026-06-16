// Corridor geometry (v1): planCorridor routes an orthogonal centerline between two
// door points (leaving each perpendicular to its wall) and returns the corridor's
// two side-wall bands. Straight when the doors align, an L for perpendicular walls,
// a Z (dogleg) for offset facing walls.
#include "drafting/DraftingCorridor.h"
#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) <= eps;
}

std::function<DraftingObjectId()> counter()
{
    auto n = std::make_shared<int>(0);
    return [n]() { return "corridor-" + std::to_string((*n)++); };
}

const WallGeometry &wallOf(const DraftingObject &object)
{
    return std::get<WallGeometry>(object.geometry);
}

bool isVerticalWall(const DraftingObject &object)
{
    return nearlyEqual(wallOf(object).a.x, wallOf(object).b.x);
}

} // namespace

int main()
{
    // Straight: two doors on facing vertical walls, sharing an x — the L/Z collapses
    // to a single run, so just the two side walls (offset by half-width in x).
    {
        CorridorSpec spec;
        spec.doorA = {0.5, 0.3};
        spec.edgeA = RoomEdge::South; // outward +y
        spec.doorB = {0.5, 0.6};
        spec.edgeB = RoomEdge::North; // outward -y
        spec.width = 0.06;
        spec.wallThickness = 0.02;

        const std::vector<DraftingObject> walls = planCorridor(spec, counter());
        assert(walls.size() == 2);
        for (const DraftingObject &w : walls) {
            assert(w.kind == DraftingShapeKind::Wall);
            assert(nearlyEqual(wallOf(w).thickness, 0.02));
            assert(w.metadata.toolProvenance == "corridor");
            assert(isVerticalWall(w)); // both side walls run vertically
        }
        // One side wall is half-width left of the centerline, the other half-width
        // right (x = 0.5 ± 0.03), each spanning the door gap in y.
        const double xa = wallOf(walls[0]).a.x;
        const double xb = wallOf(walls[1]).a.x;
        assert((nearlyEqual(xa, 0.47) && nearlyEqual(xb, 0.53))
               || (nearlyEqual(xa, 0.53) && nearlyEqual(xb, 0.47)));
    }

    // L: perpendicular walls (one horizontal normal, one vertical) → a single bend,
    // two centerline segments → two side walls each → 4 walls.
    {
        CorridorSpec spec;
        spec.doorA = {0.3, 0.5};
        spec.edgeA = RoomEdge::East; // outward +x
        spec.doorB = {0.5, 0.3};
        spec.edgeB = RoomEdge::South; // outward +y
        const std::vector<DraftingObject> walls = planCorridor(spec, counter());
        assert(walls.size() == 4);
        for (const DraftingObject &w : walls) {
            assert(w.kind == DraftingShapeKind::Wall);
        }
    }

    // Z (dogleg): facing vertical walls but offset in x → vertical-jog-vertical,
    // three centerline segments → 6 walls.
    {
        CorridorSpec spec;
        spec.doorA = {0.3, 0.3};
        spec.edgeA = RoomEdge::South;
        spec.doorB = {0.5, 0.6};
        spec.edgeB = RoomEdge::North;
        const std::vector<DraftingObject> walls = planCorridor(spec, counter());
        assert(walls.size() == 6);
    }

    // Degenerate: coincident doors route nothing.
    {
        CorridorSpec spec;
        spec.doorA = {0.5, 0.5};
        spec.edgeA = RoomEdge::South;
        spec.doorB = {0.5, 0.5};
        spec.edgeB = RoomEdge::North;
        assert(planCorridor(spec, counter()).empty());
    }

    // v2 routing: with no obstacles, the routed centerline equals the direct one.
    {
        CorridorSpec spec;
        spec.doorA = {0.1, 0.5};
        spec.edgeA = RoomEdge::East;
        spec.doorB = {0.9, 0.5};
        spec.edgeB = RoomEdge::West;
        const std::vector<Point2D> direct = corridorCenterline(spec);
        const std::vector<Point2D> routed = routeCorridorCenterline(spec, {});
        assert(routed.size() == direct.size());
    }

    // v2 routing: an obstacle straddling the direct line forces a detour AROUND it.
    {
        CorridorSpec spec;
        spec.doorA = {0.1, 0.5};
        spec.edgeA = RoomEdge::East;
        spec.doorB = {0.9, 0.5};
        spec.edgeB = RoomEdge::West;
        std::vector<CorridorObstacle> obstacles;
        obstacles.push_back({{0.4, 0.4}, 0.2, 0.2}); // blocks the straight y=0.5 path

        const std::vector<Point2D> routed = routeCorridorCenterline(spec, obstacles);
        assert(routed.size() > 2); // it detoured, not a straight line
        assert(nearlyEqual(routed.front().x, 0.1) && nearlyEqual(routed.front().y, 0.5));
        assert(nearlyEqual(routed.back().x, 0.9) && nearlyEqual(routed.back().y, 0.5));
        // No segment passes through the obstacle interior [0.4,0.6] x [0.4,0.6].
        for (std::size_t i = 1; i < routed.size(); ++i) {
            const Point2D &p = routed[i - 1];
            const Point2D &q = routed[i];
            const double lo = 0.4, hi = 0.6, e = 1e-6;
            if (std::abs(p.y - q.y) < e) { // horizontal
                const bool inY = p.y > lo + e && p.y < hi - e;
                const bool overX = std::max(std::min(p.x, q.x), lo) < std::min(std::max(p.x, q.x), hi) - e;
                assert(!(inY && overX));
            } else if (std::abs(p.x - q.x) < e) { // vertical
                const bool inX = p.x > lo + e && p.x < hi - e;
                const bool overY = std::max(std::min(p.y, q.y), lo) < std::min(std::max(p.y, q.y), hi) - e;
                assert(!(inX && overY));
            }
        }
    }

    return 0;
}
