// DM-09: planRegionFill (algorithm a — room-footprint lookup). Pins the boundary
// settled by reviewer reply 008: the enclosing region is the footprint of the FIRST
// DraftingMapRoom (document order) containing the seed, emitted as a closed,
// consistently-wound 4-corner ring; a seed in open space is a !ok refusal.
#include "drafting/DraftingRegionFill.h"
#include "drafting/DraftingDocument.h"

#include "EdiAssert.h"
#include <string>
#include <vector>

using namespace edi::drafting;

namespace {

DraftingMapRoom room(const std::string &name, double x, double y, double w, double h)
{
    return DraftingMapRoom{name, {x, y}, w, h, "stone"};
}

} // namespace

int main()
{
    // Seed strictly inside a footprint -> ok, ring = that footprint's 4 corners,
    // wound NW -> NE -> SE -> SW, in canvas units.
    {
        const std::vector<DraftingMapRoom> rooms = {room("a", 1.0, 2.0, 10.0, 6.0)};
        const RegionFillPlan plan = planRegionFill({3.0, 4.0}, rooms);
        EDI_CHECK(plan.ok);
        EDI_CHECK(plan.code == DraftingResultCode::None);
        EDI_CHECK(plan.ring.size() == 4);
        EDI_CHECK(plan.ring[0].x == 1.0 && plan.ring[0].y == 2.0);   // NW
        EDI_CHECK(plan.ring[1].x == 11.0 && plan.ring[1].y == 2.0);  // NE
        EDI_CHECK(plan.ring[2].x == 11.0 && plan.ring[2].y == 8.0);  // SE
        EDI_CHECK(plan.ring[3].x == 1.0 && plan.ring[3].y == 8.0);   // SW
        // A polygon is implicitly closed, so first != last (4 distinct corners).
        EDI_CHECK(!(plan.ring.front().x == plan.ring.back().x
                 && plan.ring.front().y == plan.ring.back().y));
    }

    // Seed in open space (no room contains it) -> !ok, empty ring, reason code set.
    {
        const std::vector<DraftingMapRoom> rooms = {room("a", 0.0, 0.0, 5.0, 5.0)};
        const RegionFillPlan plan = planRegionFill({100.0, 100.0}, rooms);
        EDI_CHECK(!plan.ok);
        EDI_CHECK(plan.ring.empty());
        EDI_CHECK(plan.code == DraftingResultCode::InvalidSelectionTarget);
        EDI_CHECK(!plan.message.empty());
    }

    // No rooms at all -> the same refusal (empty document).
    {
        const RegionFillPlan plan = planRegionFill({1.0, 1.0}, {});
        EDI_CHECK(!plan.ok && plan.ring.empty());
        EDI_CHECK(plan.code == DraftingResultCode::InvalidSelectionTarget);
    }

    // Two overlapping rooms, seed inside the overlap -> the FIRST in document order
    // wins (deterministic tie-break). Room "first" spans (0,0)-(10,10); "second"
    // spans (5,5)-(15,15); the seed (6,6) is inside both -> "first" is chosen, so the
    // ring is first's 10x10 footprint, not second's.
    {
        const std::vector<DraftingMapRoom> rooms = {
            room("first", 0.0, 0.0, 10.0, 10.0),
            room("second", 5.0, 5.0, 10.0, 10.0),
        };
        const RegionFillPlan plan = planRegionFill({6.0, 6.0}, rooms);
        EDI_CHECK(plan.ok);
        // first's SE corner is (10,10); second's would be (15,15).
        EDI_CHECK(plan.ring[2].x == 10.0 && plan.ring[2].y == 10.0);
    }

    // Seed exactly on a footprint EDGE -> inclusive (boundsContainsPoint), so it
    // fills. Pin the contract: a click on the NW corner and on the south edge both ok.
    {
        const std::vector<DraftingMapRoom> rooms = {room("a", 1.0, 2.0, 10.0, 6.0)};
        const RegionFillPlan corner = planRegionFill({1.0, 2.0}, rooms); // NW corner
        EDI_CHECK(corner.ok && corner.ring.size() == 4);
        const RegionFillPlan southEdge = planRegionFill({6.0, 8.0}, rooms); // y == y0+h
        EDI_CHECK(southEdge.ok);
    }

    return 0;
}
