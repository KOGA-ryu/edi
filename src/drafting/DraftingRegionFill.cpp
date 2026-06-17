#include "drafting/DraftingRegionFill.h"

#include "drafting/DraftingGeometry.h" // boundsContainsPoint

namespace edi::drafting {

RegionFillPlan planRegionFill(Point2D seed, const std::vector<DraftingMapRoom> &rooms)
{
    // First containing room in DOCUMENT ORDER wins — a deterministic tie-break when
    // footprints overlap (overlap is itself rejected by the spec parser, but the
    // plan stays well-defined regardless of where the seed comes from).
    for (const DraftingMapRoom &room : rooms) {
        const Bounds2D footprint{room.origin.x, room.origin.y, room.width, room.height};
        if (!boundsContainsPoint(footprint, seed)) {
            continue;
        }
        RegionFillPlan plan;
        plan.ok = true;
        plan.code = DraftingResultCode::None;
        // The four corners, wound clockwise in canvas axes (origin = NW, +y south):
        // NW → NE → SE → SW. A polygon is implicitly closed, so four distinct corners
        // ARE the closed ring; (b) will later return an arbitrary traced loop here.
        const double x0 = footprint.x;
        const double y0 = footprint.y;
        const double x1 = footprint.x + footprint.width;
        const double y1 = footprint.y + footprint.height;
        plan.ring = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
        return plan;
    }

    RegionFillPlan miss;
    miss.ok = false;
    miss.code = DraftingResultCode::InvalidSelectionTarget;
    miss.message = "no room contains the seed point";
    return miss;
}

} // namespace edi::drafting
