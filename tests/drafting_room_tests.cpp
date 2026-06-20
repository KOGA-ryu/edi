#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingRoom.h"

#include "EdiAssert.h"
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
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.objects.size() == 4);
        for (const DraftingObject &wall : plan.objects) {
            EDI_CHECK(wall.kind == DraftingShapeKind::Wall);
            EDI_CHECK(nearlyEqual(wallOf(wall).thickness, 0.05));
            EDI_CHECK(wall.metadata.material == "stone"); // neutral classification carried
        }
        // N(NW->NE) E(NE->SE) S(SE->SW) W(SW->NW), clockwise.
        const WallGeometry n = wallOf(plan.objects[0]);
        const WallGeometry e = wallOf(plan.objects[1]);
        const WallGeometry s = wallOf(plan.objects[2]);
        const WallGeometry w = wallOf(plan.objects[3]);
        EDI_CHECK(nearlyEqual(n.a.x, 0.2) && nearlyEqual(n.a.y, 0.3));
        EDI_CHECK(nearlyEqual(n.b.x, 0.6) && nearlyEqual(n.b.y, 0.3));
        EDI_CHECK(nearlyEqual(e.b.x, 0.6) && nearlyEqual(e.b.y, 0.5));
        EDI_CHECK(nearlyEqual(s.b.x, 0.2) && nearlyEqual(s.b.y, 0.5));
        EDI_CHECK(nearlyEqual(w.b.x, 0.2) && nearlyEqual(w.b.y, 0.3));
        // Corners coincide: each edge's end is the next edge's start.
        EDI_CHECK(nearlyEqual(n.b.x, e.a.x) && nearlyEqual(n.b.y, e.a.y));
        EDI_CHECK(nearlyEqual(e.b.x, s.a.x) && nearlyEqual(e.b.y, s.a.y));
        EDI_CHECK(nearlyEqual(w.b.x, n.a.x) && nearlyEqual(w.b.y, n.a.y));
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
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.objects.size() == 7);

        // The N edge (NW(0.1,0.1) -> NE(0.7,0.1)) with a gap [0.25,0.35] yields a
        // first segment (0.1,0.1)->(0.35,0.1) and a second (0.45,0.1)->(0.7,0.1).
        const WallGeometry n0 = wallOf(plan.objects[0]);
        const WallGeometry n1 = wallOf(plan.objects[1]);
        EDI_CHECK(nearlyEqual(n0.a.x, 0.1) && nearlyEqual(n0.b.x, 0.35));
        EDI_CHECK(nearlyEqual(n1.a.x, 0.45) && nearlyEqual(n1.b.x, 0.7));
        EDI_CHECK(nearlyEqual(n0.a.y, 0.1) && nearlyEqual(n1.b.y, 0.1)); // both on the N edge
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
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.objects.size() == 6); // 3 (N) + 1 + 1 + 1
        // N spans: [0,0.20], [0.30,0.60], [0.70,1.0].
        EDI_CHECK(nearlyEqual(wallOf(plan.objects[0]).b.x, 0.20));
        EDI_CHECK(nearlyEqual(wallOf(plan.objects[1]).a.x, 0.30) && nearlyEqual(wallOf(plan.objects[1]).b.x, 0.60));
        EDI_CHECK(nearlyEqual(wallOf(plan.objects[2]).a.x, 0.70));
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
        EDI_CHECK(plan.ok);
        // N edge: single span [0.10, 0.40]; its start is NOT the NW corner.
        const WallGeometry n = wallOf(plan.objects[0]);
        EDI_CHECK(nearlyEqual(n.a.x, 0.10) && nearlyEqual(n.b.x, 0.40));
        EDI_CHECK(plan.objects.size() == 4); // 1 (N) + 1 + 1 + 1 (one opening, three solid edges)
    }

    // A solid room with no openings is still four walls (the opening machinery is
    // additive, never required).
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 0.5;
        spec.height = 0.5;
        const DraftingRoomPlan plan = planDraftingRoom(spec, counter());
        EDI_CHECK(plan.ok && plan.objects.size() == 4);
    }

    // Validation: degenerate size, an oversized opening, and overlapping openings
    // are all refused (the engine never receives a broken room).
    {
        RoomSpec bad;
        bad.origin = {0.0, 0.0};
        bad.width = 0.0; // degenerate
        bad.height = 0.3;
        EDI_CHECK(!planDraftingRoom(bad, counter()).ok);

        RoomSpec oversized;
        oversized.origin = {0.0, 0.0};
        oversized.width = 0.3;
        oversized.height = 0.3;
        oversized.openings = {{RoomEdge::North, 0.15, 0.5, "door"}}; // wider than the 0.3 edge
        EDI_CHECK(!planDraftingRoom(oversized, counter()).ok);

        RoomSpec overlapping;
        overlapping.origin = {0.0, 0.0};
        overlapping.width = 0.4;
        overlapping.height = 0.4;
        overlapping.openings = {
            {RoomEdge::North, 0.15, 0.12, "door"}, // [0.09, 0.21]
            {RoomEdge::North, 0.22, 0.12, "door"}, // [0.16, 0.28] overlaps
        };
        EDI_CHECK(!planDraftingRoom(overlapping, counter()).ok);
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
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.plugs.size() == 1);

        int markerCount = 0;
        const DraftingObject *marker = nullptr;
        for (const DraftingObject &o : plan.objects) {
            if (o.kind == DraftingShapeKind::Point) {
                ++markerCount;
                marker = &o;
            }
        }
        EDI_CHECK(markerCount == 1 && marker != nullptr);
        EDI_CHECK(marker->metadata.toolProvenance == "plug");
        EDI_CHECK(marker->metadata.tags.size() == 1 && marker->metadata.tags[0] == "plug:north_door");
        const PointGeometry pg = std::get<PointGeometry>(marker->geometry);
        EDI_CHECK(nearlyEqual(pg.point.x, 0.1) && nearlyEqual(pg.point.y, 0.0)); // N edge, origin + 0.1

        const RoomPlugPlacement &placement = plan.plugs[0];
        EDI_CHECK(placement.anchorObjectId == marker->id);
        EDI_CHECK(placement.name == "north_door" && placement.type == "door");
        EDI_CHECK(nearlyEqual(placement.anchor.x, 0.1) && nearlyEqual(placement.anchor.y, 0.0));
    }

    // A plug off its wall is rejected.
    {
        RoomSpec spec;
        spec.origin = {0.0, 0.0};
        spec.width = 0.4;
        spec.height = 0.2;
        spec.plugs = {{RoomEdge::North, 0.9, "too_far", "door"}}; // 0.9 > width 0.4
        EDI_CHECK(!planDraftingRoom(spec, counter()).ok);
    }

    // Phase-1 slice 3e — RoomKind enum + per-edge wall presence (brief 065).
    // Types and fields only; drawing logic honours them in Phase 2.
    {
        // --- roomKindName / roomKindFromName round-trip + unknown → Enclosed. ---
        EDI_CHECK(roomKindFromName(roomKindName(RoomKind::Enclosed)) == RoomKind::Enclosed);
        EDI_CHECK(roomKindFromName(roomKindName(RoomKind::Open))     == RoomKind::Open);
        EDI_CHECK(std::string(roomKindName(RoomKind::Enclosed)) == "enclosed");
        EDI_CHECK(std::string(roomKindName(RoomKind::Open))     == "open");
        EDI_CHECK(roomKindFromName("unknown_future_kind") == RoomKind::Enclosed);
        EDI_CHECK(roomKindFromName("") == RoomKind::Enclosed);

        // --- Default RoomSpec: Enclosed + all four edge walls present. ---
        // A default-constructed RoomSpec must be byte-identical to the pre-3e spec:
        // planDraftingRoom still emits four walls for a plain rectangular room.
        RoomSpec defaultSpec;
        EDI_CHECK(defaultSpec.kind  == RoomKind::Enclosed);
        EDI_CHECK(defaultSpec.wallN == true);
        EDI_CHECK(defaultSpec.wallE == true);
        EDI_CHECK(defaultSpec.wallS == true);
        EDI_CHECK(defaultSpec.wallW == true);

        // --- Behavior-preserving: planDraftingRoom on a default RoomSpec still
        //     emits 4 walls (the new fields are inert this slice). ---
        defaultSpec.origin = {0.0, 0.0};
        defaultSpec.width  = 0.4;
        defaultSpec.height = 0.2;
        defaultSpec.wallThickness = 0.05;
        const DraftingRoomPlan plan = planDraftingRoom(defaultSpec, counter());
        EDI_CHECK(plan.ok);
        // 4 solid perimeter walls — same as before the new fields existed.
        EDI_CHECK(plan.objects.size() == 4);
        for (const DraftingObject &wall : plan.objects) {
            EDI_CHECK(wall.kind == DraftingShapeKind::Wall);
        }

        // --- RoomSpec can hold Open + suppressed edges without compile error.
        //     (The values are recorded; planDraftingRoom ignores them until Phase 2.) ---
        RoomSpec openSpec;
        openSpec.kind  = RoomKind::Open;
        openSpec.wallN = false;
        openSpec.wallS = false;
        // wallE and wallW still true by default.
        EDI_CHECK(openSpec.kind  == RoomKind::Open);
        EDI_CHECK(openSpec.wallN == false);
        EDI_CHECK(openSpec.wallE == true);
        EDI_CHECK(openSpec.wallS == false);
        EDI_CHECK(openSpec.wallW == true);
    }

    return 0;
}
