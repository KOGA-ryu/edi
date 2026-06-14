// Corridor geometry (v1): planCorridor routes an orthogonal centerline between two
// door points (leaving each perpendicular to its wall) and returns the corridor's
// two side-wall bands. Straight when the doors align, an L for perpendicular walls,
// a Z (dogleg) for offset facing walls.
#include "drafting/DraftingCorridor.h"
#include "drafting/DraftingGeometry.h"

#include <cassert>
#include <cmath>
#include <functional>
#include <string>

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

    return 0;
}
