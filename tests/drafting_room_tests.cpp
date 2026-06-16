#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingRoom.h"

#include <cassert>
#include <cmath>
#include <functional>
#include <memory>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

// A fresh-id minter the caller would normally back with the controller's serial.
std::function<DraftingObjectId()> counter()
{
    auto n = std::make_shared<int>(0);
    return [n]() { return "room-" + std::to_string((*n)++); };
}

const WallGeometry &wallOf(const DraftingObject &object)
{
    return std::get<WallGeometry>(object.geometry);
}

} // namespace

int main()
{
    // A plain rectangular room: four solid edges, one wall segment each, every
    // corner shared by two segments (so the painter's join pass miters them).
    {
        RoomSpec spec;
        spec.origin = {0.2, 0.3};
        spec.width = 0.4;
        spec.height = 0.2;
        spec.wallThickness = 0.05;
        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        assert(plan.ok);
        assert(plan.objects.size() == 4);
        for (const DraftingObject &wall : plan.objects) {
            assert(wall.kind == DraftingShapeKind::Wall);
            assert(nearlyEqual(wallOf(wall).thickness, 0.05));
            assert(wall.metadata.material == "stone"); // neutral classification carried
        }
        // N(NW->NE) E(NE->SE) S(SE->SW) W(SW->NW), clockwise.
        const WallGeometry n = wallOf(plan.objects[0]);
        const WallGeometry e = wallOf(plan.objects[1]);
        const WallGeometry s = wallOf(plan.objects[2]);
        const WallGeometry w = wallOf(plan.objects[3]);
        assert(nearlyEqual(n.a.x, 0.2) && nearlyEqual(n.a.y, 0.3));
        assert(nearlyEqual(n.b.x, 0.6) && nearlyEqual(n.b.y, 0.3));
        assert(nearlyEqual(e.b.x, 0.6) && nearlyEqual(e.b.y, 0.5));
        assert(nearlyEqual(s.b.x, 0.2) && nearlyEqual(s.b.y, 0.5));
        assert(nearlyEqual(w.b.x, 0.2) && nearlyEqual(w.b.y, 0.3));
        // Corners coincide: each edge's end is the next edge's start.
        assert(nearlyEqual(n.b.x, e.a.x) && nearlyEqual(n.b.y, e.a.y));
        assert(nearlyEqual(e.b.x, s.a.x) && nearlyEqual(e.b.y, s.a.y));
        assert(nearlyEqual(w.b.x, n.a.x) && nearlyEqual(w.b.y, n.a.y));
    }

    // The guard-antechamber layout: openings on three edges become GAPS, so each
    // opened edge splits into two segments. N corridor + E secret + S door + a
    // solid W wall => 2 + 2 + 2 + 1 = 7 wall segments.
    {
        RoomSpec spec;
        spec.origin = {0.1, 0.1};
        spec.width = 0.6;  // N/S edge length
        spec.height = 0.4; // E/W edge length
        spec.wallThickness = 0.02;
        spec.openings = {
            {RoomEdge::North, 0.30, 0.10, "corridor"}, // centered, 10-wide gap
            {RoomEdge::East, 0.30, 0.04, "secret"},
            {RoomEdge::South, 0.30, 0.05, "door"},
        };
        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        assert(plan.ok);
        assert(plan.objects.size() == 7);

        // The N edge (NW(0.1,0.1) -> NE(0.7,0.1)) with a gap [0.25,0.35] yields a
        // first segment (0.1,0.1)->(0.35,0.1) and a second (0.45,0.1)->(0.7,0.1).
        const WallGeometry n0 = wallOf(plan.objects[0]);
        const WallGeometry n1 = wallOf(plan.objects[1]);
        assert(nearlyEqual(n0.a.x, 0.1) && nearlyEqual(n0.b.x, 0.35));
        assert(nearlyEqual(n1.a.x, 0.45) && nearlyEqual(n1.b.x, 0.7));
        assert(nearlyEqual(n0.a.y, 0.1) && nearlyEqual(n1.b.y, 0.1)); // both on the N edge
    }

    // Two openings on one edge split it into THREE segments (the complement of
    // two disjoint gaps), while the other three edges stay solid: 3 + 1 + 1 + 1.
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 1.0;
        spec.height = 0.5;
        spec.openings = {
            {RoomEdge::North, 0.25, 0.10, "door"}, // gap [0.20, 0.30]
            {RoomEdge::North, 0.65, 0.10, "door"}, // gap [0.60, 0.70]
        };
        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        assert(plan.ok);
        assert(plan.objects.size() == 6); // 3 (N) + 1 + 1 + 1
        // N spans: [0,0.20], [0.30,0.60], [0.70,1.0].
        assert(nearlyEqual(wallOf(plan.objects[0]).b.x, 0.20));
        assert(nearlyEqual(wallOf(plan.objects[1]).a.x, 0.30) && nearlyEqual(wallOf(plan.objects[1]).b.x, 0.60));
        assert(nearlyEqual(wallOf(plan.objects[2]).a.x, 0.70));
    }

    // An opening flush to a corner leaves that corner OPEN: the edge yields one
    // segment and the corner has no shared endpoint (so it will not miter).
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 0.4;
        spec.height = 0.4;
        spec.openings = {{RoomEdge::North, 0.05, 0.10, "door"}}; // gap [0.0, 0.10], flush to NW
        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        assert(plan.ok);
        // N edge: single span [0.10, 0.40]; its start is NOT the NW corner.
        const WallGeometry n = wallOf(plan.objects[0]);
        assert(nearlyEqual(n.a.x, 0.10) && nearlyEqual(n.b.x, 0.40));
        assert(plan.objects.size() == 4); // 1 (N) + 1 + 1 + 1 (one opening, three solid edges)
    }

    // A solid room with no openings is still four walls (the opening machinery is
    // additive, never required).
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 0.5;
        spec.height = 0.5;
        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        assert(plan.ok && plan.objects.size() == 4);
    }

    // Validation: degenerate size, an oversized opening, and overlapping openings
    // are all refused (the engine never receives a broken room).
    {
        RoomSpec bad;
        bad.origin = {0.0, 0.0};
        bad.width = 0.0; // degenerate
        bad.height = 0.3;
        assert(!planDraftingRoom(bad, counter()).ok);

        RoomSpec oversized;
        oversized.origin = {0.0, 0.0};
        oversized.width = 0.3;
        oversized.height = 0.3;
        oversized.openings = {{RoomEdge::North, 0.15, 0.5, "door"}}; // wider than the 0.3 edge
        assert(!planDraftingRoom(oversized, counter()).ok);

        RoomSpec overlapping;
        overlapping.origin = {0.0, 0.0};
        overlapping.width = 0.4;
        overlapping.height = 0.4;
        overlapping.openings = {
            {RoomEdge::North, 0.15, 0.12, "door"}, // [0.09, 0.21]
            {RoomEdge::North, 0.22, 0.12, "door"}, // [0.16, 0.28] overlaps
        };
        assert(!planDraftingRoom(overlapping, counter()).ok);
    }

    // A plug authored on an edge emits a Point marker at its offset (tagged
    // plug:<name>, provenance "plug") plus a placement anchored to that marker.
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 0.4;
        spec.height = 0.2;
        spec.wallThickness = 0.02;
        spec.plugs = {{RoomEdge::North, 0.1, "north_door", "door"}};

        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        assert(plan.ok);
        assert(plan.plugs.size() == 1);

        int markerCount = 0;
        const DraftingObject *marker = nullptr;
        for (const DraftingObject &o : plan.objects) {
            if (o.kind == DraftingShapeKind::Point) {
                ++markerCount;
                marker = &o;
            }
        }
        assert(markerCount == 1 && marker != nullptr);
        assert(marker->metadata.toolProvenance == "plug");
        assert(marker->metadata.tags.size() == 1 && marker->metadata.tags[0] == "plug:north_door");
        const PointGeometry pg = std::get<PointGeometry>(marker->geometry);
        assert(nearlyEqual(pg.point.x, 0.1) && nearlyEqual(pg.point.y, 0.0)); // N edge, origin + 0.1

        const RoomPlugPlacement &placement = plan.plugs[0];
        assert(placement.anchorObjectId == marker->id);
        assert(placement.name == "north_door" && placement.type == "door");
        assert(nearlyEqual(placement.anchor.x, 0.1) && nearlyEqual(placement.anchor.y, 0.0));
    }

    // A plug off its wall is rejected.
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 0.4;
        spec.height = 0.2;
        spec.plugs = {{RoomEdge::North, 0.9, "too_far", "door"}}; // 0.9 > width 0.4
        assert(!planDraftingRoom(spec, counter()).ok);
    }

    return 0;
}
